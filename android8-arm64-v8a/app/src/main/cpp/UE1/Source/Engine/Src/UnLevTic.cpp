/*=============================================================================
	UnLevTic.cpp: Level timer tick function
	Copyright 1997 Epic MegaGames, Inc. This software is a trade secret.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "EnginePrivate.h"
#include "UnNet.h"

#if PLATFORM_64BIT && (defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__))
#include <android/log.h>
#define UE1_ANDROID64_INPUT_FLOW_LOG(...) __android_log_print(ANDROID_LOG_INFO, "UE1Diag64", __VA_ARGS__)
#else
#define UE1_ANDROID64_INPUT_FLOW_LOG(...)
#endif

#if PLATFORM_64BIT
// UNREAL_ANDROID64_SAFE_STATEFRAME_PROBE_GUARD_V75
// The original 32-bit flow assumes gameplay actors keep a valid MainFrame while
// probe-driven callbacks are evaluated.  In the current arm64 port some pawns can
// temporarily lose that frame; do not dereference/probe such a broken frame.
static inline UBOOL UE1Android64HasStateFrameForScriptProbeV75( AActor* Actor )
{
	if( !Actor )
		return 0;
	FMainFrame* Frame = Actor->GetMainFrame();
	return Frame && Frame->StateNode;
}
#endif

#if PLATFORM_64BIT
// UNREAL_ANDROID64_PLAYER_INPUT_FLOW_PROBE_V78
// Pure diagnostics: follow the local PlayerPawn input path without mutating gameplay.
static INT GUE1Android64InputFlowBudgetV78 = 0;

static inline INT UE1Android64InputFlowAbsIntV78( FLOAT Value )
{
	return Abs((INT)(Value * 1000.0f));
}

static const char* UE1Android64InputFlowStateNameV78( AActor* Actor )
{
	if( !Actor )
		return "<null>";
	FMainFrame* Frame = Actor->GetMainFrame();
	if( !Frame )
		return "<no-frame>";
	if( !Frame->StateNode )
		return "<no-state>";
	return Frame->StateNode->GetName();
}

static INT UE1Android64InputFlowCodeOffsetV78( AActor* Actor )
{
	if( !Actor )
		return INDEX_NONE;
	FMainFrame* Frame = Actor->GetMainFrame();
	if( !Frame || !Frame->Code || !Frame->Node || Frame->Node->Script.Num() <= 0 )
		return INDEX_NONE;
	BYTE* ScriptBase = &Frame->Node->Script(0);
	BYTE* ScriptEnd  = ScriptBase + Frame->Node->Script.Num();
	if( Frame->Code < ScriptBase || Frame->Code >= ScriptEnd )
		return INDEX_NONE;
	return (INT)(Frame->Code - ScriptBase);
}

static INT UE1Android64InputFlowLatentV78( AActor* Actor )
{
	FMainFrame* Frame = Actor ? Actor->GetMainFrame() : NULL;
	return Frame ? Frame->LatentAction : INDEX_NONE;
}

static const char* UE1Android64InputFlowPhysicsNameV78( BYTE Physics )
{
	switch( Physics )
	{
		case PHYS_None:          return "None";
		case PHYS_Walking:       return "Walking";
		case PHYS_Falling:       return "Falling";
		case PHYS_Swimming:      return "Swimming";
		case PHYS_Flying:        return "Flying";
		case PHYS_Rotating:      return "Rotating";
		case PHYS_Projectile:    return "Projectile";
		case PHYS_Rolling:       return "Rolling";
		case PHYS_Interpolating: return "Interpolating";
		case PHYS_MovingBrush:   return "MovingBrush";
		case PHYS_Spider:        return "Spider";
		case PHYS_Trailer:       return "Trailer";
		default:                 return "Unknown";
	}
}

static UBOOL UE1Android64InputFlowShouldLogV78( const char* Phase, APlayerPawn* PlayerPawn )
{
	// UNREAL_ANDROID64_FRAME_LOSS_SPARSE_ORIGIN_V81
	// v78/v80 produced so much logcat output that the actual first MainFrame-loss
	// transition scrolled out of the Android log buffer.  For v81 keep the
	// instrumentation compiled in, but silence the noisy per-frame input flow.
	// The sparse V81 frame watcher below is now the source of truth.
	return 0;
}

static void UE1Android64InputFlowProbeV78( const char* Phase, APlayerPawn* PlayerPawn, FLOAT DeltaSeconds )
{
	if( !UE1Android64InputFlowShouldLogV78( Phase, PlayerPawn ) )
		return;
	GUE1Android64InputFlowBudgetV78++;
	FMainFrame* Frame = PlayerPawn ? PlayerPawn->GetMainFrame() : NULL;
	UE1_ANDROID64_INPUT_FLOW_LOG(
		"ANDROID64 INPUT FLOW V78 phase=%s player=%s class=%s dt=%.5f frame=%p state=%s latent=%i code=%i physics=%i/%s loc=(%.1f %.1f %.1f) vel=(%.1f %.1f %.1f) acc=(%.1f %.1f %.1f) input=(f=%.3f s=%.3f u=%.3f turn=%.3f look=%.3f mx=%.3f my=%.3f bx=%.3f by=%.3f bz=%.3f) flags=(frozen=%i menu=%i jump=%i woke=%i collWorld=%i collAct=%i blockAct=%i interp=%i) playerPtr=%p base=%s viewTarget=%s health=%i role=%i remote=%i budget=%i",
		Phase ? Phase : "?",
		PlayerPawn ? PlayerPawn->GetFullName() : "<null>",
		PlayerPawn ? PlayerPawn->GetClassName() : "<null>",
		DeltaSeconds,
		Frame,
		UE1Android64InputFlowStateNameV78(PlayerPawn),
		UE1Android64InputFlowLatentV78(PlayerPawn),
		UE1Android64InputFlowCodeOffsetV78(PlayerPawn),
		PlayerPawn ? (INT)PlayerPawn->Physics : -1,
		PlayerPawn ? UE1Android64InputFlowPhysicsNameV78(PlayerPawn->Physics) : "<null>",
		PlayerPawn ? PlayerPawn->Location.X : 0.f, PlayerPawn ? PlayerPawn->Location.Y : 0.f, PlayerPawn ? PlayerPawn->Location.Z : 0.f,
		PlayerPawn ? PlayerPawn->Velocity.X : 0.f, PlayerPawn ? PlayerPawn->Velocity.Y : 0.f, PlayerPawn ? PlayerPawn->Velocity.Z : 0.f,
		PlayerPawn ? PlayerPawn->Acceleration.X : 0.f, PlayerPawn ? PlayerPawn->Acceleration.Y : 0.f, PlayerPawn ? PlayerPawn->Acceleration.Z : 0.f,
		PlayerPawn ? PlayerPawn->aForward : 0.f,
		PlayerPawn ? PlayerPawn->aStrafe : 0.f,
		PlayerPawn ? PlayerPawn->aUp : 0.f,
		PlayerPawn ? PlayerPawn->aTurn : 0.f,
		PlayerPawn ? PlayerPawn->aLookUp : 0.f,
		PlayerPawn ? PlayerPawn->aMouseX : 0.f,
		PlayerPawn ? PlayerPawn->aMouseY : 0.f,
		PlayerPawn ? PlayerPawn->aBaseX : 0.f,
		PlayerPawn ? PlayerPawn->aBaseY : 0.f,
		PlayerPawn ? PlayerPawn->aBaseZ : 0.f,
		PlayerPawn ? (INT)PlayerPawn->bFrozen : 0,
		PlayerPawn ? (INT)PlayerPawn->bShowMenu : 0,
		PlayerPawn ? (INT)PlayerPawn->bPressedJump : 0,
		PlayerPawn ? (INT)PlayerPawn->bWokeUp : 0,
		PlayerPawn ? (INT)PlayerPawn->bCollideWorld : 0,
		PlayerPawn ? (INT)PlayerPawn->bCollideActors : 0,
		PlayerPawn ? (INT)PlayerPawn->bBlockActors : 0,
		PlayerPawn ? (INT)PlayerPawn->bInterpolating : 0,
		PlayerPawn ? PlayerPawn->Player : NULL,
		(PlayerPawn && PlayerPawn->Base) ? PlayerPawn->Base->GetFullName() : "<none>",
		(PlayerPawn && PlayerPawn->ViewTarget) ? PlayerPawn->ViewTarget->GetFullName() : "<none>",
		PlayerPawn ? (INT)PlayerPawn->Health : 0,
		PlayerPawn ? (INT)PlayerPawn->Role : -1,
		PlayerPawn ? (INT)PlayerPawn->RemoteRole : -1,
		GUE1Android64InputFlowBudgetV78 );
}
#endif

// UNREAL_ANDROID64_PLAYER_FRAME_LOSS_BOUNDARY_PROBE_V80
// UNREAL_ANDROID64_FRAME_LOSS_SPARSE_ORIGIN_V81
// Pure diagnostics, now deliberately sparse: v80/v78 spam filled logcat and the
// first real MainFrame-loss transition was gone before the log was captured.
// v81 reports only the first observed frame, first valid frame, frame changes,
// frame loss, large dt stalls and physics changes for the local PlayerPawn.
static FMainFrame* GUE1Android64LastPlayerFrameV80 = NULL;
static BYTE        GUE1Android64LastPlayerPhysicsV80 = 255;
static INT         GUE1Android64FrameLossBudgetV80 = 0;
static UBOOL       GUE1Android64ObservedPlayerV81 = 0;
static UBOOL       GUE1Android64EverHadPlayerFrameV81 = 0;
static UBOOL       GUE1Android64ReportedInitialNullV81 = 0;
static FLOAT       GUE1Android64LastPlayerX_V80 = 0.f;
static FLOAT       GUE1Android64LastPlayerY_V80 = 0.f;
static FLOAT       GUE1Android64LastPlayerZ_V80 = 0.f;

static void UE1Android64FrameLossBoundaryProbeV80( const char* Phase, APlayerPawn* PlayerPawn, FLOAT DeltaSeconds )
{
	if( !PlayerPawn || !PlayerPawn->Player )
		return;

	FMainFrame* Frame = PlayerPawn->GetMainFrame();
	FMainFrame* LastFrame = GUE1Android64LastPlayerFrameV80;
	UBOOL bFirstObserved = !GUE1Android64ObservedPlayerV81;
	UBOOL bFirstValid    = Frame && !GUE1Android64EverHadPlayerFrameV81;
	UBOOL bFrameChanged  = (Frame != LastFrame);
	UBOOL bFrameLost     = (LastFrame != NULL && Frame == NULL);
	UBOOL bInitialNull   = (!Frame && !GUE1Android64EverHadPlayerFrameV81 && !GUE1Android64ReportedInitialNullV81);
	UBOOL bPhysicsChanged = (GUE1Android64LastPlayerPhysicsV80 != 255 && PlayerPawn->Physics != GUE1Android64LastPlayerPhysicsV80);
	UBOOL bLargeDelta = DeltaSeconds >= 0.250f;
	FLOAT MoveSq = Square(PlayerPawn->Location.X - GUE1Android64LastPlayerX_V80)
	             + Square(PlayerPawn->Location.Y - GUE1Android64LastPlayerY_V80)
	             + Square(PlayerPawn->Location.Z - GUE1Android64LastPlayerZ_V80);

	const char* Reason = NULL;
	if( bFrameLost )
		Reason = "lost-from-valid-v81";
	else if( bFirstValid )
		Reason = "first-valid-v81";
	else if( bInitialNull )
		Reason = "already-null-first-observed-v81";
	else if( bFirstObserved )
		Reason = "first-observed-v81";
	else if( bFrameChanged )
		Reason = "frame-changed-v81";
	else if( bLargeDelta )
		Reason = "large-dt-v81";
	else if( bPhysicsChanged )
		Reason = "physics-changed-v81";
	else if( MoveSq > 40000.f )
		Reason = "large-move-v81";

	if( Reason && GUE1Android64FrameLossBudgetV80 < 96 )
	{
		GUE1Android64FrameLossBudgetV80++;
		UE1_ANDROID64_INPUT_FLOW_LOG(
			"ANDROID64 FRAME WATCH V81 reason=%s phase=%s player=%s dt=%.5f levelTime=%.3f frame=%p lastFrame=%p changed=%i lost=%i everValid=%i state=%s latent=%i code=%i physics=%i/%s lastPhysics=%i loc=(%.1f %.1f %.1f) vel=(%.1f %.1f %.1f) acc=(%.1f %.1f %.1f) input=(f=%.3f s=%.3f turn=%.3f look=%.3f bx=%.3f by=%.3f bz=%.3f) base=%s health=%i role=%i remote=%i moveSq=%.1f budget=%i",
			Reason,
			Phase ? Phase : "?",
			PlayerPawn->GetFullName(),
			DeltaSeconds,
			(PlayerPawn->XLevel ? PlayerPawn->XLevel->TimeSeconds : 0.f),
			Frame,
			LastFrame,
			(INT)bFrameChanged,
			(INT)bFrameLost,
			(INT)GUE1Android64EverHadPlayerFrameV81,
			UE1Android64InputFlowStateNameV78(PlayerPawn),
			UE1Android64InputFlowLatentV78(PlayerPawn),
			UE1Android64InputFlowCodeOffsetV78(PlayerPawn),
			(INT)PlayerPawn->Physics,
			UE1Android64InputFlowPhysicsNameV78(PlayerPawn->Physics),
			(INT)GUE1Android64LastPlayerPhysicsV80,
			PlayerPawn->Location.X, PlayerPawn->Location.Y, PlayerPawn->Location.Z,
			PlayerPawn->Velocity.X, PlayerPawn->Velocity.Y, PlayerPawn->Velocity.Z,
			PlayerPawn->Acceleration.X, PlayerPawn->Acceleration.Y, PlayerPawn->Acceleration.Z,
			PlayerPawn->aForward, PlayerPawn->aStrafe, PlayerPawn->aTurn, PlayerPawn->aLookUp,
			PlayerPawn->aBaseX, PlayerPawn->aBaseY, PlayerPawn->aBaseZ,
			PlayerPawn->Base ? PlayerPawn->Base->GetFullName() : "<none>",
			(INT)PlayerPawn->Health,
			(INT)PlayerPawn->Role,
			(INT)PlayerPawn->RemoteRole,
			MoveSq,
			GUE1Android64FrameLossBudgetV80 );
	}

	GUE1Android64ObservedPlayerV81 = 1;
	if( Frame )
		GUE1Android64EverHadPlayerFrameV81 = 1;
	if( bInitialNull )
		GUE1Android64ReportedInitialNullV81 = 1;

	GUE1Android64LastPlayerFrameV80 = Frame;
	GUE1Android64LastPlayerPhysicsV80 = PlayerPawn->Physics;
	GUE1Android64LastPlayerX_V80 = PlayerPawn->Location.X;
	GUE1Android64LastPlayerY_V80 = PlayerPawn->Location.Y;
	GUE1Android64LastPlayerZ_V80 = PlayerPawn->Location.Z;
}


// UNREAL_ANDROID64_CROSS_ACTOR_FRAME_OWNER_PROBE_V84
// UNREAL_ANDROID64_CROSS_ACTOR_FRAME_EXIT_PROBE_V85
// Diagnostics only: v83/v84 showed no MoveActor/collision blocker, but a global
// MainFrame loss: a Skaarj and the local Player both become <no-frame> after a
// large dt=0.400 tick.  v84 used debugf(), which did not reliably surface in
// Android logcat.  v85 logs through UE1Diag64 and samples both tick-enter and
// tick-exit, so the actor whose tick just caused the local player's MainFrame
// to disappear is visible.  No gameplay state is repaired or changed.
static FMainFrame* GUE1Android64CrossActorPlayerFrameV84 = NULL;
static char        GUE1Android64CrossActorLastActorV84[256] = "<none>";
static char        GUE1Android64CrossActorLastClassV84[128] = "<none>";
static INT         GUE1Android64CrossActorBudgetV84 = 0;

static APlayerPawn* UE1Android64FindLocalPlayerPawnV84( ULevel* Level )
{
	if( !Level )
		return NULL;
	for( INT i=0; i<Level->Num(); i++ )
	{
		AActor* Actor = Level->Actors(i);
		APlayerPawn* PlayerPawn = Actor ? Cast<APlayerPawn>(Actor) : NULL;
		if( PlayerPawn && PlayerPawn->Player )
			return PlayerPawn;
	}
	return NULL;
}

static INT UE1Android64FrameCodeOffsetV84( FMainFrame* Frame )
{
	if( !Frame || !Frame->Node || !Frame->Code || Frame->Node->Script.Num() <= 0 )
		return INDEX_NONE;
	BYTE* Base = &Frame->Node->Script(0);
	BYTE* End  = Base + Frame->Node->Script.Num();
	return (Frame->Code >= Base && Frame->Code < End) ? (INT)(Frame->Code - Base) : INDEX_NONE;
}

static const char* UE1Android64StateNameV84( AActor* Actor )
{
	if( !Actor )
		return "<none>";
	FMainFrame* Frame = Actor->GetMainFrame();
	if( !Frame )
		return "<no-frame>";
	return Frame->StateNode ? Frame->StateNode->GetName() : "<no-state>";
}

static void UE1Android64CrossActorFrameOwnerProbeV84( const char* Phase, AActor* CurrentActor, FLOAT DeltaSeconds )
{
	if( !CurrentActor || !CurrentActor->XLevel || GUE1Android64CrossActorBudgetV84 >= 120 )
		return;

	APlayerPawn* PlayerPawn = UE1Android64FindLocalPlayerPawnV84( CurrentActor->XLevel );
	if( !PlayerPawn )
		return;

	FMainFrame* PlayerFrame = PlayerPawn->GetMainFrame();
	UBOOL bLostSinceLastSample = (GUE1Android64CrossActorPlayerFrameV84 != NULL && PlayerFrame == NULL);
	UBOOL bFrameChanged = (GUE1Android64CrossActorPlayerFrameV84 != PlayerFrame);
	UBOOL bLargeDt = DeltaSeconds >= 0.250f;
	UBOOL bCurrentInteresting = CurrentActor->IsA(APawn::StaticClass) || CurrentActor==PlayerPawn;
	UBOOL bTickExit = (Phase && appStrstr( Phase, "exit" ) != NULL);

	// v85: log through Android logcat directly.  The earlier v84 debugf(NAME_Warning)
	// messages did not appear in the captured logcat, even though the underlying
	// frame loss still happened.  Keep the signal sparse: lost frame, large-dt frame
	// change, or early large-dt pawn/player context.
	if( bLostSinceLastSample || (bLargeDt && bFrameChanged) )
	{
		FMainFrame* CurrentFrame = CurrentActor->GetMainFrame();
		UE1_ANDROID64_INPUT_FLOW_LOG(
			"ANDROID64 CROSS ACTOR FRAME V85 phase=%s current=%s currentClass=%s currentState=%s currentFrame=%p currentCode=%i currentPhys=%i currentLoc=(%.1f %.1f %.1f) currentVel=(%.1f %.1f %.1f) currentAcc=(%.1f %.1f %.1f) player=%s playerFrame=%p prevPlayerFrame=%p playerState=%s playerCode=%i playerPhys=%i playerLoc=(%.1f %.1f %.1f) playerVel=(%.1f %.1f %.1f) playerAcc=(%.1f %.1f %.1f) previousActor=%s previousClass=%s dt=%.5f lostSinceLast=%i changed=%i tickExit=%i budget=%i",
			Phase ? Phase : "?",
			CurrentActor->GetFullName(),
			CurrentActor->GetClassName(),
			UE1Android64StateNameV84(CurrentActor),
			CurrentFrame,
			UE1Android64FrameCodeOffsetV84(CurrentFrame),
			(INT)CurrentActor->Physics,
			CurrentActor->Location.X, CurrentActor->Location.Y, CurrentActor->Location.Z,
			CurrentActor->Velocity.X, CurrentActor->Velocity.Y, CurrentActor->Velocity.Z,
			CurrentActor->Acceleration.X, CurrentActor->Acceleration.Y, CurrentActor->Acceleration.Z,
			PlayerPawn->GetFullName(),
			PlayerFrame,
			GUE1Android64CrossActorPlayerFrameV84,
			UE1Android64StateNameV84(PlayerPawn),
			UE1Android64FrameCodeOffsetV84(PlayerFrame),
			(INT)PlayerPawn->Physics,
			PlayerPawn->Location.X, PlayerPawn->Location.Y, PlayerPawn->Location.Z,
			PlayerPawn->Velocity.X, PlayerPawn->Velocity.Y, PlayerPawn->Velocity.Z,
			PlayerPawn->Acceleration.X, PlayerPawn->Acceleration.Y, PlayerPawn->Acceleration.Z,
			GUE1Android64CrossActorLastActorV84,
			GUE1Android64CrossActorLastClassV84,
			DeltaSeconds,
			(INT)bLostSinceLastSample,
			(INT)bFrameChanged,
			(INT)bTickExit,
			++GUE1Android64CrossActorBudgetV84 );
	}
	else if( bCurrentInteresting && bLargeDt && GUE1Android64CrossActorBudgetV84 < 12 )
	{
		UE1_ANDROID64_INPUT_FLOW_LOG(
			"ANDROID64 CROSS ACTOR FRAME V85 phase=%s current=%s currentClass=%s currentState=%s player=%s playerFrame=%p prevPlayerFrame=%p playerState=%s playerPhys=%i previousActor=%s previousClass=%s dt=%.5f lostSinceLast=0 changed=%i tickExit=%i budget=%i",
			Phase ? Phase : "?",
			CurrentActor->GetFullName(),
			CurrentActor->GetClassName(),
			UE1Android64StateNameV84(CurrentActor),
			PlayerPawn->GetFullName(),
			PlayerFrame,
			GUE1Android64CrossActorPlayerFrameV84,
			UE1Android64StateNameV84(PlayerPawn),
			(INT)PlayerPawn->Physics,
			GUE1Android64CrossActorLastActorV84,
			GUE1Android64CrossActorLastClassV84,
			DeltaSeconds,
			(INT)bFrameChanged,
			(INT)bTickExit,
			++GUE1Android64CrossActorBudgetV84 );
	}

	GUE1Android64CrossActorPlayerFrameV84 = PlayerFrame;
	appStrncpy( GUE1Android64CrossActorLastActorV84, CurrentActor->GetFullName(), ARRAY_COUNT(GUE1Android64CrossActorLastActorV84) );
	appStrncpy( GUE1Android64CrossActorLastClassV84, CurrentActor->GetClassName(), ARRAY_COUNT(GUE1Android64CrossActorLastClassV84) );
}


// UNREAL_ANDROID64_LEVEL_ACTOR_TICK_FRAME_BOUNDARY_V87
// v85 proved the local player's MainFrame can disappear between one actor sample
// and the next actor's tick-enter.  This level-loop probe wraps the actual
// Actor->Tick() call, so early returns inside AActor::Tick cannot hide the
// actor boundary anymore.  Diagnostics only; no gameplay state is changed.
static INT GUE1Android64LevelActorBoundaryBudgetV87 = 0;

static FMainFrame* UE1Android64LevelPlayerFrameSnapshotV87( ULevel* Level )
{
	APlayerPawn* PlayerPawn = UE1Android64FindLocalPlayerPawnV84( Level );
	return PlayerPawn ? PlayerPawn->GetMainFrame() : NULL;
}

static void UE1Android64LevelActorTickBoundaryProbeV87( const char* Phase, ULevel* Level, AActor* Actor, FLOAT DeltaSeconds, FMainFrame* PlayerFrameBefore, INT TickResult )
{
	if( !Level || !Actor || GUE1Android64LevelActorBoundaryBudgetV87 >= 96 )
		return;

	APlayerPawn* PlayerPawn = UE1Android64FindLocalPlayerPawnV84( Level );
	if( !PlayerPawn )
		return;

	FMainFrame* PlayerFrameAfter = PlayerPawn->GetMainFrame();
	UBOOL bPlayerLost = (PlayerFrameBefore != NULL && PlayerFrameAfter == NULL);
	UBOOL bFrameChanged = (PlayerFrameBefore != PlayerFrameAfter);
	UBOOL bLargeDt = DeltaSeconds >= 0.250f;
	UBOOL bInterestingActor = Actor->IsA(APawn::StaticClass)
		|| Actor->IsA(AMover::StaticClass)
		|| appStrstr( Actor->GetClassName(), "Translator" ) != NULL
		|| appStrstr( Actor->GetClassName(), "Dispatcher" ) != NULL
		|| appStrstr( Actor->GetClassName(), "Trigger" ) != NULL
		|| appStrstr( Actor->GetFullName(), "Translator" ) != NULL
		|| appStrstr( Actor->GetFullName(), "Mover" ) != NULL;

	if( !bPlayerLost && !(bLargeDt && bFrameChanged) && !(bLargeDt && bInterestingActor && GUE1Android64LevelActorBoundaryBudgetV87 < 24) )
		return;

	FMainFrame* ActorFrame = Actor->GetMainFrame();
	UE1_ANDROID64_INPUT_FLOW_LOG(
		"ANDROID64 LEVEL ACTOR BOUNDARY V87 phase=%s actor=%s actorClass=%s actorState=%s actorFrame=%p actorCode=%i actorPhys=%i actorLoc=(%.1f %.1f %.1f) player=%s playerFrameBefore=%p playerFrameAfter=%p playerLost=%i changed=%i playerState=%s playerCode=%i playerPhys=%i playerLoc=(%.1f %.1f %.1f) playerVel=(%.1f %.1f %.1f) playerAcc=(%.1f %.1f %.1f) dt=%.5f tickResult=%i budget=%i",
		Phase ? Phase : "?",
		Actor->GetFullName(),
		Actor->GetClassName(),
		UE1Android64StateNameV84(Actor),
		ActorFrame,
		UE1Android64FrameCodeOffsetV84(ActorFrame),
		(INT)Actor->Physics,
		Actor->Location.X, Actor->Location.Y, Actor->Location.Z,
		PlayerPawn->GetFullName(),
		PlayerFrameBefore,
		PlayerFrameAfter,
		(INT)bPlayerLost,
		(INT)bFrameChanged,
		UE1Android64StateNameV84(PlayerPawn),
		UE1Android64FrameCodeOffsetV84(PlayerFrameAfter),
		(INT)PlayerPawn->Physics,
		PlayerPawn->Location.X, PlayerPawn->Location.Y, PlayerPawn->Location.Z,
		PlayerPawn->Velocity.X, PlayerPawn->Velocity.Y, PlayerPawn->Velocity.Z,
		PlayerPawn->Acceleration.X, PlayerPawn->Acceleration.Y, PlayerPawn->Acceleration.Z,
		DeltaSeconds,
		TickResult,
		++GUE1Android64LevelActorBoundaryBudgetV87 );
}

// UNREAL_ANDROID64_EVENT_CHAIN_FRAME_BOUNDARY_PROBE_V90
// Diagnostics only: v87/v85 narrowed the frame loss to the boundary around
// Translator/TranslatorEvent/Mover/Dispatcher/Trigger activity.  v90 samples the
// level actor loop before and after each relevant actor tick, so a loss that
// happens between the previous actor's tick-exit and the next actor's tick-enter
// is visible instead of only showing up as "previousActor=Translator" later.
static INT         GUE1Android64EventBoundaryBudgetV90 = 0;
static FMainFrame* GUE1Android64EventBoundaryLastPlayerFrameV90 = NULL;
static char        GUE1Android64EventBoundaryLastActorV90[256] = "<none>";
static char        GUE1Android64EventBoundaryLastPhaseV90[96] = "<none>";

static UBOOL UE1Android64EventBoundaryTokenV90( const char* Text, const char* Token )
{
	return Text && Token && appStrstr( Text, Token ) != NULL;
}

static UBOOL UE1Android64EventBoundaryInterestingActorV90( AActor* Actor )
{
	if( !Actor )
		return 0;
	const char* ClassName = Actor->GetClassName();
	const char* FullName  = Actor->GetFullName();
	return Actor->IsA(AMover::StaticClass)
		|| UE1Android64EventBoundaryTokenV90( ClassName, "Translator" )
		|| UE1Android64EventBoundaryTokenV90( ClassName, "TranslatorEvent" )
		|| UE1Android64EventBoundaryTokenV90( ClassName, "Dispatcher" )
		|| UE1Android64EventBoundaryTokenV90( ClassName, "Trigger" )
		|| UE1Android64EventBoundaryTokenV90( FullName,  "Translator" )
		|| UE1Android64EventBoundaryTokenV90( FullName,  "Mover18" )
		|| UE1Android64EventBoundaryTokenV90( FullName,  "Mover" )
		|| UE1Android64EventBoundaryTokenV90( FullName,  "Dispatcher" )
		|| UE1Android64EventBoundaryTokenV90( FullName,  "Trigger" );
}

static void UE1Android64EventChainBoundaryProbeV90( const char* Phase, ULevel* Level, AActor* Actor, FLOAT DeltaSeconds, FMainFrame* PlayerFrameBeforeActor, INT TickResult )
{
	if( !Level || !Actor || GUE1Android64EventBoundaryBudgetV90 >= 220 )
		return;

	APlayerPawn* PlayerPawn = UE1Android64FindLocalPlayerPawnV84( Level );
	if( !PlayerPawn )
		return;

	FMainFrame* PlayerFrameNow = PlayerPawn->GetMainFrame();
	FMainFrame* ActorFrame = Actor->GetMainFrame();
	UBOOL bInterestingActor = UE1Android64EventBoundaryInterestingActorV90( Actor );
	UBOOL bLargeDt = DeltaSeconds >= 0.250f;
	UBOOL bLostSinceLastBoundary = (GUE1Android64EventBoundaryLastPlayerFrameV90 != NULL && PlayerFrameNow == NULL);
	UBOOL bLostDuringActor = (PlayerFrameBeforeActor != NULL && PlayerFrameNow == NULL);
	UBOOL bPlayerFrameChangedSinceLast = (GUE1Android64EventBoundaryLastPlayerFrameV90 != PlayerFrameNow);
	UBOOL bActorNoFrame = (ActorFrame == NULL && bInterestingActor);

	if( bLostSinceLastBoundary || bLostDuringActor || (bLargeDt && bInterestingActor) || (bActorNoFrame && bLargeDt) )
	{
		UE1_ANDROID64_INPUT_FLOW_LOG(
			"ANDROID64 EVENT BOUNDARY V90 phase=%s actor=%s actorClass=%s actorState=%s actorFrame=%p actorCode=%i actorPhys=%i actorLoc=(%.1f %.1f %.1f) player=%s playerFrameBeforeActor=%p playerFrameNow=%p lastPlayerFrame=%p playerState=%s playerCode=%i playerPhys=%i playerLoc=(%.1f %.1f %.1f) playerVel=(%.1f %.1f %.1f) playerAcc=(%.1f %.1f %.1f) previousBoundaryActor=%s previousBoundaryPhase=%s lostSinceLastBoundary=%i lostDuringActor=%i changedSinceLast=%i largeDt=%i tickResult=%i dt=%.5f budget=%i",
			Phase ? Phase : "?",
			Actor->GetFullName(),
			Actor->GetClassName(),
			UE1Android64StateNameV84(Actor),
			ActorFrame,
			UE1Android64FrameCodeOffsetV84(ActorFrame),
			(INT)Actor->Physics,
			Actor->Location.X, Actor->Location.Y, Actor->Location.Z,
			PlayerPawn->GetFullName(),
			PlayerFrameBeforeActor,
			PlayerFrameNow,
			GUE1Android64EventBoundaryLastPlayerFrameV90,
			UE1Android64StateNameV84(PlayerPawn),
			UE1Android64FrameCodeOffsetV84(PlayerFrameNow),
			(INT)PlayerPawn->Physics,
			PlayerPawn->Location.X, PlayerPawn->Location.Y, PlayerPawn->Location.Z,
			PlayerPawn->Velocity.X, PlayerPawn->Velocity.Y, PlayerPawn->Velocity.Z,
			PlayerPawn->Acceleration.X, PlayerPawn->Acceleration.Y, PlayerPawn->Acceleration.Z,
			GUE1Android64EventBoundaryLastActorV90,
			GUE1Android64EventBoundaryLastPhaseV90,
			(INT)bLostSinceLastBoundary,
			(INT)bLostDuringActor,
			(INT)bPlayerFrameChangedSinceLast,
			(INT)bLargeDt,
			TickResult,
			DeltaSeconds,
			++GUE1Android64EventBoundaryBudgetV90 );
	}

	GUE1Android64EventBoundaryLastPlayerFrameV90 = PlayerFrameNow;
	appStrncpy( GUE1Android64EventBoundaryLastActorV90, Actor->GetFullName(), ARRAY_COUNT(GUE1Android64EventBoundaryLastActorV90) );
	appStrncpy( GUE1Android64EventBoundaryLastPhaseV90, Phase ? Phase : "?", ARRAY_COUNT(GUE1Android64EventBoundaryLastPhaseV90) );
}

// UNREAL_ANDROID64_LEVEL_TICK_CLEANUP_FRAME_BOUNDARY_PROBE_V91
// Pure diagnostics: v90 proved that the local PlayerPawn MainFrame is lost
// after a newly-spawned Translator boundary and before the next normal actor
// boundary.  This probes the non-actor tail of ULevel::Tick so we can see
// whether Mark/DynMark/CleanupDestroyed or the level tick handoff is involved.
static INT         GUE1Android64LevelTickCleanupBudgetV91 = 0;
static FMainFrame* GUE1Android64LevelTickCleanupLastFrameV91 = NULL;
static char        GUE1Android64LevelTickCleanupLastPhaseV91[128] = "<none>";

static void UE1Android64LevelTickCleanupProbeV91( const char* Phase, ULevel* Level, FLOAT DeltaSeconds )
{
	if( !Level || GUE1Android64LevelTickCleanupBudgetV91 >= 180 )
		return;

	APlayerPawn* PlayerPawn = UE1Android64FindLocalPlayerPawnV84( Level );
	if( !PlayerPawn )
		return;

	FMainFrame* PlayerFrameNow = PlayerPawn->GetMainFrame();
	UBOOL bLostSinceLast = (GUE1Android64LevelTickCleanupLastFrameV91 != NULL && PlayerFrameNow == NULL);
	UBOOL bChangedSinceLast = (GUE1Android64LevelTickCleanupLastFrameV91 != PlayerFrameNow);
	UBOOL bLargeDt = DeltaSeconds >= 0.250f;

	if( bLostSinceLast || bChangedSinceLast || bLargeDt )
	{
		UE1_ANDROID64_INPUT_FLOW_LOG(
			"ANDROID64 LEVEL CLEANUP V91 phase=%s player=%s frame=%p lastFrame=%p lostSinceLast=%i changed=%i previousPhase=%s dt=%.5f levelTime=%.3f playerPhys=%i playerLoc=(%.1f %.1f %.1f) playerVel=(%.1f %.1f %.1f) playerAcc=(%.1f %.1f %.1f) health=%i role=%i remote=%i ticked=%i inTick=%i budget=%i",
			Phase ? Phase : "?",
			PlayerPawn->GetFullName(),
			PlayerFrameNow,
			GUE1Android64LevelTickCleanupLastFrameV91,
			(INT)bLostSinceLast,
			(INT)bChangedSinceLast,
			GUE1Android64LevelTickCleanupLastPhaseV91,
			DeltaSeconds,
			Level->TimeSeconds,
			(INT)PlayerPawn->Physics,
			PlayerPawn->Location.X, PlayerPawn->Location.Y, PlayerPawn->Location.Z,
			PlayerPawn->Velocity.X, PlayerPawn->Velocity.Y, PlayerPawn->Velocity.Z,
			PlayerPawn->Acceleration.X, PlayerPawn->Acceleration.Y, PlayerPawn->Acceleration.Z,
			(INT)PlayerPawn->Health,
			(INT)PlayerPawn->Role,
			(INT)PlayerPawn->RemoteRole,
			(INT)Level->Ticked,
			(INT)Level->InTick,
			++GUE1Android64LevelTickCleanupBudgetV91 );
	}

	GUE1Android64LevelTickCleanupLastFrameV91 = PlayerFrameNow;
	appStrncpy( GUE1Android64LevelTickCleanupLastPhaseV91, Phase ? Phase : "?", ARRAY_COUNT(GUE1Android64LevelTickCleanupLastPhaseV91) );
}





/*-----------------------------------------------------------------------------
	Helper classes.
-----------------------------------------------------------------------------*/

//
// Priority sortable list for appSort.
//
struct FActorPriority
{
	AActor*			Actor;		// Actor.
	FActorChannel*	Channel;	// Actor channel.
	FLOAT			Priority;	// Update priority, higher = more important.
	FActorPriority()
	{}
	FActorPriority( UNetConnection* InConnection, AActor* InActor )
	{
		Actor   = InActor;
		Channel = InConnection->GetActorChannel( Actor );
		if( Channel )
		{
			// Priority of updating an existing actor.
			Priority = Actor->NetPriority * (InConnection->Driver->Time - Channel->LastUpdateTime);
		}
		else
		{
			// Priority of spawning a new actor = high.
			Priority = Actor->NetPriority * InConnection->Driver->SpawnPrioritySeconds;
		}
		if( InActor->bNetOptional )
		{
			// Update after all other actors.
			Priority -= 100000.0;
		}
	}
	friend inline INT Compare( const FActorPriority& A, const FActorPriority& B )
	{
		return B.Priority - A.Priority;
	}
};

/*-----------------------------------------------------------------------------
	Tick a single actor.
-----------------------------------------------------------------------------*/

UBOOL AActor::Tick( FLOAT DeltaSeconds, ELevelTick TickType )
{
	guard(AActor::Tick);

#if PLATFORM_64BIT
	UE1Android64CrossActorFrameOwnerProbeV84( "tick-enter-v84", this, DeltaSeconds );
#endif

	// Ignore actors in stasis
	if ( bStasis 
		&& (bForceStasis || (Physics==PHYS_None) || (Physics == PHYS_Rotating))
#if PLATFORM_64BIT
		&& XLevel && XLevel->Model && XLevel->Model->Nodes
		&& Region.ZoneNumber>=0 && Region.ZoneNumber<XLevel->Model->Nodes->NumZones
#endif
		&& (XLevel->TimeSeconds - XLevel->Model->Nodes->Zones[Region.ZoneNumber].LastRenderTime > 5)
		&& (Level->NetMode == NM_Standalone) )
		return 1;

	// Handle owner-first updating.
	if( Owner && (INT)Owner->bTicked!=XLevel->Ticked )
	{
		XLevel->NewlySpawned = new(GDynMem)FActorLink(this,XLevel->NewlySpawned);
		return 0;
	}
	bTicked = XLevel->Ticked;

	APawn* Pawn = NULL;
	if( bIsPawn )
		Pawn = Cast<APawn>(this);

	INT bSimulatedPawn = ( Pawn && (Role == ROLE_SimulatedProxy) );

	// Update all animation, including multiple passes if necessary.
	INT Iterations = 0;
	FLOAT Seconds = DeltaSeconds;
	//if ( bSimulatedPawn )
	//	debugf("Animation %s frame %f rate %f tween %f",*AnimSequence,AnimFrame, AnimRate, TweenRate);
	while
	(	IsAnimating()
	//&&	(Role>=ROLE_SimulatedProxy)
	&&	(Seconds>0.0)
	&&	(++Iterations <= 4) )
	{

		// Remember the old frame.
		FLOAT OldAnimFrame = AnimFrame;

		// Update animation, and possibly overflow it.
		if( AnimFrame >= 0.0 )
		{
			// Update regular or velocity-scaled animation.
			if( AnimRate >= 0.0 )
				AnimFrame += AnimRate * Seconds;
			else
				AnimFrame += ::Max( AnimMinRate, Velocity.Size() * -AnimRate ) * Seconds;

			// Handle all animation sequence notifys.
			if( bAnimNotify && Mesh )
			{
				const FMeshAnimSeq* Seq = Mesh->GetAnimSeq( AnimSequence );
				if( Seq )
				{
					FLOAT BestElapsedFrames = 100000.0;
					const FMeshAnimNotify* BestNotify = NULL;
					for( INT i=0; i<Seq->Notifys.Num(); i++ )
					{
						const FMeshAnimNotify& Notify = Seq->Notifys(i);
						if( OldAnimFrame<Notify.Time && AnimFrame>=Notify.Time )
						{
							FLOAT ElapsedFrames = Notify.Time - OldAnimFrame;
							if( BestNotify==NULL || ElapsedFrames<BestElapsedFrames )
							{
								BestElapsedFrames = ElapsedFrames;
								BestNotify        = &Notify;
							}
						}
					}
					if( BestNotify )
					{
						Seconds   = Seconds * (AnimFrame - BestNotify->Time) / (AnimFrame - OldAnimFrame);
						AnimFrame = BestNotify->Time;
						UFunction* Function = FindFunction( BestNotify->Function );
						if( Function )
							ProcessEvent( Function, NULL );
						continue;
					}
				}
			}

			// Handle end of animation sequence.
			if( AnimFrame<AnimLast )
			{
				// We have finished the animation updating for this tick.
				break;
			}
			else if( bAnimLoop )
			{
				if( AnimFrame < 1.0 )
				{
					// Still looping.
					Seconds = 0.0;
				}
				else
				{
					// Just passed end, so loop it.
					Seconds = Seconds * (AnimFrame - 1.0) / (AnimFrame - OldAnimFrame);
					AnimFrame = 0.0;
				}
				if( OldAnimFrame < AnimLast )
				{
					FMainFrame* Android64AnimFrameV75 = GetMainFrame();
					if( Android64AnimFrameV75 && Android64AnimFrameV75->LatentAction == EPOLL_FinishAnim )
						bAnimFinished = 1;
					if ( !bSimulatedPawn )
						eventAnimEnd();
				}
			}
			else 
			{
				// Just passed end-minus-one frame.
				Seconds = Seconds * (AnimFrame - AnimLast) / (AnimFrame - OldAnimFrame);
				AnimFrame	 = AnimLast;
				bAnimFinished = 1;
				AnimRate      = 0.0;
				if ( !bSimulatedPawn )
					eventAnimEnd();
				
				if ( (RemoteRole < ROLE_SimulatedProxy) && !IsA(AWeapon::StaticClass) )
				{
					SimAnim.X = 10000 * AnimFrame;
					SimAnim.Y = 10000 * AnimRate;
				}
			}
		}
		else
		{
			// Update tweening.
			AnimFrame += TweenRate * Seconds;
			if( AnimFrame >= 0.0 )
			{
				// Finished tweening.
				Seconds          = Seconds * (AnimFrame-0) / (AnimFrame - OldAnimFrame);
				AnimFrame = 0.0;
				if( AnimRate == 0.0 )
				{
					bAnimFinished = 1;
					if ( !bSimulatedPawn )
						eventAnimEnd();
				}
			}
			else
			{
				// Finished tweening.
				break;
			}
		}
	}

	// This actor is tickable.
	if ( bSimulatedPawn )
		//simulated pawns just predict location, no script execution
		moveSmooth(Velocity * DeltaSeconds);
	else if ( RemoteRole == ROLE_AutonomousProxy ) 
	{
		if ( Role == ROLE_Authority )
		{
			// server handles timers for autonomous proxy
			if( (TimerRate>0.0) && (TimerCounter+=DeltaSeconds)>=TimerRate )
			{
				// Normalize the timer count.
				INT TimerTicksPassed = 1;
				if( TimerRate > 0.0 )
				{
					TimerTicksPassed     = (int)(TimerCounter/TimerRate);
					TimerCounter -= TimerRate * TimerTicksPassed;
					if( TimerTicksPassed && !bTimerLoop )
					{
						// Only want a one-shot timer message.
						TimerTicksPassed = 1;
						TimerRate = 0.0;
					}
				}

				// Call timer routine with count of timer events that have passed.
				eventTimer();
			}
		}
	}
	else if( Role>=ROLE_SimulatedProxy )
	{
		APlayerPawn* PlayerPawn = NULL;
		if ( Pawn )
			PlayerPawn = Cast<APlayerPawn>(this);
		if( !PlayerPawn || !PlayerPawn->Player )
		{
			// Non-player update.
			if( TickType==LEVELTICK_ViewportsOnly )
				return 1;

			// Tick the nonplayer.
			if ( IsProbing(NAME_Tick)
#if PLATFORM_64BIT
				&& ( !Pawn || UE1Android64HasStateFrameForScriptProbeV75(this) )
#endif
				)
				eventTick(DeltaSeconds);
		}
		else
		{
			// Player update.
			if( PlayerPawn->IsA(ACamera::StaticClass) && !(PlayerPawn->ShowFlags & SHOW_PlayerCtrl) )
				return 1;

			// Process PlayerTick with input.
#if PLATFORM_64BIT
			UE1Android64FrameLossBoundaryProbeV80( "tick-enter-before-ReadInput-v80", PlayerPawn, DeltaSeconds );
			UE1Android64InputFlowProbeV78( "before-ReadInput", PlayerPawn, DeltaSeconds );
#endif
			PlayerPawn->Player->ReadInput( DeltaSeconds );
#if PLATFORM_64BIT
			UE1Android64InputFlowProbeV78( "after-ReadInput", PlayerPawn, DeltaSeconds );
#endif
			PlayerPawn->eventPlayerInput( DeltaSeconds );
#if PLATFORM_64BIT
			UE1Android64InputFlowProbeV78( "after-eventPlayerInput", PlayerPawn, DeltaSeconds );
#endif
			PlayerPawn->eventPlayerTick( DeltaSeconds );
#if PLATFORM_64BIT
			UE1Android64InputFlowProbeV78( "after-eventPlayerTick", PlayerPawn, DeltaSeconds );
#endif
			PlayerPawn->Player->ReadInput( 0.0 );
#if PLATFORM_64BIT
			UE1Android64InputFlowProbeV78( "after-ReadInputClear", PlayerPawn, 0.0f );
#endif
		}

		// Update the actor's script state code.
#if PLATFORM_64BIT
		if( PlayerPawn && PlayerPawn->Player )
		{
			UE1Android64FrameLossBoundaryProbeV80( "before-ProcessState-v80", PlayerPawn, DeltaSeconds );
			UE1Android64InputFlowProbeV78( "before-ProcessState", PlayerPawn, DeltaSeconds );
		}
#endif
		ProcessState( DeltaSeconds );
#if PLATFORM_64BIT
		if( PlayerPawn && PlayerPawn->Player )
		{
			UE1Android64InputFlowProbeV78( "after-ProcessState", PlayerPawn, DeltaSeconds );
			UE1Android64FrameLossBoundaryProbeV80( "after-ProcessState-v80", PlayerPawn, DeltaSeconds );
		}
#endif

		// Update timers.
		if( TimerRate>0.0 && (TimerCounter+=DeltaSeconds)>=TimerRate )
		{
			// Normalize the timer count.
			INT TimerTicksPassed = 1;
			if( TimerRate > 0.0 )
			{
				TimerTicksPassed     = (int)(TimerCounter/TimerRate);
				TimerCounter -= TimerRate * TimerTicksPassed;
				if( TimerTicksPassed && !bTimerLoop )
				{
					// Only want a one-shot timer message.
					TimerTicksPassed = 1;
					TimerRate = 0.0;
				}
			}

			// Call timer routine with count of timer events that have passed.
			eventTimer();
		}
#if PLATFORM_64BIT
		if( PlayerPawn && PlayerPawn->Player )
			UE1Android64FrameLossBoundaryProbeV80( "after-Timer-v80", PlayerPawn, DeltaSeconds );
#endif

		// Update LifeSpan.
		if( LifeSpan!=0.f )
		{
			LifeSpan -= DeltaSeconds;
			if( LifeSpan <= 0.0001 )
			{
				// Actor's LifeSpan expired.
				eventExpired();
				XLevel->DestroyActor( this );
				return 1;
			}
		}
#if PLATFORM_64BIT
		if( PlayerPawn && PlayerPawn->Player )
			UE1Android64FrameLossBoundaryProbeV80( "after-LifeSpan-v80", PlayerPawn, DeltaSeconds );
#endif

		// Perform physics.
		if( (Physics!=PHYS_None) && (Role!=ROLE_AutonomousProxy) )
		{
#if PLATFORM_64BIT
			// UNREAL_ANDROID64_PLAYER_PHYSICS_AFTER_INPUT_PROBE_V79
			// Pure diagnostics: v78 proved that input can reach PlayerTick and
			// Acceleration can be set.  Now log the exact physics boundary without
			// changing movement, collision, base, velocity or input state.
			if( PlayerPawn && PlayerPawn->Player )
			{
				UE1Android64FrameLossBoundaryProbeV80( "before-Physics-v80", PlayerPawn, DeltaSeconds );
				UE1Android64InputFlowProbeV78( "before-performPhysics-v79", PlayerPawn, DeltaSeconds );
			}
#endif
			performPhysics( DeltaSeconds );
#if PLATFORM_64BIT
			if( PlayerPawn && PlayerPawn->Player )
			{
				UE1Android64InputFlowProbeV78( "after-performPhysics-v79", PlayerPawn, DeltaSeconds );
				UE1Android64FrameLossBoundaryProbeV80( "after-Physics-v80", PlayerPawn, DeltaSeconds );
			}
#endif
		}

		if ( (Role == ROLE_AutonomousProxy) 
			&& Base && Base->IsA(AMover::StaticClass)
			&& (Physics == PHYS_Walking) )
		{
			AActor* OldBase = Base;
			XLevel->FarMoveActor( Base, Base->Location + Base->Velocity * DeltaSeconds, 0, 1 );
			moveSmooth(OldBase->Velocity * DeltaSeconds);
			SetBase(OldBase);
		}
	}

	// Update eyeheight and send visibility updates
	// with PVS, monsters look for other monsters, rather than sending msgs
	// Also sends PainTimer messages if PainTime
	if( Pawn )
	{
		if ( Pawn->bIsPlayer && (Role >= ROLE_AutonomousProxy) )
		{
			Pawn->eventUpdateEyeHeight(DeltaSeconds);
#if PLATFORM_64BIT
			APlayerPawn* Android64PostEyePlayerV80 = Cast<APlayerPawn>(Pawn);
			if( Android64PostEyePlayerV80 && Android64PostEyePlayerV80->Player )
				UE1Android64FrameLossBoundaryProbeV80( "after-UpdateEyeHeight-v80", Android64PostEyePlayerV80, DeltaSeconds );
#endif
		}

		if ( (Role == ROLE_Authority) && (TickType==LEVELTICK_All) )
		{
			if( Pawn->SightCounter < 0.0 )
				Pawn->SightCounter += 0.2;

			Pawn->SightCounter = Pawn->SightCounter - DeltaSeconds; 
			if( Pawn->bIsPlayer && !Pawn->bHidden )
				Pawn->ShowSelf();

			if( (Pawn->SightCounter < 0.0)
#if PLATFORM_64BIT
			&& UE1Android64HasStateFrameForScriptProbeV75(Pawn)
#endif
			&& Pawn->IsProbing(NAME_EnemyNotVisible) )
			{
				Pawn->CheckEnemyVisible();
				Pawn->SightCounter = 0.1;
			}

			if( Pawn->PainTime > 0.0 )
			{
				Pawn->PainTime -= DeltaSeconds;
				if (Pawn->PainTime < 0.001)
				{
					Pawn->PainTime = 0.0;
					Pawn->eventPainTimer();
				}
			}

			if( Pawn->SpeechTime > 0.0 )
			{
				Pawn->SpeechTime -= DeltaSeconds;
				if (Pawn->SpeechTime < 0.001)
				{
					Pawn->SpeechTime = 0.0;
					Pawn->eventSpeechTimer();
				}
			}
		}
	}
#if PLATFORM_64BIT
	if( Pawn )
	{
		APlayerPawn* Android64TickExitPlayerV80 = Cast<APlayerPawn>(Pawn);
		if( Android64TickExitPlayerV80 && Android64TickExitPlayerV80->Player )
			UE1Android64FrameLossBoundaryProbeV80( "tick-exit-v80", Android64TickExitPlayerV80, DeltaSeconds );
	}
	// UNREAL_ANDROID64_CROSS_ACTOR_FRAME_EXIT_PROBE_V85
	// Sample again after this actor's entire tick.  If the local player's frame was
	// valid at tick-enter and is null now, this actor's tick path is the direct
	// boundary we need to inspect next.
	UE1Android64CrossActorFrameOwnerProbeV84( "tick-exit-v85", this, DeltaSeconds );
#endif
	return 1;
	unguard;
}

/*-----------------------------------------------------------------------------
	Network client tick.
-----------------------------------------------------------------------------*/

void ULevel::TickNetClient( FLOAT DeltaSeconds )
{
	guard(ULevel::TickNetClient);
	uclock(NetTickCycles);
	if( NetDriver->ServerConnection->State==USOCK_Open )
	{
		for( FTypedChannelIterator<FActorChannel> It(NetDriver->ServerConnection); It; ++It )
		{
			guard(UpdateLocalActors);
			check(*It);
			check(It.GetIndex()>=0);
			check(It.GetIndex()<UNetConnection::MAX_CHANNELS);
			check(It->ChType==CHTYPE_Actor);
			if( It->State==UCHAN_Open && It->Actor )
				check(GetActorIndex(It->Actor)!=INDEX_NONE);
			if( It->State==UCHAN_Open && It->Actor && It->Actor->IsA(APlayerPawn::StaticClass) )
			{
				guard(CheckPawn);
				FActorChannel* ActorChannel = *It;
				APlayerPawn* Pawn = (APlayerPawn*)ActorChannel->Actor;
				if( Pawn->Player )
					ActorChannel->ReplicateActor( 1 );
				unguard;
			}
			unguard;
		}
		NetDriver->ServerConnection->FlushNet( NetDriver->DuplicateClientMoves );
	}
	else if( NetDriver->ServerConnection->State==USOCK_Closed )
	{
		// Server disconnected.
		Engine->SetClientTravel(NULL,"?failed",1,0,TRAVEL_Absolute);
	}
	uunclock(NetTickCycles);
	unguard;
}

/*-----------------------------------------------------------------------------
	Network server ticking individual client.
-----------------------------------------------------------------------------*/

INT ULevel::ServerTickClient( UNetConnection* Connection, FLOAT DeltaSeconds )
{
	guard(ULevel::ServerTickClient);
	check(Connection->State==USOCK_Pending || Connection->State==USOCK_Open || Connection->State==USOCK_Closed);
	INT Updated=0;

	// Handle closed channel.
	if( Connection->State==USOCK_Closed )
	{
		debugf( NAME_DevNet, "Destroying %s because connection closed", Connection->GetName() );
		delete Connection;
		return 0;
	}

	// Handle not ready channel.
	if
	(	!Connection->Actor
	||	!Connection->IsNetReady()
	||	Connection->State!=USOCK_Open )
		return 0;

	// Get list of visible/relevant actors.
	AActor* Relevant[256];
	INT NumRelevant = GetRelevantActors( Connection->Actor, Relevant, ARRAY_COUNT(Relevant) );

	// If an actor's relevence has timed out, delete its channel; otherwise
	// treat it as relevant for now.
	for( FTypedChannelIterator<FActorChannel> It(Connection); It; ++It )
	{
		AActor* Actor=It->Actor;
		if( It->State==UCHAN_Open && Actor )
		{
			if( Actor->NetTag==NetTag )
			{
				// This actor is relevant, so update the channel.
				It->RelevantTime = NetDriver->Time;
			}
			else if
			(	(Actor->Role==ROLE_SimulatedProxy)
			?	(NetDriver->Time-It->RelevantTime<NetDriver->SimulatedProxyTimeout)
			:	(NetDriver->Time-It->RelevantTime<NetDriver->DumbProxyTimeout) )
			{
				// This actor's relevence hasn't timed out yet.
				if( NumRelevant<ARRAY_COUNT(Relevant) )
					Relevant[NumRelevant++] = Actor;
			}
			else
			{
				// Relevence has timed out, so destroy the channel.
				check(It->OpenedLocally);
				check(Actor!=Connection->Actor);
				debugfSlow( NAME_DevNetTraffic, "Irrelevant %s", Actor->GetFullName() );
				It->Close();
			}
		}
	}

	// Make priority-sorted list.
	FMemMark Mark(GMem);
	FActorPriority* PriorityActors = new(GMem,NumRelevant)FActorPriority;
	INT j;
	for( j=0; j<NumRelevant; j++ )
		PriorityActors[j] = FActorPriority( Connection, Relevant[j] );
	appSort( PriorityActors, NumRelevant );

	// Update all relevant actors in sorted order.
	for( j=0; j<NumRelevant && Connection->IsNetReady(); j++ )
	{
		// Find or create the channel for this actor.
		//debugf("%i...%f...%f",j,PriorityActors[j].Priority,PriorityActors[j].Channel ? PriorityActors[j].Channel->LastUpdateTime:0);
		FActorChannel* Channel = PriorityActors[j].Channel;
		if( !Channel && NetDriver->Map.ObjectToIndex(PriorityActors[j].Actor->GetClass())!=INDEX_NONE )
		{
			// Create a new channel for this actor.
			Channel = (FActorChannel *)Connection->CreateChannel( CHTYPE_Actor, 1 );
			Channel->Actor = PriorityActors[j].Actor;
		}

		// Send updates to the remote player.
		if( Channel )
		{
			check(Channel->State==UCHAN_Open);
			if( Channel->IsNetReady(0) )
			{
				Channel->ReplicateActor( 1 );
				if( Connection->OutNum > UNetConnection::IDEAL_PACKET_SIZE )
					Connection->FlushNet();
				Updated++;
			}
		}
	}
	Mark.Pop();

	return Updated;
	unguard;
}

/*-----------------------------------------------------------------------------
	Network server tick.
-----------------------------------------------------------------------------*/

void ULevel::TickNetServer( FLOAT DeltaSeconds )
{
	guard(ULevel::TickNetServer);

	// Update all clients.
	uclock(NetTickCycles);
	INT Updated=0;
	INT i;
	for( i=0; i<NetDriver->Connections.Num(); i++ )
		Updated += ServerTickClient( NetDriver->Connections(i), DeltaSeconds );
	uunclock(NetTickCycles);

	// Stats.
	if( Updated ) for( i=0; i<NetDriver->Connections.Num(); i++ )
	{
		UNetConnection* Connection = NetDriver->Connections(i);
		if
		(	Connection->Actor
		&&	Connection->State==USOCK_Open
		&&	Connection->Actor->bExtra0 )
		{
			// Send stats.
			char Stats[256];
			INT NumActors=0;
			for( INT i=0; i<Num(); i++ )
				NumActors += Actors(i)!=NULL;
			appSprintf
			(
				Stats,
				"cli=%i act=%03.1f (%i) see=%03.1f net=%03.1f pv/c=%i rep/c=%i",
				NetDriver->Connections.Num(),
				GSecondsPerCycle*1000 * ActorTickCycles,
				NumActors,
				GSecondsPerCycle*1000 * GetRelevantCycles,
				GSecondsPerCycle*1000 * (NetTickCycles - GetRelevantCycles),
				NumPV/NetDriver->Connections.Num(),
				NumReps/NetDriver->Connections.Num()
			);
			Connection->Actor->eventClientMessage(Stats);
		}
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	Main level timer tick handler.
-----------------------------------------------------------------------------*/

//
// Update the level after a variable amount of time, DeltaSeconds, has passed.
// All child actors are ticked after their owners have been ticked.
//
void ULevel::Tick( ELevelTick TickType, FLOAT DeltaSeconds )
{
	guard(ULevel::Tick);
	InitStats();
	FMemMark Mark(GMem);
	FMemMark DynMark(GDynMem);
	GInitRunaway();
	InTick=1;

	// Update the net code and fetch all incoming packets.
	if( NetDriver )
	{
		NetDriver->Tick();
		if( NetDriver->ServerConnection )
			TickNetClient( DeltaSeconds );
	}

	// Update collision.
	if( Hash )
		Hash->Tick();

	// Update time.
	ALevelInfo* Info = GetLevelInfo();
	DeltaSeconds *= Info->TimeDilation;
	TimeSeconds += DeltaSeconds;
	Info->TimeSeconds = TimeSeconds;
	appSystemTime( Info->Year, Info->Month, Info->DayOfWeek, Info->Day, Info->Hour, Info->Minute, Info->Second, Info->Millisecond );
	if( Info->bPlayersOnly )
		TickType = LEVELTICK_ViewportsOnly;

	// Clamp time between 1000 fps and 2.5 fps.
	// Generally it is more useful to outright disable this line as 1000+ FPS will speed the game up.
	DeltaSeconds = Clamp(DeltaSeconds,0.001f,0.40f);
#if PLATFORM_64BIT
	UE1Android64LevelTickCleanupProbeV91( "tick-after-clamp-v91", this, DeltaSeconds );
#endif

	// If caller wants time update only, or we are paused, skip the rest.
	if
	(	(TickType!=LEVELTICK_TimeOnly)
	&&	(!Info->Pauser[0])
	&&	(!NetDriver || !NetDriver->ServerConnection || NetDriver->ServerConnection->State==USOCK_Open) )
	{
		// Tick all actors, owners before owned.
		uclock(ActorTickCycles);
		NewlySpawned=NULL;
		INT Updated=0;
#if PLATFORM_64BIT
		UE1Android64LevelTickCleanupProbeV91( "before-level-actor-loop-v91", this, DeltaSeconds );
#endif
		for( INT iActor=iFirstDynamicActor; iActor<Num(); iActor++ )
			if( Actors(iActor) )
			{
				AActor* Android64ActorV87 = Actors(iActor);
#if PLATFORM_64BIT
				FMainFrame* Android64PlayerFrameBeforeActorTickV87 = UE1Android64LevelPlayerFrameSnapshotV87( this );
				UE1Android64EventChainBoundaryProbeV90( "level-loop-before-actor-v90", this, Android64ActorV87, DeltaSeconds, Android64PlayerFrameBeforeActorTickV87, INDEX_NONE );
#endif
				INT Android64TickResultV87 = Android64ActorV87->Tick(DeltaSeconds,TickType);
				Updated += Android64TickResultV87;
#if PLATFORM_64BIT
				UE1Android64LevelActorTickBoundaryProbeV87( "level-loop-after-actor-v87", this, Android64ActorV87, DeltaSeconds, Android64PlayerFrameBeforeActorTickV87, Android64TickResultV87 );
				UE1Android64EventChainBoundaryProbeV90( "level-loop-after-actor-v90", this, Android64ActorV87, DeltaSeconds, Android64PlayerFrameBeforeActorTickV87, Android64TickResultV87 );
#endif
			}
#if PLATFORM_64BIT
		UE1Android64LevelTickCleanupProbeV91( "after-level-actor-loop-v91", this, DeltaSeconds );
#endif
		while( NewlySpawned && Updated )
		{
#if PLATFORM_64BIT
			UE1Android64LevelTickCleanupProbeV91( "newlyspawned-loop-enter-v91", this, DeltaSeconds );
#endif
			FActorLink* Link=NewlySpawned;
			NewlySpawned=NULL;
			Updated=0;
			for( Link; Link; Link=Link->Next )
			{
#if PLATFORM_64BIT
				FMainFrame* Android64PlayerFrameBeforeSpawnedTickV87 = UE1Android64LevelPlayerFrameSnapshotV87( this );
				UE1Android64EventChainBoundaryProbeV90( "newlyspawned-before-actor-v90", this, Link->Actor, DeltaSeconds, Android64PlayerFrameBeforeSpawnedTickV87, INDEX_NONE );
#endif
				INT Android64SpawnedTickResultV87 = Link->Actor->Tick( DeltaSeconds, TickType );
				Updated += Android64SpawnedTickResultV87;
#if PLATFORM_64BIT
				UE1Android64LevelActorTickBoundaryProbeV87( "newlyspawned-after-actor-v87", this, Link->Actor, DeltaSeconds, Android64PlayerFrameBeforeSpawnedTickV87, Android64SpawnedTickResultV87 );
				UE1Android64EventChainBoundaryProbeV90( "newlyspawned-after-actor-v90", this, Link->Actor, DeltaSeconds, Android64PlayerFrameBeforeSpawnedTickV87, Android64SpawnedTickResultV87 );
#endif
			}
		}
#if PLATFORM_64BIT
		UE1Android64LevelTickCleanupProbeV91( "after-newlyspawned-loop-v91", this, DeltaSeconds );
#endif
	}
	else if( Info->Pauser[0] )
	{
		// Absorb input if paused.
		for( INT iActor=iFirstDynamicActor; iActor<Num(); iActor++ )
		{
			APlayerPawn* PlayerPawn=Cast<APlayerPawn>(Actors(iActor));
			if( PlayerPawn && PlayerPawn->Player )
			{
				PlayerPawn->Player->ReadInput( DeltaSeconds );
				PlayerPawn->eventPlayerInput( DeltaSeconds );
				for( TFieldIterator<UFloatProperty> It(PlayerPawn->GetClass()); It; ++It )
					if( It->PropertyFlags & CPF_Input )
						*(FLOAT*)((BYTE*)PlayerPawn + It->Offset) = 0.f;
			}
			else if( Actors(iActor) && Actors(iActor)->bAlwaysTick )
				Actors(iActor)->Tick(DeltaSeconds,TickType);
		}
	}
	uunclock(ActorTickCycles);
#if PLATFORM_64BIT
	UE1Android64LevelTickCleanupProbeV91( "after-actor-tick-unclock-v91", this, DeltaSeconds );
#endif

	// Update net server.
	if( NetDriver && !NetDriver->ServerConnection )
	{
#if PLATFORM_64BIT
		UE1Android64LevelTickCleanupProbeV91( "before-netserver-v91", this, DeltaSeconds );
#endif
		TickNetServer( DeltaSeconds );
#if PLATFORM_64BIT
		UE1Android64LevelTickCleanupProbeV91( "after-netserver-v91", this, DeltaSeconds );
#endif
	}

	// Finish up.
#if PLATFORM_64BIT
	UE1Android64LevelTickCleanupProbeV91( "before-ticked-toggle-v91", this, DeltaSeconds );
#endif
	Ticked = !Ticked;
#if PLATFORM_64BIT
	UE1Android64LevelTickCleanupProbeV91( "after-ticked-toggle-v91", this, DeltaSeconds );
#endif
	InTick = 0;
#if PLATFORM_64BIT
	UE1Android64LevelTickCleanupProbeV91( "after-inTick-zero-v91", this, DeltaSeconds );
	UE1Android64LevelTickCleanupProbeV91( "before-mark-pop-v91", this, DeltaSeconds );
#endif
	Mark.Pop();
#if PLATFORM_64BIT
	UE1Android64LevelTickCleanupProbeV91( "after-mark-pop-v91", this, DeltaSeconds );
	UE1Android64LevelTickCleanupProbeV91( "before-dynmark-pop-v91", this, DeltaSeconds );
#endif
	DynMark.Pop();
#if PLATFORM_64BIT
	UE1Android64LevelTickCleanupProbeV91( "after-dynmark-pop-v91", this, DeltaSeconds );
	UE1Android64LevelTickCleanupProbeV91( "before-cleanupdestroyed-v91", this, DeltaSeconds );
#endif
	CleanupDestroyed( 0 );
#if PLATFORM_64BIT
	UE1Android64LevelTickCleanupProbeV91( "after-cleanupdestroyed-v91", this, DeltaSeconds );
#endif
	unguardf(( "(NetMode=%i)", GetLevelInfo()->NetMode ));
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
