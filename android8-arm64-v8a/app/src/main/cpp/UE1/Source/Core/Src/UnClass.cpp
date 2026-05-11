/*=============================================================================
	UnClass.cpp: Object class implementation.
	Copyright 1997 Epic MegaGames, Inc. This software is a trade secret.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
	FPropertyTag.
-----------------------------------------------------------------------------*/

//
// A tag describing a class property, to aid in serialization.
//
struct FPropertyTag
{
	// Archive for counting property sizes.
	class FArchiveCountSize : public FArchive
	{
	public:
		FArchiveCountSize( FArchive& InSaveAr )
		: Size(0), SaveAr(InSaveAr)
		{
			ArIsSaving = 1;
		}
		INT Size;
	private:
		FArchive& SaveAr;
		FArchive& operator<<( UObject*& Obj )
		{
			INT Index = SaveAr.MapObject(Obj);
			return *this << AR_INDEX(Index);
		}
		FArchive& operator<<( FName& Name )
		{
			INT Index = SaveAr.MapName(&Name);
			return *this << AR_INDEX(Index);
		}
		FArchive& Serialize( void* V, int Length )
		{
			Size += Length;
			return *this;
		}
	};

	// Variables.
	BYTE	Type;		// Type of property, 0=end.
	BYTE	Info;		// Packed info byte.
	FName	Name;		// Name of property.
	FName	ItemName;	// Struct name if UStructProperty.
	INT		Size;       // Property size.
	INT		ArrayIndex;	// Index if an array; else 0.

	// Constructors.
	FPropertyTag()
	{}
	FPropertyTag( FArchive& InSaveAr, UProperty* Property, INT InIndex, BYTE* Value )
	:	Type		( Property->GetID() )
	,	Name		( Property->GetFName() )
	,	ItemName	( NAME_None     )
	,	Size		( 0             )
	,	ArrayIndex	( InIndex       )
	,	Info		( Property->GetID() )
	{
		// Handle structs.
		UStructProperty* StructProperty = Cast<UStructProperty>( Property );
		if( StructProperty )
			ItemName = StructProperty->Struct->GetFName();

		// Set size.
		FArchiveCountSize ArCount( InSaveAr );
		SerializeTaggedProperty( ArCount, Property, Value );
		Size = ArCount.Size;

		// Update info bits.
		Info |=
		(	Size==1		? 0x00
		:	Size==2     ? 0x10
		:	Size==4     ? 0x20
		:	Size==12	? 0x30
		:	Size==16	? 0x40
		:	Size<=255	? 0x50
		:	Size<=65536 ? 0x60
		:			      0x70);
		UBoolProperty* Bool = Cast<UBoolProperty>( Property );
		if( ArrayIndex || (Bool && (*(DWORD*)Value & Bool->BitMask)) )
			Info |= 0x80;
	}

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FPropertyTag& Tag )
	{
		static char PrevTag[NAME_SIZE]="";
		guard(FPropertyTag<<);
		BYTE SizeByte;
		_WORD SizeWord;
		INT SizeInt;

		// Name.
		guard(TagName);
		Ar << Tag.Name;
		if( Tag.Name == NAME_None )
			return Ar;
		appStrcpy( PrevTag, *Tag.Name );
		unguard;

		// Packed info byte:
		// Bit 0..3 = raw type.
		// Bit 4..6 = serialized size: [1 2 4 12 16 byte word int].
		// Bit 7    = array flag.
		Ar << Tag.Info;
		Tag.Type = Tag.Info & 0x0f;
		if( Tag.Type == NAME_StructProperty )
			Ar << Tag.ItemName;
		switch( Tag.Info & 0x70 )
		{
			case 0x00:
				Tag.Size = 1;
				break;
			case 0x10:
				Tag.Size = 2;
				break;
			case 0x20:
				Tag.Size = 4;
				break;
			case 0x30:
				Tag.Size = 12;
				break;
			case 0x40:
				Tag.Size = 16;
				break;
			case 0x50:
				SizeByte =  Tag.Size;
				Ar       << SizeByte;
				Tag.Size =  SizeByte;
				break;
			case 0x60:
				SizeWord =  Tag.Size;
				Ar       << SizeWord;
				Tag.Size =  SizeWord;
				break;
			case 0x70:
				SizeInt		=  Tag.Size;
				Ar          << SizeInt;
				Tag.Size    =  SizeInt;
				break;
		}
		if( (Tag.Info&0x80) && Tag.Type!=NAME_BoolProperty )
		{
			BYTE B
			=	(Tag.ArrayIndex<=127  ) ? (Tag.ArrayIndex    )
			:	(Tag.ArrayIndex<=16383) ? (Tag.ArrayIndex>>8 )+0x80
			:	                          (Tag.ArrayIndex>>24)+0xC0;
			Ar << B;
			if( (B & 0x80)==0 )
			{
				Tag.ArrayIndex = B;
			}
			else if( (B & 0xC0)==0x80 )
			{
				BYTE C = Tag.ArrayIndex & 255;
				Ar << C;
				Tag.ArrayIndex = ((INT)(B&0x7F)<<8) + ((INT)C);
			}
			else
			{
				BYTE C = Tag.ArrayIndex>>16;
				BYTE D = Tag.ArrayIndex>>8;
				BYTE E = Tag.ArrayIndex;
				Ar << C << D << E;
				Tag.ArrayIndex = ((INT)(B&0x3F)<<24) + ((INT)C<<16) + ((INT)D<<8) + ((INT)E);
			}
		}
		else Tag.ArrayIndex = 0;
		return Ar;
		unguardf(( "(After %s)", PrevTag ));
	}

	// Property serializer.
	void SerializeTaggedProperty( FArchive& Ar, UProperty* Property, BYTE* Value )
	{
		guard(FPropertyTag::SerializeTaggedProperty);
		if( Property->GetClass()==UBoolProperty::StaticClass )
		{
			UBoolProperty* Bool = CastChecked<UBoolProperty>( Property );
			check(Bool->BitMask!=0);
			if( Ar.IsLoading() )				
			{
				if( Info&0x80)	*(DWORD *)Value |=  Bool->BitMask;
				else			*(DWORD *)Value &= ~Bool->BitMask;
			}
		}
		else
		{
			Property->SerializeItem( Ar, Value );
		}
		unguard;
	}
};

/*-----------------------------------------------------------------------------
	UField implementation.
-----------------------------------------------------------------------------*/

UField::UField( EIntrinsicConstructor, UClass* InClass, FName InName, FName InPackageName )
: UObject( EC_IntrinsicConstructor, InClass, InName, InPackageName )
, SuperField( NULL )
, Next( NULL )
, HashNext( NULL )
{}
UField::UField( UField* InSuperField )
:	SuperField( InSuperField )
{}
UClass* UField::GetOwnerClass()
{
	guardSlow(UField::GetOwnerClass);
	UObject* Obj;
	for( Obj=this; Obj->GetClass()!=UClass::StaticClass; Obj=Obj->GetParent() );
	return (UClass*)Obj;
	unguardSlow;
}
void UField::Bind()
{
	guard(UField::Bind);
	unguard;
}
void UField::PostLoad()
{
	guard(UField::PostLoad);
	UObject::PostLoad();
	Bind();
	unguard;
}
void UField::Serialize( FArchive& Ar )
{
	guard(UField::Serialize);
	UObject::Serialize( Ar );

	Ar << SuperField << Next;
	if( Ar.IsLoading() )
		HashNext = NULL;

	unguardobj;
}
IMPLEMENT_CLASS(UField)

/*-----------------------------------------------------------------------------
	Hash building.
-----------------------------------------------------------------------------*/

// Virtual function hash builder.
static void BuildVfHashes( UStruct* Struct )
{
	guard(BuildVfHashes);

	// Link the references.
	UObjectProperty** RefLink = &Struct->RefLink;
	for( TFieldIterator<UObjectProperty> ItR(Struct); ItR; ++ItR,RefLink=&(*RefLink)->NextReference )
		*RefLink = *ItR;
	*RefLink = NULL;

	// Link the structs.
	UStructProperty** StructLink = &Struct->StructLink;
	for( TFieldIterator<UStructProperty> ItS(Struct); ItS; ++ItS,StructLink=&(*StructLink)->NextStruct )
		*StructLink = *ItS;
	*StructLink = NULL;

	// Handle state stuff.
	UState* State = Cast<UState>( Struct );
	if( State )
	{
		// Allocate hash.
		if( !State->VfHash )
			State->VfHash = new UField*[UField::HASH_COUNT];

		// Initialize hash.
		UField** PrevLink[UField::HASH_COUNT];
		for( INT i=0; i<UField::HASH_COUNT; i++ )
		{
			State->VfHash[i] = NULL;
			PrevLink[i] = &State->VfHash[i];
		}

		// Add all stuff at this node to the hash.
		for( TFieldIterator<UStruct> It(State); It; ++It )
		{
			INT iHash          = It->GetFName().GetIndex() & (UField::HASH_COUNT-1);
			*PrevLink[iHash]   = *It;
			PrevLink[iHash]    = &It->HashNext;
			It->HashNext       = NULL;
		}
	}

	// Build hash for child states.
	for( TFieldIterator<UStruct> It(Struct); It && It.GetStruct()==Struct; ++It )
		BuildVfHashes( *It );

	unguard;
}

/*-----------------------------------------------------------------------------
	UStruct implementation.
-----------------------------------------------------------------------------*/

//
// Constructors.
//
UStruct::UStruct( EIntrinsicConstructor, INT InSize, FName InName, FName InPackageName )
:	UField( EC_IntrinsicConstructor, UClass::StaticClass, InName, InPackageName )
,	ScriptText( NULL )
,	Children( NULL )
,	PropertiesSize( InSize )
,	FriendlyName( InName )
,	TextPos( 0 )
,	Line( 0 )
{}
UStruct::UStruct( UStruct* InSuperStruct )
:	UField( InSuperStruct )
,	PropertiesSize( InSuperStruct ? InSuperStruct->GetPropertiesSize() : 0 )
,	FriendlyName( GetFName() )
{}

//
// Link offsets.
//
void UStruct::LinkOffsets( FArchive& Ar )
{
	guard(UStruct::LinkOffsets);
	PropertiesSize = 0;
	if( GetInheritanceSuper() )
	{
		Ar.Preload( GetInheritanceSuper() );
		PropertiesSize = Align(GetInheritanceSuper()->GetPropertiesSize(),4);
	}
	UProperty* Prev = NULL;
	for( UField* Field=Children; Field; Field=Field->Next )
	{
		Ar.Preload( Field );
		UProperty* Property = Cast<UProperty>( Field );
		if( Property && Field->GetParent()==this )
		{
			Property->Link( Ar, Prev );
			PropertiesSize = Property->Offset + Property->GetSize();
			Prev = Property;
		}
	}
	PropertiesSize = Align(PropertiesSize,4);
#if PLATFORM_64BIT
	if( UE1AndroidIsDynamicArrayStruct64(this) )
		PropertiesSize = sizeof(FArray);
#endif
	unguard;
}

//
// Serialize all of the class's data that belongs in a particular
// bin and resides in Data.
//
void UStruct::SerializeBin( FArchive& Ar, BYTE* Data )
{
	FName PropertyName(NAME_None);
	INT Index=0;
	guard(UStruct::SerializeBin);
	for( TFieldIterator<UProperty> It(this); It; ++It )
	{
		PropertyName = It->GetFName();
		if( !(It->PropertyFlags & (CPF_Transient|CPF_Intrinsic)) )
			for( Index=0; Index<It->ArrayDim; Index++ )
				It->SerializeItem( Ar, Data + It->Offset + Index*It->GetElementSize() );
	}
	unguardf(( "(%s %s[%i])", GetFullName(), *PropertyName, Index ));
}
void UStruct::SerializeTaggedProperties( FArchive& Ar, BYTE* Data, UClass* DefaultsClass )
{
	FName PropertyName(NAME_None);
	INT Index=-1;
	guard(UStruct::SerializeTaggedProperties);
	check(Ar.IsLoading() || Ar.IsSaving());

	// Find defaults.
	BYTE* Defaults      = NULL;
	INT   DefaultsCount = 0;
	if( DefaultsClass && DefaultsClass->Defaults.Num() )
	{
		Defaults      = &DefaultsClass->Defaults(0);
		DefaultsCount =  DefaultsClass->Defaults.Num();
	}

	// Load/save.
	if( Ar.IsLoading() )
	{
		// Load all stored properties.
		INT Count=0;
		guard(LoadStream);
		while( 1 )
		{
			FPropertyTag Tag;
			Ar << Tag;
			if( Tag.Name == NAME_None )
				break;
			PropertyName = Tag.Name;
			if( Tag.Type==NAME_StructProperty && appStricmp(*Tag.ItemName,"Rotation")==0 )//oldver
				Tag.ItemName = "Rotator";
			if( Tag.Type==NAME_StructProperty && appStricmp(*Tag.ItemName,"Region")==0 )//oldver
				Tag.ItemName = "PointRegion";
			TFieldIterator<UProperty> It(this);
			for( ; It; ++It )
				if( It->GetFName()==Tag.Name )
					break;
			if( !It )
			{
				debugf( NAME_Warning, "Property %s of %s not found", *Tag.Name, GetName() );
			}
			else if( Tag.Type!=It->GetID() )
			{
				debugf( NAME_Warning, "Type mismatch in %s of %s: file %i, class %i", *Tag.Name, GetName(), Tag.Type, It->GetID() );
			}
			else if( Tag.ArrayIndex>=It->ArrayDim )
			{
				debugf( NAME_Warning, "Array bounds in %s of %s: %i/%i", *Tag.Name, GetName(), Tag.ArrayIndex, It->ArrayDim );
			}
			else if( Tag.Type==NAME_StructProperty && Tag.ItemName!=CastChecked<UStructProperty>(*It)->Struct->GetFName() )
			{
				debugf( NAME_Warning, "Property %s of %s struct type mismatch %s/%s", *Tag.Name, GetName(), *Tag.ItemName, CastChecked<UStructProperty>(*It)->Struct->GetName() );
			}
			else if( It->PropertyFlags & (CPF_Transient | CPF_Intrinsic) )
			{
				debugf( NAME_Warning, "Property %s of %s is not serialiable", *Tag.Name, GetName() );
			}
			else
			{
				// This property is ok.
				Tag.SerializeTaggedProperty( Ar, *It, Data + It->Offset + Tag.ArrayIndex*It->GetElementSize() );
				continue;
			}

			// Skip unknown or bad property.
			if( Ar.Ver()<36 && Tag.Type==NAME_ObjectProperty )//oldver
			{
				UObject* Tmp;
				Ar << Tmp;
			}
			else
			{
				BYTE B;
				for( int i=0; i<Tag.Size; i++ )
					Ar << B;
			}
		}
		unguardf(( "(Count %i)", Count ));
	}
	else
	{
		// Save tagged properties.
		guard(SaveStream);
		for( TFieldIterator<UProperty> It(this); It; ++It )
		{
			if( !(It->PropertyFlags & (CPF_Transient | CPF_Intrinsic)) )
			{
				PropertyName = It->GetFName();
				for( Index=0; Index<It->ArrayDim; Index++ )
				{
					INT Offset = It->Offset + Index*It->GetElementSize();
					if( !It->Matches( Data, (Offset+It->GetElementSize()<=DefaultsCount) ? Defaults : NULL, Index) )
					{
 						FPropertyTag Tag( Ar, *It, Index, Data + Offset );
						Ar << Tag;
						Tag.SerializeTaggedProperty( Ar, *It, Data + Offset );
					}
				}
			}
		}
		FName Temp(NAME_None);
		Ar << Temp;
		unguard;
	}
	unguardf(( "(%s[%i])", *PropertyName, Index ));
}
#if PLATFORM_64BIT
static inline void UE1AndroidEnsureScriptCapacity64( TArray<BYTE>& Script, INT Needed )
{
	if( Needed > Script.Num() )
		Script.AddZeroed( Needed - Script.Num() );
}

template<class T> static inline void UE1AndroidAppendScriptValue64( TArray<BYTE>& Script, INT& NativeCode, const T& Value )
{
	UE1AndroidEnsureScriptCapacity64( Script, NativeCode + (INT)sizeof(T) );
	appMemcpy( &Script(NativeCode), &Value, sizeof(T) );
	NativeCode += sizeof(T);
}

static inline void UE1AndroidPatchScriptValue64( TArray<BYTE>& Script, INT NativeOffset, _WORD Value )
{
	appMemcpy( &Script(NativeOffset), &Value, sizeof(_WORD) );
}

static inline void UE1AndroidPatchScriptValue64( TArray<BYTE>& Script, INT NativeOffset, INT Value )
{
	appMemcpy( &Script(NativeOffset), &Value, sizeof(INT) );
}

struct FUE1AndroidScriptOffsetPatch64
{
	INT   NativeOffset;
	INT   LegacyOffset;
	BYTE  Width;
};

struct FUE1AndroidScriptOffsetMap64
{
	UStruct* Struct;
	TArray<INT> LegacyToNative;
};

static TArray<FUE1AndroidScriptOffsetMap64> GUE1AndroidScriptOffsetMaps64;

static INT UE1AndroidFindScriptOffsetMapIndex64( UStruct* Struct )
{
	for( INT i=0; i<GUE1AndroidScriptOffsetMaps64.Num(); ++i )
		if( GUE1AndroidScriptOffsetMaps64(i).Struct == Struct )
			return i;
	return INDEX_NONE;
}

static void UE1AndroidStoreScriptOffsetMap64( UStruct* Struct, const TArray<INT>& LegacyToNative )
{
	guard(UE1AndroidStoreScriptOffsetMap64);
	if( !Struct || LegacyToNative.Num() <= 0 )
		return;
	INT Index = UE1AndroidFindScriptOffsetMapIndex64( Struct );
	if( Index == INDEX_NONE )
	{
		Index = GUE1AndroidScriptOffsetMaps64.AddZeroed();
		GUE1AndroidScriptOffsetMaps64(Index).Struct = Struct;
	}
	GUE1AndroidScriptOffsetMaps64(Index).LegacyToNative = LegacyToNative;
	unguard;
}

INT UE1AndroidRemapScriptOffset64( UStruct* Struct, INT LegacyOffset )
{
	guard(UE1AndroidRemapScriptOffset64);
	if( !Struct || LegacyOffset == INDEX_NONE || LegacyOffset == MAXWORD )
		return LegacyOffset;
	INT Index = UE1AndroidFindScriptOffsetMapIndex64( Struct );
	if( Index != INDEX_NONE )
	{
		const TArray<INT>& LegacyToNative = GUE1AndroidScriptOffsetMaps64(Index).LegacyToNative;
		if( LegacyOffset >= 0 && LegacyOffset < LegacyToNative.Num() && LegacyToNative(LegacyOffset) >= 0 )
			return LegacyToNative(LegacyOffset);
	}
	return LegacyOffset;
	unguard;
}

INT UE1AndroidUnmapScriptOffset64( UStruct* Struct, INT NativeOffset )
{
	guard(UE1AndroidUnmapScriptOffset64);
	if( !Struct || NativeOffset == INDEX_NONE || NativeOffset == MAXWORD )
		return NativeOffset;
	INT Index = UE1AndroidFindScriptOffsetMapIndex64( Struct );
	if( Index != INDEX_NONE )
	{
		const TArray<INT>& LegacyToNative = GUE1AndroidScriptOffsetMaps64(Index).LegacyToNative;
		for( INT i=0; i<LegacyToNative.Num(); ++i )
			if( LegacyToNative(i) == NativeOffset )
				return i;
	}
	return NativeOffset;
	unguard;
}

static inline void UE1AndroidAddScriptOffsetPatch64( TArray<FUE1AndroidScriptOffsetPatch64>& Patches, INT NativeOffset, INT LegacyOffset, BYTE Width )
{
	INT Index = Patches.AddZeroed();
	Patches(Index).NativeOffset = NativeOffset;
	Patches(Index).LegacyOffset = LegacyOffset;
	Patches(Index).Width        = Width;
}

static BYTE UE1AndroidNativeParmSize64( UStruct* Owner, INT ParmIndex, BYTE LegacySize )
{
	UFunction* Function = Cast<UFunction>( Owner );
	if( !Function || LegacySize==0 )
		return LegacySize;

	INT CurrentParm = 0;
	UProperty* ThisParm = NULL;
	UProperty* NextParm = NULL;
	for( TFieldIterator<UProperty> It(Function); It && It.GetStruct()==Function; ++It )
	{
		if( It->PropertyFlags & CPF_Parm )
		{
			if( CurrentParm == ParmIndex )
				ThisParm = *It;
			else if( CurrentParm == ParmIndex+1 )
			{
				NextParm = *It;
				break;
			}
			CurrentParm++;
		}
	}
	if( !ThisParm )
		return LegacySize;

	INT NativeSize = NextParm ? (NextParm->Offset - ThisParm->Offset) : ThisParm->GetSize();
	if( NativeSize <= 0 || NativeSize > 255 )
		return LegacySize;
	return (BYTE)NativeSize;
}

static EExprToken UE1AndroidSerializeExpr64
(
	UStruct* Owner,
	TArray<BYTE>& Script,
	INT& LegacyCode,
	INT& NativeCode,
	FArchive& Ar,
	TArray<INT>& LegacyToNative,
	TArray<FUE1AndroidScriptOffsetPatch64>& OffsetPatches
)
{
	EExprToken Expr=(EExprToken)0;
	guard(UE1AndroidSerializeExpr64);

	#define UE1_ANDROID_MARK_LEGACY_OFFSET() \
	{	\
		if( LegacyCode >= 0 && LegacyCode < LegacyToNative.Num() ) \
			LegacyToNative(LegacyCode) = NativeCode; \
	}
	#define XFER_LEGACY_VALUE(T) { T V; appMemset(&V,0,sizeof(V)); Ar << V; UE1AndroidAppendScriptValue64(Script,NativeCode,V); LegacyCode += sizeof(T); }
	#define XFER_LEGACY_PTR(T) { T V=NULL; Ar << *(UObject**)&V; UE1AndroidAppendScriptValue64(Script,NativeCode,V); LegacyCode += 4; }
	#define XFER_ABSOLUTE_WORD_OFFSET() \
	{	\
		_WORD LegacyOffset=0; \
		Ar << LegacyOffset; \
		INT NativePatchOffset = NativeCode; \
		UE1AndroidAppendScriptValue64( Script, NativeCode, LegacyOffset ); \
		UE1AndroidAddScriptOffsetPatch64( OffsetPatches, NativePatchOffset, LegacyOffset, sizeof(_WORD) ); \
		LegacyCode += sizeof(_WORD); \
	}

	UE1_ANDROID_MARK_LEGACY_OFFSET();

	BYTE ExprByte=0;
	Ar << ExprByte;
	UE1AndroidAppendScriptValue64( Script, NativeCode, ExprByte );
	LegacyCode += 1;
	Expr = (EExprToken)ExprByte;

	if( Expr >= EX_MinConversion && Expr < EX_MaxConversion )
	{
		UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
	}
	else if( Expr >= EX_FirstIntrinsic )
	{
		while( UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches ) != EX_EndFunctionParms );
	}
	else if( Expr >= EX_ExtendedIntrinsic )
	{
		XFER_LEGACY_VALUE(BYTE);
		while( UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches ) != EX_EndFunctionParms );
	}
	else switch( Expr )
	{
		case EX_LocalVariable:
		case EX_InstanceVariable:
		case EX_DefaultVariable:
		case EX_IntrinsicParm:
		{
			XFER_LEGACY_PTR(UProperty*);
			break;
		}
		case EX_BoolVariable:
		{
			// EX_BoolVariable is a wrapper around a following variable expression.
			// The old 32-bit serializer could leave that nested expression to the
			// next top-level parse because it did not rewrite operand sizes.  The
			// arm64 serializer must consume it here, otherwise EX_Skip spans for
			// &&/|| are calculated too small and the VM lands in the middle of the
			// wrapped variable expression.
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			break;
		}
		case EX_ValidateObject:
		case EX_Nothing:
		case EX_EndFunctionParms:
		case EX_IntZero:
		case EX_IntOne:
		case EX_True:
		case EX_False:
		case EX_NoObject:
		case EX_Self:
		case EX_IteratorPop:
		case EX_EndCode:
		case EX_Stop:
		case EX_Return:
		case EX_IteratorNext:
		{
			break;
		}
		case EX_ClassContext:
		case EX_Context:
		{
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			_WORD LegacySkip=0;
			Ar << LegacySkip;
			LegacyCode += sizeof(_WORD);
			INT NativeSkipOffset = NativeCode;
			UE1AndroidAppendScriptValue64( Script, NativeCode, LegacySkip );
			BYTE LegacyZeroFill=0;
			Ar << LegacyZeroFill;
			// Keep the serialized zero-fill width exactly as authored.  This byte is
			// the result expression size, not a reliable object-pointer marker: many
			// bool/int/name context expressions are also 4 bytes.  Widening every 4
			// to 8 corrupts adjacent arm64 locals and breaks timers/events/cutscenes.
			// Temporary object buffers are cleared before stepping in execContext.
			BYTE NativeZeroFill = LegacyZeroFill;
			UE1AndroidAppendScriptValue64( Script, NativeCode, NativeZeroFill );
			LegacyCode += 1;
			INT NativeExprStart = NativeCode;
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			_WORD NativeSkip = (_WORD)(NativeCode - NativeExprStart);
			UE1AndroidPatchScriptValue64( Script, NativeSkipOffset, NativeSkip );
			break;
		}
		case EX_ArrayElement:
		{
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			break;
		}
		case EX_VirtualFunction:
		case EX_GlobalFunction:
		{
			XFER_LEGACY_VALUE(FName);
			while( UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches ) != EX_EndFunctionParms );
			break;
		}
		case EX_FinalFunction:
		{
			XFER_LEGACY_PTR(UStruct*);
			while( UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches ) != EX_EndFunctionParms );
			break;
		}
		case EX_IntConst:
		{
			XFER_LEGACY_VALUE(INT);
			break;
		}
		case EX_FloatConst:
		{
			XFER_LEGACY_VALUE(FLOAT);
			break;
		}
		case EX_StringConst:
		{
			BYTE B=0;
			do { Ar << B; UE1AndroidAppendScriptValue64(Script,NativeCode,B); LegacyCode += 1; } while( B );
			break;
		}
		case EX_ObjectConst:
		{
			XFER_LEGACY_PTR(UObject*);
			break;
		}
		case EX_NameConst:
		{
			XFER_LEGACY_VALUE(FName);
			break;
		}
		case EX_RotationConst:
		{
			XFER_LEGACY_VALUE(INT); XFER_LEGACY_VALUE(INT); XFER_LEGACY_VALUE(INT);
			break;
		}
		case EX_VectorConst:
		{
			XFER_LEGACY_VALUE(FLOAT); XFER_LEGACY_VALUE(FLOAT); XFER_LEGACY_VALUE(FLOAT);
			break;
		}
		case EX_ByteConst:
		case EX_IntConstByte:
		{
			XFER_LEGACY_VALUE(BYTE);
			break;
		}
		case EX_ResizeString:
		{
			XFER_LEGACY_VALUE(BYTE);
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			break;
		}
		case EX_MetaCast:
		case EX_DynamicCast:
		{
			XFER_LEGACY_PTR(UClass*);
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			break;
		}
		case EX_JumpIfNot:
		{
			XFER_ABSOLUTE_WORD_OFFSET();
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			break;
		}
		case EX_Iterator:
		{
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			XFER_ABSOLUTE_WORD_OFFSET();
			break;
		}
		case EX_Switch:
		{
			XFER_LEGACY_VALUE(BYTE);
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			break;
		}
		case EX_Jump:
		{
			XFER_ABSOLUTE_WORD_OFFSET();
			break;
		}
		case EX_Assert:
		{
			XFER_LEGACY_VALUE(_WORD);
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			break;
		}
		case EX_Case:
		{
			_WORD LegacyOffset=0;
			Ar << LegacyOffset;
			INT NativePatchOffset = NativeCode;
			UE1AndroidAppendScriptValue64( Script, NativeCode, LegacyOffset );
			LegacyCode += sizeof(_WORD);
			if( LegacyOffset != MAXWORD )
			{
				UE1AndroidAddScriptOffsetPatch64( OffsetPatches, NativePatchOffset, LegacyOffset, sizeof(_WORD) );
				UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			}
			break;
		}
		case EX_LabelTable:
		{
			while( (NativeCode & 3) != 0 )
			{
				BYTE Zero=0;
				UE1AndroidAppendScriptValue64( Script, NativeCode, Zero );
			}

			// UState::LabelTableOffset points to the FLabelEntry table data, not to
			// the EX_LabelTable token itself.  The generic expression marker above
			// only maps the token start.  On arm64 the script stream may have grown
			// before this point, so leaving the table-data offset in 32-bit space
			// makes GotoState/GotoLabel read garbage and actor state scripts never
			// reach Begin/Auto labels.  Record the post-token, post-alignment data
			// offset explicitly before copying the label entries.
			if( LegacyCode >= 0 && LegacyCode < LegacyToNative.Num() )
				LegacyToNative(LegacyCode) = NativeCode;

			for( ; ; )
			{
				FName LabelName(NAME_None);
				INT   LegacyLabelCode=0;
				Ar << LabelName;
				Ar << LegacyLabelCode;
				UE1AndroidAppendScriptValue64( Script, NativeCode, LabelName );
				INT NativePatchOffset = NativeCode;
				UE1AndroidAppendScriptValue64( Script, NativeCode, LegacyLabelCode );
				LegacyCode += sizeof(FLabelEntry);
				if( LabelName == NAME_None )
					break;
				UE1AndroidAddScriptOffsetPatch64( OffsetPatches, NativePatchOffset, LegacyLabelCode, sizeof(INT) );
			}
			break;
		}
		case EX_GotoLabel:
		{
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			break;
		}
		case EX_Let:
		{
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			break;
		}
		case EX_Skip:
		{
			_WORD LegacySkip=0;
			Ar << LegacySkip;
			LegacyCode += sizeof(_WORD);
			INT SkipOffsetNative = NativeCode;
			UE1AndroidAppendScriptValue64( Script, NativeCode, LegacySkip );
			INT NativeExprStart = NativeCode;
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			_WORD NativeSkip = (_WORD)((NativeCode - NativeExprStart) + 1);
			UE1AndroidPatchScriptValue64( Script, SkipOffsetNative, NativeSkip );
			break;
		}
		case EX_BeginFunction:
		{
			for( INT ParmIndex=0; ; ParmIndex++ )
			{
				BYTE LegacySize=0;
				Ar << LegacySize;
				LegacyCode += 1;
				BYTE NativeSize = UE1AndroidNativeParmSize64( Owner, ParmIndex, LegacySize );
				UE1AndroidAppendScriptValue64( Script, NativeCode, NativeSize );
				if( LegacySize == 0 )
					break;
				BYTE OutParm=0;
				Ar << OutParm;
				LegacyCode += 1;
				UE1AndroidAppendScriptValue64( Script, NativeCode, OutParm );
			}
			break;
		}
		case EX_StructCmpEq:
		case EX_StructCmpNe:
		{
			XFER_LEGACY_PTR(UStruct*);
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			break;
		}
		case EX_StructMember:
		{
			XFER_LEGACY_PTR(UProperty*);
			UE1AndroidSerializeExpr64( Owner, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
			break;
		}
		default:
		{
			appErrorf( "Bad expr token %02x", Expr );
			break;
		}
	}
	return Expr;

	#undef UE1_ANDROID_MARK_LEGACY_OFFSET
	#undef XFER_LEGACY_VALUE
	#undef XFER_LEGACY_PTR
	#undef XFER_ABSOLUTE_WORD_OFFSET
	unguardf(( "(%02X)", Expr ));
}

static void UE1AndroidPatchScriptOffsets64( TArray<BYTE>& Script, const TArray<INT>& LegacyToNative, const TArray<FUE1AndroidScriptOffsetPatch64>& OffsetPatches )
{
	guard(UE1AndroidPatchScriptOffsets64);
	for( INT i=0; i<OffsetPatches.Num(); i++ )
	{
		INT LegacyOffset = OffsetPatches(i).LegacyOffset;
		if( LegacyOffset < 0 || LegacyOffset >= LegacyToNative.Num() || LegacyToNative(LegacyOffset) < 0 )
			appErrorf( "Bad arm64 script offset remap: legacy=%i", LegacyOffset );

		INT NativeOffset = LegacyToNative(LegacyOffset);
		if( OffsetPatches(i).Width == sizeof(_WORD) )
		{
			if( NativeOffset < 0 || NativeOffset > MAXWORD )
				appErrorf( "arm64 script offset too large: native=%i", NativeOffset );
			_WORD W = (_WORD)NativeOffset;
			UE1AndroidPatchScriptValue64( Script, OffsetPatches(i).NativeOffset, W );
		}
		else
		{
			UE1AndroidPatchScriptValue64( Script, OffsetPatches(i).NativeOffset, NativeOffset );
		}
	}
	unguard;
}
#endif

void UStruct::Serialize( FArchive& Ar )
{
	guard(UStruct::Serialize);
	UField::Serialize(Ar);

	// Serialize stuff.
	Ar << ScriptText << Children;
	Ar << FriendlyName << Line << TextPos;
	check(FriendlyName!=NAME_None);

	// Link the properties.
	if( Ar.IsLoading() )
		LinkOffsets( Ar );

	// Script code.
	INT ScriptSize = Script.Num();
	Ar << ScriptSize;
#if PLATFORM_64BIT
	if( Ar.IsLoading() )
	{
		Script.Empty();
		Script.AddZeroed( ScriptSize + 256 );
		TArray<INT> LegacyToNative;
		LegacyToNative.AddZeroed( ScriptSize + 1 );
		for( INT i=0; i<LegacyToNative.Num(); i++ )
			LegacyToNative(i) = -1;
		TArray<FUE1AndroidScriptOffsetPatch64> OffsetPatches;
		INT LegacyCode = 0;
		INT NativeCode = 0;
		while( LegacyCode < ScriptSize )
			UE1AndroidSerializeExpr64( this, Script, LegacyCode, NativeCode, Ar, LegacyToNative, OffsetPatches );
		if( LegacyCode == ScriptSize )
			LegacyToNative(ScriptSize) = NativeCode;
		UE1AndroidPatchScriptOffsets64( Script, LegacyToNative, OffsetPatches );
		UE1AndroidStoreScriptOffsetMap64( this, LegacyToNative );
		Script.SetNum( NativeCode );
		if( LegacyCode != ScriptSize )
			appErrorf( "Script serialization mismatch: Got legacy %i, expected %i", LegacyCode, ScriptSize );
	}
	else
#endif
	{
		Script.SetNum( ScriptSize );
		INT iCode = 0;
		while( iCode < ScriptSize )
			SerializeExpr( iCode, Ar );
		if( iCode != ScriptSize )
			appErrorf( "Script serialization mismatch: Got %i, expected %i", iCode, ScriptSize );
	}

	unguardobj;
}

//
// Actor reference cleanup.
//
#if PLATFORM_64BIT
static UBOOL UE1AndroidCleanupKnownObjectRef64( UObject* Object )
{
	if( !Object )
		return 0;
	const UPTRINT Value = (UPTRINT)Object;
	if( Value < 0x10000 || (Value & (sizeof(void*)-1)) != 0 )
		return 0;
	for( TObjectIterator<UObject> It; It; ++It )
		if( *It == Object )
			return 1;
	return 0;
}
#endif

void UStruct::CleanupDestroyed( BYTE* Data )
{
	guard(UStruct::CleanupDestroyed);
	if( GIsEditor )
	{
		// Slow cleanup.
		for( TFieldIterator<UProperty> It(this); It; ++It )
		{
			UProperty* Property = *It;
			if( Property->IsA(UObjectProperty::StaticClass) )
			{
				// Cleanup object reference.
				UObject** LinkedObjects = (UObject**)(Data + Property->Offset);
				for( INT k=0; k<Property->ArrayDim; k++ )
				{
					if( LinkedObjects[k] )
					{
#if PLATFORM_64BIT
						if( !UE1AndroidCleanupKnownObjectRef64( LinkedObjects[k] ) )
						{
							LinkedObjects[k] = NULL;
							continue;
						}
#else
						check(LinkedObjects[k]->IsValid());
#endif
						if( LinkedObjects[k]->IsPendingKill() )
						{
							// Remove this reference.
							LinkedObjects[k]->Modify();
							LinkedObjects[k] = NULL;
						}
					}
				}
			}
			else if( Property->GetClass()==UStructProperty::StaticClass )
			{
				// Cleanup substructure.
				for( INT k=0; k<Property->ArrayDim; k++ )
					((UStructProperty*)Property)->Struct->CleanupDestroyed( Data + Property->Offset + k*Property->GetElementSize() );
			}
		}
	}
	else
	{
		// Optimal cleanup.
		for( UObjectProperty* Ref=RefLink; Ref; Ref=Ref->NextReference )
		{
			UObject** LinkedObjects = (UObject**)(Data+Ref->Offset);
			for( INT k=0; k<Ref->ArrayDim; k++ )
			{
				if( LinkedObjects[k] )
				{
#if PLATFORM_64BIT
					if( !UE1AndroidCleanupKnownObjectRef64( LinkedObjects[k] ) )
					{
						LinkedObjects[k] = NULL;
						continue;
					}
#else
					check(LinkedObjects[k]->IsValid());
#endif
					if( LinkedObjects[k]->IsPendingKill() )
						LinkedObjects[k] = NULL;
				}
			}
		}
		for( UStructProperty* St=StructLink; St; St=St->NextStruct )
		{
			for( INT k=0; k<St->ArrayDim; k++ )
				St->Struct->CleanupDestroyed( Data + St->Offset + k*St->GetElementSize() );
		}
	}
	unguardobj;
}

IMPLEMENT_CLASS(UStruct);

/*-----------------------------------------------------------------------------
	UClass implementation.
-----------------------------------------------------------------------------*/

//
// Find the class's intrinsic constructor.
//
void UClass::Bind()
{
	guard(UClass::Bind);
	UStruct::Bind();
	if( GetFlags() & RF_Intrinsic )
	{
		// Find the intrinsic implementation.
		char ProcName[256];
		appSprintf( ProcName, "autoclass%s", GetNameCPP() );

		// Find export from the DLL.
		UPackage* ClassPackage = CastChecked<UPackage>(GetParent());
		UClass* ClassPtr = (UClass*)ClassPackage->GetDllExport( ProcName, 0 );
		if( ClassPtr )
		{
			if( !Constructor )
				Constructor = ClassPtr->Constructor;
#if PLATFORM_64BIT
			INT NativeSize = ClassPtr->GetNativePropertiesSize();
			if( NativeSize <= 0 )
				NativeSize = ClassPtr->GetPropertiesSize();
			SetNativePropertiesSize( NativeSize );
#endif
		}
		else if( !Constructor && !GIsEditor )
			appErrorf( "Can't bind to intrinsic class %s", GetPathName() );
	}
	if( !Constructor && GetSuperClass() )
	{
		// Chase down constructor in parent class.
		GetSuperClass()->Bind();
		Constructor = GetSuperClass()->Constructor;
	}
#if PLATFORM_64BIT
	EnsureNativePropertiesSize();
#endif
	unguardobj;
}

/*-----------------------------------------------------------------------------
	UClass UObject implementation.
-----------------------------------------------------------------------------*/

static void RecursiveTagNames( UClass* Class )
{
	guard(RecursiveTagNames);
	if( (Class->GetFlags() & RF_TagExp) && (Class->GetFlags() & RF_Intrinsic) )
		for( TFieldIterator<UFunction> Function(Class); Function && Function.GetStruct()==Class; ++Function )
			if
			(	(Function->FunctionFlags & FUNC_Event)
			&&	!Function->GetSuperFunction() )
				Function->GetFName().SetFlags( RF_TagExp );
	for( TObjectIterator<UClass> It; It; ++It )
		if( It->GetSuperClass()==Class )
			RecursiveTagNames( *It );
	unguard;
}

void UClass::Export( FOutputDevice& Out, const char* FileType, int Indent )
{
	guard(UClass::Export);
	static int RecursionDepth=0, DidTop, i;
	if( appStricmp(FileType,"H")==0 )
	{
		char API[256];
		appStrcpy( API, GetParent()->GetName() );
		appStrupr( API );

		// Export as C++ header.
		if( RecursionDepth==0 )
		{
			DidTop = 0;
			RecursiveTagNames( this );
		}

		// Export this.
		if( (GetFlags() & RF_TagExp) && (GetFlags() & RF_Intrinsic) )
		{
			// Top of file.
			if( !DidTop )
			{
				DidTop=1;
				Out.Logf
				(
					"/*===========================================================================\r\n"
					"	C++ class definitions exported from UnrealScript.\r\n"
					"\r\n"
					"   This is automatically generated using 'Unreal.exe -make -h'\r\n"
					"   DO NOT modify this manually! Edit the corresponding .uc files instead!\r\n"
					"===========================================================================*/\r\n"
					"#pragma pack (push,%i)\r\n"
					"\r\n"
					"#ifndef %s_API\r\n"
					"#define %s_API DLL_IMPORT\r\n"
					"#endif\r\n"
					"\r\n"
					"#ifndef NAMES_ONLY\r\n"
					"#define DECLARE_NAME(name) extern %s_API FName %s_##name;\r\n"
					"#endif\r\n"
					"\r\n",
					PROPERTY_ALIGNMENT,
					API,
					API,
					API,
					API
				);
				for( INT i=0; i<FName::GetMaxNames(); i++ )
					if( FName::GetEntry(i) && (FName::GetEntry(i)->Flags & RF_TagExp) )
						Out.Logf( "DECLARE_NAME(%s)\r\n", *FName((EName)(i)) );
				for( i=0; i<FName::GetMaxNames(); i++ )
					if( FName::GetEntry(i) )
						FName::GetEntry(i)->Flags &= ~RF_TagExp;
				Out.Logf( "\r\n#ifndef NAMES_ONLY\r\n\r\n" );
			}

			// Enum definitions.
			for( TFieldIterator<UEnum> ItE(this); ItE && ItE.GetStruct()==this; ++ItE )
			{
				// Export enum.
				if( ItE->GetParent()==this )
					ItE->Export( Out, FileType, Indent );
				else
					Out.Logf( "enum %s;\r\n", ItE->GetName() );
			}

			// Struct definitions.
			for( TFieldIterator<UStruct> ItS(this); ItS && ItS.GetStruct()==this; ++ItS )
			{
				if( ItS->GetFlags() & RF_Intrinsic )
				{
					// Export struct.
					Out.Logf( "struct %s_API %s", API, ItS->GetNameCPP() );
					if( ItS->SuperField )
						Out.Logf(" : public %s\r\n", ItS->GetSuperStruct()->GetNameCPP() );
					Out.Logf("\r\n{\r\n" );
					for( TFieldIterator<UProperty> It2(*ItS); It2; ++It2 )
					{
						if( It2.GetStruct()==*ItS )
						{
							Out.Logf( appSpc(Indent+4) );
							It2->ExportCPP( Out, 0, 0 );
							Out.Logf( ";\r\n" );
						}
					}
					Out.Logf("};\r\n\r\n" );
				}
			}

			// Constants.
			INT Consts=0;
			for( TFieldIterator<UConst> ItC(this); ItC && ItC.GetStruct()==this; ++ItC )
				Out.Logf( "#define UCONST_%s %s\r\n", ItC->GetName(), *ItC->Value ),Consts++;
			if( Consts )
				Out.Logf( "\r\n" );

			// Class definition.
			Out.Logf( "class %s_API %s", API, GetNameCPP() );
			if( GetSuperClass() )
				Out.Logf(" : public %s\r\n", GetSuperClass()->GetNameCPP() );
			Out.Logf("{\r\npublic:\r\n" );

			// All per-object properties defined in this class.
			for( TFieldIterator<UProperty> It(this); It; ++It )
			{
				if( It.GetStruct()==this && It->GetElementSize() )
				{
					Out.Logf( appSpc(Indent+4) );
					It->ExportCPP( Out, 0, 0 );
					Out.Logf( ";\r\n" );
				}
			}

			// C++ -> UnrealScript stubs.
			TFieldIterator<UFunction> Function(this);
			for( ; Function && Function.GetStruct()==this; ++Function )
			{
				if( Function->FunctionFlags & FUNC_Intrinsic )
				{
					Out.Logf( "    void exec%s( FFrame& Stack, BYTE*& Result )", Function->GetName() );
					Out.Logf( ";\r\n", Function->GetName() );
				}
			}

			// UnrealScript -> C++ proxies.
			for( Function = TFieldIterator<UFunction>(this); Function && Function.GetStruct()==this; ++Function )
			{
				if
				(	(Function->FunctionFlags & FUNC_Event)
				&&	(!Function->GetSuperFunction()) )
				{
					// Return type.
					UProperty* Return = Function->GetReturnProperty();
					Out.Log( "    " );
					if( !Return )
						Out.Log( "void" );
					else
						Return->ExportCPPItem( Out );

					// Function name and parms.
					INT ParmCount=0;
					Out.Logf( " event%s(", Function->GetName() );
					TFieldIterator<UProperty> It(*Function);
					for( ; It && (It->PropertyFlags&(CPF_Parm|CPF_ReturnParm))==CPF_Parm; ++It )
					{
						if( ParmCount++ )
							Out.Log(", ");
						It->ExportCPP( Out, 0, 1 );
					}
					Out.Log( ")\r\n" );

					// Function call.
					Out.Log( "    {\r\n" );
					if( ParmCount )
					{
						// Parms struct definition.
						Out.Log( "        struct {" );
						for( It=TFieldIterator<UProperty>(*Function); It && (It->PropertyFlags&CPF_Parm); ++It )
						{
							It->ExportCPP( Out, 1, 0 );
							Out.Log( "; " );
						}
						Out.Log( "} Parms;\r\n");

						// Parms struct initialization.
						for( It=TFieldIterator<UProperty>(*Function); It && (It->PropertyFlags&(CPF_Parm|CPF_ReturnParm))==CPF_Parm; ++It )
						{
							UStringProperty* StringProp = Cast<UStringProperty>( *It );
							if( StringProp )
								Out.Logf( "        appStrncpy(Parms.%s,%s,%i);\r\n", It->GetName(), It->GetName(), StringProp->StringSize );
							else
								Out.Logf( "        Parms.%s=%s;\r\n", It->GetName(), It->GetName() );
						}
						if( Return )
							Out.Logf( "        Parms.%s=0;\r\n", Return->GetName() );
						Out.Logf( "        ProcessEvent(FindFunctionChecked(%s_%s),&Parms);\r\n", API, Function->GetName() );
					}
					else Out.Logf( "        ProcessEvent(FindFunctionChecked(%s_%s),NULL);\r\n", API, Function->GetName() );

					// Out parm copying.
					for( It=TFieldIterator<UProperty>(*Function); It && (It->PropertyFlags&(CPF_Parm|CPF_ReturnParm))==CPF_Parm; ++It )
					{
						if( It->PropertyFlags & CPF_OutParm )
						{
							if( It->IsA(UStringProperty::StaticClass) )
								Out.Logf( "        appStrcpy(%s,Parms.%s);\r\n", It->GetName(), It->GetName() );
							else
								Out.Logf( "        %s=Parms.%s;\r\n", It->GetName(), It->GetName() );
						}
					}

					// Return value.
					if( Return )
						Out.Logf( "        return Parms.%s;\r\n", Return->GetName() );
					Out.Log( "    }\r\n" );
				}
			}

			// Code.
			if( 1 )
			{
				Out.Logf( "    DECLARE_CLASS(%s,", GetNameCPP() ); //warning: GetNameCPP uses static storage.
				Out.Logf( "%s,0", GetSuperClass()->GetNameCPP() );
				if( ClassFlags & CLASS_Transient      )
					Out.Log("|CLASS_Transient"      );
				if( ClassFlags & CLASS_Config     )
					Out.Log("|CLASS_Config"     );
				Out.Logf( ")\r\n" );
				char Filename[256];
				appSprintf( Filename, "..\\%s\\Inc\\%s.h", GetParent()->GetName(), GetNameCPP() );
				if( appFSize(Filename) > 0 )
					Out.Logf( "    #include \"%s.h\"\r\n", GetNameCPP() );
				else
					Out.Logf( "    NO_DEFAULT_CONSTRUCTOR(%s)\r\n", GetNameCPP() );
			}
			Out.Logf( "};\r\n\r\n" );
		}

		// Export all child classes that are tagged for export.
		RecursionDepth++;
		for( TObjectIterator<UClass> It; It; ++It )
			if( It->GetSuperClass()==this )
				It->Export( Out, FileType, Indent );
		RecursionDepth--;

		// Finish C++ header.
		if( RecursionDepth==0 )
		{
			Out.Logf( "#undef DECLARE_NAME\r\n" );
			Out.Logf( "#endif\r\n" );
			Out.Logf( "#pragma pack (pop)\r\n" );
		}
	}
	else if( appStricmp(FileType,"UC")==0 )
	{
		// Export script text.
		check(Defaults.Num());
		check(ScriptText!=NULL);
		ScriptText->Export( Out, FileType, Indent );

		// Export default properties that differ from parent's.
		Out.Log( "\r\n\r\ndefaultproperties\r\n{\r\n" );
		GObj.ExportProperties
		(
			this,
			&Defaults(0),
			&Out,
			Indent+4,
			GetSuperClass(),
			GetSuperClass() ? &GetSuperClass()->Defaults(0) : NULL
		);
		Out.Log( "}\r\n" );
	}
	unguardobj;
}
#if PLATFORM_64BIT
struct FUE1NativeClassSizeEntry
{
	FName PackageName;
	FName ClassName;
	INT   NativeSize;
	FUE1NativeClassSizeEntry* Next;
};

static FUE1NativeClassSizeEntry* GUE1NativeClassSizeList = NULL;

void UClass::RegisterNativePropertiesSize( FName PackageName, FName ClassName, INT NativeSize )
{
	guard(UClass::RegisterNativePropertiesSize);
	if( NativeSize <= 0 || ClassName == NAME_None )
		return;
	for( FUE1NativeClassSizeEntry* It=GUE1NativeClassSizeList; It; It=It->Next )
	{
		if( It->PackageName==PackageName && It->ClassName==ClassName )
		{
			if( NativeSize > It->NativeSize )
				It->NativeSize = NativeSize;
			return;
		}
	}
	FUE1NativeClassSizeEntry* Entry = new FUE1NativeClassSizeEntry;
	Entry->PackageName = PackageName;
	Entry->ClassName   = ClassName;
	Entry->NativeSize  = NativeSize;
	Entry->Next        = GUE1NativeClassSizeList;
	GUE1NativeClassSizeList = Entry;
	unguard;
}

INT UClass::FindRegisteredNativePropertiesSize() const
{
	guard(UClass::FindRegisteredNativePropertiesSize);
	FName PackageName = GetParent() ? GetParent()->GetFName() : NAME_None;
	FName ClassName   = GetFName();
	INT ExactResult = 0;
	INT ClassNameFallback = 0;
	for( FUE1NativeClassSizeEntry* It=GUE1NativeClassSizeList; It; It=It->Next )
	{
		if( It->ClassName==ClassName )
		{
			ClassNameFallback = Max( ClassNameFallback, It->NativeSize );
			if( It->PackageName==PackageName || PackageName==NAME_None )
				ExactResult = Max( ExactResult, It->NativeSize );
		}
	}
	return ExactResult ? ExactResult : ClassNameFallback;
	unguardobj;
}

void UClass::SetNativePropertiesSize( INT NewSize )
{
	guard(UClass::SetNativePropertiesSize);

	if( NewSize > NativePropertiesSize )
		NativePropertiesSize = NewSize;
	if( NativePropertiesSize > GetPropertiesSize() )
		SetPropertiesSize( NativePropertiesSize );
	if( Defaults.Num() && Defaults.Num() < GetPropertiesSize() )
	{
		INT OldNum = Defaults.Num();
		Defaults.SetNum( GetPropertiesSize() );
		appMemset( &Defaults(OldNum), 0, Defaults.Num() - OldNum );
		if( Defaults.Num() >= (INT)sizeof(UObject) )
			((UObject*)&Defaults(0))->SetClass( this );
	}

	unguardobj;
}

void UClass::ForceNativePropertiesSize( INT NewSize )
{
	guard(UClass::ForceNativePropertiesSize);

	if( NewSize <= 0 )
		return;

	NativePropertiesSize = NewSize;
	if( GetPropertiesSize() != NewSize )
		SetPropertiesSize( NewSize );

	if( Defaults.Num() )
	{
		INT OldNum = Defaults.Num();
		if( OldNum != NewSize )
		{
			Defaults.SetNum( NewSize );
			if( NewSize > OldNum )
				appMemset( &Defaults(OldNum), 0, NewSize - OldNum );
		}
		if( Defaults.Num() >= (INT)sizeof(UObject) )
			((UObject*)&Defaults(0))->SetClass( this );
	}

	unguardobj;
}

void UClass::EnsureNativePropertiesSize()
{
	guard(UClass::EnsureNativePropertiesSize);
	INT RegisteredNativeSize = FindRegisteredNativePropertiesSize();
	if( RegisteredNativeSize > 0 )
	{
		ForceNativePropertiesSize( RegisteredNativeSize );
	}
	else if( NativePropertiesSize > 0 )
	{
		SetNativePropertiesSize( NativePropertiesSize );
	}
	unguardobj;
}
#endif

void UClass::Destroy()
{
	guard(UClass::Destroy);

	// Free replication links.
	FRepLink* Next;
	for( FRepLink* Link=Reps; Link; Link=Next )
	{
		Next = Link->Next;
		delete Link;
	}

	Super::Destroy();
	unguard;
}
void UClass::PostLoad()
{
	guard(UClass::PostLoad);
	Super::PostLoad();

	// Bind with C++.
	Bind();
#if PLATFORM_64BIT
	EnsureNativePropertiesSize();
#endif

	// Update default bin.
	if( Defaults.Num() )
	{
		*((void**)&Defaults(0)) = *((void**)GetParent());
		((UObject*)&Defaults(0))->SetClass( this );
	}

	// Build linked list of replicated properties.
	if( !Reps )
	{
		for( TFieldIterator<UProperty> It(this); It && It->GetOwnerClass()==this; ++It )
		{
			if( It->PropertyFlags & CPF_Net )
			{
				Reps = new FRepLink( *It, Reps );
				for( FRepLink* Other=Reps; Other; Other=Other->Next )
					if( Reps->Property->RepOffset==Other->Property->RepOffset )
						Reps->Condition = Other;
			}
		}
	}
	unguardobj;
}
void UClass::Serialize( FArchive& Ar )
{
	guard(UClass::Serialize);
#if PLATFORM_64BIT
	// UE1 packages store native class metadata with 32-bit property sizes.
	// On arm64 the real native sizeof(TClass) must survive class replacement
	// and UStruct::LinkOffsets(), otherwise native objects such as UFireTexture
	// are allocated too small before PostLoad ever runs.
	INT SavedNativePropertiesSize64 = NativePropertiesSize;
#endif
	UState::Serialize( Ar );
#if PLATFORM_64BIT
	if( Ar.IsLoading() && SavedNativePropertiesSize64 > 0 )
		SetNativePropertiesSize( SavedNativePropertiesSize64 );
#endif

	// Variables.
	Ar << ClassRecordSize << ClassFlags << ClassGuid;
	Ar << Dependencies << PackageImports;

	// Defaults.
	if( Ar.IsLoading() )
	{
#if PLATFORM_64BIT
		if( GetPropertiesSize() < (INT)sizeof(UObject) )
			SetPropertiesSize( sizeof(UObject) );
#endif
		Defaults.SetNum(GetPropertiesSize());
		check(Defaults.Num()>=sizeof(UObject));
		if( GetSuperClass() )
			Ar.Preload( GetSuperClass() );
		appMemset( &Defaults(0), 0, sizeof(UObject) );
		GObj.InitProperties( this, Defaults.Num() ? &Defaults(0) : NULL, Defaults.Num(), GetSuperClass(), NULL, 0 );
		DefaultPropText = NULL;
		ClassUnique = 0;
		SerializeTaggedProperties( Ar, &Defaults(0), GetSuperClass() );
		GetDefaultObject()->SetClass(this);
		GetDefaultObject()->LoadConfig(NAME_Config);
		GetDefaultObject()->LoadConfig(NAME_Localized);
	}
	else if( Ar.IsSaving() )
	{
		check(Defaults.Num()==0 || Defaults.Num()==GetPropertiesSize());
		if( Defaults.Num() )
			SerializeTaggedProperties( Ar, &Defaults(0), GetSuperClass() );
	}
	else
	{
		Ar << DefaultPropText;
		if( Defaults.Num() )
		{
			Defaults.SetNum(GetPropertiesSize());
			SerializeBin( Ar, &Defaults(0) );
		}
	}
#if PLATFORM_64BIT
	if( Ar.IsLoading() )
		EnsureNativePropertiesSize();
#endif
	if( Ar.Ver() < 57 )//oldver
	{
		FName X, Y;
		Ar << X << Y;
	}
	unguardobj;
}

/*-----------------------------------------------------------------------------
	UClass constructors.
-----------------------------------------------------------------------------*/

//
// Create a new UClass given its parent.
//
UClass::UClass( UClass* InBaseClass )
:	UState( InBaseClass )
#if PLATFORM_64BIT
,	NativePropertiesSize( 0 )
#endif
{
	guard(UClass::UClass);

	// Copy defaults from superclass.
	if( GetSuperClass() )
		Defaults = GetSuperClass()->Defaults;

	// Find C++ class, if any.
	if( GetSuperClass() )
	{
		Bind();		
		ClassRecordSize = GetSuperClass()->ClassRecordSize;
	}

	// Done creating class.
	unguardobj;
}

//
// UClass autoregistry constructor.
//warning: Called at DLL init time.
//
UClass::UClass
(
	DWORD		InSize,
	DWORD		InRecordSize,
	DWORD		InClassFlags,
	UClass*		InSuperClass,
	FGuid		InGuid,
	const char*	InNameStr,
	FName		InPackageName,
	void		(*InConstructor)(void*),
	void		(*InClassInitializer)(UClass*)
)
:	UState( EC_IntrinsicConstructor, InSize, FName(InNameStr+1), InPackageName )
,	Reps  ( NULL )
#if PLATFORM_64BIT
,	NativePropertiesSize( InSize )
#endif
{
	guard(UClass::UClass);

	// Set object flags.
	SetFlags( RF_Public | RF_Standalone | RF_Transient | RF_Intrinsic );

	// Set info.
	Constructor			= InConstructor;
	ClassInitializer    = InClassInitializer;
	ClassRecordSize		= InRecordSize;
	ClassFlags			= InClassFlags | CLASS_Parsed | CLASS_Compiled;
	SuperField			= InSuperClass!=this ? InSuperClass : NULL;
	ClassGuid			= InGuid;

	// Init defaults.
	INT NativeDefaultSize = InSize;
#if PLATFORM_64BIT
	if( NativeDefaultSize < (INT)sizeof(UObject) )
		NativeDefaultSize = sizeof(UObject);
	NativePropertiesSize = NativeDefaultSize;
	RegisterNativePropertiesSize( InPackageName, GetFName(), NativeDefaultSize );
	ForceNativePropertiesSize( NativeDefaultSize );
#endif
	Defaults.SetNum( NativeDefaultSize );
	appMemset( &Defaults(0), 0, NativeDefaultSize );
	((UObject*)&Defaults(0))->SetClass( this );

	// Call the initializer.
	if( GObj.GetInitialized() )
	{
		ClassInitializer( this );
		GetDefaultObject()->LoadConfig( NAME_Config );
		GetDefaultObject()->LoadConfig( NAME_Localized );
	}

	unguard;
}

IMPLEMENT_CLASS(UClass);

/*-----------------------------------------------------------------------------
	FDependency.
-----------------------------------------------------------------------------*/

//
// FDepdendency inlines.
//
FDependency::FDependency()
{}
FDependency::FDependency( UClass* InClass, UBOOL InDeep )
:	Class( InClass )
,	Deep( InDeep )
,	ScriptTextCRC( Class ? Class->GetScriptTextCRC() : 0 )
{}
UBOOL FDependency::IsUpToDate()
{
	guard(FDependency::IsUpToDate);
	check(Class!=NULL);
	return Class->GetScriptTextCRC()==ScriptTextCRC;
	unguard;
}
CORE_API FArchive& operator<<( FArchive& Ar, FDependency& Dep )
{
	return Ar << Dep.Class << Dep.Deep << Dep.ScriptTextCRC;
}

/*-----------------------------------------------------------------------------
	FLabelEntry.
-----------------------------------------------------------------------------*/

FLabelEntry::FLabelEntry( FName InName, INT iInCode )
:	Name	(InName)
,	iCode	(iInCode)
{}
CORE_API FArchive& operator<<( FArchive& Ar, FLabelEntry &Label )
{
	Ar << Label.Name;
	Ar << Label.iCode;
	return Ar;
}

/*-----------------------------------------------------------------------------
	UStruct implementation.
-----------------------------------------------------------------------------*/

//
// Serialize an expression to an archive.
// Returns expression token.
//
EExprToken UStruct::SerializeExpr( INT& iCode, FArchive& Ar )
{
	EExprToken Expr=(EExprToken)0;
	guard(SerializeExpr);
	#define XFER(T) {Ar << *(T*)&Script(iCode); iCode += sizeof(T); }

	// Get expr token.
	XFER(BYTE);
	Expr = (EExprToken)Script(iCode-1);
	if( Expr >= EX_MinConversion && Expr < EX_MaxConversion )
	{
		// A type conversion.
		SerializeExpr( iCode, Ar );
	}
	else if( Expr >= EX_FirstIntrinsic )
	{
		// Intrinsic final function with id 1-127.
		while( SerializeExpr( iCode, Ar ) != EX_EndFunctionParms );
	}
	else if( Expr >= EX_ExtendedIntrinsic )
	{
		// Intrinsic final function with id 128-16383.
		XFER(BYTE);
		while( SerializeExpr( iCode, Ar ) != EX_EndFunctionParms );
	}
	else switch( Expr )
	{
		case EX_LocalVariable:
		case EX_InstanceVariable:
		case EX_DefaultVariable:
		{
			XFER(UProperty*);
			break;
		}
		case EX_BoolVariable:
		case EX_ValidateObject:
		case EX_Nothing:
		case EX_EndFunctionParms:
		case EX_IntZero:
		case EX_IntOne:
		case EX_True:
		case EX_False:
		case EX_NoObject:
		case EX_Self:
		case EX_IteratorPop:
		case EX_EndCode:
		case EX_Stop:
		case EX_Return:
		case EX_IteratorNext:
		{
			break;
		}
		case EX_IntrinsicParm:
		{
			XFER(UProperty*);
			break;
		}
		case EX_ClassContext:
		case EX_Context:
		{
			SerializeExpr( iCode, Ar ); // Object expression.
			XFER(_WORD); // Code offset for NULL expressions.
			XFER(BYTE); // Zero-fill size if skipped.
			SerializeExpr( iCode, Ar ); // Context expression.
			break;
		}
		case EX_ArrayElement:
		{
			SerializeExpr( iCode, Ar ); // Index expression.
			SerializeExpr( iCode, Ar ); // Base expression.
			break;
		}
		case EX_VirtualFunction:
		case EX_GlobalFunction:
		{
			XFER(FName); // Virtual function name.
			while( SerializeExpr( iCode, Ar ) != EX_EndFunctionParms ); // Parms.
			break;
		}
		case EX_FinalFunction:
		{
			XFER(UStruct*); // Stack node.
			while( SerializeExpr( iCode, Ar ) != EX_EndFunctionParms ); // Parms.
			break;
		}
		case EX_IntConst:
		{
			XFER(INT);
			break;
		}
		case EX_FloatConst:
		{
			XFER(FLOAT);
			break;
		}
		case EX_StringConst:
		{
			do XFER(BYTE) while( Script(iCode-1) );
			break;
		}
		case EX_ObjectConst:
		{
			XFER(UObject*);
			break;
		}
		case EX_NameConst:
		{
			XFER(FName);
			break;
		}
		case EX_RotationConst:
		{
			XFER(INT); XFER(INT); XFER(INT);
			break;
		}
		case EX_VectorConst:
		{
			XFER(FLOAT); XFER(FLOAT); XFER(FLOAT);
			break;
		}
		case EX_ByteConst:
		case EX_IntConstByte:
		{
			XFER(BYTE);
			break;
		}
		case EX_ResizeString:
		{
			XFER(BYTE);
			SerializeExpr( iCode, Ar );
			break;
		}
		case EX_MetaCast:
		{
			XFER(UClass*);
			SerializeExpr( iCode, Ar );
			break;
		}
		case EX_DynamicCast:
		{
			XFER(UClass*);
			SerializeExpr( iCode, Ar );
			break;
		}
		case EX_JumpIfNot:
		{
			XFER(_WORD); // Code offset.
			SerializeExpr( iCode, Ar ); // Boolean expr.
			break;
		}
		case EX_Iterator:
		{
			SerializeExpr( iCode, Ar ); // Iterator expr.
			XFER(_WORD); // Code offset.
			break;
		}
		case EX_Switch:
		{
			XFER(BYTE); // Value size.
			SerializeExpr( iCode, Ar ); // Switch expr.
			break;
		}
		case EX_Jump:
		{
			XFER(_WORD); // Code offset.
			break;
		}
		case EX_Assert:
		{
			XFER(_WORD); // Line number.
			SerializeExpr( iCode, Ar ); // Assert expr.
			break;
		}
		case EX_Case:
		{
			_WORD *W=(_WORD*)&Script(iCode);
			XFER(_WORD);; // Code offset.
			if( *W != MAXWORD )
				SerializeExpr( iCode, Ar ); // Boolean expr.
			break;
		}
		case EX_LabelTable:
		{
			check((iCode&3)==0);
			for( ; ; )
			{
				FLabelEntry* E = (FLabelEntry*)&Script(iCode);
				XFER(FLabelEntry);
				if( E->Name == NAME_None )
					break;
			}
			break;
		}
		case EX_GotoLabel:
		{
			SerializeExpr( iCode, Ar ); // Label name expr.
			break;
		}
		case EX_Let:
		{
			SerializeExpr( iCode, Ar ); // Variable expr.
			SerializeExpr( iCode, Ar ); // Assignment expr.
			break;
		}
		case EX_Skip:
		{
			XFER(_WORD); // Skip size.
			SerializeExpr( iCode, Ar ); // Expression to possibly skip.
			break;
		}
		case EX_BeginFunction:
		{
			for( ; ; )
			{
				XFER(BYTE); // Parm size.
				if( Script(iCode-1) == 0 )
					break;
				XFER(BYTE); // OutParm flag.
			}
			break;
		}
		case EX_StructCmpEq:
		case EX_StructCmpNe:
		{
			XFER(UStruct*); // Struct.
			SerializeExpr( iCode, Ar ); // Left expr.
			SerializeExpr( iCode, Ar ); // Right expr.
			break;
		}
		case EX_StructMember:
		{
			XFER(UProperty*); // Property.
			SerializeExpr( iCode, Ar ); // Inner expr.
			break;
		}
		default:
		{
			// This should never occur.
			appErrorf( "Bad expr token %02x", Expr );
			break;
		}
	}
	return Expr;
	#undef GRAB
	unguardf(( "(%02X)", Expr ));
}

void UStruct::PostLoad()
{
	guard(UStruct::PostLoad);
	UField::PostLoad();
	if( !GIsEditor )
		BuildVfHashes( this );
	unguard;
}

/*-----------------------------------------------------------------------------
	UFunction.
-----------------------------------------------------------------------------*/

UFunction::UFunction( UFunction* InSuperFunction )
: UStruct( InSuperFunction )
{}
void UFunction::Serialize( FArchive& Ar )
{
	guard(UFunction::Serialize);
	UStruct::Serialize( Ar );

	// Function info.
	Ar << ParmsSize << iIntrinsic;
	Ar << NumParms << OperPrecedence;
	Ar << ReturnValueOffset << FunctionFlags;

	// Replication info.
	if( FunctionFlags & FUNC_Net )
		Ar << RepOffset;

#if PLATFORM_64BIT
	if( Ar.IsLoading() )
	{
		// UE1 packages store UFunction parameter frame sizes for 32-bit.
		// On arm64, object/class parameters and return values are pointer-sized.
		// Recompute ParmsSize and ReturnValueOffset from the freshly linked
		// property offsets so native event wrappers such as GameInfo.Login copy
		// the full 64-bit SpawnClass/ReturnValue frame instead of the old 32-bit
		// tail. This fixes corrupted class pointers reaching AActor::execSpawn.
		INT NativeParmsSize = 0;
		INT NativeReturnValueOffset = MAXWORD;
		for( TFieldIterator<UProperty> It(this); It && It.GetStruct()==this; ++It )
		{
			UProperty* Property = *It;
			if( Property->PropertyFlags & CPF_Parm )
			{
				NativeParmsSize = Max( NativeParmsSize, Property->Offset + Property->GetSize() );
				if( Property->PropertyFlags & CPF_ReturnParm )
					NativeReturnValueOffset = Property->Offset;
			}
		}
		if( NativeParmsSize > MAXWORD )
			appErrorf( "arm64 UFunction parameter frame too large: %s size=%i", GetFullName(), NativeParmsSize );
		if( NativeParmsSize > 0 )
			ParmsSize = (_WORD)NativeParmsSize;
		if( NativeReturnValueOffset != MAXWORD )
		{
			if( NativeReturnValueOffset > MAXWORD )
				appErrorf( "arm64 UFunction return offset too large: %s offset=%i", GetFullName(), NativeReturnValueOffset );
			ReturnValueOffset = (_WORD)NativeReturnValueOffset;
		}
	}
#endif

	unguard;
}
void UFunction::PostLoad()
{
	guard(UFunction::PostLoad);
	UStruct::PostLoad();
	unguard;
}
UProperty* UFunction::GetReturnProperty()
{
	guard(UFunction::GetReturnProperty);
	for( TFieldIterator<UProperty> It(this); It && (It->PropertyFlags & CPF_Parm); ++It )
		if( It->PropertyFlags & CPF_ReturnParm )
			return *It;
	return NULL;
	unguard;
}
void UFunction::Bind()
{
	guard(UFunction::Bind);
	if( FunctionFlags & FUNC_Intrinsic )
	{
		if( iIntrinsic != 0 )
		{
			// Find hardcoded intrinsic.
			check(iIntrinsic<EX_Max);
			check(GIntrinsics[iIntrinsic]!=NULL);
			Func = GIntrinsics[iIntrinsic];
		}
		else
		{
			// Find dynamic intrinsic.
			char Proc[256];
			appSprintf( Proc, "int%sexec%s", GetOwnerClass()->GetNameCPP(), GetName() );
			UPackage* ClassPackage = CastChecked<UPackage>( GetOwnerClass()->GetParent() );
			void** Ptr = (void**)ClassPackage->GetDllExport( Proc, 1 );
			if( Ptr )
				*(void**)&Func = *Ptr;
		}
	}
	else check(iIntrinsic==0);
	unguard;
}
IMPLEMENT_CLASS(UFunction);

/*-----------------------------------------------------------------------------
	UState.
-----------------------------------------------------------------------------*/

UState::UState( UState* InSuperState )
: UStruct( InSuperState )
{}
UState::UState( EIntrinsicConstructor, INT InSize, FName InName, FName InPackageName )
:	UStruct( EC_IntrinsicConstructor, InSize, InName, InPackageName )
,	ProbeMask( 0 )
,	IgnoreMask( 0 )
,	VfHash( NULL )
,	StateFlags( 0 )
,	LabelTableOffset( 0 )
{}
void UState::Destroy()
{
	guard(UState::Destroy);
	if( VfHash )
		delete VfHash;
	UStruct::Destroy();
	unguard;
}
void UState::Serialize( FArchive& Ar )
{
	guard(UState::Serialize);
	UStruct::Serialize( Ar );

	// Class/State-specific union info.
	Ar << ProbeMask << IgnoreMask;
	Ar << LabelTableOffset << StateFlags;
#if PLATFORM_64BIT
	if( Ar.IsLoading() && LabelTableOffset != MAXWORD )
		LabelTableOffset = (_WORD)UE1AndroidRemapScriptOffset64( this, LabelTableOffset );
#endif
	if( Ar.IsLoading() )
		VfHash = NULL;

	unguard;
}
IMPLEMENT_CLASS(UState);

/*-----------------------------------------------------------------------------
	UConst.
-----------------------------------------------------------------------------*/

UConst::UConst( UConst* InSuperConst, const char* InValue )
:	UField( InSuperConst )
,	Value( InValue )
{}
void UConst::Serialize( FArchive& Ar )
{
	guard(UConst::Serialize);
	UField::Serialize( Ar );
	Ar << Value;
	unguard;
}
IMPLEMENT_CLASS(UConst);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
