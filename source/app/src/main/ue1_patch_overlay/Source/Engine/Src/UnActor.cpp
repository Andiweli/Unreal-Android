/*=============================================================================
	UnActor.cpp: AActor implementation
	Copyright 1997 Epic MegaGames, Inc. This software is a trade secret.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "EnginePrivate.h"
#include "UnNet.h"

/*-----------------------------------------------------------------------------
	AActor object implementations.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(AActor);
IMPLEMENT_CLASS(AWeapon);
IMPLEMENT_CLASS(ALevelInfo);
IMPLEMENT_CLASS(AGameInfo);
IMPLEMENT_CLASS(ACamera);
IMPLEMENT_CLASS(AZoneInfo);
IMPLEMENT_CLASS(ASkyZoneInfo);
IMPLEMENT_CLASS(APathNode);
IMPLEMENT_CLASS(ANavigationPoint);
IMPLEMENT_CLASS(AScout);
IMPLEMENT_CLASS(AInterpolationPoint);
IMPLEMENT_CLASS(ADecoration);
IMPLEMENT_CLASS(AProjectile);
IMPLEMENT_CLASS(AWarpZoneInfo);
IMPLEMENT_CLASS(ATeleporter);
IMPLEMENT_CLASS(APlayerStart);
IMPLEMENT_CLASS(AKeypoint);
IMPLEMENT_CLASS(AInventory);
IMPLEMENT_CLASS(AInventorySpot);
IMPLEMENT_CLASS(ATriggers);
IMPLEMENT_CLASS(ATrigger);
IMPLEMENT_CLASS(ATriggerMarker);
IMPLEMENT_CLASS(AButtonMarker);
IMPLEMENT_CLASS(AWarpZoneMarker);
IMPLEMENT_CLASS(AHUD);
IMPLEMENT_CLASS(AMenu);
IMPLEMENT_CLASS(ASavedMove);
IMPLEMENT_CLASS(ACarcass);
IMPLEMENT_CLASS(ALiftCenter);
IMPLEMENT_CLASS(ALiftExit);


#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
// UNREAL_ANDROID_CONTROLLER_SEMANTIC_TOGGLES_V21
// UNREAL_ANDROID_INFINITE_AMMO_FREEZE_V23
// Native Android gameplay toggles.  These deliberately do not execute the
// original cheat commands: health protection is applied directly to the local
// pawn and ammo consumption is neutralized around Ammo.UseAmmo.
static APlayerPawn* GAndroidTogglePlayerV21 = NULL;
static UBOOL GAndroidInfiniteHealthV21 = 0;
static UBOOL GAndroidInfiniteAmmoV21 = 0;
static FName GAndroidSavedReducedDamageTypeV21;
static INT GAndroidProtectedHealthV21 = 0;

struct FAndroidAmmoSnapshotV21
{
	AActor* AmmoObject;
	INT OriginalAmount;
	INT ProtectedAmount;
};

static const INT GAndroidMaxAmmoSnapshotsV21 = 64;
static FAndroidAmmoSnapshotV21 GAndroidAmmoSnapshotsV21[GAndroidMaxAmmoSnapshotsV21];
static INT GAndroidAmmoSnapshotCountV21 = 0;

static UBOOL UE1AndroidReadAmmoAmountV21( AActor* Ammo, INT& OutAmount, UProperty*& OutProperty )
{
	OutProperty = NULL;
	if( !Ammo || Ammo->bDeleteMe )
		return 0;

	UProperty* Property = FindField<UProperty>( Ammo->GetClass(), "AmmoAmount" );
	if( !Property )
		return 0;

	BYTE* Address = (BYTE*)Ammo + Property->Offset;
	switch( Property->GetElementSize() )
	{
		case 1: OutAmount = *(BYTE*)Address; break;
		case 2: OutAmount = *(SWORD*)Address; break;
		case 4: OutAmount = *(INT*)Address; break;
		default: return 0;
	}
	OutProperty = Property;
	return 1;
}

static void UE1AndroidWriteAmmoAmountV21( AActor* Ammo, UProperty* Property, INT Amount )
{
	if( !Ammo || Ammo->bDeleteMe || !Property )
		return;

	BYTE* Address = (BYTE*)Ammo + Property->Offset;
	switch( Property->GetElementSize() )
	{
		case 1: *(BYTE*)Address = (BYTE)Clamp( Amount, 0, 255 ); break;
		case 2: *(SWORD*)Address = (SWORD)Clamp( Amount, -32768, 32767 ); break;
		case 4: *(INT*)Address = Amount; break;
	}
}

static UBOOL UE1AndroidActorOwnedByTogglePlayerV21( AActor* Actor )
{
	INT OwnerDepth = 0;
	for( AActor* Test=Actor; Test && OwnerDepth<8; Test=Test->Owner, ++OwnerDepth )
	{
		if( Test == GAndroidTogglePlayerV21 )
			return 1;
	}
	return 0;
}

static INT UE1AndroidFindAmmoSnapshotV21( AActor* Ammo )
{
	if( !Ammo )
		return INDEX_NONE;

	for( INT i=0; i<GAndroidAmmoSnapshotCountV21; ++i )
	{
		if( GAndroidAmmoSnapshotsV21[i].AmmoObject == Ammo )
			return i;
	}
	return INDEX_NONE;
}

static void UE1AndroidCaptureCurrentAmmoV21( APlayerPawn* Player )
{
	if( !Player )
		return;

	INT GuardCount = 0;
	for( AInventory* Item=Player->Inventory; Item && GuardCount<256; Item=Item->Inventory, ++GuardCount )
	{
		AActor* Ammo = (AActor*)Item;
		INT Amount = 0;
		UProperty* Property = NULL;
		if( !UE1AndroidReadAmmoAmountV21( Ammo, Amount, Property ) )
			continue;
		if( UE1AndroidFindAmmoSnapshotV21( Ammo ) != INDEX_NONE )
			continue;
		if( GAndroidAmmoSnapshotCountV21 >= GAndroidMaxAmmoSnapshotsV21 )
			break;

		FAndroidAmmoSnapshotV21& Snapshot = GAndroidAmmoSnapshotsV21[GAndroidAmmoSnapshotCountV21++];
		Snapshot.AmmoObject = Ammo;
		Snapshot.OriginalAmount = Amount;
		Snapshot.ProtectedAmount = Amount;
	}
}


static void UE1AndroidProtectCurrentAmmoV23( APlayerPawn* Player )
{
	if( !Player )
		return;

	INT GuardCount = 0;
	for( AInventory* Item=Player->Inventory; Item && GuardCount<256; Item=Item->Inventory, ++GuardCount )
	{
		AActor* Ammo = (AActor*)Item;
		INT Amount = 0;
		UProperty* Property = NULL;
		if( !UE1AndroidReadAmmoAmountV21( Ammo, Amount, Property ) )
			continue;

		INT SnapshotIndex = UE1AndroidFindAmmoSnapshotV21( Ammo );
		if( SnapshotIndex == INDEX_NONE )
		{
			if( GAndroidAmmoSnapshotCountV21 >= GAndroidMaxAmmoSnapshotsV21 )
				continue;

			SnapshotIndex = GAndroidAmmoSnapshotCountV21++;
			FAndroidAmmoSnapshotV21& NewSnapshot = GAndroidAmmoSnapshotsV21[SnapshotIndex];
			NewSnapshot.AmmoObject = Ammo;
			NewSnapshot.OriginalAmount = Amount;
			NewSnapshot.ProtectedAmount = Amount;
		}

		FAndroidAmmoSnapshotV21& Snapshot = GAndroidAmmoSnapshotsV21[SnapshotIndex];
		if( Amount < Snapshot.ProtectedAmount )
		{
			UE1AndroidWriteAmmoAmountV21( Ammo, Property, Snapshot.ProtectedAmount );
		}
		else if( Amount > Snapshot.ProtectedAmount )
		{
			// Pickups may still increase ammo while the toggle is enabled.
			Snapshot.ProtectedAmount = Amount;
		}
	}
}

static void UE1AndroidRestoreAmmoSnapshotsV21( APlayerPawn* Player )
{
	if( Player )
	{
		INT GuardCount = 0;
		for( AInventory* Item=Player->Inventory; Item && GuardCount<256; Item=Item->Inventory, ++GuardCount )
		{
			AActor* Ammo = (AActor*)Item;
			const INT SnapshotIndex = UE1AndroidFindAmmoSnapshotV21( Ammo );
			if( SnapshotIndex == INDEX_NONE )
				continue;

			INT CurrentAmount = 0;
			UProperty* Property = NULL;
			if( UE1AndroidReadAmmoAmountV21( Ammo, CurrentAmount, Property ) )
				UE1AndroidWriteAmmoAmountV21( Ammo, Property, GAndroidAmmoSnapshotsV21[SnapshotIndex].OriginalAmount );
		}
	}

	GAndroidAmmoSnapshotCountV21 = 0;
	appMemset( GAndroidAmmoSnapshotsV21, 0, sizeof(GAndroidAmmoSnapshotsV21) );
}

static void UE1AndroidAttachTogglePlayerV21( APlayerPawn* Player )
{
	if( Player == GAndroidTogglePlayerV21 )
		return;

	// A level/load transition replaces the pawn.  Never dereference the old
	// pointer here; simply start the optional toggles disabled for the new pawn.
	GAndroidTogglePlayerV21 = Player;
	GAndroidInfiniteHealthV21 = 0;
	GAndroidProtectedHealthV21 = 0;
	GAndroidInfiniteAmmoV21 = 0;
	GAndroidAmmoSnapshotCountV21 = 0;
	appMemset( GAndroidAmmoSnapshotsV21, 0, sizeof(GAndroidAmmoSnapshotsV21) );
}

extern "C" ENGINE_API UBOOL UE1AndroidToggleInfiniteHealthV21( APlayerPawn* Player )
{
	UE1AndroidAttachTogglePlayerV21( Player );
	if( !Player )
		return 0;

	if( !GAndroidInfiniteHealthV21 )
	{
		GAndroidSavedReducedDamageTypeV21 = Player->ReducedDamageType;
		GAndroidProtectedHealthV21 = Player->Health;
		GAndroidInfiniteHealthV21 = 1;
		Player->ReducedDamageType = FName("All");
	}
	else
	{
		GAndroidInfiniteHealthV21 = 0;
		GAndroidProtectedHealthV21 = 0;
		Player->ReducedDamageType = GAndroidSavedReducedDamageTypeV21;
	}
	return GAndroidInfiniteHealthV21;
}

extern "C" ENGINE_API UBOOL UE1AndroidToggleInfiniteAmmoV21( APlayerPawn* Player )
{
	UE1AndroidAttachTogglePlayerV21( Player );
	if( !Player )
		return 0;

	if( !GAndroidInfiniteAmmoV21 )
	{
		GAndroidAmmoSnapshotCountV21 = 0;
		appMemset( GAndroidAmmoSnapshotsV21, 0, sizeof(GAndroidAmmoSnapshotsV21) );
		UE1AndroidCaptureCurrentAmmoV21( Player );
		GAndroidInfiniteAmmoV21 = 1;
	}
	else
	{
		GAndroidInfiniteAmmoV21 = 0;
		UE1AndroidRestoreAmmoSnapshotsV21( Player );
	}
	return GAndroidInfiniteAmmoV21;
}

extern "C" ENGINE_API void UE1AndroidUpdateInfiniteTogglesV21( APlayerPawn* Player )
{
	UE1AndroidAttachTogglePlayerV21( Player );
	if( !Player )
		return;

	if( GAndroidInfiniteHealthV21 )
	{
		Player->ReducedDamageType = FName("All");
		// Standard damage is blocked by ReducedDamageType.  The monotonic floor
		// also catches scripted/direct Health subtraction while still allowing
		// medkits and other increases to raise the protected value.
		if( Player->Health < GAndroidProtectedHealthV21 )
			Player->Health = GAndroidProtectedHealthV21;
		else if( Player->Health > GAndroidProtectedHealthV21 )
			GAndroidProtectedHealthV21 = Player->Health;
	}
	if( GAndroidInfiniteAmmoV21 )
		UE1AndroidProtectCurrentAmmoV23( Player );
}

extern "C" ENGINE_API void UE1AndroidRefreshInfiniteAmmoAfterLevelTickV23( ULevel* Level )
{
	if( !GAndroidInfiniteAmmoV21 || !GAndroidTogglePlayerV21 || !Level )
		return;
	if( GAndroidTogglePlayerV21->bDeleteMe || GAndroidTogglePlayerV21->XLevel != Level )
		return;

	UE1AndroidProtectCurrentAmmoV23( GAndroidTogglePlayerV21 );
}

// Keep the older ProcessEvent guard as an immediate secondary safety net for
// native-to-script UseAmmo calls. Script-to-script UseAmmo calls are covered by
// the post-level-tick freeze above.
static UBOOL UE1AndroidShouldPreserveAmmoUseV21( AActor* Actor, UFunction* Function, INT& Before, UProperty*& Property )
{
	if( !GAndroidInfiniteAmmoV21 || !GAndroidTogglePlayerV21 || !Actor || !Function )
		return 0;

	static FName UseAmmoNameV21( "UseAmmo" );
	if( Function->GetFName() != UseAmmoNameV21 )
		return 0;
	if( !UE1AndroidActorOwnedByTogglePlayerV21( Actor ) )
		return 0;

	return UE1AndroidReadAmmoAmountV21( Actor, Before, Property );
}
#endif

/*-----------------------------------------------------------------------------
	APlayerPawn implementation.
-----------------------------------------------------------------------------*/

//
// Set the player.
//
void APlayerPawn::SetPlayer( UPlayer* InPlayer )
{
	guard(APlayerPawn::SetPlayer);
	check(InPlayer!=NULL);

	// Detach old player.
	if( InPlayer->Actor )
	{
		InPlayer->Actor->Player = NULL;
		InPlayer->Actor = NULL;
	}

	// Set the viewport.
	Player = InPlayer;
	InPlayer->Actor = this;

	// Send possess message to script.
	eventPossess();

	// Debug message.
	debugf( NAME_Log, "Possessed PlayerPawn: %s", GetFullName() );

	unguard;
}

/*-----------------------------------------------------------------------------
	ALevelInfo.
-----------------------------------------------------------------------------*/

void ALevelInfo::execGetAddressURL( FFrame& Stack, BYTE*& Result )
{
	guardSlow(ALevelInfo::execGetAddressURL);

	P_FINISH;

	FString Str;
	XLevel->URL.String(Str);
	appStrncpy( (char*)Result, *Str, 240 );
	char* Tmp = appStrchr( (char*)Result, '?' );
	if( Tmp )
		*Tmp = 0;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( ALevelInfo, INDEX_NONE, execGetAddressURL );

/*-----------------------------------------------------------------------------
	AZoneInfo.
-----------------------------------------------------------------------------*/

void AZoneInfo::PostEditChange()
{
	guard(AZoneInfo::PostEditChange);
	Super::PostEditChange();
	if( GIsEditor )
		GCache.Flush();
	unguard;
}

/*-----------------------------------------------------------------------------
	AActor.
-----------------------------------------------------------------------------*/

void AActor::ProcessEvent( UFunction* Function, void* Parms )
{
	guardSlow(AActor::ProcessEvent);
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	INT AmmoBeforeV21 = 0;
	UProperty* AmmoPropertyV21 = NULL;
	const UBOOL PreserveAmmoV21 = UE1AndroidShouldPreserveAmmoUseV21( this, Function, AmmoBeforeV21, AmmoPropertyV21 );
#endif
	if( Level->bBegunPlay )
		Super::ProcessEvent( Function, Parms );
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	if( PreserveAmmoV21 )
	{
		INT AmmoAfterV21 = 0;
		UProperty* CurrentPropertyV21 = NULL;
		if( UE1AndroidReadAmmoAmountV21( this, AmmoAfterV21, CurrentPropertyV21 ) && AmmoAfterV21 < AmmoBeforeV21 )
			UE1AndroidWriteAmmoAmountV21( this, CurrentPropertyV21 ? CurrentPropertyV21 : AmmoPropertyV21, AmmoBeforeV21 );
	}
#endif
	unguardSlow;
}

void AActor::PostEditChange()
{
	guard(AActor::PostEditChange);
	Super::PostEditChange();
	if( GIsEditor )
		bLightChanged = 1;
	unguard;
}

//
// Set the actor's collision properties.
//
void AActor::SetCollision
(
	UBOOL NewCollideActors,
	UBOOL NewBlockActors,
	UBOOL NewBlockPlayers
)
{
	guard(AActor::SetCollision);

	// Untouch this actor.
	if( bCollideActors && GetLevel()->Hash )
		GetLevel()->Hash->RemoveActor( this );

	// Set properties.
	bCollideActors = NewCollideActors;
	bBlockActors   = NewBlockActors;
	bBlockPlayers  = NewBlockPlayers;

	// Touch this actor.
	if( bCollideActors && GetLevel()->Hash )
		GetLevel()->Hash->AddActor( this );

	unguard;
}

//
// Set collision size.
//
void AActor::SetCollisionSize( FLOAT NewRadius, FLOAT NewHeight )
{
	guard(AActor::SetCollisionSize);

	// Untouch this actor.
	if( bCollideActors && GetLevel()->Hash )
		GetLevel()->Hash->RemoveActor( this );

	// Set properties.
	CollisionRadius = NewRadius;
	CollisionHeight = NewHeight;

	// Touch this actor.
	if( bCollideActors && GetLevel()->Hash )
		GetLevel()->Hash->AddActor( this );

	unguard;
}

//
// Return whether this actor overlaps another.
//
UBOOL AActor::IsOverlapping( const AActor* Other ) const
{
	guardSlow(AActor::IsOverlapping);
	debug(Other!=NULL);

	if( !IsBrush() && !Other->IsBrush() && Other!=Level )
	{
		// See if cylinder actors are overlapping.
		return
			Square(Location.X      - Other->Location.X)
		+	Square(Location.Y      - Other->Location.Y)
		<	Square(CollisionRadius + Other->CollisionRadius) 
		&&	Square(Location.Z      - Other->Location.Z)
		<	Square(CollisionHeight + Other->CollisionHeight);
	}
	else
	{
		// We cannot detect whether these actors are overlapping so we say they aren't.
		return 0;
	}
	unguardSlow;
}

/*-----------------------------------------------------------------------------
	Actor touch minions.
-----------------------------------------------------------------------------*/

static UBOOL TouchTo( AActor* Actor, AActor* Other )
{
	guard(TouchTo);
	check(Actor);
	check(Other);
	check(Actor!=Other);

	INT Available=-1;
	INT i;
	for( i=0; i<ARRAY_COUNT(Actor->Touching); i++ )
	{
		if( Actor->Touching[i] == NULL )
		{
			// Found an available slot.
			Available = i;
		}
		else if( Actor->Touching[i] == Other )
		{
			// Already touching.
			return 1;
		}
	}
	if( Available == -1 )
	{
		// Try to prune touches.
		for( i=0; i<ARRAY_COUNT(Actor->Touching); i++ )
		{
			check(Actor->Touching[i]->IsValid());
			if( Actor->Touching[i]->Physics == PHYS_None )
			{
				Actor->EndTouch( Actor->Touching[i], 0 );
				Available = i;
			}
		}
		if ( (Available == -1) && Other->IsA(APawn::StaticClass) )
		{
			// try to prune in favor of 1. players, 2. other pawns
			for( i=0; i<ARRAY_COUNT(Actor->Touching); i++ )
			{
				if( !Actor->Touching[i]->IsA(APawn::StaticClass) )
				{
					Actor->EndTouch( Actor->Touching[i], 0 );
					Available = i;
					break;
				}
			}
			if ( (Available == -1) && ((APawn *)Other)->bIsPlayer )
				for( i=0; i<ARRAY_COUNT(Actor->Touching); i++ )
				{
					if( !Actor->Touching[i]->IsA(APawn::StaticClass) || !((APawn *)Actor->Touching[i])->bIsPlayer )
					{
						Actor->EndTouch( Actor->Touching[i], 0 );
						Available = i;
						break;
					}
				}
		}
	}

	if( Available >= 0 )
	{
		// Make Actor touch TouchActor.
		Actor->Touching[Available] = Other;
		Actor->eventTouch( Other );

		// See if first actor did something that caused an UnTouch.
		if( Actor->Touching[Available] != Other )
			return 0;
	}

	return 1;
	unguard;
}

//
// Note that TouchActor has begun touching Actor.
//
// If an actor's touch list overflows, neither actor receives the
// touch messages, as if they are not touching.
//
// This routine is reflexive.
//
// Handles the case of the first-notified actor changing its touch status.
//
void AActor::BeginTouch( AActor* Other )
{
	guard(AActor::BeginTouch);

	// Perform reflective touch.
	if( TouchTo( this, Other ) )
		TouchTo( Other, this );

	unguard;
}

//
// Note that TouchActor is no longer touching Actor.
//
// If NoNotifyActor is specified, Actor is not notified but
// TouchActor is (this happens during actor destruction).
//
void AActor::EndTouch( AActor* Other, UBOOL NoNotifySelf )
{
	guard(AActor::EndTouch);
	check(Other!=this);

	// Notify Actor.
	int i;
	for( i=0; i<ARRAY_COUNT(Touching); i++ )
	{
		if( Touching[i] == Other )
		{
			Touching[i] = NULL;
			if( !NoNotifySelf )
				eventUnTouch( Other );
			break;
		}
	}

	// Notify TouchActor.
	for( i=0; i<ARRAY_COUNT(Other->Touching); i++ )
	{
		if( Other->Touching[i] == this )
		{
			Other->Touching[i] = NULL;
			Other->eventUnTouch( this );
			break;
		}
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	AActor member functions.
-----------------------------------------------------------------------------*/

//
// Destroy the actor.
//
void AActor::Serialize( FArchive& Ar )
{
	guard(AActor::Serialize);
	Super::Serialize( Ar );
	if( Ar.Ver()<57 )//oldver
		InitialState = GObj.GetTempState();
	if( Ar.Ver()<58 )
		Group = GObj.GetTempGroup();
	unguard;
}

/*-----------------------------------------------------------------------------
	Relations.
-----------------------------------------------------------------------------*/

//
// Change the actor's owner.
//
void AActor::SetOwner( AActor *NewOwner )
{
	guard(AActor::SetOwner);

	// Sets this actor's parent to the specified actor.
	if( Owner != NULL )
		Owner->eventLostChild( this );

	Owner = NewOwner;

	if( Owner != NULL )
		Owner->eventGainedChild( this );

	unguard;
}

//
// Change the actor's base.
//
void AActor::SetBase( AActor* NewBase, int bNotifyActor )
{
	guard(AActor::SetBase);
	//debugf("SetBase %s -> %s",GetName(),NewBase ? NewBase->GetName() : "NULL");

	// Verify no recursion.
	for( AActor* Loop=NewBase; Loop!=NULL; Loop=Loop->Base )
		if ( Loop == this ) 
			return;

	if( NewBase != Base )
	{
		// Notify old base, unless it's the level.
		if( Base && Base!=Level )
		{
			Base->StandingCount--;
			Base->eventDetach( this );
		}

		// Set base.
		Base = NewBase;

		// Notify new base, unless it's the level.
		if( Base && Base!=Level )
		{
			Base->StandingCount++;
			Base->eventAttach( this );
		}

		// Notify this actor of his new floor.
		if ( bNotifyActor )
			eventBaseChange();
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	The end.
-----------------------------------------------------------------------------*/
