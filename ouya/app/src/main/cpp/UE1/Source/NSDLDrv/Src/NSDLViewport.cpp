#ifndef AndroidOuyaAlignX
#define AndroidOuyaAlignX(x) (x)
#endif
#ifndef UNREAL_ANDROID_OUYA_INTERNAL_RENDER_W
#define UNREAL_ANDROID_OUYA_INTERNAL_RENDER_W 960
#endif

#ifndef UNREAL_ANDROID_OUYA_INTERNAL_RENDER_H
#define UNREAL_ANDROID_OUYA_INTERNAL_RENDER_H 540
#endif

// OUYA hotfix:
// The previous 960x540 path only changed UE1/SDL viewport sizes but did not implement
// a real GLES framebuffer/downscale pass. On Tegra3/OUYA this can cause unstable,
// flickering garbage output. For now use the stable generic Android drawable-size path.
// A real 960x540 render target must later be implemented inside the GLES renderer.
#ifdef UNREAL_ANDROID_OUYA
#undef UNREAL_ANDROID_OUYA
#define UNREAL_ANDROID_OUYA_VIEWPORT_HOTFIX_DISABLED 1
#endif

#include <string.h>
#include <ctype.h>

#include "NSDLDrv.h"
#include "UnRender.h"
#include "NSDLOuyaResolutionMenu.h"

IMPLEMENT_CLASS( UNSDLViewport );

#if PLATFORM_ANDROID
extern "C" int UE1AndroidShouldIgnoreEarlyQuit();
#endif


#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
struct FAndroidGammaMode
{
	FLOAT Value;
	const char* Label;
};

static const FAndroidGammaMode GAndroidGammaModes[] =
{
	{ 1.00f, "Default"    },
	{ 1.10f, "Default +1" },
	{ 1.20f, "Default +2" },
	{ 1.30f, "Default +3" },
	{ 1.40f, "Default +4" },
};

static FLOAT UE1AndroidGetConfiguredGammaModeValue()
{
	guard(UE1AndroidGetConfiguredGammaModeValue);

	char Value[64];
	if( GConfigCache.GetString( "NSDLDrv.NSDLClient", "Gamma", Value, sizeof(Value) ) )
		return Clamp( appAtof( Value ), 0.5f, 3.0f );

	if( GConfigCache.GetString( "NOpenGLESDrv.NOpenGLESRenderDevice", "WorldGamma", Value, sizeof(Value) ) )
		return Clamp( appAtof( Value ), 0.5f, 3.0f );

	return 1.0f;

	unguard;
}

static INT UE1AndroidGetNearestGammaModeIndex()
{
	guard(UE1AndroidGetNearestGammaModeIndex);

	const FLOAT Current = UE1AndroidGetConfiguredGammaModeValue();
	INT Nearest = 0;
	FLOAT BestDiff = 999.0f;
	for( INT i=0; i<(INT)(sizeof(GAndroidGammaModes)/sizeof(GAndroidGammaModes[0])); ++i )
	{
		const FLOAT Diff = ( Current > GAndroidGammaModes[i].Value ) ? ( Current - GAndroidGammaModes[i].Value ) : ( GAndroidGammaModes[i].Value - Current );
		if( Diff < BestDiff )
		{
			BestDiff = Diff;
			Nearest = i;
		}
	}
	return Nearest;

	unguard;
}

static void UE1AndroidBuildGammaMenuLabel( char* OutLabel, INT OutSize )
{
	guard(UE1AndroidBuildGammaMenuLabel);

	if( !OutLabel || OutSize <= 0 )
		return;

	const INT ModeIndex = UE1AndroidGetNearestGammaModeIndex();

	// Match the classic UE1 menu layout: option name on the left,
	// current value in the AUDIO/VIDEO value column. The current Android 8
	// menu font/layout needs two more spaces here to line up with Texture Detail.
	appSprintf( OutLabel, "Toggle Gamma              %s", GAndroidGammaModes[ModeIndex].Label );
	OutLabel[OutSize-1] = 0;

	unguard;
}

static UBOOL UE1AndroidStringLooksLikeGammaFullscreenMenuItem( const char* Text )
{
	guard(UE1AndroidStringLooksLikeGammaFullscreenMenuItem);

	return Text
		&& ( appStrfind( Text, "Toggle Fullscreen" ) != NULL
		  || appStrfind( Text, "Toggle Gamma" ) != NULL );

	unguard;
}

static UBOOL UE1AndroidSetGammaMenuLabelOnObject( UObject* Object, UProperty* MenuListProperty, const char* Label )
{
	guard(UE1AndroidSetGammaMenuLabelOnObject);

	if( !Object || !MenuListProperty || !Label || MenuListProperty->ArrayDim <= 0 )
		return 0;

	const INT ElementSize = MenuListProperty->GetElementSize();
	if( ElementSize <= 1 )
		return 0;

	// UnrealVideoMenu uses one-based menu indices in UnrealScript.
	// In the stock Android package the fullscreen item is MenuList(2).
	// A previously recompiled test package may have shifted it to MenuList(3),
	// so patch only the element that still contains Toggle Fullscreen/Gamma.
	static const INT CandidateIndices[] = { 2, 3 };
	BYTE* Base = (BYTE*)Object + MenuListProperty->Offset;
	for( INT Candidate=0; Candidate<(INT)(sizeof(CandidateIndices)/sizeof(CandidateIndices[0])); ++Candidate )
	{
		const INT Index = CandidateIndices[Candidate];
		if( Index < 0 || Index >= MenuListProperty->ArrayDim )
			continue;

		char* MenuText = (char*)(Base + Index * ElementSize);
		if( UE1AndroidStringLooksLikeGammaFullscreenMenuItem( MenuText ) )
		{
			appStrncpy( MenuText, Label, ElementSize );
			MenuText[ElementSize-1] = 0;
			return 1;
		}
	}

	// Fallback for the normal, unmodified Unreal.u: keep the change restricted to
	// the known fullscreen slot directly below Brightness.
	if( MenuListProperty->ArrayDim > 2 )
	{
		char* MenuText = (char*)(Base + 2 * ElementSize);
		appStrncpy( MenuText, Label, ElementSize );
		MenuText[ElementSize-1] = 0;
		return 1;
	}

	return 0;

	unguard;
}

static void UE1AndroidRefreshGammaMenuLabel( UObject* SpecificMenuObject = NULL )
{
	guard(UE1AndroidRefreshGammaMenuLabel);

	UClass* MenuClass = ::FindObject<UClass>( ANY_PACKAGE, "UnrealVideoMenu" );
	if( !MenuClass )
		return;

	UProperty* MenuListProperty = FindField<UProperty>( MenuClass, "MenuList" );
	if( !MenuListProperty )
		return;

	char Label[64];
	UE1AndroidBuildGammaMenuLabel( Label, sizeof(Label) );

	UE1AndroidSetGammaMenuLabelOnObject( MenuClass->GetDefaultObject(), MenuListProperty, Label );

	// Hot path protection: when a live menu object is known, patch only that
	// instance and avoid walking the complete UObject list every repaint.
	if( SpecificMenuObject )
	{
		if( SpecificMenuObject->IsA( MenuClass ) )
			UE1AndroidSetGammaMenuLabelOnObject( SpecificMenuObject, MenuListProperty, Label );
		return;
	}

	for( TObjectIterator<UObject> It; It; ++It )
	{
		if( It->IsA( MenuClass ) )
			UE1AndroidSetGammaMenuLabelOnObject( *It, MenuListProperty, Label );
	}

	unguard;
}

static void UE1AndroidToggleGammaMode( UNSDLViewport* Viewport, FOutputDevice* Out )
{
	guard(UE1AndroidToggleGammaMode);

	const INT Nearest = UE1AndroidGetNearestGammaModeIndex();
	const INT NewIndex = ( Nearest + 1 ) % (INT)(sizeof(GAndroidGammaModes)/sizeof(GAndroidGammaModes[0]));
	const FAndroidGammaMode& NewMode = GAndroidGammaModes[NewIndex];

	char NewValue[32];
	appSprintf( NewValue, "%.2f", NewMode.Value );

	// NSDLDrv.NSDLClient/Gamma is the runtime value read by the Android GLES renderer.
	// Mirroring to WorldGamma keeps the renderer section readable and avoids stale advanced config values.
	GConfigCache.SetString( "NSDLDrv.NSDLClient", "Gamma", NewValue );
	GConfigCache.SetString( "NOpenGLESDrv.NOpenGLESRenderDevice", "WorldGamma", NewValue );
	GConfigCache.SaveAllConfigs();
	UE1AndroidRefreshGammaMenuLabel();

	char Message[128];
	appSprintf( Message, "Gamma: %s (%.2f)", NewMode.Label, NewMode.Value );

	if( Viewport && Viewport->Actor )
		Viewport->Actor->eventClientMessage( Message );
	if( Out )
		Out->Log( Message );

	debugf( NAME_Log, "Android gamma mode toggled via ToggleFullscreen: %s", Message );

	unguard;
}


static AMenu* UE1AndroidGetActiveMenu( UNSDLViewport* Viewport )
{
	guard(UE1AndroidGetActiveMenu);

	if( !Viewport || !Viewport->Actor || !Viewport->Actor->myHUD )
		return NULL;
	return Viewport->Actor->myHUD->MainMenu;

	unguard;
}

static UBOOL UE1AndroidMenuIsExactClass( AMenu* Menu, const char* ClassName )
{
	guard(UE1AndroidMenuIsExactClass);

	if( !Menu || !ClassName || !Menu->GetClass() )
		return 0;
	return !appStricmp( Menu->GetClass()->GetName(), ClassName );

	unguard;
}

struct FAndroidPlayerClassChoice
{
	const char* PrimaryClass;
	const char* FallbackClass;
	const char* Label;
};

static const FAndroidPlayerClassChoice GAndroidPlayerClassChoices[] =
{
	{ "Unreal.FemaleOne",      "UnrealI.FemaleOne",      "Female1"    },
	{ "Unreal.FemaleTwo",      "UnrealI.FemaleTwo",      "Female2"    },
	{ "Unreal.UnrealSpectator", "UnrealI.UnrealSpectator", "Spectator"  },
	{ "Unreal.SkaarjPlayer",    "UnrealI.SkaarjPlayer",    "Sktrooper1" },
	{ "Unreal.MaleThree",      "UnrealI.MaleThree",      "Male3"      },
	{ "Unreal.MaleTwo",        "UnrealI.MaleTwo",        "Male2"      },
	{ "Unreal.MaleOne",        "UnrealI.MaleOne",        "Male 1"     },
};

static UClass* UE1AndroidLoadPlayerClassChoice( const FAndroidPlayerClassChoice& Choice )
{
	guard(UE1AndroidLoadPlayerClassChoice);

	UClass* Result = LoadClass<APlayerPawn>( NULL, Choice.PrimaryClass, NULL, LOAD_NoWarn | LOAD_KeepImports, NULL );
	if( !Result && Choice.FallbackClass )
		Result = LoadClass<APlayerPawn>( NULL, Choice.FallbackClass, NULL, LOAD_NoWarn | LOAD_KeepImports, NULL );
	return Result;

	unguard;
}

static INT UE1AndroidFindCurrentPlayerClassChoice( APlayerPawn* PlayerOwner )
{
	guard(UE1AndroidFindCurrentPlayerClassChoice);

	char ConfigClass[128] = "";
	GConfigCache.GetString( "DefaultPlayer", "Class", ConfigClass, sizeof(ConfigClass) );

	for( INT i=0; i<(INT)(sizeof(GAndroidPlayerClassChoices)/sizeof(GAndroidPlayerClassChoices[0])); ++i )
	{
		const FAndroidPlayerClassChoice& Choice = GAndroidPlayerClassChoices[i];
		if( ConfigClass[0]
		&& ( !appStricmp( ConfigClass, Choice.PrimaryClass ) || !appStricmp( ConfigClass, Choice.FallbackClass )
		  || appStrfind( ConfigClass, appStrchr( Choice.PrimaryClass, '.' ) ? appStrchr( Choice.PrimaryClass, '.' ) + 1 : Choice.PrimaryClass ) != NULL ) )
		{
			return i;
		}
	}

	if( PlayerOwner && PlayerOwner->GetClass() )
	{
		const char* CurrentName = PlayerOwner->GetClass()->GetName();
		for( INT i=0; i<(INT)(sizeof(GAndroidPlayerClassChoices)/sizeof(GAndroidPlayerClassChoices[0])); ++i )
		{
			const char* ShortName = appStrchr( GAndroidPlayerClassChoices[i].PrimaryClass, '.' );
			ShortName = ShortName ? ShortName + 1 : GAndroidPlayerClassChoices[i].PrimaryClass;
			if( !appStricmp( CurrentName, ShortName ) )
				return i;
		}
	}

	return 0;

	unguard;
}

static UBOOL UE1AndroidApplyPlayerClassChoice( UNSDLViewport* Viewport, AMenu* Menu, INT ChoiceIndex )
{
	guard(UE1AndroidApplyPlayerClassChoice);

	if( !Viewport || !Menu || !Menu->PlayerOwner )
		return 0;

	const INT Count = (INT)(sizeof(GAndroidPlayerClassChoices)/sizeof(GAndroidPlayerClassChoices[0]));
	if( ChoiceIndex < 0 ) ChoiceIndex = Count - 1;
	if( ChoiceIndex >= Count ) ChoiceIndex = 0;

	const FAndroidPlayerClassChoice& Choice = GAndroidPlayerClassChoices[ChoiceIndex];
	UClass* PlayerClass = UE1AndroidLoadPlayerClassChoice( Choice );
	if( !PlayerClass )
	{
		debugf( NAME_Log, "Android player setup class not found: %s / %s", Choice.PrimaryClass, Choice.FallbackClass ? Choice.FallbackClass : "" );
		return 0;
	}

	APlayerPawn* DefaultPawn = (APlayerPawn*)PlayerClass->GetDefaultObject();
	if( !DefaultPawn )
		return 0;

	// Keep the menu preview and the current player pawn visually in sync.
	Menu->Mesh = DefaultPawn->Mesh;
	Menu->Skin = DefaultPawn->Skin;
	Menu->PlayerOwner->Mesh = DefaultPawn->Mesh;
	Menu->PlayerOwner->Skin = DefaultPawn->Skin;

	// Persist the class for the next travel/join and also add it to the runtime URL.
	const char* ClassPath = PlayerClass->GetPathName();
	GConfigCache.SetString( "DefaultPlayer", "Class", ClassPath );
	GConfigCache.SaveAllConfigs();

	if( Viewport->Actor && Viewport->Actor->GetLevel() && Viewport->Actor->GetLevel()->Engine )
	{
		char URLClass[160];
		appSprintf( URLClass, "Class=%s", ClassPath );
		((UGameEngine*)Viewport->Actor->GetLevel()->Engine)->LastURL.AddOption( URLClass );
	}

	char URLSkin[160];
	if( DefaultPawn->Skin )
		appSprintf( URLSkin, "Skin=%s", DefaultPawn->Skin->GetPathName() );
	else
		appSprintf( URLSkin, "Skin=" );
	if( Viewport->Actor && Viewport->Actor->GetLevel() && Viewport->Actor->GetLevel()->Engine )
		((UGameEngine*)Viewport->Actor->GetLevel()->Engine)->LastURL.AddOption( URLSkin );

	char Message[128];
	appSprintf( Message, "Class: %s", Choice.Label );
	if( Viewport->Actor )
		Viewport->Actor->eventClientMessage( Message );
	debugf( NAME_Log, "Android player setup class changed: %s", ClassPath );
	return 1;

	unguard;
}

static UBOOL UE1AndroidHandlePlayerSetupClassInput( UNSDLViewport* Viewport, INT iKey, EInputAction Action )
{
	guard(UE1AndroidHandlePlayerSetupClassInput);

	if( Action != IST_Press || ( iKey != IK_Left && iKey != IK_Right ) )
		return 0;

	AMenu* Menu = UE1AndroidGetActiveMenu( Viewport );
	if( !UE1AndroidMenuIsExactClass( Menu, "UnrealPlayerMenu" ) )
		return 0;
	if( Menu->Selection != 4 )
		return 0;

	INT Index = UE1AndroidFindCurrentPlayerClassChoice( Menu->PlayerOwner );
	if( iKey == IK_Right )
		Index++;
	else
		Index--;
	return UE1AndroidApplyPlayerClassChoice( Viewport, Menu, Index );

	unguard;
}

static void UE1AndroidPostProcessJoinMenuInput( UNSDLViewport* Viewport, INT iKey, EInputAction Action )
{
	guard(UE1AndroidPostProcessJoinMenuInput);

	if( Action != IST_Press || ( iKey != IK_Up && iKey != IK_Down ) )
		return;

	AMenu* Menu = UE1AndroidGetActiveMenu( Viewport );
	if( !UE1AndroidMenuIsExactClass( Menu, "UnrealJoinGameMenu" ) )
		return;

	if( Menu->Selection == 2 )
		Menu->Selection = ( iKey == IK_Up ) ? 1 : 3;
	else if( Menu->Selection < 1 )
		Menu->Selection = 1;
	else if( Menu->Selection > 4 )
		Menu->Selection = 4;

	unguard;
}

static void UE1AndroidSetFixedStringArrayElement( UObject* Object, UProperty* Property, INT Index, const char* Text )
{
	guard(UE1AndroidSetFixedStringArrayElement);

	if( !Object || !Property || !Text || Index < 0 || Index >= Property->ArrayDim )
		return;

	const INT ElementSize = Property->GetElementSize();
	if( ElementSize <= 1 )
		return;

	char* Value = (char*)((BYTE*)Object + Property->Offset + Index * ElementSize);
	appStrncpy( Value, Text, ElementSize );
	Value[ElementSize-1] = 0;

	unguard;
}

static void UE1AndroidSetIntPropertyValue( UObject* Object, UProperty* Property, INT Value )
{
	guard(UE1AndroidSetIntPropertyValue);

	if( !Object || !Property )
		return;

	*(INT*)((BYTE*)Object + Property->Offset) = Value;

	unguard;
}

static void UE1AndroidClampMenuSelection( UObject* Object, UProperty* SelectionProperty, INT MaxSelection )
{
	guard(UE1AndroidClampMenuSelection);

	if( !Object || !SelectionProperty || MaxSelection <= 0 )
		return;

	INT* Selection = (INT*)((BYTE*)Object + SelectionProperty->Offset);
	if( *Selection > MaxSelection )
		*Selection = MaxSelection;
	else if( *Selection < 1 )
		*Selection = 1;

	unguard;
}

static void UE1AndroidPatchMenuObjectLengthAndTail( UObject* Object, UProperty* MenuLengthProperty, UProperty* SelectionProperty, UProperty* MenuListProperty, UProperty* HelpMessageProperty, INT NewLength, INT FirstHiddenIndex )
{
	guard(UE1AndroidPatchMenuObjectLengthAndTail);

	if( !Object )
		return;

	UE1AndroidSetIntPropertyValue( Object, MenuLengthProperty, NewLength );
	UE1AndroidClampMenuSelection( Object, SelectionProperty, NewLength );

	// UnrealScript menu arrays are effectively one-based in these classes.
	// Clear the now-hidden tail so stale text/help cannot reappear if a menu
	// draw briefly happens before MenuLength is observed.
	UE1AndroidSetFixedStringArrayElement( Object, MenuListProperty, FirstHiddenIndex, "" );
	UE1AndroidSetFixedStringArrayElement( Object, HelpMessageProperty, FirstHiddenIndex, "" );

	unguard;
}

static UBOOL UE1AndroidPatchMenuClassLengthAndTail( const char* ClassName, INT NewLength, INT FirstHiddenIndex )
{
	guard(UE1AndroidPatchMenuClassLengthAndTail);

	UClass* MenuClass = ::FindObject<UClass>( ANY_PACKAGE, ClassName );
	if( !MenuClass )
		return 0;

	UProperty* MenuLengthProperty = FindField<UProperty>( MenuClass, "MenuLength" );
	UProperty* SelectionProperty   = FindField<UProperty>( MenuClass, "Selection" );
	UProperty* MenuListProperty    = FindField<UProperty>( MenuClass, "MenuList" );
	UProperty* HelpMessageProperty = FindField<UProperty>( MenuClass, "HelpMessage" );

	if( !MenuLengthProperty )
		return 0;

	UE1AndroidPatchMenuObjectLengthAndTail( MenuClass->GetDefaultObject(), MenuLengthProperty, SelectionProperty, MenuListProperty, HelpMessageProperty, NewLength, FirstHiddenIndex );

	for( TObjectIterator<UObject> It; It; ++It )
	{
		if( It->IsA( MenuClass ) )
			UE1AndroidPatchMenuObjectLengthAndTail( *It, MenuLengthProperty, SelectionProperty, MenuListProperty, HelpMessageProperty, NewLength, FirstHiddenIndex );
	}

	debugf( NAME_Log, "Android runtime menu trim applied: %s MenuLength=%i hiddenIndex=%i", ClassName, NewLength, FirstHiddenIndex );
	return 1;

	unguard;
}

static void UE1AndroidPatchVideoMenuCosmeticsOnObject( UObject* Object, UProperty* MenuListProperty )
{
	guard(UE1AndroidPatchVideoMenuCosmeticsOnObject);

	if( !Object || !MenuListProperty )
		return;

	// UnrealVideoMenu default slot 3 is "Select Resolution". On Android the
	// action is still the same, but the shorter label better matches the old
	// low-width menu layout and leaves more room for the value column.
	UE1AndroidSetFixedStringArrayElement( Object, MenuListProperty, 3, "Resolution" );

	unguard;
}

static UBOOL UE1AndroidPatchVideoMenuCosmetics()
{
	guard(UE1AndroidPatchVideoMenuCosmetics);

	UClass* MenuClass = ::FindObject<UClass>( ANY_PACKAGE, "UnrealVideoMenu" );
	if( !MenuClass )
		return 0;

	UProperty* MenuListProperty = FindField<UProperty>( MenuClass, "MenuList" );
	if( !MenuListProperty )
		return 0;

	UE1AndroidPatchVideoMenuCosmeticsOnObject( MenuClass->GetDefaultObject(), MenuListProperty );

	for( TObjectIterator<UObject> It; It; ++It )
	{
		if( It->IsA( MenuClass ) )
			UE1AndroidPatchVideoMenuCosmeticsOnObject( *It, MenuListProperty );
	}

	debugf( NAME_Log, "Android runtime video menu cosmetic labels applied" );
	return 1;

	unguard;
}

static void UE1AndroidPatchJoinMenuOnObject( UObject* Object, UProperty* MenuLengthProperty, UProperty* SelectionProperty, UProperty* MenuListProperty, UProperty* HelpMessageProperty )
{
	guard(UE1AndroidPatchJoinMenuOnObject);

	if( !Object )
		return;

	UE1AndroidSetIntPropertyValue( Object, MenuLengthProperty, 4 );
	UE1AndroidClampMenuSelection( Object, SelectionProperty, 4 );

	// Keep the original hardcoded UnrealJoinGameMenu selection indices intact:
	// 1=Find Local Servers, 3=Open, 4=Optimized for. The draw filter in
	// UnCanvas shifts rows 3/4 up visually over the hidden favorites row.
	UE1AndroidSetFixedStringArrayElement( Object, MenuListProperty,    2, "" );
	UE1AndroidSetFixedStringArrayElement( Object, HelpMessageProperty, 2, "" );
	UE1AndroidSetFixedStringArrayElement( Object, MenuListProperty,    3, "Connect to Server via IP" );
	UE1AndroidSetFixedStringArrayElement( Object, HelpMessageProperty, 3, "Type in a server IP address and connect directly." );
	UE1AndroidSetFixedStringArrayElement( Object, MenuListProperty,    5, "" );
	UE1AndroidSetFixedStringArrayElement( Object, HelpMessageProperty, 5, "" );

	AMenu* Menu = Cast<AMenu>( Object );
	if( Menu && Menu->Selection == 2 )
		Menu->Selection = 3;

	unguard;
}

static UBOOL UE1AndroidPatchJoinMenu()
{
	guard(UE1AndroidPatchJoinMenu);

	UClass* MenuClass = ::FindObject<UClass>( ANY_PACKAGE, "UnrealJoinGameMenu" );
	if( !MenuClass )
		return 0;

	UProperty* MenuLengthProperty = FindField<UProperty>( MenuClass, "MenuLength" );
	UProperty* SelectionProperty   = FindField<UProperty>( MenuClass, "Selection" );
	UProperty* MenuListProperty    = FindField<UProperty>( MenuClass, "MenuList" );
	UProperty* HelpMessageProperty = FindField<UProperty>( MenuClass, "HelpMessage" );
	if( !MenuLengthProperty || !MenuListProperty )
		return 0;

	UE1AndroidPatchJoinMenuOnObject( MenuClass->GetDefaultObject(), MenuLengthProperty, SelectionProperty, MenuListProperty, HelpMessageProperty );
	for( TObjectIterator<UObject> It; It; ++It )
	{
		if( It->GetClass() == MenuClass )
			UE1AndroidPatchJoinMenuOnObject( *It, MenuLengthProperty, SelectionProperty, MenuListProperty, HelpMessageProperty );
	}

	debugf( NAME_Log, "Android runtime multiplayer JOIN menu patch applied" );
	return 1;

	unguard;
}

static void UE1AndroidPatchPlayerMenuOnObject( UObject* Object, UProperty* MenuLengthProperty, UProperty* SelectionProperty, UProperty* MenuListProperty, UProperty* HelpMessageProperty )
{
	guard(UE1AndroidPatchPlayerMenuOnObject);

	if( !Object )
		return;

	UE1AndroidSetIntPropertyValue( Object, MenuLengthProperty, 4 );
	UE1AndroidClampMenuSelection( Object, SelectionProperty, 4 );
	UE1AndroidSetFixedStringArrayElement( Object, MenuListProperty,    4, "Class:" );
	UE1AndroidSetFixedStringArrayElement( Object, HelpMessageProperty, 4, "Change your player class using the left and right arrows." );

	unguard;
}

static UBOOL UE1AndroidPatchPlayerSetupMenu()
{
	guard(UE1AndroidPatchPlayerSetupMenu);

	UClass* MenuClass = ::FindObject<UClass>( ANY_PACKAGE, "UnrealPlayerMenu" );
	if( !MenuClass )
		return 0;

	UProperty* MenuLengthProperty = FindField<UProperty>( MenuClass, "MenuLength" );
	UProperty* SelectionProperty   = FindField<UProperty>( MenuClass, "Selection" );
	UProperty* MenuListProperty    = FindField<UProperty>( MenuClass, "MenuList" );
	UProperty* HelpMessageProperty = FindField<UProperty>( MenuClass, "HelpMessage" );
	if( !MenuLengthProperty || !MenuListProperty )
		return 0;

	UE1AndroidPatchPlayerMenuOnObject( MenuClass->GetDefaultObject(), MenuLengthProperty, SelectionProperty, MenuListProperty, HelpMessageProperty );
	for( TObjectIterator<UObject> It; It; ++It )
	{
		// Exact class only. UnrealMeshMenu inherits from UnrealPlayerMenu and already
		// has its own 5-row Start Game layout.
		if( It->GetClass() == MenuClass )
			UE1AndroidPatchPlayerMenuOnObject( *It, MenuLengthProperty, SelectionProperty, MenuListProperty, HelpMessageProperty );
	}

	debugf( NAME_Log, "Android runtime player setup class row patch applied" );
	return 1;

	unguard;
}

static void UE1AndroidRefreshRuntimeMenuPatches( UNSDLViewport* Viewport = NULL, UBOOL bForce = 0 )

{
	guard(UE1AndroidRefreshRuntimeMenuPatches);

	static UBOOL VideoMenuPatched    = 0;
	static UBOOL OptionsMenuPatched  = 0;
	static UBOOL VideoMenuCosmetics  = 0;
	static UBOOL JoinMenuPatched     = 0;
	static UBOOL ServerMenuPatched   = 0;
	static UBOOL PlayerMenuPatched   = 0;
	static UBOOL GammaLabelPatched   = 0;
	static DOUBLE LastPatchAttempt   = -1000.0;

	AMenu* ActiveMenu = UE1AndroidGetActiveMenu( Viewport );

	// This function used to run a number of FindObject/TObjectIterator passes on
	// every Repaint(). On OUYA/Tegra 3 that is measurable CPU work even during
	// normal gameplay. Keep the runtime UnrealScript-menu fixes, but only scan
	// when a menu is actually active, when Exec() explicitly asks for it, or at a
	// throttled interval while a menu is open.
	if( !bForce && !ActiveMenu )
		return;

	const DOUBLE Now = appSeconds();
	if( !bForce && ( Now - LastPatchAttempt ) < 0.50 )
		return;
	LastPatchAttempt = Now;

	// AUDIO/VIDEO: hide the bottom Sound Quality entry. MenuLength=6 keeps
	// Brightness, Toggle Gamma, Select Resolution, Texture Detail, Music Volume
	// and Sound Volume. The old Sound Quality Low/High value is drawn before
	// the help panel, so moving the help panel up via MenuLength also covers the
	// obsolete value row on the stock UnrealVideoMenu.
	if( !VideoMenuPatched )
		VideoMenuPatched = UE1AndroidPatchMenuClassLengthAndTail( "UnrealVideoMenu", 6, 7 );
	if( !VideoMenuCosmetics )
		VideoMenuCosmetics = UE1AndroidPatchVideoMenuCosmetics();

	// Keep the AUDIO/VIDEO label correct without doing a full UObject iteration
	// every frame. If a specific active menu exists, patch only that instance;
	// otherwise do one full pass on forced Exec()/initial refresh.
	if( bForce || !GammaLabelPatched || ActiveMenu )
	{
		UE1AndroidRefreshGammaMenuLabel( ActiveMenu );
		GammaLabelPatched = 1;
	}

	// MULTIPLAYER / JOIN: hide favorites and the obsolete Epic server list,
	// rename Open to direct IP connect, while preserving the stock hardcoded
	// selection indices used by UnrealJoinGameMenu.ProcessSelection().
	if( !JoinMenuPatched )
		JoinMenuPatched = UE1AndroidPatchJoinMenu();

	// MULTIPLAYER / START GAME: hide the bottom Launch Dedicated Server entry.
	if( !ServerMenuPatched )
		ServerMenuPatched = UE1AndroidPatchMenuClassLengthAndTail( "UnrealServerMenu", 4, 5 );

	// MULTIPLAYER / PLAYER SETUP: restore the missing Class row. Runtime input
	// handling below cycles the class on left/right without rebuilding Unreal.u.
	if( !PlayerMenuPatched )
		PlayerMenuPatched = UE1AndroidPatchPlayerSetupMenu();

	// OPTIONS: hide the bottom Advanced Options entry cleanly. It is already the
	// final item, so MenuLength=14 is enough and does not disturb other hardcoded
	// option selections such as HUD Configuration or View Bob.
	if( !OptionsMenuPatched )
		OptionsMenuPatched = UE1AndroidPatchMenuClassLengthAndTail( "UnrealOptionsMenu", 14, 15 );

	unguard;
}
#endif
// UE1_ANDROID_SOFT_KEYBOARD_PATCH_BEGIN
#if PLATFORM_ANDROID
static UBOOL GAndroidSoftKeyboardActive = 0;
static FLOAT GAndroidSoftKeyboardTimeout = 0.0f;
static FLOAT GAndroidSoftKeyboardLastKick = -1000.0f;
static FLOAT GAndroidSoftKeyboardPulseUntil = 0.0f;
static UNSDLViewport* GAndroidSoftKeyboardPulseViewport = NULL;

static UBOOL AndroidViewportIsMenuing( UNSDLViewport* Viewport )
{
	return Viewport &&
		Viewport->Console &&
		((UObject*)Viewport->Console)->GetMainFrame() &&
		((UObject*)Viewport->Console)->GetMainFrame()->StateNode &&
		(
			((UObject*)Viewport->Console)->GetMainFrame()->StateNode->GetFName() == "Menuing" ||
			((UObject*)Viewport->Console)->GetMainFrame()->StateNode->GetFName() == "Typing" ||
			((UObject*)Viewport->Console)->GetMainFrame()->StateNode->GetFName() == "Console"
		);
}

static void AndroidSetSoftKeyboardRect( UNSDLViewport* Viewport )
{
	if( !Viewport )
		return;

	SDL_Rect TextRect;
	TextRect.x = 0;
	TextRect.y = ( Viewport->SizeY > 180 ) ? ( Viewport->SizeY - 180 ) : 0;
	TextRect.w = ( Viewport->SizeX > 0 ) ? Viewport->SizeX : 1;
	TextRect.h = 180;
	SDL_SetTextInputRect( &TextRect );
}

static void AndroidKeepSoftKeyboardAlive( UNSDLViewport* Viewport, FLOAT KeepAliveSeconds )
{
	if( !Viewport )
		return;

	AndroidSetSoftKeyboardRect( Viewport );

	if( !SDL_IsTextInputActive() )
	{
		SDL_StartTextInput();
	}

	GAndroidSoftKeyboardActive = 1;
	GAndroidSoftKeyboardTimeout = appSeconds() + KeepAliveSeconds;
}

static void AndroidKickSoftKeyboard( UNSDLViewport* Viewport, FLOAT KeepAliveSeconds, UBOOL bForceRestart )
{
	if( !Viewport )
		return;

	const FLOAT Now = appSeconds();

	AndroidSetSoftKeyboardRect( Viewport );

	if( bForceRestart )
	{
		if( SDL_IsTextInputActive() )
		{
			SDL_StopTextInput();
		}
		SDL_StartTextInput();
		GAndroidSoftKeyboardLastKick = Now;
		debugf( NAME_Log, "Android soft keyboard force requested" );
	}
	else if( !SDL_IsTextInputActive() )
	{
		SDL_StartTextInput();
		GAndroidSoftKeyboardLastKick = Now;
	}

	GAndroidSoftKeyboardActive = 1;
	GAndroidSoftKeyboardTimeout = Now + KeepAliveSeconds;
}

static void AndroidRequestSoftKeyboard( UNSDLViewport* Viewport, FLOAT KeepAliveSeconds )
{
	if( !Viewport )
		return;

	const FLOAT Now = appSeconds();
	GAndroidSoftKeyboardPulseViewport = Viewport;
	GAndroidSoftKeyboardPulseUntil = Now + 1.5f;

	// Force once immediately. This reopens the IME even if SDL still reports text input as active.
	AndroidKickSoftKeyboard( Viewport, KeepAliveSeconds, 1 );
}

static void AndroidShowSoftKeyboard( UNSDLViewport* Viewport, FLOAT KeepAliveSeconds )
{
	// Compatibility wrapper for older patch hooks.
	AndroidRequestSoftKeyboard( Viewport, KeepAliveSeconds );
}

static void AndroidHideSoftKeyboard()
{
	if( GAndroidSoftKeyboardActive || SDL_IsTextInputActive() )
	{
		SDL_StopTextInput();
	}
	GAndroidSoftKeyboardActive = 0;
	GAndroidSoftKeyboardTimeout = 0.0f;
	GAndroidSoftKeyboardPulseUntil = 0.0f;
	GAndroidSoftKeyboardPulseViewport = NULL;
}

static void AndroidUpdateSoftKeyboardTimeout( FLOAT CurTime )
{
	if( GAndroidSoftKeyboardPulseViewport && GAndroidSoftKeyboardPulseUntil > CurTime )
	{
		// One extra gentle kick after the menu processed the click/A button and entered text mode.
		if( CurTime - GAndroidSoftKeyboardLastKick > 0.45f )
		{
			AndroidKickSoftKeyboard( GAndroidSoftKeyboardPulseViewport, 90.0f, 0 );
		}
	}
	else
	{
		GAndroidSoftKeyboardPulseUntil = 0.0f;
		GAndroidSoftKeyboardPulseViewport = NULL;
	}

	if( GAndroidSoftKeyboardActive && GAndroidSoftKeyboardTimeout > 0.0f && CurTime > GAndroidSoftKeyboardTimeout )
	{
		AndroidHideSoftKeyboard();
	}
}
#endif
// UE1_ANDROID_SOFT_KEYBOARD_PATCH_END

/*-----------------------------------------------------------------------------
	UNSDLViewport implementation.
-----------------------------------------------------------------------------*/

//
// SDL_BUTTON_ -> EInputKey translation map.
//
const BYTE UNSDLViewport::MouseButtonMap[6] =
{
	/* invalid           */ IK_None,
	/* SDL_BUTTON_LEFT   */ IK_LeftMouse,
	/* SDL_BUTTON_MIDDLE */ IK_MiddleMouse,
	/* SDL_BUTTON_RIGHT  */ IK_RightMouse,
	/* SDL_BUTTON_X1     */ IK_None,
	/* SDL_BUTTON_X2     */ IK_None
};

//
// SDL_CONTROLLER_BUTTON_ -> EInputKey translation map.
//
const BYTE UNSDLViewport::JoyButtonMap[SDL_CONTROLLER_BUTTON_MAX] =
{
	/* BUTTON_A             */ IK_Joy1,
	/* BUTTON_B             */ IK_Joy2,
	/* BUTTON_X             */ IK_Joy3,
	/* BUTTON_Y             */ IK_Joy4,
	/* BUTTON_BACK          */ IK_Joy5,
	/* BUTTON_GUIDE         */ IK_Joy6,
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_START_IS_ESCAPE
	/* BUTTON_START         */ IK_Escape,
#else
	/* BUTTON_START         */ IK_Joy7,
#endif
	/* BUTTON_LEFTSTICK     */ IK_Joy8,
	/* BUTTON_RIGHTSTICK    */ IK_Joy9,
	/* BUTTON_LEFTSHOULDER  */ IK_Joy10,
	/* BUTTON_RIGHTSHOULDER */ IK_Joy11,
	/* BUTTON_DPAD_UP       */ IK_JoyPovUp,
	/* BUTTON_DPAD_DOWN     */ IK_JoyPovDown,
	/* BUTTON_DPAD_LEFT     */ IK_JoyPovLeft,
	/* BUTTON_DPAD_RIGHT    */ IK_JoyPovRight,
};

//
// SDL_CONTROLLER_BUTTON_ -> EInputKey translation map for UI controls.
//
const BYTE UNSDLViewport::JoyButtonMapUI[SDL_CONTROLLER_BUTTON_MAX] =
{
	/* BUTTON_A             */ IK_Enter,
	/* BUTTON_B             */ IK_Escape,
	/* BUTTON_X             */ IK_N,
	/* BUTTON_Y             */ IK_Y,
	/* BUTTON_BACK          */ IK_Escape,
	/* BUTTON_GUIDE         */ IK_Escape,
	/* BUTTON_START         */ IK_Escape,
	/* BUTTON_LEFTSTICK     */ IK_Joy8,
	/* BUTTON_RIGHTSTICK    */ IK_Joy9,
	/* BUTTON_LEFTSHOULDER  */ IK_Joy10,
	/* BUTTON_RIGHTSHOULDER */ IK_Joy11,
	/* BUTTON_DPAD_UP       */ IK_Up,
	/* BUTTON_DPAD_DOWN     */ IK_Down,
	/* BUTTON_DPAD_LEFT     */ IK_Left,
	/* BUTTON_DPAD_RIGHT    */ IK_Right,
};

//
// SDL_CONTROLLER_BUTTON_ -> EInputKey translation map.
//
const BYTE UNSDLViewport::JoyAxisMap[SDL_CONTROLLER_AXIS_MAX] =
{
	/* AXIS_LEFT_X          */ IK_JoyX,
	/* AXIS_LEFT_Y          */ IK_JoyY,
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_RIGHT_STICK_MOUSELOOK
	/* AXIS_RIGHT_X         */ IK_MouseX,
	/* AXIS_RIGHT_Y         */ IK_MouseY,
#else
	/* AXIS_RIGHT_X         */ IK_JoyU,
	/* AXIS_RIGHT_Y         */ IK_JoyV,
#endif
	/* AXIS_LTRIGGER        */ IK_Joy12,
	/* AXIS_RTRIGGER        */ IK_Joy13,
};

//
// Additional scale to apply per SDL axis.
//
const FLOAT UNSDLViewport::JoyAxisDefaultScale[SDL_CONTROLLER_AXIS_MAX] =
{
	/* AXIS_LEFT_X          */ +60.f,
	/* AXIS_LEFT_Y          */ -60.f,
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_MOUSELOOK_SLOWER
	/* AXIS_RIGHT_X         */ +4.f,
	/* AXIS_RIGHT_Y         */ +4.f,
#else
	/* AXIS_RIGHT_X         */ +60.f,
	/* AXIS_RIGHT_Y         */ +60.f,
#endif
	/* AXIS_LTRIGGER        */ +60.f,
	/* AXIS_RTRIGGER        */ +60.f,
};

//
// SDL_Scancode -> EInputKey translation map.
//
BYTE UNSDLViewport::KeyMap[512];
void UNSDLViewport::InitKeyMap()
{
	#define INIT_KEY_RANGE( AStart, AEnd, BStart, BEnd ) \
		for( DWORD Key = AStart; Key <= AEnd; ++Key ) KeyMap[Key] = BStart + ( Key - AStart )

	appMemset( KeyMap, 0, sizeof( KeyMap ) );

	// TODO: IK_LControl, IK_LShift, etc exist, what are they for?
	KeyMap[SDL_SCANCODE_LSHIFT] = IK_Shift;
	KeyMap[SDL_SCANCODE_RSHIFT] = IK_Shift;
	KeyMap[SDL_SCANCODE_LCTRL] = IK_Ctrl;
	KeyMap[SDL_SCANCODE_RCTRL] = IK_Ctrl;
	KeyMap[SDL_SCANCODE_LALT] = IK_Alt;
	KeyMap[SDL_SCANCODE_RALT] = IK_Alt;
	KeyMap[SDL_SCANCODE_GRAVE] = IK_Tilde;
	KeyMap[SDL_SCANCODE_ESCAPE] = IK_Escape;
	KeyMap[SDL_SCANCODE_SPACE] = IK_Space;
	KeyMap[SDL_SCANCODE_RETURN] = IK_Enter;
	KeyMap[SDL_SCANCODE_BACKSPACE] = IK_Backspace;
	KeyMap[SDL_SCANCODE_CAPSLOCK] = IK_CapsLock;
	KeyMap[SDL_SCANCODE_TAB] = IK_Tab;
	KeyMap[SDL_SCANCODE_DELETE] = IK_Delete;
	KeyMap[SDL_SCANCODE_INSERT] = IK_Insert;
	KeyMap[SDL_SCANCODE_HOME] = IK_Home;
	KeyMap[SDL_SCANCODE_END] = IK_End;
	KeyMap[SDL_SCANCODE_PAGEUP] = IK_PageUp;
	KeyMap[SDL_SCANCODE_PAGEDOWN] = IK_PageDown;
	KeyMap[SDL_SCANCODE_PRINTSCREEN] = IK_PrintScrn;
	KeyMap[SDL_SCANCODE_EQUALS] = IK_Equals;
	KeyMap[SDL_SCANCODE_SEMICOLON] = IK_Semicolon;
	KeyMap[SDL_SCANCODE_BACKSLASH] = IK_Backslash;
	KeyMap[SDL_SCANCODE_SLASH] = IK_Slash;
	KeyMap[SDL_SCANCODE_LEFTBRACKET] = IK_LeftBracket;
	KeyMap[SDL_SCANCODE_RIGHTBRACKET] = IK_RightBracket;
	KeyMap[SDL_SCANCODE_COMMA] = IK_Comma;
	KeyMap[SDL_SCANCODE_PERIOD] = IK_Period;
	KeyMap[SDL_SCANCODE_LEFT] = IK_Left;
	KeyMap[SDL_SCANCODE_UP] = IK_Up;
	KeyMap[SDL_SCANCODE_RIGHT] = IK_Right;
	KeyMap[SDL_SCANCODE_DOWN] = IK_Down;
	KeyMap[SDL_SCANCODE_0] = IK_0;
	KeyMap[SDL_SCANCODE_KP_0] = IK_NumPad0;
	KeyMap[SDL_SCANCODE_KP_PERIOD] = IK_NumPadPeriod;

	INIT_KEY_RANGE( SDL_SCANCODE_1,    SDL_SCANCODE_9,    IK_1,       IK_9 );
	INIT_KEY_RANGE( SDL_SCANCODE_A,    SDL_SCANCODE_Z,    IK_A,       IK_Z );
	INIT_KEY_RANGE( SDL_SCANCODE_KP_1, SDL_SCANCODE_KP_9, IK_NumPad1, IK_NumPad9 );
	INIT_KEY_RANGE( SDL_SCANCODE_F1,   SDL_SCANCODE_F12,  IK_F1,      IK_F12 );
	INIT_KEY_RANGE( SDL_SCANCODE_F13,  SDL_SCANCODE_F24,  IK_F13,     IK_F24 );

	#undef INIT_KEY_RANGE
}

//
// Static init.
//
void UNSDLViewport::InternalClassInitializer( UClass* Class )
{
	guard(UNSDLViewport::InternalClassInitializer);
	// Fill in keymap.
	InitKeyMap();
	unguard;
}

//
// Constructor.
//
UNSDLViewport::UNSDLViewport( ULevel* InLevel, UNSDLClient* InClient )
:	UViewport( InLevel, InClient )
,	Client( InClient )
{
	guard(UNSDLViewport::UNSDLViewport);

	// Set color bytes based on screen resolution.
	SDL_DisplayMode Mode;
	SDL_GetDesktopDisplayMode( InClient->DefaultDisplay, &Mode );
	ColorBytes = SDL_BYTESPERPIXEL( Mode.format );
	Caps = 0;
	if( ColorBytes == 2 && SDL_PIXELLAYOUT( Mode.format ) == SDL_PACKEDLAYOUT_565 )
	{
		Caps |= CC_RGB565;
	}

	// Inherit default display until we have a window.
	DisplayIndex = InClient->DefaultDisplay;
	DisplaySize.w = InClient->GetDefaultDisplayMode().w;
	DisplaySize.h = InClient->GetDefaultDisplayMode().h;

	// Init input.
	if( GIsEditor )
		Input->Init( this, GSystem );

	Destroyed = false;
	QuitRequested = false;

	unguard;
}

// UObject interface.
void UNSDLViewport::Destroy()
{
	guard(UNSDLViewport::Destroy);
	if( Client->FullscreenViewport == this )
	{
		Client->FullscreenViewport = NULL;
	}
	UViewport::Destroy();
	unguard;
}

//
// Set the mouse cursor according to Unreal or UnrealEd's mode, or to
// an hourglass if a slow task is active. Not implemented.
//
void UNSDLViewport::SetModeCursor()
{
	guard(UNSDLViewport::SetModeCursor);
	unguard;
}

//
// Update user viewport interface.
//
void UNSDLViewport::UpdateWindow()
{
	guard(UNSDLViewport::UpdateViewportWindow);

	// If not a window, exit.
	if( hWnd==NULL || OnHold )
		return;

	// Set viewport window's name to show resolution.
	char WindowName[80];
	if( !GIsEditor || (Actor->ShowFlags&SHOW_PlayerCtrl) )
	{
		appSprintf( WindowName, LocalizeGeneral("Product","Core") );
	}
	else switch( Actor->RendMap )
	{
		case REN_Wire:		strcpy(WindowName,LocalizeGeneral("ViewPersp")); break;
		case REN_OrthXY:	strcpy(WindowName,LocalizeGeneral("ViewXY")); break;
		case REN_OrthXZ:	strcpy(WindowName,LocalizeGeneral("ViewXZ")); break;
		case REN_OrthYZ:	strcpy(WindowName,LocalizeGeneral("ViewYZ")); break;
		default:			strcpy(WindowName,LocalizeGeneral("ViewOther")); break;
	}

	// Set window title.
	if( SizeX && SizeY )
	{
		appSprintf(WindowName+strlen(WindowName)," (%i x %i)",SizeX,SizeY);
		if( this == Client->CurrentViewport() )
			strcat( WindowName, " *" );
	}
	SDL_SetWindowTitle( hWnd, WindowName );

	unguard;
}

//
// Open a viewport window.
//
void UNSDLViewport::OpenWindow( void* InParentWindow, UBOOL Temporary, INT NewX, INT NewY, INT OpenX, INT OpenY )
{
	guard(UNSDLViewport::OpenWindow);
	check(Actor);
	check(!OnHold);
	UBOOL DoRepaint=0, DoSetActive=0;
	UBOOL DoOpenGL=0;
	UBOOL NoHard=ParseParam( appCmdLine(), "nohard" );
	SDL_GLprofile GLProfile = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY;
	NewX = Align(NewX,4);

	if( !Temporary && !GIsEditor && !NoHard )
	{
		// HACK: Just check if we're about to load OpenGLDrv. Not sure how else you would know to add the GL flag.
		char Temp[256] = "";
		GetConfigString( "Engine.Engine", "GameRenderDevice", Temp, ARRAY_COUNT(Temp) );
		appStrupr( Temp );
		if( !appStrstr( Temp, "OPENGL" ) )
		{
			GetConfigString( "Engine.Engine", "WindowedRenderDevice", Temp, ARRAY_COUNT(Temp) );
			appStrupr( Temp );
			if( appStrstr( Temp, "OPENGL" ) )
				DoOpenGL = 1;
		}
		else
		{
			DoOpenGL = 1;
		}
		if( DoOpenGL && appStrstr( Temp, "GLES" ) )
			GLProfile = SDL_GL_CONTEXT_PROFILE_ES;
	}

	// User window of launcher if no parent window was specified.
	if( !InParentWindow )
	{
		QWORD ParentPtr;
		Parse( appCmdLine(), "HWND=", ParentPtr );
		InParentWindow = (void*)ParentPtr;
	}

	if( Temporary )
	{
		// Create in-memory data.
		ColorBytes = 2;
		ScreenPointer = (BYTE*)appMalloc( 2 * NewX * NewY, "TemporaryViewportData" );	
		hWnd = NULL;
		debugf( NAME_Log, "Opened temporary viewport" );
	}
	else
	{
		// Get flags.
		DWORD Flags = 0;
		if( InParentWindow && (Actor->ShowFlags & SHOW_ChildWindow) )
		{
			Flags = SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS;
		}
		else
		{
			Flags = SDL_WINDOW_HIDDEN;
		}
		if( DoOpenGL )
		{
			Flags |= SDL_WINDOW_OPENGL;
		}
#ifdef PLATFORM_ANDROID
		// UNREAL_ANDROID_ACTIVITY_FULLSCREEN_ONLY
		// The Android Activity/SurfaceView already owns fullscreen. Requesting
		// SDL_WINDOW_FULLSCREEN here can make Android allocate a rotated buffer
		// (for example 972x1920 transform=7) while the surface is 1920x1080.
		Flags |= SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE;
#endif

		// Set OpenGL attributes if needed.
		if( DoOpenGL )
		{
			if( GLProfile == SDL_GL_CONTEXT_PROFILE_ES )
			{
				// Request GLES2.
				SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
				SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
			}
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_GLES_PROFILE_ES
			GLProfile = SDL_GL_CONTEXT_PROFILE_ES;
#endif
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, GLProfile );
		}

		// Set position and size.
		if( OpenX==-1 )
			OpenX = SDL_WINDOWPOS_UNDEFINED;
		if( OpenY==-1 )
			OpenY = SDL_WINDOWPOS_UNDEFINED;

		// If switching renderers, destroy the old window.
		if( hWnd && ( DoOpenGL != !!( SDL_GetWindowFlags( hWnd ) & SDL_WINDOW_OPENGL ) ) )
		{
			CloseWindow();
		}

#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_PRECREATE_DISPLAY_SIZE
		// Android/SDL may otherwise use UE1's historical 800x600 startup
		// size to decide orientation and SurfaceView bounds. Use the real
		// display size before SDL_CreateWindow, then resync with drawable size.
		SDL_DisplayMode AndroidMode;
		if( SDL_GetCurrentDisplayMode( 0, &AndroidMode ) == 0 && AndroidMode.w > 0 && AndroidMode.h > 0 )
		{
			INT AndroidW = AndroidMode.w;
			INT AndroidH = AndroidMode.h;
			if( AndroidH > AndroidW )
				Exchange( AndroidW, AndroidH );
			if( NewX != AndroidW || NewY != AndroidH )
				debugf( NAME_Log, "Android pre-create display size: requested=%ix%i -> %ix%i", NewX, NewY, AndroidW, AndroidH );
			NewX = Align( AndroidW, 4 );
			NewY = AndroidH;
		}
#endif

		// Create or update the window.
		if( !hWnd )
		{
			// Creating new viewport.
			hWnd = SDL_CreateWindow( "", OpenX, OpenY, NewX, NewY, Flags );
			if( !hWnd && DoOpenGL )
			{
				// Try without GL.
				debugf( NAME_Warning, "Could not create OpenGL window: %s. Trying without OpenGL.", SDL_GetError() );
				Flags &= ~SDL_WINDOW_OPENGL;
				DoOpenGL = 0;
				hWnd = SDL_CreateWindow( "", OpenX, OpenY, NewX, NewY, Flags );
			}
			if( !hWnd )
			{
				appErrorf( "Could not create SDL window: %s", SDL_GetError() );
			}

			// Set parent window.
			if( InParentWindow && (Actor->ShowFlags & SHOW_ChildWindow) )
			{
				SDL_SetWindowModalFor( hWnd, (SDL_Window*)InParentWindow );
			}

			debugf( NAME_Log, "Opened viewport" );
			DoSetActive = DoRepaint = 1;
		}
		else
		{
			// Resizing existing viewport.
			SetClientSize( NewX, NewY, false );
		}

		// Create GL context or SDL renderer if needed.
		if( DoOpenGL )
		{
			if( !GLCtx )
			{
				GLCtx = SDL_GL_CreateContext( hWnd );
				if( !GLCtx )
				{
					appErrorf( "Could not create GL context: %s", SDL_GetError() );
				}
			}
			SDL_GL_MakeCurrent( hWnd, GLCtx );
		}
		else
		{
			SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "nearest" );
			SDLRen = SDL_CreateRenderer( hWnd, -1, 0 );
			if( !SDLRen )
			{
				// Fallback to software.
				debugf( NAME_Warning, "Could not create SDL renderer: %s. Trying software.", SDL_GetError() );
				SDLRen = SDL_CreateRenderer( hWnd, -1, SDL_RENDERER_SOFTWARE );
				if( !SDLRen )
				{
					appErrorf( "Could not create SDL renderer: %s", SDL_GetError() );
				}
			}
			// Create framebuffer texture.
			SDLTexFormat = SDL_PIXELFORMAT_ARGB8888;
			ColorBytes = SDL_BYTESPERPIXEL( SDLTexFormat );
			Caps = ( SDL_PIXELLAYOUT( SDLTexFormat ) == SDL_PACKEDLAYOUT_565 ) ? CC_RGB565 : 0;
			SDLTex = SDL_CreateTexture( SDLRen, SDLTexFormat, SDL_TEXTUREACCESS_STREAMING, NewX, NewY );
			if( !SDLTex )
			{
				appErrorf( "Could not create framebuffer texture: %s", SDL_GetError() );
			}
		}

		SDL_ShowWindow( hWnd );

#if defined(UNREAL_ANDROID_OUYA)
		debugf( NAME_Log, "OUYA internal render size: display/window kept fullscreen, UE1 viewport=%ix%i", UNREAL_ANDROID_OUYA_INTERNAL_RENDER_W, UNREAL_ANDROID_OUYA_INTERNAL_RENDER_H );
		NewX = AndroidOuyaAlignX( UNREAL_ANDROID_OUYA_INTERNAL_RENDER_W );
		NewY = UNREAL_ANDROID_OUYA_INTERNAL_RENDER_H;
#elif defined(PLATFORM_ANDROID) // UNREAL_ANDROID_SYNC_DRAWABLE_SIZE
		// Android may create a fullscreen surface whose real drawable size differs
		// from UE1's requested startup viewport. If SizeX/SizeY keep that old
		// value, GLES renders only into the lower-left part of the screen.
		int AndroidWindowW = 0, AndroidWindowH = 0;
		int AndroidDrawW = 0, AndroidDrawH = 0;
		SDL_GetWindowSize( hWnd, &AndroidWindowW, &AndroidWindowH );
		if( DoOpenGL )
			SDL_GL_GetDrawableSize( hWnd, &AndroidDrawW, &AndroidDrawH );
		if( AndroidDrawW <= 0 || AndroidDrawH <= 0 )
		{
			AndroidDrawW = AndroidWindowW;
			AndroidDrawH = AndroidWindowH;
		}
		if( AndroidDrawW > 0 && AndroidDrawH > 0 )
		{
			INT FixedX = Align( AndroidDrawW, 4 );
			INT FixedY = AndroidDrawH;
			if( NewX != FixedX || NewY != FixedY )
			{
				debugf( NAME_Log, "Android drawable size: window=%ix%i drawable=%ix%i viewport=%ix%i -> %ix%i", AndroidWindowW, AndroidWindowH, AndroidDrawW, AndroidDrawH, NewX, NewY, FixedX, FixedY );
				NewX = FixedX;
				NewY = FixedY;
			}
		}
#endif

		// Get this window's display parameters.
		SDL_DisplayMode DisplayMode;
		DisplayIndex = SDL_GetWindowDisplayIndex( hWnd );
		if( SDL_GetWindowDisplayMode( hWnd, &DisplayMode ) == 0 )
		{
			DisplaySize.w = DisplayMode.w;
			DisplaySize.h = DisplayMode.h;
		}
	}

	SizeX = NewX;
	SizeY = NewY;

	if( !RenDev && Temporary )
		Client->TryRenderDevice( this, "SoftDrv.SoftwareRenderDevice", 0 );
	if( !RenDev && !GIsEditor && !NoHard )
		Client->TryRenderDevice( this, "ini:Engine.Engine.GameRenderDevice", Client->StartupFullscreen );
	if( !RenDev )
		Client->TryRenderDevice( this, "ini:Engine.Engine.WindowedRenderDevice", 0 );
	check(RenDev);

	if( !Temporary )
		UpdateWindow();
	if( DoRepaint )
		Repaint();

	unguard;
}

//
// Close a viewport window.  Assumes that the viewport has been opened with
// OpenViewportWindow.  Does not affect the viewport's object, only the
// platform-specific information associated with it.
//
void UNSDLViewport::CloseWindow()
{
	guard(UNSDLViewport::CloseWindow);

#if PLATFORM_ANDROID
	AndroidHideSoftKeyboard();
#endif

	if( hWnd )
	{
		if( SDLTex )
		{
			SDL_DestroyTexture( SDLTex );
			SDLTex = NULL;
		}
		if( SDLRen )
		{
			SDL_DestroyRenderer( SDLRen );
			SDLRen = NULL;
		}
		if( GLCtx )
		{
			SDL_GL_DeleteContext( GLCtx );
			GLCtx = NULL;
		}
		SDL_DestroyWindow( hWnd );
		hWnd = NULL;
	}

	unguard;
}

//
// Lock the viewport window and set the approprite Screen and RealScreen fields
// of Viewport.  Returns 1 if locked successfully, 0 if failed.  Note that a
// lock failing is not a critical error; it's a sign that a DirectDraw mode
// has ended or the user has closed a viewport window.
//
UBOOL UNSDLViewport::Lock( FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize )
{
	guard(UNSDLViewport::LockWindow);
	uclock(Client->DrawCycles);

	// Make sure window is lockable.
	if( !hWnd )
	{
		return 0;
	}

	if( OnHold || !SizeX || !SizeY )
	{
		appErrorf( "Failed locking viewport" );
		return 0;
	}

	if( SDLRen && SDLTex )
	{
		// Obtain pointer to screen.
		Stride = SizeX;
		ScreenPointer = NULL;
		SDL_LockTexture( SDLTex, NULL, (void **)&ScreenPointer, &Stride );
		Stride /= ColorBytes;
		check(ScreenPointer);
	}

	// Success.
	uunclock(Client->DrawCycles);

	return UViewport::Lock( FlashScale, FlashFog, ScreenClear, RenderLockFlags, HitData, HitSize );

	unguard;
}

//
// Unlock the viewport window.  If Blit=1, blits the viewport's frame buffer.
//
void UNSDLViewport::Unlock( UBOOL Blit )
{
	guard(UNSDLViewport::Unlock);

	Client->DrawCycles=0;
	uclock(Client->DrawCycles);

	// Unlock base.
	UViewport::Unlock( Blit );

	// Blit, if desired.
	if( Blit && hWnd && !OnHold )
	{
		if( GLCtx )
		{
			// Flip OpenGL buffers.
			SDL_GL_SwapWindow( hWnd );
		}
		else if( SDLRen && SDLTex )
		{
			// Blitting with SDLRenderer.
			SDL_UnlockTexture( SDLTex );
			SDL_RenderCopy( SDLRen, SDLTex, NULL, NULL );
			SDL_RenderPresent( SDLRen );
		}
	}

	uunclock(Client->DrawCycles);

	unguard;
}

//
// Make this viewport the current one.
// If Viewport=0, makes no viewport the current one.
//
void UNSDLViewport::MakeCurrent()
{
	guard(UNSDLViewport::MakeCurrent);
	Current = 1;
	for( INT i=0; i<Client->Viewports.Num(); i++ )
	{
		UViewport* OldViewport = Client->Viewports(i);
		if( OldViewport->Current && OldViewport != this )
		{
			OldViewport->Current = 0;
			OldViewport->UpdateWindow();
		}
	}
	if( GLCtx )
	{
		SDL_GL_MakeCurrent( hWnd, GLCtx );
	}
	UpdateWindow();
	unguard;
}

//
// Repaint the viewport.
//
void UNSDLViewport::Repaint()
{
	guard(UNSDLViewport::Repaint);
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	UE1AndroidRefreshRuntimeMenuPatches( this, 0 );
#endif
	if( !OnHold && RenDev && SizeX && SizeY )
		Client->Engine->Draw( this, 0 );
	unguard;
}

//
// Set the client size (viewport view size) of a viewport.
//
void UNSDLViewport::SetClientSize( INT NewX, INT NewY, UBOOL UpdateProfile )
{
	guard(UNSDLViewport::SetClientSize);

	if( hWnd )
	{
#ifndef PLATFORM_ANDROID // UNREAL_ANDROID_NO_SETWINDOWSIZE_800X600
		SDL_SetWindowSize( hWnd, NewX, NewY );
#endif
#if defined(UNREAL_ANDROID_OUYA)
		NewX = AndroidOuyaAlignX( UNREAL_ANDROID_OUYA_INTERNAL_RENDER_W );
		NewY = UNREAL_ANDROID_OUYA_INTERNAL_RENDER_H;
		debugf( NAME_Log, "OUYA internal render size SetClientSize=%ix%i", NewX, NewY );
#elif defined(PLATFORM_ANDROID) // UNREAL_ANDROID_SETCLIENT_DRAWABLE_SIZE
		int AndroidWindowW = 0, AndroidWindowH = 0;
		int AndroidDrawW = 0, AndroidDrawH = 0;
		SDL_GetWindowSize( hWnd, &AndroidWindowW, &AndroidWindowH );
		if( GLCtx )
			SDL_GL_GetDrawableSize( hWnd, &AndroidDrawW, &AndroidDrawH );
		if( AndroidDrawW <= 0 || AndroidDrawH <= 0 )
		{
			AndroidDrawW = AndroidWindowW;
			AndroidDrawH = AndroidWindowH;
		}
		if( AndroidDrawW > 0 && AndroidDrawH > 0 )
		{
			NewX = Align( AndroidDrawW, 4 );
			NewY = AndroidDrawH;
		}
#endif
		// Resize output texture if required.
		if( SDLRen && SDLTex )
		{
			SDL_DestroyTexture( SDLTex );
			SDLTex = SDL_CreateTexture( SDLRen, SDLTexFormat, SDL_TEXTUREACCESS_STREAMING, NewX, NewY );
			if( !SDLTex )
			{
				appErrorf( "Could not create framebuffer texture: %s", SDL_GetError() );
			}
		}
	}

	SizeX = NewX;
	SizeY = NewY;

	// Optionally save this size in the profile.
	if( UpdateProfile )
	{
		Client->ViewportX = NewX;
		Client->ViewportY = NewY;
		Client->SaveConfig();
	}

	unguard;
}

//
// Return the viewport's window.
//
void* UNSDLViewport::GetWindow()
{
	return (void*)hWnd;
}

//
// Try to make this viewport fullscreen, matching the fullscreen
// mode of the nearest x-size to the current window. If already in
// fullscreen, returns to non-fullscreen.
//
void UNSDLViewport::MakeFullscreen( INT NewX, INT NewY, UBOOL UpdateProfile )
{
	guard(UNSDLViewport::MakeFullscreen);

	// If someone else is fullscreen, stop them.
	if( Client->FullscreenViewport )
		Client->EndFullscreen();

	// Save this window.
	SavedX = SizeX;
	SavedY = SizeY;

	// Fullscreen rendering. For now no borderless.
	Client->FullscreenViewport = this;
#if defined(UNREAL_ANDROID_OUYA)
	NewX = AndroidOuyaAlignX( UNREAL_ANDROID_OUYA_INTERNAL_RENDER_W );
	NewY = UNREAL_ANDROID_OUYA_INTERNAL_RENDER_H;
	debugf( NAME_Log, "OUYA fullscreen keeps SDL surface fullscreen and UE1 viewport=%ix%i", NewX, NewY );
	SetClientSize( NewX, NewY, false );
#elif defined(PLATFORM_ANDROID) // UNREAL_ANDROID_FULLSCREEN_NO_MODE_SWITCH
	// Keep Android in Activity fullscreen only. A real SDL fullscreen mode switch
	// can produce a portrait-sized rotated buffer on some handhelds.
	int AndroidWindowW = 0, AndroidWindowH = 0;
	int AndroidDrawW = 0, AndroidDrawH = 0;
	SDL_GetWindowSize( hWnd, &AndroidWindowW, &AndroidWindowH );
	if( GLCtx )
		SDL_GL_GetDrawableSize( hWnd, &AndroidDrawW, &AndroidDrawH );
	if( AndroidDrawW <= 0 || AndroidDrawH <= 0 )
	{
		AndroidDrawW = AndroidWindowW;
		AndroidDrawH = AndroidWindowH;
	}
	if( AndroidDrawW > 0 && AndroidDrawH > 0 )
	{
		NewX = Align( AndroidDrawW, 4 );
		NewY = AndroidDrawH;
	}
	SetClientSize( NewX, NewY, false );
#else
	SetClientSize( NewX, NewY, false );
	SDL_SetWindowFullscreen( hWnd, SDL_WINDOW_FULLSCREEN );
#endif

	if( UpdateProfile )
	{
		Client->ViewportX = NewX;
		Client->ViewportY = NewY;
		Client->SaveConfig();
	}

	SetMouseCapture(1, 1, 0);

	unguard;
}

//
//
//
void UNSDLViewport::EndFullscreen()
{
	guard(UNSDLViewport::EndFullscreen);

#if defined(UNREAL_ANDROID_OUYA)
	SetClientSize( AndroidOuyaAlignX( UNREAL_ANDROID_OUYA_INTERNAL_RENDER_W ), UNREAL_ANDROID_OUYA_INTERNAL_RENDER_H, false );
#elif defined(PLATFORM_ANDROID) // UNREAL_ANDROID_FULLSCREEN_END_NOOP
	// Stay in Activity fullscreen on Android; just resync to current surface.
	int AndroidWindowW = 0, AndroidWindowH = 0;
	int AndroidDrawW = 0, AndroidDrawH = 0;
	SDL_GetWindowSize( hWnd, &AndroidWindowW, &AndroidWindowH );
	if( GLCtx )
		SDL_GL_GetDrawableSize( hWnd, &AndroidDrawW, &AndroidDrawH );
	if( AndroidDrawW <= 0 || AndroidDrawH <= 0 )
	{
		AndroidDrawW = AndroidWindowW;
		AndroidDrawH = AndroidWindowH;
	}
	if( AndroidDrawW > 0 && AndroidDrawH > 0 )
		SetClientSize( Align(AndroidDrawW,4), AndroidDrawH, false );
#else
	SDL_SetWindowFullscreen( hWnd, 0 );
	SetClientSize( SavedX, SavedY, false );
#endif

	unguard;
}

//
// Update input for viewport.
//
void UNSDLViewport::UpdateInput( UBOOL Reset )
{
	guard(UNSDLViewport::UpdateInput);

	if( Reset )
		appMemset( (void*)JoyAxis, 0, sizeof(JoyAxis) );

	unguard;
}

//
// If the cursor is currently being captured, stop capturing, clipping, and 
// hiding it, and move its position back to where it was when it was initially
// captured.
//
void UNSDLViewport::SetMouseCapture( UBOOL Capture, UBOOL Clip, UBOOL OnlyFocus )
{
	guard(UNSDLViewport::SetMouseCapture);

	// If only focus, reject.
	if( OnlyFocus )
		if( hWnd != SDL_GetMouseFocus() )
			return;

	// If capturing, windows requires clipping in order to keep focus.
	Clip |= Capture;

	// Handle capturing.
	SDL_SetRelativeMouseMode( (SDL_bool)Capture );

	unguard;
}

UBOOL UNSDLViewport::CauseInputEvent( INT iKey, EInputAction Action, FLOAT Delta )
{
	guard(UWindowsViewport::CauseInputEvent);

	// Route to engine if a valid key
	if( iKey > 0 )
	{
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
		if( UE1AndroidHandlePlayerSetupClassInput( this, iKey, Action ) )
			return 1;
#endif
		UBOOL Result = Client->Engine->InputEvent( this, (EInputKey)iKey, Action, Delta );
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
		UE1AndroidPostProcessJoinMenuInput( this, iKey, Action );
#endif
		return Result;
	}
	else
		return 0;

	unguard;
}

#ifdef __ANDROID__
// ANDROID_SOFT_KEYBOARD_V5_MENUTYPING_HELPER
//
// Android soft keyboard policy:
// UE1 menu text input uses Console state 'MenuTyping'.
// Keep the Android IME active only while that state is active.
static UBOOL UE1Android_IsConsoleMenuTyping( UObject* ConsoleObject )
{
	if( !ConsoleObject )
		return 0;

	if( !ConsoleObject->GetMainFrame() )
		return 0;

	if( !ConsoleObject->GetMainFrame()->StateNode )
		return 0;

	return ConsoleObject->GetMainFrame()->StateNode->GetFName() == "MenuTyping";
}

static void UE1Android_UpdateSoftKeyboardForMenuTyping( UObject* ConsoleObject, INT ViewSizeX, INT ViewSizeY )
{
	static UBOOL bKeyboardWanted = 0;
	static Uint32 LastStartTicks = 0;

	const UBOOL bWantKeyboard = UE1Android_IsConsoleMenuTyping( ConsoleObject );
	const Uint32 NowTicks = SDL_GetTicks();

	if( bWantKeyboard )
	{
		SDL_Rect TextRect;
		TextRect.x = 0;
		TextRect.y = ( ViewSizeY > 0 ) ? ( ( ViewSizeY * 2 ) / 3 ) : 0;
		TextRect.w = ( ViewSizeX > 0 ) ? ViewSizeX : 1;
		TextRect.h = ( ViewSizeY > 3 ) ? ( ViewSizeY / 3 ) : 1;
		SDL_SetTextInputRect( &TextRect );

		// Keep-alive: some Android keyboards ignore a one-shot StartTextInput
		// if focus was not fully settled yet.
		if( !bKeyboardWanted || !SDL_IsTextInputActive() || ( NowTicks - LastStartTicks ) > 750 )
		{
			SDL_StartTextInput();
			LastStartTicks = NowTicks;
		}

		bKeyboardWanted = 1;
	}
	else
	{
		if( bKeyboardWanted || SDL_IsTextInputActive() )
		{
			SDL_StopTextInput();
		}

		bKeyboardWanted = 0;
		LastStartTicks = 0;
	}
}
#endif

UBOOL UNSDLViewport::TickInput()
{
	guard(UNSDLViewport::TickInput);

	SDL_Event Ev;
	INT Tmp;
	const FLOAT CurTime = appSeconds();
	const FLOAT DeltaTime = CurTime - InputUpdateTime;
#ifdef __ANDROID__
	// ANDROID_SOFT_KEYBOARD_V5_MENUTYPING_TICK
	UE1Android_UpdateSoftKeyboardForMenuTyping( (UObject*)Console, SizeX, SizeY );
#endif
#if PLATFORM_ANDROID
	AndroidUpdateSoftKeyboardTimeout( CurTime );
#endif

	while( SDL_PollEvent( &Ev ) )
	{
		switch( Ev.type )
		{
			case SDL_QUIT:
#if PLATFORM_ANDROID
				if( UE1AndroidShouldIgnoreEarlyQuit() )
				{
					debugf( NAME_Warning, "Ignoring early SDL_QUIT during Android startup guard" );
					break;
				}
#endif
				// signal to client and remember set a flag just in case
				QuitRequested = true;
				return true;
			case SDL_TEXTINPUT:
#if PLATFORM_ANDROID
				AndroidKeepSoftKeyboardAlive( this, 90.0f );
#endif
				for( const char *p = Ev.text.text; *p && p < Ev.text.text + sizeof( Ev.text.text ); ++p )
				{
					if( *p < 0 )
						break;
					if( isprint( *p ) || *p == '\r' )
						Client->Engine->Key( this, (EInputKey)*p );
				}
				break;
			case SDL_KEYDOWN:
#if PLATFORM_ANDROID
				if( Ev.key.keysym.sym == SDLK_RETURN || Ev.key.keysym.sym == SDLK_ESCAPE )
				{
					AndroidHideSoftKeyboard();
				}
#endif
				if( Ev.key.keysym.sym == SDLK_RETURN && (Ev.key.keysym.mod & KMOD_ALT) )
				{
					Exec("ToggleFullscreen", this);
					break;
				}
			case SDL_KEYUP:
				CauseInputEvent( KeyMap[Ev.key.keysym.scancode], ( Ev.type == SDL_KEYDOWN ) ? IST_Press : IST_Release );
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
#if PLATFORM_ANDROID
				if( ( Ev.type == SDL_MOUSEBUTTONDOWN || Ev.type == SDL_MOUSEBUTTONUP ) && AndroidViewportIsMenuing( this ) )
				{
					// v4.2 selective: disabled broad keyboard trigger from pointer/button event.
					// original: AndroidRequestSoftKeyboard( this, 90.0f );
				}
#endif
				CauseInputEvent( MouseButtonMap[Ev.button.button], ( Ev.type == SDL_MOUSEBUTTONDOWN ) ? IST_Press : IST_Release );
				break;
			case SDL_MOUSEWHEEL:
				if( Ev.wheel.y )
				{
					CauseInputEvent( IK_MouseW, IST_Axis, Ev.wheel.y );
					if( Ev.wheel.y < 0 )
					{
						CauseInputEvent( IK_MouseWheelDown, IST_Press );
						CauseInputEvent( IK_MouseWheelDown, IST_Release );
					}
					else if( Ev.wheel.y > 0 )
					{
						CauseInputEvent( IK_MouseWheelUp, IST_Press );
						CauseInputEvent( IK_MouseWheelUp, IST_Release );
					}
				}
				break;
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
				{
					// HACK: Swap to alternate bindings when in menus, but not when waiting for keypress in the keybind menu.
					const UBOOL bIsInUI = Console &&
						((UObject*)Console)->GetMainFrame() &&
						((UObject*)Console)->GetMainFrame()->StateNode &&
						((UObject*)Console)->GetMainFrame()->StateNode->GetFName() == "Menuing";
					const BYTE* JoyMap = bIsInUI ? JoyButtonMapUI : JoyButtonMap;
#if PLATFORM_ANDROID
					if( bIsInUI && Ev.type == SDL_CONTROLLERBUTTONDOWN && Ev.cbutton.button == SDL_CONTROLLER_BUTTON_A )
					{
						// v4.2 selective: disabled broad keyboard trigger from pointer/button event.
						// original: AndroidRequestSoftKeyboard( this, 90.0f );
					}
#endif
					CauseInputEvent( JoyMap[Ev.cbutton.button], ( Ev.type == SDL_CONTROLLERBUTTONDOWN ) ? IST_Press : IST_Release );
				}
				break;
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				{
					// Raw joystick events are only a fallback for devices that SDL does not
					// expose as GameController. If a GameController is active, SDL may emit
					// both controller and joystick events for the same physical button. Feeding
					// both into UE1 makes menus skip entries and confirms one level too deep.
					if( Client && Client->GetController() )
						break;

					const UBOOL bIsInUI = Console &&
						((UObject*)Console)->GetMainFrame() &&
						((UObject*)Console)->GetMainFrame()->StateNode &&
						((UObject*)Console)->GetMainFrame()->StateNode->GetFName() == "Menuing";
					const BYTE* JoyMap = bIsInUI ? JoyButtonMapUI : JoyButtonMap;
					BYTE Key = IK_None;

					if( Ev.jbutton.button < SDL_CONTROLLER_BUTTON_MAX )
					{
						Key = JoyMap[Ev.jbutton.button];
					}
					else
					{
						switch( Ev.jbutton.button )
						{
							case 6:
							case 7:
								Key = IK_Escape;
								break;
							case 4:
							case 8:
								Key = bIsInUI ? IK_Escape : IK_Joy5;
								break;
							default:
								Key = IK_None;
								break;
						}
					}

					if( Key != IK_None )
						CauseInputEvent( Key, ( Ev.type == SDL_JOYBUTTONDOWN ) ? IST_Press : IST_Release );
				}
				break;

			case SDL_JOYHATMOTION:
				// Same duplicate-event guard as above. D-Pad from GameController is handled
				// via SDL_CONTROLLERBUTTONDOWN/UP and mapped to IK_Up/IK_Down in UI.
				if( Client && Client->GetController() )
					break;

				CauseInputEvent( IK_JoyPovUp,    ( Ev.jhat.value & SDL_HAT_UP    ) ? IST_Press : IST_Release );
				CauseInputEvent( IK_JoyPovDown,  ( Ev.jhat.value & SDL_HAT_DOWN  ) ? IST_Press : IST_Release );
				CauseInputEvent( IK_JoyPovLeft,  ( Ev.jhat.value & SDL_HAT_LEFT  ) ? IST_Press : IST_Release );
				CauseInputEvent( IK_JoyPovRight, ( Ev.jhat.value & SDL_HAT_RIGHT ) ? IST_Press : IST_Release );
				break;
#endif
			case SDL_CONTROLLERAXISMOTION:
				{
					const BYTE Key = JoyAxisMap[Ev.caxis.axis];
					const INT PrevValue = JoyAxis[Ev.caxis.axis];
					INT NewValue = Ev.caxis.value;
					INT DeadZone = 0;
					if ( Key < IK_JoyX
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_MOUSELOOK_AXIS_DEADZONE
						&& Key != IK_MouseX && Key != IK_MouseY
#endif
					)
					{
						// Treat the axis like a trigger.
						if ( PrevValue < JoyAxisPressThreshold && NewValue >= JoyAxisPressThreshold )
							CauseInputEvent( Key, IST_Press );
						else if ( PrevValue >= JoyAxisPressThreshold && NewValue < JoyAxisPressThreshold )
							CauseInputEvent( Key, IST_Release );
					}
					else
					{
						// Apply deadzone.
						if ( Key >= IK_JoyX && Key <= IK_JoyZ )
							DeadZone = Client->DeadZoneXYZ * 32767.f;
						else if ( Key == IK_JoyR || Key == IK_JoyU || Key == IK_JoyV
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_MOUSELOOK_AXIS_DEADZONE_RUV
							|| Key == IK_MouseX || Key == IK_MouseY
#endif
						)
							DeadZone = Client->DeadZoneRUV * 32767.f;
						if ( Abs(NewValue) < DeadZone )
							NewValue = 0;
					}
					JoyAxis[Ev.caxis.axis] = NewValue;
				}
				break;
			case SDL_MOUSEMOTION:
				if( !Client->FullscreenViewport && !SDL_GetRelativeMouseMode() )
				{
					// If cursor isn't captured, just do MousePosition.
					Client->Engine->MousePosition( this, 0, Ev.motion.x, Ev.motion.y );
				}
				else
				{
					DWORD ViewportButtonFlags = 0;
					if( Ev.motion.state & SDL_BUTTON_LMASK ) ViewportButtonFlags |= MOUSE_Left;
					if( Ev.motion.state & SDL_BUTTON_RMASK ) ViewportButtonFlags |= MOUSE_Right;
					if( Ev.motion.state & SDL_BUTTON_MMASK ) ViewportButtonFlags |= MOUSE_Middle;
					if( Ev.motion.xrel || Ev.motion.yrel )
					{
						Client->Engine->MouseDelta( this, ViewportButtonFlags, Ev.motion.xrel, -Ev.motion.yrel );
						if( Ev.motion.xrel ) CauseInputEvent( IK_MouseX, IST_Axis, Ev.motion.xrel );
						if( Ev.motion.yrel ) CauseInputEvent( IK_MouseY, IST_Axis, -Ev.motion.yrel );
					}
				}
				break;
			default:
				break;
		}
	}

	// Constantly hammer the input system with axis events for axes that are not zero.
	for ( INT i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i )
	{
		const BYTE Key = JoyAxisMap[i];
		const SWORD Value = JoyAxis[i];
		if ( Value && Key && Key >= IK_JoyX )
		{
			const FLOAT FltValue = Clamp( Value / 32767.f, -1.f, 1.f );
			FLOAT Scale = ( Key >= IK_JoyX && Key <= IK_JoyZ ) ? Client->ScaleXYZ : Client->ScaleRUV;
			Scale *= JoyAxisDefaultScale[i] * DeltaTime;
			if ( ( Client->InvertV && Key == IK_JoyV ) || ( Client->InvertY && Key == IK_JoyY ) )
				Scale = -Scale;
			CauseInputEvent( Key, IST_Axis, FltValue * Scale );
		}
	}

	InputUpdateTime = CurTime;

	return QuitRequested;

	unguard;
}

/*-----------------------------------------------------------------------------
	Command line.
-----------------------------------------------------------------------------*/

UBOOL UNSDLViewport::Exec( const char* Cmd, FOutputDevice* Out )
{
	guard(UNSDLViewport::Exec);
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	UE1AndroidRefreshRuntimeMenuPatches( this, 0 );
	const char* GammaCmd = Cmd;
	if( ParseCommand( &GammaCmd, "ToggleFullscreen" ) )
	{
		UE1AndroidToggleGammaMode( this, Out );
		return 1;
	}
#endif
#if PLATFORM_ANDROID && ANDROID_LEGACY_API16
	if( NSDL_OuyaExecResolutionCommand( this, Cmd, Out ) )
	{
		return 1;
	}
#endif
	if( UViewport::Exec( Cmd, Out ) )
	{
		return 1;
	}
	else if( ParseCommand(&Cmd, "ToggleFullscreen") )
	{
		// Toggle fullscreen.
		if( Client->FullscreenViewport )
			Client->EndFullscreen();
		else if( !(Actor->ShowFlags & SHOW_ChildWindow) )
			Client->TryRenderDevice( this, "ini:Engine.Engine.GameRenderDevice", 1 );
		return 1;
	}
	else if( ParseCommand(&Cmd, "GetCurrentRes") )
	{
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
		UE1AndroidRefreshGammaMenuLabel();
#endif
		Out->Logf( "%ix%i", SizeX, SizeY );
		return 1;
	}
	else if( ParseCommand(&Cmd, "SetRes") )
	{
		INT X=appAtoi(Cmd), Y=appAtoi(appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : "");
		if( X && Y )
		{
			if( Client->FullscreenViewport )
				MakeFullscreen( X, Y, 1 );
			else
				SetClientSize( X, Y, 1 );
		}
		return 1;
	}
	else if( ParseCommand(&Cmd, "Preferences") )
	{
		if( Client->FullscreenViewport )
			Client->EndFullscreen();
		return 1;
	}
	else return 0;
	unguard;
}

// ANDROID_SOFT_KEYBOARD_V4_2_SELECTIVE_APPLIED


