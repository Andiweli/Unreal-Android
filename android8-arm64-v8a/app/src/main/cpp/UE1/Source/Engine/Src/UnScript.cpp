/*=============================================================================
	UnScript.cpp: UnrealScript engine support code.
	Copyright 1997 Epic MegaGames, Inc. This software is a trade secret.

Description:
	UnrealScript execution and support code.

Revision history:
	* Created by Tim Sweeney
=============================================================================*/

#include "EnginePrivate.h"
#include "UnRender.h"
#include "UnNet.h"
#if PLATFORM_ANDROID && PLATFORM_64BIT
#include <android/log.h>
#endif
#if PLATFORM_ANDROID && PLATFORM_64BIT
#define UE1_ANDROID64_GAMEPLAY_DIAG_LOG(...) __android_log_print(ANDROID_LOG_INFO, "UE1Diag64", __VA_ARGS__)
#else
#define UE1_ANDROID64_GAMEPLAY_DIAG_LOG(...)
#endif


/*-----------------------------------------------------------------------------
	Tim's physics modes.
-----------------------------------------------------------------------------*/


#if PLATFORM_64BIT
// UNREAL_ANDROID64_GAMEPLAY_DIAG_V47
// UNREAL_ANDROID64_MAINFRAME_LOSS_PROBE_V51
// UNREAL_ANDROID64_LATENT_CONTINUATION_PROBE_V60

// UNREAL_ANDROID64_LATENT_RESUME_SHADOW_V61
static INT GUE1Android64StateDiagBudget = 0;
static INT GUE1Android64FrameLossBudget = 0;
static INT GUE1Android64LatentContinuationBudgetV60 = 0;
static INT GUE1Android64ProcessStateOriginBudgetV76 = 0; // UNREAL_ANDROID64_GOTOSTATE_MAINFRAME_GUARD_V76

static inline UBOOL UE1Android64StateDiagBadPtr( const void* Ptr )
{
	const UPTRINT Value = (UPTRINT)Ptr;
	return !Ptr || Value < 0x10000 || ((Value & (sizeof(void*)-1)) != 0);
}

static const char* UE1Android64StateDiagFrameState( AActor* Actor )
{
	if( !Actor || UE1Android64StateDiagBadPtr(Actor) )
		return "<bad-actor>";
	FMainFrame* Frame = Actor->GetMainFrame();
	if( !Frame || UE1Android64StateDiagBadPtr(Frame) )
		return "<no-frame>";
	if( !Frame->StateNode || UE1Android64StateDiagBadPtr(Frame->StateNode) )
		return "<bad-state>";
	return Frame->StateNode->GetName();
}

static INT UE1Android64StateDiagLatent( AActor* Actor )
{
	if( !Actor || UE1Android64StateDiagBadPtr(Actor) )
		return -1;
	FMainFrame* Frame = Actor->GetMainFrame();
	return (Frame && !UE1Android64StateDiagBadPtr(Frame)) ? Frame->LatentAction : -1;
}

static INT UE1Android64StateDiagCodeOffset( AActor* Actor )
{
	if( !Actor || UE1Android64StateDiagBadPtr(Actor) )
		return -1;
	FMainFrame* Frame = Actor->GetMainFrame();
	if( !Frame || UE1Android64StateDiagBadPtr(Frame) || !Frame->Node || !Frame->Code || UE1Android64StateDiagBadPtr(Frame->Node) )
		return -1;
	return Frame->Code - &Frame->Node->Script(0);
}

static UBOOL UE1Android64StateDiagInterestingActor( AActor* Actor )
{
	if( !Actor || UE1Android64StateDiagBadPtr(Actor) )
		return 0;

	// UNREAL_ANDROID64_REAL_PLAYER_PROBE_V58
	// Keep the old state probe useful, but stop the Unreal intro camera pawn from
	// consuming the budget.  Focus on the actual player pawn, gameplay Skaarj, and
	// pawns whose Enemy is a real PlayerPawn.
	const char* FullName = Actor->GetFullName();
	if( FullName && appStrstr(FullName,"Unreal.MaleOne") )
		return 0;

	APlayerPawn* PlayerPawn = Cast<APlayerPawn>(Actor);
	if( PlayerPawn )
	{
		if( PlayerPawn->Player && !UE1Android64StateDiagBadPtr(PlayerPawn->Player) )
			return 1;
		if( FullName && (appStrstr(FullName,"Vortex2.") || appStrstr(FullName,"Save")) )
			return 1;
	}

	const char* ClassName = Actor->GetClassName();
	if( ClassName && (appStrstr(ClassName,"Skaarj") || appStrstr(ClassName,"ScriptedPawn")) )
		return !FullName || appStrstr(FullName,"Vortex2.") || appStrstr(FullName,"Save");
	if( Actor->bIsPawn )
	{
		APawn* Pawn = Cast<APawn>(Actor);
		return Pawn && Pawn->Enemy && !UE1Android64StateDiagBadPtr(Pawn->Enemy) && Pawn->Enemy->IsValid() && Pawn->Enemy->IsA(APlayerPawn::StaticClass);
	}
	return 0;
}

static void UE1Android64StateDiag( const char* Phase, AActor* Actor, FLOAT DeltaSeconds, const char* Extra=NULL )
{
	if( !Actor || !UE1Android64StateDiagInterestingActor(Actor) || GUE1Android64StateDiagBudget++ >= 256 )
		return;
	APawn* Pawn = Actor->bIsPawn ? Cast<APawn>(Actor) : NULL;
	UE1_ANDROID64_GAMEPLAY_DIAG_LOG("ANDROID64 DIAG STATE phase=%s actor=%s class=%s state=%s latent=%i code=%i role=%i remote=%i physics=%i loc=(%.1f %.1f %.1f) vel=(%.1f %.1f %.1f) acc=(%.1f %.1f %.1f) enemy=%s moveTarget=%s dt=%.5f flags=%08x extra=%s",
		Phase ? Phase : "?",
		Actor->GetFullName(),
		Actor->GetClassName(),
		UE1Android64StateDiagFrameState(Actor),
		UE1Android64StateDiagLatent(Actor),
		UE1Android64StateDiagCodeOffset(Actor),
		(INT)Actor->Role,
		(INT)Actor->RemoteRole,
		(INT)Actor->Physics,
		Actor->Location.X, Actor->Location.Y, Actor->Location.Z,
		Actor->Velocity.X, Actor->Velocity.Y, Actor->Velocity.Z,
		Actor->Acceleration.X, Actor->Acceleration.Y, Actor->Acceleration.Z,
		(Pawn && Pawn->Enemy && !UE1Android64StateDiagBadPtr(Pawn->Enemy) && Pawn->Enemy->IsValid()) ? Pawn->Enemy->GetFullName() : "<none>",
		(Pawn && Pawn->MoveTarget && !UE1Android64StateDiagBadPtr(Pawn->MoveTarget) && Pawn->MoveTarget->IsValid()) ? Pawn->MoveTarget->GetFullName() : "<none>",
		DeltaSeconds,
		Actor->GetFlags(),
		Extra ? Extra : "" );
}

static void UE1Android64ProcessStateOriginLogV76( const char* Phase, AActor* Actor, FLOAT DeltaSeconds, FMainFrame* Frame )
{
	if( !Actor || !UE1Android64StateDiagInterestingActor(Actor) || GUE1Android64ProcessStateOriginBudgetV76++ >= 128 )
		return;
	const char* FrameState = UE1Android64StateDiagFrameState(Actor);
	UE1_ANDROID64_GAMEPLAY_DIAG_LOG("ANDROID64 PROCESSSTATE ORIGIN V76 phase=%s actor=%s class=%s frame=%p state=%s latent=%i code=%i role=%i physics=%i loc=(%.1f %.1f %.1f) vel=(%.1f %.1f %.1f) acc=(%.1f %.1f %.1f) flags=%08x dt=%.5f",
		Phase ? Phase : "?",
		Actor->GetFullName(),
		Actor->GetClassName(),
		Frame,
		FrameState,
		UE1Android64StateDiagLatent(Actor),
		UE1Android64StateDiagCodeOffset(Actor),
		(INT)Actor->Role,
		(INT)Actor->Physics,
		Actor->Location.X, Actor->Location.Y, Actor->Location.Z,
		Actor->Velocity.X, Actor->Velocity.Y, Actor->Velocity.Z,
		Actor->Acceleration.X, Actor->Acceleration.Y, Actor->Acceleration.Z,
		Actor->GetFlags(),
		DeltaSeconds );
}

static void UE1Android64FrameLossProbeV51( const char* Phase, AActor* Actor, FMainFrame* Before, FMainFrame* After, UState* BeforeState, BYTE* BeforeCode, INT BeforeLatent )
{
	if( !Actor || !UE1Android64StateDiagInterestingActor(Actor) || GUE1Android64FrameLossBudget++ >= 96 )
		return;
	const INT BeforeCodeOffset = (Before && !UE1Android64StateDiagBadPtr(Before) && Before->Node && BeforeCode && !UE1Android64StateDiagBadPtr(Before->Node)) ? (BeforeCode - &Before->Node->Script(0)) : -1;
	const INT AfterCodeOffset  = (After  && !UE1Android64StateDiagBadPtr(After)  && After->Node  && After->Code && !UE1Android64StateDiagBadPtr(After->Node))  ? (After->Code - &After->Node->Script(0)) : -1;
	UE1_ANDROID64_GAMEPLAY_DIAG_LOG("ANDROID64 MAINFRAME LOSS PROBE V51 phase=%s actor=%s class=%s beforeFrame=%p afterFrame=%p beforeState=%s afterState=%s beforeLatent=%i afterLatent=%i beforeCode=%i afterCode=%i flags=%08x physics=%i loc=(%.1f %.1f %.1f)",
		Phase ? Phase : "?",
		Actor->GetFullName(),
		Actor->GetClassName(),
		Before,
		After,
		BeforeState && !UE1Android64StateDiagBadPtr(BeforeState) ? BeforeState->GetName() : "<none>",
		UE1Android64StateDiagFrameState(Actor),
		BeforeLatent,
		(After && !UE1Android64StateDiagBadPtr(After)) ? After->LatentAction : -1,
		BeforeCodeOffset,
		AfterCodeOffset,
		Actor->GetFlags(),
		(INT)Actor->Physics,
		Actor->Location.X, Actor->Location.Y, Actor->Location.Z );
}


static INT UE1Android64FrameCodeOffsetV60( FFrame* Frame, BYTE* CodeOverride=NULL )
{
	if( !Frame || UE1Android64StateDiagBadPtr(Frame) || !Frame->Node || UE1Android64StateDiagBadPtr(Frame->Node) )
		return -1;
	BYTE* Code = CodeOverride ? CodeOverride : Frame->Code;
	// UNREAL_ANDROID64_LATENT_RESUME_SHADOW_V61:
	// Code is bytecode and may be BYTE-aligned. Do not reject it with UObject
	// pointer alignment checks.  Instead verify that it points inside Node->Script.
	if( !Code )
		return -1;
	BYTE* Base = &Frame->Node->Script(0);
	BYTE* End  = Base + Frame->Node->Script.Num();
	return (Code >= Base && Code < End) ? (INT)(Code - Base) : -1;
}

static INT UE1Android64FrameOpcodeV60( FFrame* Frame )
{
	if( !Frame || UE1Android64StateDiagBadPtr(Frame) || !Frame->Node || UE1Android64StateDiagBadPtr(Frame->Node) || !Frame->Code )
		return -1;
	BYTE* Base = &Frame->Node->Script(0);
	BYTE* End  = Base + Frame->Node->Script.Num();
	return (Frame->Code >= Base && Frame->Code < End) ? (INT)*Frame->Code : -1;
}

static void UE1Android64LatentContinuationProbeV60( const char* Phase, AActor* Actor, FMainFrame* BeforeFrame, FMainFrame* AfterFrame, UState* BeforeState, BYTE* BeforeCode, INT BeforeLatent, FLOAT DeltaSeconds )
{
	if( !Actor || !UE1Android64StateDiagInterestingActor(Actor) || GUE1Android64LatentContinuationBudgetV60++ >= 160 )
		return;
	APawn* Pawn = Actor->bIsPawn ? Cast<APawn>(Actor) : NULL;
	const INT BeforeCodeOffset = UE1Android64FrameCodeOffsetV60( BeforeFrame, BeforeCode );
	const INT AfterCodeOffset  = UE1Android64FrameCodeOffsetV60( AfterFrame, NULL );
	UE1_ANDROID64_GAMEPLAY_DIAG_LOG("ANDROID64 LATENT CONTINUATION PROBE V60 phase=%s actor=%s class=%s beforeFrame=%p afterFrame=%p beforeState=%s afterState=%s beforeLatent=%i afterLatent=%i beforeCode=%i afterCode=%i afterOpcode=%i dt=%.5f physics=%i loc=(%.1f %.1f %.1f) vel=(%.1f %.1f %.1f) acc=(%.1f %.1f %.1f) enemy=%s moveTarget=%s flags=%08x",
		Phase ? Phase : "?",
		Actor->GetFullName(),
		Actor->GetClassName(),
		BeforeFrame,
		AfterFrame,
		BeforeState && !UE1Android64StateDiagBadPtr(BeforeState) ? BeforeState->GetName() : "<none>",
		UE1Android64StateDiagFrameState(Actor),
		BeforeLatent,
		(AfterFrame && !UE1Android64StateDiagBadPtr(AfterFrame)) ? AfterFrame->LatentAction : -1,
		BeforeCodeOffset,
		AfterCodeOffset,
		UE1Android64FrameOpcodeV60( AfterFrame ),
		DeltaSeconds,
		(INT)Actor->Physics,
		Actor->Location.X, Actor->Location.Y, Actor->Location.Z,
		Actor->Velocity.X, Actor->Velocity.Y, Actor->Velocity.Z,
		Actor->Acceleration.X, Actor->Acceleration.Y, Actor->Acceleration.Z,
		(Pawn && Pawn->Enemy && !UE1Android64StateDiagBadPtr(Pawn->Enemy) && Pawn->Enemy->IsValid()) ? Pawn->Enemy->GetFullName() : "<none>",
		(Pawn && Pawn->MoveTarget && !UE1Android64StateDiagBadPtr(Pawn->MoveTarget) && Pawn->MoveTarget->IsValid()) ? Pawn->MoveTarget->GetFullName() : "<none>",
		Actor->GetFlags() );
}


// UNREAL_ANDROID64_STATEFRAME_CODE_ORIGIN_PROBE_V62
static INT GUE1Android64StateFrameOriginBudgetV62 = 0;

static INT UE1Android64FindStateLabelOffsetV62( UState* State, FName LabelName )
{
	for( UState* Source=State; Source && !UE1Android64StateDiagBadPtr(Source); Source=Source->GetSuperState() )
	{
		if( Source->LabelTableOffset == MAXWORD || Source->Script.Num() <= 0 || Source->LabelTableOffset >= Source->Script.Num() )
			continue;
		BYTE* TableBase = &Source->Script(Source->LabelTableOffset);
		BYTE* ScriptEnd = &Source->Script(0) + Source->Script.Num();
		for( FLabelEntry* Label=(FLabelEntry*)TableBase; (BYTE*)(Label+1) <= ScriptEnd; ++Label )
		{
			if( Label->Name == NAME_None )
				break;
			if( Label->Name == LabelName )
				return Label->iCode;
		}
	}
	return INDEX_NONE;
}

static void UE1Android64StateFrameOriginProbeV62( const char* Phase, AActor* Actor, FLOAT DeltaSeconds )
{
	if( !Actor || !UE1Android64StateDiagInterestingActor(Actor) || GUE1Android64StateFrameOriginBudgetV62++ >= 192 )
		return;
	FMainFrame* Frame = Actor->GetMainFrame();
	UState* StateNode = (Frame && !UE1Android64StateDiagBadPtr(Frame)) ? Frame->StateNode : NULL;
	UStruct* Node = (Frame && !UE1Android64StateDiagBadPtr(Frame)) ? Frame->Node : NULL;
	const INT ScriptNum  = (Node && !UE1Android64StateDiagBadPtr(Node)) ? Node->Script.Num() : -1;
	const INT LabelTable = (StateNode && !UE1Android64StateDiagBadPtr(StateNode)) ? StateNode->LabelTableOffset : -1;
	const INT BeginCode  = (StateNode && !UE1Android64StateDiagBadPtr(StateNode)) ? UE1Android64FindStateLabelOffsetV62( StateNode, NAME_Begin ) : INDEX_NONE;
	const INT CurrentCode= UE1Android64FrameCodeOffsetV60( Frame, NULL );
	APawn* Pawn = Actor->bIsPawn ? Cast<APawn>(Actor) : NULL;
	UE1_ANDROID64_GAMEPLAY_DIAG_LOG("ANDROID64 STATEFRAME PROBE V62 phase=%s actor=%s class=%s frame=%p node=%s state=%s latent=%i code=%i opcode=%i scriptNum=%i labelTable=%i begin=%i probe=%08x%08x role=%i remote=%i physics=%i dt=%.5f loc=(%.1f %.1f %.1f) vel=(%.1f %.1f %.1f) acc=(%.1f %.1f %.1f) enemy=%s moveTarget=%s flags=%08x",
		Phase ? Phase : "?",
		Actor->GetFullName(),
		Actor->GetClassName(),
		Frame,
		(Node && !UE1Android64StateDiagBadPtr(Node)) ? Node->GetName() : "<none>",
		(StateNode && !UE1Android64StateDiagBadPtr(StateNode)) ? StateNode->GetName() : "<none>",
		(Frame && !UE1Android64StateDiagBadPtr(Frame)) ? Frame->LatentAction : -1,
		CurrentCode,
		UE1Android64FrameOpcodeV60( Frame ),
		ScriptNum,
		LabelTable,
		BeginCode,
		(Frame && !UE1Android64StateDiagBadPtr(Frame)) ? (INT)(Frame->ProbeMask>>32) : 0,
		(Frame && !UE1Android64StateDiagBadPtr(Frame)) ? (INT)Frame->ProbeMask : 0,
		(INT)Actor->Role,
		(INT)Actor->RemoteRole,
		(INT)Actor->Physics,
		DeltaSeconds,
		Actor->Location.X, Actor->Location.Y, Actor->Location.Z,
		Actor->Velocity.X, Actor->Velocity.Y, Actor->Velocity.Z,
		Actor->Acceleration.X, Actor->Acceleration.Y, Actor->Acceleration.Z,
		(Pawn && Pawn->Enemy && !UE1Android64StateDiagBadPtr(Pawn->Enemy) && Pawn->Enemy->IsValid()) ? Pawn->Enemy->GetFullName() : "<none>",
		(Pawn && Pawn->MoveTarget && !UE1Android64StateDiagBadPtr(Pawn->MoveTarget) && Pawn->MoveTarget->IsValid()) ? Pawn->MoveTarget->GetFullName() : "<none>",
		Actor->GetFlags() );
}

// UNREAL_ANDROID64_REAL_PLAYER_PROBE_V58
// v56 replaces the very chatty v55 per-frame path probe with milestone-only
// logging.  This is still diagnosis only: no physics, target, collision or input
// state is changed here.  The goal is to prove whether PHYS_Interpolating reaches
// InterpolateEnd / advances from InterpolationPoint0 to InterpolationPoint1.
static INT GUE1Android64InterpPathBudgetV56 = 0;

struct FUE1Android64InterpTrackV56
{
	AActor* Actor;
	AActor* Target;
	INT     LastBucket;
	INT     LastPhysics;
	INT     LastInterp;
	FLOAT   LastAlpha;
};

static FUE1Android64InterpTrackV56 GUE1Android64InterpTracksV56[8] =
{
	{ NULL, NULL, -1, -1, -1, -1.f },
	{ NULL, NULL, -1, -1, -1, -1.f },
	{ NULL, NULL, -1, -1, -1, -1.f },
	{ NULL, NULL, -1, -1, -1, -1.f },
	{ NULL, NULL, -1, -1, -1, -1.f },
	{ NULL, NULL, -1, -1, -1, -1.f },
	{ NULL, NULL, -1, -1, -1, -1.f },
	{ NULL, NULL, -1, -1, -1, -1.f }
};

static const char* UE1Android64InterpActorNameV56( AActor* Actor )
{
	return (Actor && !UE1Android64StateDiagBadPtr(Actor) && Actor->IsValid()) ? Actor->GetFullName() : "<none>";
}

static const char* UE1Android64InterpActorClassV56( AActor* Actor )
{
	return (Actor && !UE1Android64StateDiagBadPtr(Actor) && Actor->IsValid()) ? Actor->GetClassName() : "<none>";
}

static UBOOL UE1Android64InterpInterestingV56( AActor* Actor )
{
	if( !Actor || UE1Android64StateDiagBadPtr(Actor) || !Actor->IsValid() )
		return 0;

	// UNREAL_ANDROID64_REAL_PLAYER_PROBE_V58
	// Keep interpolation milestones only for actual player candidates now.
	const char* FullName = Actor->GetFullName();
	if( FullName && appStrstr(FullName,"Unreal.MaleOne") )
		return 0;
	APlayerPawn* PlayerPawn = Cast<APlayerPawn>(Actor);
	if( PlayerPawn )
	{
		if( PlayerPawn->Player && !UE1Android64StateDiagBadPtr(PlayerPawn->Player) )
			return 1;
		if( FullName && (appStrstr(FullName,"Vortex2.") || appStrstr(FullName,"Save")) )
			return 1;
	}
	return 0;
}

static INT UE1Android64InterpAlphaBucketV56( FLOAT Alpha )
{
	if( Alpha >= 0.999f ) return 100;
	if( Alpha >= 0.950f ) return 95;
	if( Alpha >= 0.900f ) return 90;
	if( Alpha >= 0.750f ) return 75;
	if( Alpha >= 0.500f ) return 50;
	if( Alpha >= 0.250f ) return 25;
	if( Alpha >= 0.100f ) return 10;
	return 0;
}

static FUE1Android64InterpTrackV56* UE1Android64InterpFindTrackV56( AActor* Actor )
{
	INT Empty = -1;
	for( INT i=0; i<ARRAY_COUNT(GUE1Android64InterpTracksV56); i++ )
	{
		if( GUE1Android64InterpTracksV56[i].Actor == Actor )
			return &GUE1Android64InterpTracksV56[i];
		if( !GUE1Android64InterpTracksV56[i].Actor && Empty < 0 )
			Empty = i;
	}
	if( Empty < 0 )
		Empty = 0;
	GUE1Android64InterpTracksV56[Empty].Actor       = Actor;
	GUE1Android64InterpTracksV56[Empty].Target      = NULL;
	GUE1Android64InterpTracksV56[Empty].LastBucket  = -1;
	GUE1Android64InterpTracksV56[Empty].LastPhysics = -1;
	GUE1Android64InterpTracksV56[Empty].LastInterp  = -1;
	GUE1Android64InterpTracksV56[Empty].LastAlpha   = -1.f;
	return &GUE1Android64InterpTracksV56[Empty];
}

static UBOOL UE1Android64InterpCriticalPhaseV56( const char* Phase )
{
	return Phase &&
	(
		appStrstr(Phase,"overflow") ||
		appStrstr(Phase,"no-next") ||
		appStrstr(Phase,"no-dest") ||
		appStrstr(Phase,"target-changed")
	);
}

static void UE1Android64InterpDiagV56( const char* Phase, AActor* Actor, FLOAT DeltaTime, AInterpolationPoint* Dest, FLOAT OldAlpha, FLOAT DestAlpha, FLOAT RateModifier )
{
	if( !UE1Android64InterpInterestingV56(Actor) )
		return;

	FUE1Android64InterpTrackV56* Track = UE1Android64InterpFindTrackV56( Actor );
	const INT Bucket        = UE1Android64InterpAlphaBucketV56( Actor->PhysAlpha );
	const UBOOL IsAfterAlpha = Phase && appStrstr(Phase,"after-alpha");
	const UBOOL IsEnter      = Phase && appStrstr(Phase,"physPathing.enter");
	const UBOOL Critical     = UE1Android64InterpCriticalPhaseV56( Phase );
	const UBOOL TargetChanged  = Track && Track->LastBucket >= 0 && Track->Target != Actor->Target;
	const UBOOL PhysicsChanged = Track && Track->LastBucket >= 0 && (Track->LastPhysics != (INT)Actor->Physics || Track->LastInterp != (INT)Actor->bInterpolating);
	const UBOOL BucketChanged  = Track && Track->LastBucket != Bucket;
	const UBOOL AlphaWentBack  = Track && Track->LastAlpha >= 0.f && Actor->PhysAlpha + 0.02f < Track->LastAlpha;

	UBOOL ShouldLog = Critical || TargetChanged || PhysicsChanged || AlphaWentBack;
	if( !ShouldLog && IsEnter && Track && Track->LastBucket < 0 )
		ShouldLog = 1;
	if( !ShouldLog && IsAfterAlpha && BucketChanged && (Bucket==10 || Bucket==25 || Bucket==50 || Bucket==75 || Bucket==90 || Bucket==95 || Bucket==100) )
		ShouldLog = 1;

	if( Track )
	{
		Track->Target      = Actor->Target;
		Track->LastBucket  = Bucket;
		Track->LastPhysics = (INT)Actor->Physics;
		Track->LastInterp  = (INT)Actor->bInterpolating;
		Track->LastAlpha   = Actor->PhysAlpha;
	}

	if( !ShouldLog || GUE1Android64InterpPathBudgetV56++ >= 140 )
		return;

	APlayerPawn* PlayerPawn = Cast<APlayerPawn>(Actor);
	UE1_ANDROID64_GAMEPLAY_DIAG_LOG("ANDROID64 INTERP END PROBE V57 phase=%s actor=%s class=%s state=%s latent=%i code=%i physics=%i interp=%i alpha=%.4f oldAlpha=%.4f destAlpha=%.4f bucket=%i rate=%.4f rateMod=%.4f dt=%.5f loc=(%.1f %.1f %.1f) vel=(%.1f %.1f %.1f) acc=(%.1f %.1f %.1f) input=(f=%.3f s=%.3f turn=%.3f look=%.3f) coll=(world=%i actors=%i block=%i) target=%s targetClass=%s dest=%s prev=%s next=%s changed=(target=%i phys=%i alphaBack=%i) flags=%08x",
		Phase ? Phase : "?",
		Actor->GetFullName(),
		Actor->GetClassName(),
		UE1Android64StateDiagFrameState(Actor),
		UE1Android64StateDiagLatent(Actor),
		UE1Android64StateDiagCodeOffset(Actor),
		(INT)Actor->Physics,
		(INT)Actor->bInterpolating,
		Actor->PhysAlpha,
		OldAlpha,
		DestAlpha,
		Bucket,
		Actor->PhysRate,
		RateModifier,
		DeltaTime,
		Actor->Location.X, Actor->Location.Y, Actor->Location.Z,
		Actor->Velocity.X, Actor->Velocity.Y, Actor->Velocity.Z,
		Actor->Acceleration.X, Actor->Acceleration.Y, Actor->Acceleration.Z,
		PlayerPawn ? PlayerPawn->aForward : 0.f,
		PlayerPawn ? PlayerPawn->aStrafe : 0.f,
		PlayerPawn ? PlayerPawn->aTurn : 0.f,
		PlayerPawn ? PlayerPawn->aLookUp : 0.f,
		(INT)Actor->bCollideWorld,
		(INT)Actor->bCollideActors,
		(INT)Actor->bBlockActors,
		UE1Android64InterpActorNameV56(Actor->Target),
		UE1Android64InterpActorClassV56(Actor->Target),
		UE1Android64InterpActorNameV56(Dest),
		(Dest && !UE1Android64StateDiagBadPtr(Dest)) ? UE1Android64InterpActorNameV56(Dest->Prev) : "<none>",
		(Dest && !UE1Android64StateDiagBadPtr(Dest)) ? UE1Android64InterpActorNameV56(Dest->Next) : "<none>",
		(INT)TargetChanged,
		(INT)PhysicsChanged,
		(INT)AlphaWentBack,
		Actor->GetFlags() );
}
#endif

FLOAT Splerp( FLOAT F )
{
	FLOAT S = Square(F);
	return (1.0/16.0)*S*S - (1.0/2.0)*S + 1;
}

//
// Interpolating along a path.
//
void AActor::physPathing( FLOAT DeltaTime )
{
	guard(AActor::physPathing);

#if PLATFORM_64BIT
	UE1Android64InterpDiagV56( "physPathing.enter", this, DeltaTime, Cast<AInterpolationPoint>(Target), PhysAlpha, PhysAlpha, 1.0f );
#endif
	// Linear interpolate from Target to Target.Next.
	while( PhysRate!=0.0 && bInterpolating && DeltaTime>0.0 )
	{
		// Find destination interpolation point, if any.
		AInterpolationPoint* Dest = Cast<AInterpolationPoint>( Target );

		// Compute rate modifier.
		FLOAT RateModifier = 1.0;
		if( Dest && Dest->Next )
			RateModifier = Dest->RateModifier * (1.0 - PhysAlpha) + Dest->Next->RateModifier * PhysAlpha;

		// Update alpha.
		FLOAT OldAlpha  = PhysAlpha;
		FLOAT DestAlpha = PhysAlpha + PhysRate * RateModifier * DeltaTime;
#if PLATFORM_64BIT
		UE1Android64InterpDiagV56( "physPathing.before-alpha", this, DeltaTime, Dest, OldAlpha, DestAlpha, RateModifier );
#endif
		PhysAlpha       = Clamp( DestAlpha, 0.f, 1.f );
#if PLATFORM_64BIT
		UE1Android64InterpDiagV56( "physPathing.after-alpha", this, DeltaTime, Dest, OldAlpha, DestAlpha, RateModifier );
#endif

		// Move and rotate.
		if( Dest && Dest->Next )
		{
			FCheckResult Hit;
			FVector NewLocation;
			FRotator NewRotation;
			if( Dest->Prev && Dest->Next->Next )
			{
				// Cubic spline interpolation.
				FLOAT W0 = Splerp(PhysAlpha+1.0);
				FLOAT W1 = Splerp(PhysAlpha+0.0);
				FLOAT W2 = Splerp(PhysAlpha-1.0);
				FLOAT W3 = Splerp(PhysAlpha-2.0);
				FLOAT RW = 1.0 / (W0 + W1 + W2 + W3);
				NewLocation = (W0*Dest->Prev->Location + W1*Dest->Location + W2*Dest->Next->Location + W3*Dest->Next->Next->Location)*RW;
				NewRotation = (W0*Dest->Prev->Rotation + W1*Dest->Rotation + W2*Dest->Next->Rotation + W3*Dest->Next->Next->Rotation)*RW;
			}
			else
			{
				// Linear interpolation.
				FLOAT W0 = 1.0 - PhysAlpha;
				FLOAT W1 = PhysAlpha;
				NewLocation = W0*Dest->Location + W1*Dest->Next->Location;
				NewRotation = W0*Dest->Rotation + W1*Dest->Next->Rotation;
			}
			XLevel->MoveActor( this, NewLocation - Location, NewRotation, Hit );
			if( IsA(APawn::StaticClass) )
				((APawn*)this)->ViewRotation = Rotation;
#if PLATFORM_64BIT
			UE1Android64InterpDiagV56( "physPathing.after-move", this, DeltaTime, Dest, OldAlpha, DestAlpha, RateModifier );
#endif
		}
#if PLATFORM_64BIT
		else
		{
			UE1Android64InterpDiagV56( Dest ? "physPathing.no-next" : "physPathing.no-dest", this, DeltaTime, Dest, OldAlpha, DestAlpha, RateModifier );
		}
#endif

		// If overflowing, notify and go to next place.
		if( PhysRate>0.0 && DestAlpha>1.0 )
		{
			PhysAlpha = 0.0;
			DeltaTime *= (DestAlpha - 1.0) / (DestAlpha - OldAlpha);
			if( Target )
			{
#if PLATFORM_64BIT
				AActor* NotifyTarget = Target;
				UE1Android64InterpDiagV56( "physPathing.overflow-pos.before-target-event", this, DeltaTime, Dest, OldAlpha, DestAlpha, RateModifier );
#endif
				Target->eventInterpolateEnd(this);
#if PLATFORM_64BIT
				UE1Android64InterpDiagV56( "physPathing.overflow-pos.after-target-event", this, DeltaTime, Dest, OldAlpha, DestAlpha, RateModifier );
				UE1Android64InterpDiagV56( "physPathing.overflow-pos.before-self-event", this, DeltaTime, Dest, OldAlpha, DestAlpha, RateModifier );
#endif
				eventInterpolateEnd(Target);
#if PLATFORM_64BIT
				UE1Android64InterpDiagV56( "physPathing.overflow-pos.after-self-event", this, DeltaTime, Dest, OldAlpha, DestAlpha, RateModifier );
				if( Target != NotifyTarget )
					UE1Android64InterpDiagV56( "physPathing.overflow-pos.target-changed-by-script", this, DeltaTime, Dest, OldAlpha, DestAlpha, RateModifier );
#endif
				if( Dest )
					Target = Dest->Next;
#if PLATFORM_64BIT
				UE1Android64InterpDiagV56( "physPathing.overflow-pos.after-target-advance", this, DeltaTime, Cast<AInterpolationPoint>(Target), OldAlpha, DestAlpha, RateModifier );
#endif
			}
#if PLATFORM_64BIT
			else
			{
				UE1Android64InterpDiagV56( "physPathing.overflow-pos.no-target", this, DeltaTime, Dest, OldAlpha, DestAlpha, RateModifier );
			}
#endif
		}
		else if( PhysRate<0.0 && DestAlpha<0.0 )
		{
			PhysAlpha = 1.0;
			DeltaTime *= (0.0 - DestAlpha) / (OldAlpha - DestAlpha);
			if( Target )
			{
#if PLATFORM_64BIT
				UE1Android64InterpDiagV56( "physPathing.overflow-neg.before-target-event", this, DeltaTime, Dest, OldAlpha, DestAlpha, RateModifier );
#endif
				Target->eventInterpolateEnd(this);
#if PLATFORM_64BIT
				UE1Android64InterpDiagV56( "physPathing.overflow-neg.after-target-event", this, DeltaTime, Dest, OldAlpha, DestAlpha, RateModifier );
#endif
				eventInterpolateEnd(Target);
#if PLATFORM_64BIT
				UE1Android64InterpDiagV56( "physPathing.overflow-neg.after-self-event", this, DeltaTime, Dest, OldAlpha, DestAlpha, RateModifier );
#endif
				if( Target->IsA(AInterpolationPoint::StaticClass) )
					Target = Dest->Prev;
			}
			eventInterpolateEnd(NULL);
#if PLATFORM_64BIT
			UE1Android64InterpDiagV56( "physPathing.overflow-neg.after-null-self-event", this, DeltaTime, Cast<AInterpolationPoint>(Target), OldAlpha, DestAlpha, RateModifier );
#endif
		}
		else DeltaTime=0.0;
	};
#if PLATFORM_64BIT
	UE1Android64InterpDiagV56( "physPathing.leave", this, DeltaTime, Cast<AInterpolationPoint>(Target), PhysAlpha, PhysAlpha, 1.0f );
#endif
	unguard;
}

//
// Moving brush.
//
void AActor::physMovingBrush( FLOAT DeltaTime )
{
	guard(physMovingBrush);
	if( IsA(AMover::StaticClass) )
	{
		AMover* Mover  = (AMover*)this;
		INT KeyNum     = Clamp( (INT)Mover->KeyNum, (INT)0, (INT)ARRAY_COUNT(Mover->KeyPos) );
		while( Mover->bInterpolating && DeltaTime>0.0 )
		{
			// We are moving.
			FLOAT NewAlpha = Mover->PhysAlpha + DeltaTime * Mover->PhysRate;
			if( NewAlpha > 1.0 )
			{
				DeltaTime *= (NewAlpha - 1.0) / (NewAlpha - PhysAlpha);
				NewAlpha   = 1.0;
			}
			else DeltaTime = 0.0;

			// Compute alpha.
			FLOAT RenderAlpha;
			if( Mover->MoverGlideType == MV_GlideByTime )
			{
				// Make alpha time-smooth and time-continuous.
				// f(0)=0, f(1)=1, f'(0)=f'(1)=0.
				RenderAlpha = 3.0*NewAlpha*NewAlpha - 2.0*NewAlpha*NewAlpha*NewAlpha;
			}
			else RenderAlpha = NewAlpha;

			// Move.
			FCheckResult Hit(1.0);
			if( XLevel->MoveActor
			(
				Mover,
				Mover->OldPos + ((Mover->BasePos + Mover->KeyPos[KeyNum]) - Mover->OldPos) * RenderAlpha - Mover->Location,
				Mover->OldRot + ((Mover->BaseRot + Mover->KeyRot[KeyNum]) - Mover->OldRot) * RenderAlpha,
				Hit
			) )
			{
				// Successfully moved.
				Mover->PhysAlpha = NewAlpha;
				if( NewAlpha == 1.0 )
				{
					// Just finished moving.
					Mover->bInterpolating = 0;
					Mover->eventInterpolateEnd(NULL);
				}
			}
		}
	}
	unguard;
}

//
// Initialize execution.
//
void AActor::InitExecution()
{
	guard(AActor::InitExecution);

	UObject::InitExecution();

	check(GetMainFrame());
	check(GetMainFrame()->Object==this);
	check(XLevel!=NULL);
	check(XLevel->Actors(0)!=NULL);
	check(XLevel->Actors(0)==Level);
	check(Level!=NULL);

	unguardobj;
}

/*-----------------------------------------------------------------------------
	Intrinsics.
-----------------------------------------------------------------------------*/

void AActor::execRotationConst( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execRotationConst);
	((FRotator*)Result)->Pitch = Stack.ReadInt();
	((FRotator*)Result)->Yaw   = Stack.ReadInt();
	((FRotator*)Result)->Roll  = Stack.ReadInt();
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, EX_RotationConst, execRotationConst );

void AActor::execVectorConst( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execVectorConst);
#ifdef PLATFORM_ARM
	// try to avoid potential unaligned accesses
	appMemcpy( (void*)Result, (void*)Stack.Code, sizeof(FVector) );
#else
	*(FVector*)Result = *(FVector*)Stack.Code;
#endif
	Stack.Code += sizeof(FVector);
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, EX_VectorConst, execVectorConst );

/////////////////
// Conversions //
/////////////////

void AActor::execStringToVector( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execStringToVector);
	BYTE Buffer[MAX_STRING_CONST_SIZE], *Addr=Buffer;
	Stack.Step( Stack.Object, Addr );
	*(FVector*)Result = FVector(0,0,0);
	GetFVECTOR((char*)Addr,*(FVector*)Result);
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, EX_StringToVector, execStringToVector );

void AActor::execStringToRotation( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execStringToRotation);
	BYTE Buffer[MAX_STRING_CONST_SIZE], *Addr=Buffer;
	Stack.Step( Stack.Object, Addr );
	*(FRotator*)Result = FRotator(0,0,0);
	GetFROTATOR((char*)Addr,*(FRotator*)Result,1.0);
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, EX_StringToRotation, execStringToRotation );

void AActor::execVectorToBool( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execVectorToBool);
	BYTE Buffer[MAX_STRING_CONST_SIZE], *Addr=Buffer;
	Stack.Step( Stack.Object, Addr );
	*(DWORD*)Result = ((FVector*)Addr)->IsZero() ? 0 : 1;
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, EX_VectorToBool, execVectorToBool );

void AActor::execVectorToString( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execVectorToString);
	BYTE Buffer[MAX_CONST_SIZE], *Addr=Buffer;
	Stack.Step( Stack.Object, Addr );
	appSprintf((char*)Result,"%f,%f,%f",((FVector*)Addr)->X,((FVector*)Addr)->Y,((FVector*)Addr)->Z);
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, EX_VectorToString, execVectorToString );

void AActor::execVectorToRotation( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execVectorToRotation);
	BYTE Buffer[MAX_CONST_SIZE], *Addr=Buffer;
	Stack.Step( Stack.Object, Addr );
	*(FRotator*)Result = ((FVector*)Addr)->Rotation();
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, EX_VectorToRotation, execVectorToRotation );

void AActor::execRotationToBool( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execRotationToBool);
	BYTE Buffer[MAX_CONST_SIZE], *Addr=Buffer;
	Stack.Step( Stack.Object, Addr );
	*(DWORD*)Result = ((FRotator*)Addr)->IsZero() ? 0 : 1;
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, EX_RotationToBool, execRotationToBool );

void AActor::execRotationToVector( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execRotationToVector);
	BYTE Buffer[MAX_CONST_SIZE], *Addr=Buffer;
	Stack.Step( Stack.Object, Addr );
	*(FVector*)Result = ((FRotator*)Addr)->Vector();
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, EX_RotationToVector, execRotationToVector );

void AActor::execRotationToString( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execRotationToString);
	BYTE Buffer[MAX_CONST_SIZE], *Addr=Buffer;
	Stack.Step( Stack.Object, Addr );
	appSprintf((char*)Result,"%i,%i,%i",((FRotator*)Addr)->Pitch&65535,((FRotator*)Addr)->Yaw&65535,((FRotator*)Addr)->Roll&65535);
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, EX_RotationToString, execRotationToString );

////////////////////////////////////
// Vector operators and functions //
////////////////////////////////////

void AActor::execSubtract_PreVector( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execSubtract_PreVector);

	P_GET_VECTOR(A);
	P_FINISH;

	*(FVector*)Result = -A;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 83, execSubtract_PreVector );

void AActor::execMultiply_VectorFloat( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execMultiply_VectorFloat);

	P_GET_VECTOR(A);
	P_GET_FLOAT (B);
	P_FINISH;

	*(FVector*)Result = A*B;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 84, execMultiply_VectorFloat );

void AActor::execMultiply_FloatVector( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execMultiply_FloatVector);

	P_GET_FLOAT (A);
	P_GET_VECTOR(B);
	P_FINISH;

	*(FVector*)Result = A*B;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 85, execMultiply_FloatVector );

void AActor::execMultiply_VectorVector( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execMultiply_VectorVector);

	P_GET_VECTOR(A);
	P_GET_VECTOR(B);
	P_FINISH;

	*(FVector*)Result = A*B;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 296, execMultiply_VectorVector );

void AActor::execDivide_VectorFloat( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execDivide_VectorFloat);

	P_GET_VECTOR(A);
	P_GET_FLOAT (B);
	P_FINISH;

	*(FVector*)Result = A/B;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 86, execDivide_VectorFloat );

void AActor::execAdd_VectorVector( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execAdd_VectorVector);

	P_GET_VECTOR(A);
	P_GET_VECTOR(B);
	P_FINISH;

	*(FVector*)Result = A+B;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 87, execAdd_VectorVector );

void AActor::execSubtract_VectorVector( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execSubtract_VectorVector);

	P_GET_VECTOR(A);
	P_GET_VECTOR(B);
	P_FINISH;

	*(FVector*)Result = A-B;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 88, execSubtract_VectorVector );

void AActor::execLessLess_VectorRotator( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execLessLess_VectorRotator);

	P_GET_VECTOR(A);
	P_GET_ROTATOR(B);
	P_FINISH;

	*(FVector*)Result = A.TransformVectorBy(GMath.UnitCoords / B);

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 275, execLessLess_VectorRotator );

void AActor::execGreaterGreater_VectorRotator( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execGreaterGreater_VectorRotator);

	P_GET_VECTOR(A);
	P_GET_ROTATOR(B);
	P_FINISH;

	*(FVector*)Result = A.TransformVectorBy(GMath.UnitCoords * B);

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 276, execGreaterGreater_VectorRotator );

void AActor::execEqualEqual_VectorVector( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execEqualEqual_VectorVector);

	P_GET_VECTOR(A);
	P_GET_VECTOR(B);
	P_FINISH;

	*(DWORD*)Result = A.X==B.X && A.Y==B.Y && A.Z==B.Z;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 89, execEqualEqual_VectorVector );

void AActor::execNotEqual_VectorVector( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execNotEqual_VectorVector);

	P_GET_VECTOR(A);
	P_GET_VECTOR(B);
	P_FINISH;

	*(DWORD*)Result = A.X!=B.X || A.Y!=B.Y || A.Z!=B.Z;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 90, execNotEqual_VectorVector );

void AActor::execDot_VectorVector( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execDot_VectorVector);

	P_GET_VECTOR(A);
	P_GET_VECTOR(B);
	P_FINISH;

	*(FLOAT*)Result = A|B;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 91, execDot_VectorVector );

void AActor::execCross_VectorVector( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execCross_VectorVector);

	P_GET_VECTOR(A);
	P_GET_VECTOR(B);
	P_FINISH;

	*(FVector*)Result = A^B;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 92, execCross_VectorVector );

void AActor::execMultiplyEqual_VectorFloat( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execMultiplyEqual_VectorFloat);

	P_GET_VECTOR_REF(A);
	P_GET_FLOAT(B);
	P_FINISH;

	*(FVector*)Result = (*A *= B);

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 93, execMultiplyEqual_VectorFloat );

void AActor::execMultiplyEqual_VectorVector( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execMultiplyEqual_VectorVector);

	P_GET_VECTOR_REF(A);
	P_GET_VECTOR(B);
	P_FINISH;

	*(FVector*)Result = (*A *= B);

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 297, execMultiplyEqual_VectorVector );

void AActor::execDivideEqual_VectorFloat( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execDivideEqual_VectorFloat);

	P_GET_VECTOR_REF(A);
	P_GET_FLOAT(B);
	P_FINISH;

	*(FVector*)Result = (*A /= B);

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 94, execDivideEqual_VectorFloat );

void AActor::execAddEqual_VectorVector( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execAddEqual_VectorVector);

	P_GET_VECTOR_REF(A);
	P_GET_VECTOR(B);
	P_FINISH;

	*(FVector*)Result = (*A += B);

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 95, execAddEqual_VectorVector );

void AActor::execSubtractEqual_VectorVector( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execSubtractEqual_VectorVector);

	P_GET_VECTOR_REF(A);
	P_GET_VECTOR(B);
	P_FINISH;

	*(FVector*)Result = (*A -= B);

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 96, execSubtractEqual_VectorVector );

void AActor::execVSize( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execVSize);

	P_GET_VECTOR(A);
	P_FINISH;

	*(FLOAT*)Result = A.Size();

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 97, execVSize );

void AActor::execNormal( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execNormal);

	P_GET_VECTOR(A);
	P_FINISH;

	*(FVector*)Result = A.SafeNormal();

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 98, execNormal );

void AActor::execInvert( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execInvert);

	P_GET_VECTOR_REF(X);
	P_GET_VECTOR_REF(Y);
	P_GET_VECTOR_REF(Z);
	P_FINISH;

	FCoords Temp = FCoords( FVector(0,0,0), *X, *Y, *Z ).Inverse();
	*X           = Temp.XAxis;
	*Y           = Temp.YAxis;
	*Z           = Temp.ZAxis;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 99, execInvert );

void AActor::execVRand( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execVRand);
	P_FINISH;
	*((FVector*)Result) = VRand();
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 124, execVRand );

void AActor::execRotRand( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execRotRand);
	P_GET_UBOOL_OPT(bRoll, 0);
	P_FINISH;

	FRotator RRot;
	RRot.Yaw = ((2 * appRand()) % 65535);
	RRot.Pitch = ((2 * appRand()) % 65535);
	if ( bRoll )
		RRot.Roll = ((2 * appRand()) % 65535);
	else
		RRot.Roll = 0;
	*((FRotator*)Result) = RRot;
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 320, execRotRand );

void AActor::execMirrorVectorByNormal( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execMirrorVectorByNormal);

	P_GET_VECTOR(A);
	P_GET_VECTOR(B);
	P_FINISH;

	B = B.SafeNormal();
	*(FVector*)Result = A - 2.f * B * (B | A);

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 300, execMirrorVectorByNormal );

//////////////////////////////////////
// Rotation operators and functions //
//////////////////////////////////////

void AActor::execEqualEqual_RotatorRotator( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execEqualEqual_RotatorRotator);

	P_GET_ROTATOR(A);
	P_GET_ROTATOR(B);
	P_FINISH;

	*(DWORD*)Result = A.Pitch==B.Pitch && A.Yaw==B.Yaw && A.Roll==B.Roll;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 14, execEqualEqual_RotatorRotator );

void AActor::execNotEqual_RotatorRotator( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execNotEqual_RotatorRotator);

	P_GET_ROTATOR(A);
	P_GET_ROTATOR(B);
	P_FINISH;

	*(DWORD*)Result = A.Pitch!=B.Pitch || A.Yaw!=B.Yaw || A.Roll!=B.Roll;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 75, execNotEqual_RotatorRotator );

void AActor::execMultiply_RotatorFloat( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execMultiply_RotatorFloat);

	P_GET_ROTATOR(A);
	P_GET_FLOAT(B);
	P_FINISH;

	*(FRotator*)Result = A * B;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 287, execMultiply_RotatorFloat );

void AActor::execMultiply_FloatRotator( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execMultiply_FloatRotator);

	P_GET_FLOAT(A);
	P_GET_ROTATOR(B);
	P_FINISH;

	*(FRotator*)Result = B * A;

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 288, execMultiply_FloatRotator );

void AActor::execDivide_RotatorFloat( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execDivide_RotatorFloat);

	P_GET_ROTATOR(A);
	P_GET_FLOAT(B);
	P_FINISH;

	*(FRotator*)Result = A * (1.0/B);

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 289, execDivide_RotatorFloat );

void AActor::execMultiplyEqual_RotatorFloat( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execMultiplyEqual_RotatorFloat);

	P_GET_ROTATOR_REF(A);
	P_GET_FLOAT(B);
	P_FINISH;

	*(FRotator*)Result = (*A *= B);

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 290, execMultiplyEqual_RotatorFloat );

void AActor::execDivideEqual_RotatorFloat( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execDivideEqual_RotatorFloat);

	P_GET_ROTATOR_REF(A);
	P_GET_FLOAT(B);
	P_FINISH;

	*(FRotator*)Result = (*A *= (1.0/B));

	unguardexecSlow;
}	
AUTOREGISTER_INTRINSIC( AActor, 291, execDivideEqual_RotatorFloat );

void AActor::execAdd_RotatorRotator( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execAdd_RotatorRotator);

	P_GET_ROTATOR(A);
	P_GET_ROTATOR(B);
	P_FINISH;

	*(FRotator*)Result = A + B;

	unguardSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 316, execAdd_RotatorRotator );

void AActor::execSubtract_RotatorRotator( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execSubtract_RotatorRotator);

	P_GET_ROTATOR(A);
	P_GET_ROTATOR(B);
	P_FINISH;

	*(FRotator*)Result = A - B;

	unguardSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 317, execSubtract_RotatorRotator );

void AActor::execAddEqual_RotatorRotator( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execAddEqual_RotatorRotator);

	P_GET_ROTATOR_REF(A);
	P_GET_ROTATOR(B);
	P_FINISH;

	*(FRotator*)Result = (*A += B);

	unguardSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 318, execAddEqual_RotatorRotator );

void AActor::execSubtractEqual_RotatorRotator( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execSubtractEqual_RotatorRotator);

	P_GET_ROTATOR_REF(A);
	P_GET_ROTATOR(B);
	P_FINISH;

	*(FRotator*)Result = (*A -= B);

	unguardSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 319, execSubtractEqual_RotatorRotator );

void AActor::execGetAxes( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execGetAxes);

	P_GET_ROTATOR(A);
	P_GET_VECTOR_REF(X);
	P_GET_VECTOR_REF(Y);
	P_GET_VECTOR_REF(Z);
	P_FINISH;

	FCoords Coords = GMath.UnitCoords / A;
	*X = Coords.XAxis;
	*Y = Coords.YAxis;
	*Z = Coords.ZAxis;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 101, execGetAxes );

void AActor::execGetUnAxes( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execGetUnAxes);

	P_GET_ROTATOR(A);
	P_GET_VECTOR_REF(X);
	P_GET_VECTOR_REF(Y);
	P_GET_VECTOR_REF(Z);
	P_FINISH;

	FCoords Coords = GMath.UnitCoords * A;
	*X = Coords.XAxis;
	*Y = Coords.YAxis;
	*Z = Coords.ZAxis;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 102, execGetUnAxes );

/////////////////////////////
// Log and error functions //
/////////////////////////////

void AActor::execError( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execError);

	P_GET_STRING(S);
	P_FINISH;

	Stack.ScriptWarn( 0, S );
	XLevel->DestroyActor( this );

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 0x80 + 105, execError );

//////////////////////////
// Clientside functions //
//////////////////////////

void APlayerPawn::execClientMessage( FFrame& Stack, BYTE*& Result )
{
	guardSlow(APlayerPawn::execClientMessage);

	P_GET_STRING(S);
	P_FINISH;

	if( Player && Player->IsA(UViewport::StaticClass) )
		((UViewport*)Player)->Log( NAME_Play, S );

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( APlayerPawn, INDEX_NONE, execClientMessage );

void APlayerPawn::execClientTravel( FFrame& Stack, BYTE*& Result )
{
	guardSlow(APlayerPawn::execClientTravel);

	P_GET_STRING(URL);
	P_GET_BYTE(TravelType);
	P_GET_UBOOL(bItems);
	P_FINISH;

	if( Player )
		XLevel->Engine->SetClientTravel( Player, URL, 1, bItems, (ETravelType)TravelType );

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( APlayerPawn, INDEX_NONE, execClientTravel );

void ALevelInfo::execGetLocalURL( FFrame& Stack, BYTE*& Result )
{
	guardSlow(ALevelInfo::execGetLocalURL);

	P_FINISH;

	FString Str;
	XLevel->URL.String(Str);
	appStrncpy( (char*)Result, *Str, 240 );

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( ALevelInfo, INDEX_NONE, execGetLocalURL );

void APawn::execClientHearSound( FFrame& Stack, BYTE*& Result )
{
	guard(APawn::execClientHearSound);

	P_GET_OBJECT(AActor,Actor);
	P_GET_INT(Id);
	P_GET_OBJECT(USound,Sound);
	P_GET_VECTOR(SoundLocation);
	P_GET_VECTOR(Parameters);
	P_FINISH;

	FLOAT Volume = 0.01 * Parameters.X;
	FLOAT Radius = Parameters.Y;
	FLOAT Pitch = 0.01 * Parameters.Z;
	if
	(	IsA(APlayerPawn::StaticClass) 
	&&	((APlayerPawn*)this)->Player
	&&	((APlayerPawn*)this)->Player->IsA(UViewport::StaticClass)
	&&	XLevel->Engine->Audio )
	{
		if( Actor && Actor->bDeleteMe )
			Actor = NULL;
		XLevel->Engine->Audio->PlaySound( Actor, Id, Sound, SoundLocation, Volume, Radius ? Radius : 1600.f, Pitch );
	}
	unguardexec;
}
AUTOREGISTER_INTRINSIC( APawn, INDEX_NONE, execClientHearSound );

//////////////////////////////
// Slow function initiators //
//////////////////////////////

void AActor::execSleep( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execSleep);

	P_GET_FLOAT(Seconds);
	P_FINISH;

	GetMainFrame()->LatentAction = EPOLL_Sleep;
	LatentFloat  = Seconds;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 256, execSleep );

void AActor::execFinishAnim( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execFinishAnim);

	P_FINISH;

	// If we are looping, finish at the next sequence end.
	if( bAnimLoop )
	{
		bAnimLoop     = 0;
		bAnimFinished = 0;
	}

	// If animation is playing, wait for it to finish.
	if( IsAnimating() && AnimFrame<AnimLast )
		GetMainFrame()->LatentAction = EPOLL_FinishAnim;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 261, execFinishAnim );

void AActor::execFinishInterpolation( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execFinishInterpolation);

	P_FINISH;

	GetMainFrame()->LatentAction = EPOLL_FinishInterpolation;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 301, execFinishInterpolation );

///////////////////////////
// Slow function pollers //
///////////////////////////

void AActor::execPollSleep( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execPollSleep);

#ifdef PLATFORM_ARM
	// try to avoid potential unaligned accesses
	FLOAT DeltaSeconds = 0.0f;
	appMemcpy( (void*)&DeltaSeconds, (void*)Result, sizeof(FLOAT) );
#else
	FLOAT DeltaSeconds = *(FLOAT*)Result;
#endif
	if( (LatentFloat-=DeltaSeconds) < 0.5 * DeltaSeconds )
	{
		// Awaken.
		GetMainFrame()->LatentAction = 0;
	}
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, EPOLL_Sleep, execPollSleep );

void AActor::execPollFinishAnim( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execPollFinishAnim);

	if( bAnimFinished )
		GetMainFrame()->LatentAction = 0;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, EPOLL_FinishAnim, execPollFinishAnim );

void AActor::execPollFinishInterpolation( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execPollFinishInterpolation);

	if( !bInterpolating )
		GetMainFrame()->LatentAction = 0;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, EPOLL_FinishInterpolation, execPollFinishInterpolation );

/////////////////////////
// Animation functions //
/////////////////////////

void AActor::execPlayAnim( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execPlayAnim);

	P_GET_NAME(SequenceName);
	P_GET_FLOAT_OPT(PlayAnimRate,1.0);
	P_GET_FLOAT_OPT(TweenTime,-1.0);
	P_FINISH;

	// Set one-shot animation.
	if( Mesh )
	{
		const FMeshAnimSeq* Seq = Mesh->GetAnimSeq( SequenceName );
		if( Seq )
		{
			if( AnimSequence == NAME_None )
				TweenTime = 0.0;
			AnimSequence  = SequenceName;
			AnimRate      = PlayAnimRate * Seq->Rate / Seq->NumFrames;
			AnimLast      = 1.0 - 1.0 / Seq->NumFrames;
			bAnimNotify   = Seq->Notifys.Num()!=0;
			bAnimFinished = 0;
			bAnimLoop     = 0;
			if( AnimLast == 0.0 )
			{
				AnimMinRate   = 0.0;
				bAnimNotify   = 0;
				OldAnimRate   = 0;
				if( TweenTime > 0.0 )
					TweenRate = 1.0 / TweenTime;
				else
					TweenRate = 10.0; //tween in 0.1 sec
				AnimFrame = -1.0/Seq->NumFrames;
				AnimRate = 0;
			}
			else if( TweenTime>0.0 )
			{
				TweenRate = 1.0 / (TweenTime * Seq->NumFrames);
				AnimFrame = -1.0/Seq->NumFrames;
			}
			else if ( TweenTime == -1.0 )
			{
				AnimFrame = -1.0/Seq->NumFrames;
				if ( OldAnimRate > 0 )
					TweenRate = OldAnimRate;
				else if ( OldAnimRate < 0 ) //was velocity based looping
					TweenRate = ::Max(0.5f * AnimRate, -1 * Velocity.Size() * OldAnimRate );
				else
					TweenRate =  1.0/(0.025 * Seq->NumFrames);
			}
			else
			{
				TweenRate = 0.0;
				AnimFrame = 0.001;
			}
			FPlane OldSimAnim = SimAnim;
			SimAnim.X = 10000 * AnimFrame;
			SimAnim.Y = 10000 * AnimRate;
			SimAnim.Z = 1000 * TweenRate;
			SimAnim.W = 10000 * AnimLast;
			/*
			if ( IsA(AWeapon::StaticClass)
				&& (PlayAnimRate * Seq->Rate < 0.21) )
			{
				SimAnim.X = 0;
				SimAnim.Z = 0;
			} */
				
			if ( OldSimAnim == SimAnim )
				SimAnim.W = SimAnim.W + 1;
			OldAnimRate = AnimRate;
			//debugf("%s PlayAnim %f %f %f %f", GetName(), SimAnim.X, SimAnim.Y, SimAnim.Z, SimAnim.W);
		}
		else Stack.ScriptWarn( 0, "PlayAnim: Sequence '%s' not found in Mesh '%s'", *SequenceName, Mesh->GetName() );
	} else Stack.ScriptWarn( 0, "PlayAnim: No mesh" );
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 259, execPlayAnim );

void AActor::execLoopAnim( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execLoopAnim);
	
	P_GET_NAME(SequenceName);
	P_GET_FLOAT_OPT(PlayAnimRate,1.0);
	P_GET_FLOAT_OPT(TweenTime,-1.0);
	P_GET_FLOAT_OPT(MinRate,0.0);
	P_FINISH;

	// Set looping animation.
	if( Mesh )
	{
		const FMeshAnimSeq* Seq = Mesh->GetAnimSeq( SequenceName );
		if( Seq )
		{
			if ( (AnimSequence == SequenceName) && bAnimLoop && IsAnimating() )
			{
				AnimRate      = PlayAnimRate * Seq->Rate / Seq->NumFrames;
				bAnimFinished = 0;
				AnimMinRate   = MinRate!=0.0 ? MinRate * (Seq->Rate / Seq->NumFrames) : 0.0;
				FPlane OldSimAnim = SimAnim;
				OldAnimRate   = AnimRate;		
				SimAnim.Y = 10000 * AnimRate;
				SimAnim.W = -10000 * (1.0 - 1.0 / Seq->NumFrames);
				if ( OldSimAnim == SimAnim )
					SimAnim.W = SimAnim.W + 1;
				return;
			}
			if( AnimSequence == NAME_None )
				TweenTime = 0.0;
			AnimSequence  = SequenceName;
			AnimRate      = PlayAnimRate * Seq->Rate / Seq->NumFrames;
			AnimLast      = 1.0 - 1.0 / Seq->NumFrames;
			AnimMinRate   = MinRate!=0.0 ? MinRate * (Seq->Rate / Seq->NumFrames) : 0.0;
			bAnimNotify   = Seq->Notifys.Num()!=0;
			bAnimFinished = 0;
			bAnimLoop     = 1;
			if ( AnimLast == 0.0 )
			{
				AnimMinRate   = 0.0;
				bAnimNotify   = 0;
				OldAnimRate   = 0;
				if ( TweenTime > 0.0 )
					TweenRate = 1.0 / TweenTime;
				else
					TweenRate = 10.0; //tween in 0.1 sec
				AnimFrame = -1.0/Seq->NumFrames;
				AnimRate = 0;
			}
			else if( TweenTime>0.0 )
			{
				TweenRate = 1.0 / (TweenTime * Seq->NumFrames);
				AnimFrame = -1.0/Seq->NumFrames;
			}
			else if ( TweenTime == -1.0 )
			{
				AnimFrame = -1.0/Seq->NumFrames;
				if ( OldAnimRate > 0 )
					TweenRate = OldAnimRate;
				else if ( OldAnimRate < 0 ) //was velocity based looping
					TweenRate = ::Max(0.5f * AnimRate, -1 * Velocity.Size() * OldAnimRate );
				else
					TweenRate =  1.0/(0.025 * Seq->NumFrames);
			}
			else
			{
				TweenRate = 0.0;
				AnimFrame = 0.0001;
			}
			OldAnimRate = AnimRate;
			SimAnim.X = 10000 * AnimFrame;
			SimAnim.Y = 10000 * AnimRate;
			SimAnim.Z = 1000 * TweenRate;
			SimAnim.W = -10000 * AnimLast;
			//debugf("%s LoopAnim %f %f %f %f", GetName(), SimAnim.X, SimAnim.Y, SimAnim.Z, SimAnim.W);
		}
		else Stack.ScriptWarn( 0, "LoopAnim: Sequence '%s' not found in Mesh '%s'", *SequenceName, Mesh->GetName() );
	} else Stack.ScriptWarn( 0, "LoopAnim: No mesh" );
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 260, execLoopAnim );

void AActor::execTweenAnim( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execTweenAnim);

	P_GET_NAME(SequenceName);
	P_GET_FLOAT(TweenTime);
	P_FINISH;

	// Tweening an animation from wherever it is, to the start of a specified sequence.
	if( Mesh )
	{
		const FMeshAnimSeq* Seq = Mesh->GetAnimSeq( SequenceName );
		if( Seq )
		{
			AnimSequence  = SequenceName;
			AnimLast      = 0.0;
			AnimMinRate   = 0.0;
			bAnimNotify   = 0;
			bAnimFinished = 0;
			bAnimLoop     = 0;
			AnimRate      = 0;
			OldAnimRate   = 0;
			if( TweenTime>0.0 )
			{
				TweenRate =  1.0/(TweenTime * Seq->NumFrames);
				AnimFrame = -1.0/Seq->NumFrames;
			}
			else
			{
				TweenRate = 0.0;
				AnimFrame = 0.0;
			}
			SimAnim.X = 10000 * AnimFrame;
			SimAnim.Y = 10000 * AnimRate;
			SimAnim.Z = 1000 * TweenRate;
			SimAnim.W = 10000 * AnimLast;
			//debugf("%s TweenAnim %f %f %f %f", GetName(), SimAnim.X, SimAnim.Y, SimAnim.Z, SimAnim.W);
		}
		else Stack.ScriptWarn( 0, "TweenAnim: Sequence '%s' not found in Mesh '%s'", *SequenceName, Mesh->GetName() );
	} else Stack.ScriptWarn( 0, "TweenAnim: No mesh" );
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 294, execTweenAnim );

void AActor::execIsAnimating( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execIsAnimating);

	P_FINISH;

	*(DWORD*)Result = IsAnimating();

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 282, execIsAnimating );

void AActor::execGetAnimGroup( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execGetAnimGroup);

	P_GET_NAME(SequenceName);
	P_FINISH;

	// Return the animation group.
	*(FName*)Result = NAME_None;
	if( Mesh )
	{
		const FMeshAnimSeq* Seq = Mesh->GetAnimSeq( SequenceName );
		if( Seq )
		{
			*(FName*)Result = Seq->Group;
		}
		else Stack.ScriptWarn( 0, "GetAnimGroup: Sequence '%s' not found in Mesh '%s'", *SequenceName, Mesh->GetName() );
	} else Stack.ScriptWarn( 0, "GetAnimGroup: No mesh" );

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 293, execGetAnimGroup );

///////////////
// Collision //
///////////////

void AActor::execSetCollision( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execSetCollision);

	P_GET_UBOOL_OPT(NewCollideActors,bCollideActors);
	P_GET_UBOOL_OPT(NewBlockActors,  bBlockActors  );
	P_GET_UBOOL_OPT(NewBlockPlayers, bBlockPlayers );
	P_FINISH;

	SetCollision( NewCollideActors, NewBlockActors, NewBlockPlayers );

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 262, execSetCollision );

void AActor::execSetCollisionSize( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execSetCollisionSize);

	P_GET_FLOAT(NewRadius);
	P_GET_FLOAT(NewHeight);
	P_FINISH;

	SetCollisionSize( NewRadius, NewHeight );

	// Return boolean success or failure.
	*(DWORD*)Result = 1;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 283, execSetCollisionSize );

void AActor::execSetBase( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execSetFloor);

	P_GET_OBJECT(AActor,NewBase);
	P_FINISH;

	SetBase( NewBase );

	unguardSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 298, execSetBase );

///////////
// Audio //
///////////

void AActor::execPlaySound( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execPlaySound);

	// Get parameters.
	P_GET_OBJECT(USound,Sound);
	P_GET_BYTE_OPT(Slot,SLOT_Misc);
	P_GET_FLOAT_OPT(Volume,TransientSoundVolume);
	P_GET_UBOOL_OPT(bNoOverride,0);
	P_GET_FLOAT_OPT(Radius,0.0);
	P_GET_FLOAT_OPT(Pitch,1.0);
	P_FINISH;
	if( !Sound )
		return;
	INT Id = GetIndex()*16 + Slot*2 + bNoOverride;
	FLOAT RadiusSquared = Square( Radius ? Radius : 1600.f );
	FVector Parameters = FVector(100 * Volume, Radius, 100 * Pitch);

#if PLATFORM_64BIT
	// arm64: play local effects directly instead of round-tripping through the
	// generated ClientHearSound event frame.  That event frame is pack(4) and has
	// repeatedly been the source of corrupted USound*/Actor* parameters during the
	// 32->64 migration.  This keeps local gameplay audio alive without changing
	// menu, renderer or network code paths; if local playback fails, the original
	// event propagation below still runs as a fallback.
	UBOOL bPlayedLocalDirect = 0;
	if( XLevel && XLevel->Engine && XLevel->Engine->Audio && XLevel->Engine->Client )
	{
		UClient* Client = XLevel->Engine->Client;
		for( INT i=0; i<Client->Viewports.Num(); i++ )
		{
			APlayerPawn* Hearer = Client->Viewports(i)->Actor;
			if( Hearer && Hearer->XLevel==XLevel )
			{
				FLOAT NewRadiusSquared = RadiusSquared / 1.3f;
				if( (Hearer->Location-Location).SizeSquared() < NewRadiusSquared )
					bPlayedLocalDirect |= XLevel->Engine->Audio->PlaySound( this, Id, Sound, Location, Volume, Radius ? Radius : 1600.f, Pitch );
			}
		}
	}
	if( bPlayedLocalDirect )
		return;
#endif

	// See if the function is simulated.
	UFunction* Caller = Cast<UFunction>( Stack.Node );
	if( Caller && (Caller->FunctionFlags & FUNC_Simulated) )
	{
		// Called from a simulated function, so propagate locally only.
		UClient* Client = XLevel->Engine->Client;
		if( Client )
		{
			for( INT i=0; i<Client->Viewports.Num(); i++ )
			{
				APlayerPawn* Hearer = Client->Viewports(i)->Actor;
				if( Hearer && Hearer->XLevel==XLevel )
				{
					INT Portals = GetLevel()->ZoneDist[Region.ZoneNumber][Hearer->Region.ZoneNumber];
					FLOAT NewRadiusSquared = RadiusSquared;
						NewRadiusSquared /= 1.3;
					if( (Hearer->Location-Location).SizeSquared() < NewRadiusSquared )
						Hearer->eventClientHearSound( this, Id, Sound, Location, Parameters );
				}
			}
		}
	}
	else
	{
		// Propagate to all player actors.
		for( APawn* Hearer=Level->PawnList; Hearer; Hearer=Hearer->nextPawn )
		{
			if( Hearer->bIsPlayer )
			{
				INT Portals = GetLevel()->ZoneDist[Region.ZoneNumber][Hearer->Region.ZoneNumber];
				FLOAT NewRadiusSquared = RadiusSquared;
					NewRadiusSquared /= 1.3;
				if( (Hearer->Location-Location).SizeSquared() < NewRadiusSquared )
					Hearer->eventClientHearSound( this, Id, Sound, Location, Parameters );
			}
		}
	}
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 264, execPlaySound );

//////////////
// Movement //
//////////////

void AActor::execMove( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execMove);

	P_GET_VECTOR(Delta);
	P_FINISH;

	FCheckResult Hit(1.0);
	*(DWORD*)Result = GetLevel()->MoveActor( this, Delta, Rotation, Hit );

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 266, execMove );

void AActor::execSetLocation( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execSetLocation);

	P_GET_VECTOR(NewLocation);
	P_FINISH;

	*(DWORD*)Result = GetLevel()->FarMoveActor( this, NewLocation );

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 267, execSetLocation );

void AActor::execSetRotation( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execSetRotation);

	P_GET_ROTATOR(NewRotation);
	P_FINISH;

	FCheckResult Hit(1.0);
	*(DWORD*)Result = GetLevel()->MoveActor( this, FVector(0,0,0), NewRotation, Hit );

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 299, execSetRotation );

///////////////
// Relations //
///////////////

void AActor::execSetOwner( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execSetOwner);

	P_GET_ACTOR(NewOwner);
	P_FINISH;

	SetOwner( NewOwner );

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 272, execSetOwner );

void AActor::execIsA( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execIsA);

	P_GET_NAME(ClassName);
	P_FINISH;

	int success = 0;
#if PLATFORM_64BIT
	// Only guard the actual dereference.  Earlier builds filtered bad contexts in
	// execContext itself, which kept the game alive but also turned legitimate
	// Trigger/Timer/AI event paths into silent None-results.  Here an impossible
	// context simply makes IsA false, while valid script contexts still execute.
	if( ((UPTRINT)this >= 0x10000) && (((UPTRINT)this & (sizeof(void*)-1)) == 0) && ClassName.IsValid() )
#endif
	{
		for( UClass *TempClass=GetClass(); TempClass; TempClass=TempClass->GetSuperClass() )
			if( TempClass->GetFName() == ClassName )
			{
				success = 1;
				break;
			}
	}

	*(DWORD*)Result = success;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 303, execIsA );

//////////////////
// Line tracing //
//////////////////

void AActor::execTrace( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execTrace);

	P_GET_VECTOR_REF(HitLocation);
	P_GET_VECTOR_REF(HitNormal);
	P_GET_VECTOR(TraceEnd);
	P_GET_VECTOR_OPT(TraceStart,Location);
	P_GET_UBOOL_OPT(bTraceActors,bCollideActors);
	P_GET_VECTOR_OPT(TraceExtent,FVector(0,0,0));
	P_FINISH;

	// Trace the line.
	FCheckResult Hit(1.0);
	DWORD TraceFlags;
	if( bTraceActors )
		TraceFlags = TRACE_AllColliding | TRACE_ProjTargets;
	else
		TraceFlags = TRACE_VisBlocking;

	XLevel->SingleLineCheck( Hit, this, TraceEnd, TraceStart, TraceFlags, TraceExtent );
	*(AActor**)Result = Hit.Actor;
	*HitLocation      = Hit.Location;
	*HitNormal        = Hit.Normal;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 277, execTrace );

///////////////////////
// Spawn and Destroy //
///////////////////////

#if PLATFORM_64BIT
static UBOOL UE1AndroidKnownUObjectPointer64( const UObject* Object )
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

static UBOOL UE1AndroidKnownClassPointer64( UClass* Class, UClass* RequiredParent )
{
	if( !UE1AndroidKnownUObjectPointer64( (UObject*)Class ) )
		return 0;
	if( !((UObject*)Class)->IsA(UClass::StaticClass) )
		return 0;
	return !RequiredParent || Class->IsChildOf( RequiredParent );
}
#endif


#if PLATFORM_64BIT
// UNREAL_ANDROID64_EXPLOSION_DAMAGE_ORIGIN_PROBE_V82
static INT GUE1Android64ExplosionScriptBudgetV82 = 0;

static UBOOL UE1Android64ScriptV82Contains( const char* Text, const char* Token )
{
	return Text && Token && appStrfind((char*)Text,(char*)Token)!=NULL;
}

static UBOOL UE1Android64ScriptV82InterestingName( const char* Name )
{
	return UE1Android64ScriptV82Contains(Name,"Explosion")
		|| UE1Android64ScriptV82Contains(Name,"Explode")
		|| UE1Android64ScriptV82Contains(Name,"Barrel")
		|| UE1Android64ScriptV82Contains(Name,"Fragment")
		|| UE1Android64ScriptV82Contains(Name,"Chunk")
		|| UE1Android64ScriptV82Contains(Name,"Smoke")
		|| UE1Android64ScriptV82Contains(Name,"Flame")
		|| UE1Android64ScriptV82Contains(Name,"Blast")
		|| UE1Android64ScriptV82Contains(Name,"Flash")
		|| UE1Android64ScriptV82Contains(Name,"Spark")
		|| UE1Android64ScriptV82Contains(Name,"Shock")
		|| UE1Android64ScriptV82Contains(Name,"Sludge")
		|| UE1Android64ScriptV82Contains(Name,"Projectile")
		|| UE1Android64ScriptV82Contains(Name,"Rocket")
		|| UE1Android64ScriptV82Contains(Name,"Grenade")
		|| UE1Android64ScriptV82Contains(Name,"Flak")
		|| UE1Android64ScriptV82Contains(Name,"ASMD")
		|| UE1Android64ScriptV82Contains(Name,"Dispersion");
}

static const char* UE1Android64ScriptV82StateName( AActor* Actor )
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

static INT UE1Android64ScriptV82CodeOffset( AActor* Actor )
{
	if( !Actor )
		return -1;
	FMainFrame* Frame = Actor->GetMainFrame();
	if( !Frame || !Frame->Node || !Frame->Code || Frame->Node->Script.Num() <= 0 )
		return -1;
	BYTE* Base = &Frame->Node->Script(0);
	BYTE* End  = Base + Frame->Node->Script.Num();
	return (Frame->Code >= Base && Frame->Code < End) ? (INT)(Frame->Code - Base) : -1;
}

static void UE1Android64ScriptV82Log( const char* Phase, AActor* Context, UClass* SpawnClass=NULL, AActor* Spawned=NULL )
{
	if( GUE1Android64ExplosionScriptBudgetV82++ >= 192 )
		return;
	FMainFrame* ContextFrame = Context ? Context->GetMainFrame() : NULL;
	FMainFrame* SpawnedFrame = Spawned ? Spawned->GetMainFrame() : NULL;
	debugf( NAME_Warning, "ANDROID64 EXPLOSION SCRIPT V82 phase=%s context=%s contextClass=%s contextFrame=%p contextState=%s contextCode=%i contextLatent=%i contextPhys=%i contextLoc=(%.1f %.1f %.1f) contextVel=(%.1f %.1f %.1f) spawnClass=%s spawned=%s spawnedFrame=%p spawnedState=%s spawnedCode=%i spawnedPhys=%i spawnedLoc=(%.1f %.1f %.1f) owner=%s instigator=%s budget=%i",
		Phase ? Phase : "?",
		Context ? Context->GetFullName() : "<null>",
		Context ? Context->GetClassName() : "<null>",
		ContextFrame,
		UE1Android64ScriptV82StateName(Context),
		UE1Android64ScriptV82CodeOffset(Context),
		ContextFrame ? ContextFrame->LatentAction : -1,
		Context ? (INT)Context->Physics : -1,
		Context ? Context->Location.X : 0.f,
		Context ? Context->Location.Y : 0.f,
		Context ? Context->Location.Z : 0.f,
		Context ? Context->Velocity.X : 0.f,
		Context ? Context->Velocity.Y : 0.f,
		Context ? Context->Velocity.Z : 0.f,
		SpawnClass ? SpawnClass->GetName() : "<none>",
		Spawned ? Spawned->GetFullName() : "<none>",
		SpawnedFrame,
		UE1Android64ScriptV82StateName(Spawned),
		UE1Android64ScriptV82CodeOffset(Spawned),
		Spawned ? (INT)Spawned->Physics : -1,
		Spawned ? Spawned->Location.X : 0.f,
		Spawned ? Spawned->Location.Y : 0.f,
		Spawned ? Spawned->Location.Z : 0.f,
		Context && Context->Owner ? Context->Owner->GetFullName() : "<none>",
		Context && Context->Instigator ? Context->Instigator->GetFullName() : "<none>",
		GUE1Android64ExplosionScriptBudgetV82 );
}
#endif

void AActor::execSpawn( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execSpawn);

	P_GET_OBJECT(UClass,SpawnClass);
	P_GET_OBJECT_OPT(SpawnOwner, NULL); 
	P_GET_NAME_OPT(SpawnName,NAME_None);
	P_GET_VECTOR_OPT(SpawnLocation,Location);
	P_GET_ROTATOR_OPT(SpawnRotation,Rotation);
	P_FINISH;

#if PLATFORM_64BIT
	if( SpawnClass && !UE1AndroidKnownClassPointer64( SpawnClass, AActor::StaticClass ) )
	{
		static UBOOL WarnedBadSpawnClass = 0;
		if( !WarnedBadSpawnClass )
		{
			WarnedBadSpawnClass = 1;
			debugf( NAME_Warning, "arm64 Spawn rejected invalid class pointer %p", SpawnClass );
		}
		SpawnClass = NULL;
	}
#endif

	// Spawn and return actor.
#if PLATFORM_64BIT
	UBOOL bAndroid64ExplosionScriptSpawnV82 = SpawnClass && UE1Android64ScriptV82InterestingName( SpawnClass->GetName() );
	if( bAndroid64ExplosionScriptSpawnV82 )
		UE1Android64ScriptV82Log( "execSpawn.before", this, SpawnClass, NULL );
#endif
	AActor *Spawned = SpawnClass ? GetLevel()->SpawnActor
	(
		SpawnClass,
		NAME_None,
		(AActor*)SpawnOwner,
		Instigator,
		SpawnLocation,
		SpawnRotation
	) : NULL;
	if( Spawned )
		Spawned->Tag = SpawnName;
#if PLATFORM_64BIT
	if( bAndroid64ExplosionScriptSpawnV82 || (Spawned && UE1Android64ScriptV82InterestingName(Spawned->GetClassName())) )
		UE1Android64ScriptV82Log( "execSpawn.after", this, SpawnClass, Spawned );
#endif
	*(AActor**)Result = Spawned;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 278, execSpawn );

void AActor::execDestroy( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execDestroy);

	P_FINISH;
	
#if PLATFORM_64BIT
	UBOOL bAndroid64ExplosionScriptDestroyV82 = UE1Android64ScriptV82InterestingName( GetName() ) || UE1Android64ScriptV82InterestingName( GetClassName() );
	if( bAndroid64ExplosionScriptDestroyV82 )
		UE1Android64ScriptV82Log( "execDestroy.before", this, NULL, NULL );
#endif
	*(DWORD*)Result = GetLevel()->DestroyActor( this );
#if PLATFORM_64BIT
	if( bAndroid64ExplosionScriptDestroyV82 )
		UE1Android64ScriptV82Log( "execDestroy.after", this, NULL, NULL );
#endif

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 279, execDestroy );

////////////
// Timing //
////////////

void AActor::execSetTimer( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execSetTimer);

	P_GET_FLOAT(NewTimerRate);
	P_GET_UBOOL(bLoop);
	P_FINISH;

	TimerCounter = 0.0;
	TimerRate    = NewTimerRate;
	bTimerLoop   = bLoop;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 280, execSetTimer );

////////////////
// Warp zones //
////////////////

void AWarpZoneInfo::execWarp( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AWarpZoneInfo::execWarp);

	P_GET_VECTOR_REF(WarpLocation);
	P_GET_VECTOR_REF(WarpVelocity);
	P_GET_ROTATOR_REF(WarpRotation);
	P_FINISH;

	// Perform warping.
	*WarpLocation = (*WarpLocation).TransformPointBy ( WarpCoords.Transpose() );
	*WarpVelocity = (*WarpVelocity).TransformVectorBy( WarpCoords.Transpose() );
	*WarpRotation = (GMath.UnitCoords / *WarpRotation * WarpCoords.Transpose()).OrthoRotation();

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AWarpZoneInfo, 314, execWarp );

void AWarpZoneInfo::execUnWarp( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AWarpZoneInfo::execUnWarp);

	P_GET_VECTOR_REF(WarpLocation);
	P_GET_VECTOR_REF(WarpVelocity);
	P_GET_ROTATOR_REF(WarpRotation);
	P_FINISH;

	// Perform unwarping.
	*WarpLocation = (*WarpLocation).TransformPointBy ( WarpCoords );
	*WarpVelocity = (*WarpVelocity).TransformVectorBy( WarpCoords );
	*WarpRotation = (GMath.UnitCoords / *WarpRotation * WarpCoords).OrthoRotation();

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AWarpZoneInfo, 315, execUnWarp );

/*-----------------------------------------------------------------------------
	Intrinsic iterator functions.
-----------------------------------------------------------------------------*/

void AActor::execAllActors( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execAllActors);

	// Get the parms.
	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_GET_NAME_OPT(TagName,NAME_None);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass;
	INT iActor=0;

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		while( iActor<XLevel->Num() && *OutActor==NULL )
		{
			AActor* TestActor = XLevel->Actors(iActor++);
			if(	TestActor && TestActor->IsA(BaseClass) && (TagName==NAME_None || TestActor->Tag==TagName) )
				*OutActor = TestActor;
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 304, execAllActors );

void AActor::execChildActors( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execChildActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass;
	INT iActor=0;

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		while( iActor<XLevel->Num() && *OutActor==NULL )
		{
			AActor* TestActor = XLevel->Actors(iActor++);
			if(	TestActor && TestActor->IsA(BaseClass) && TestActor->IsOwnedBy( this ) )
				*OutActor = TestActor;
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 305, execChildActors );

void AActor::execBasedActors( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execBasedActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass;
	INT iActor=0;

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		while( iActor<XLevel->Num() && *OutActor==NULL )
		{
			AActor* TestActor = XLevel->Actors(iActor++);
			if(	TestActor && TestActor->IsA(BaseClass) && TestActor->Base==this )
				*OutActor = TestActor;
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 306, execBasedActors );

void AActor::execTouchingActors( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execTouchingActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass;
	INT iTouching=0;

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		for( ; iTouching<ARRAY_COUNT(Touching) && *OutActor==NULL; iTouching++ )
		{
			AActor* TestActor = Touching[iTouching];
			if(	TestActor && TestActor->IsA(BaseClass) )
				*OutActor = TestActor;
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 307, execTouchingActors );

void AActor::execTraceActors( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execTraceActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_GET_VECTOR_REF(HitLocation);
	P_GET_VECTOR_REF(HitNormal);
	P_GET_VECTOR(End);
	P_GET_VECTOR_OPT(Start,Location);
	P_GET_VECTOR_OPT(TraceExtent,FVector(0,0,0));
	P_FINISH;

	FMemMark Mark(GMem);
	BaseClass         = BaseClass ? BaseClass : AActor::StaticClass;
	FCheckResult* Hit = XLevel->MultiLineCheck( GMem, End, Start, TraceExtent, 1, Level, 0 );

	PRE_ITERATOR;
		if( Hit )
		{
			*OutActor    = Hit->Actor;
			*HitLocation = Hit->Location;
			*HitNormal   = Hit->Normal;
			Hit          = Hit->GetNext();
		}
		else
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			*OutActor = NULL;
			break;
		}
	POST_ITERATOR;
	Mark.Pop();

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 309, execTraceActors );

void AActor::execRadiusActors( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execRadiusActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_GET_FLOAT(Radius);
	P_GET_VECTOR_OPT(TraceLocation,Location);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass;
	INT iActor=0;

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		while( iActor<XLevel->Num() && *OutActor==NULL )
		{
			AActor* TestActor = XLevel->Actors(iActor++);
			if
			(	TestActor
			&&	TestActor->IsA(BaseClass) 
			&&	(TestActor->Location - TraceLocation).SizeSquared() < Square(Radius + TestActor->CollisionRadius) )
				*OutActor = TestActor;
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 310, execRadiusActors );

void AActor::execVisibleActors( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execVisibleActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_GET_FLOAT_OPT(Radius,0.0);
	P_GET_VECTOR_OPT(TraceLocation,Location);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass;
	INT iActor=0;

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		FCheckResult Hit;
		while( iActor<XLevel->Num() && *OutActor==NULL )
		{
			AActor* TestActor = XLevel->Actors(iActor++);
			if
			(	TestActor
			&& !TestActor->bHidden
			&&	TestActor->IsA(BaseClass)
			&&	(Radius==0.0 || (TestActor->Location-TraceLocation).SizeSquared() < Square(Radius))
			&&	TestActor->GetLevel()->SingleLineCheck( Hit, this, TestActor->Location, TraceLocation, TRACE_VisBlocking ) )
				*OutActor = TestActor;
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 311, execVisibleActors );

void AActor::execVisibleCollidingActors( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AActor::execVisibleCollidingActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_GET_FLOAT_OPT(Radius,0.0);
	P_GET_VECTOR_OPT(TraceLocation,Location);
	P_FINISH;

	Radius = Radius ? Radius : 1000;
	BaseClass = BaseClass ? BaseClass : AActor::StaticClass;
	INT iActor=0;
	FMemMark Mark(GMem);
	FCheckResult* Link=GetLevel()->Hash->ActorRadiusCheck( GMem, TraceLocation, Radius, 0 );
	
	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		FCheckResult Hit;
		if ( Link )
		{
			while
			(	Link
			&&	(!Link->Actor
			||	!Link->Actor->IsA(BaseClass) 
			||	!GetLevel()->SingleLineCheck( Hit, this, Link->Actor->Location, TraceLocation, TRACE_VisBlocking )) )
				Link=Link->GetNext();

			if ( Link )
			{
				*OutActor = Link->Actor;
				Link=Link->GetNext();
			}
		}
		if ( *OutActor == NULL ) 
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	Mark.Pop();
	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AActor, 312, execVisibleCollidingActors );

void AZoneInfo::execZoneActors( FFrame& Stack, BYTE*& Result )
{
	guardSlow(AZoneInfo::execZoneActors);

	P_GET_OBJECT(UClass,BaseClass);
	P_GET_ACTOR_REF(OutActor);
	P_FINISH;

	BaseClass = BaseClass ? BaseClass : AActor::StaticClass;
	INT iActor=0;

	PRE_ITERATOR;
		// Fetch next actor in the iteration.
		*OutActor = NULL;
		while( iActor<XLevel->Num() && *OutActor==NULL )
		{
			AActor* TestActor = XLevel->Actors(iActor++);
			if
			(	TestActor
			&&	TestActor->IsA(BaseClass)
			&&	TestActor->IsInZone(this) )
				*OutActor = TestActor;
		}
		if( *OutActor == NULL )
		{
			Stack.Code = &Stack.Node->Script(wEndOffset + 1);
			break;
		}
	POST_ITERATOR;

	unguardexecSlow;
}
AUTOREGISTER_INTRINSIC( AZoneInfo, 308, execZoneActors );

/*-----------------------------------------------------------------------------
	Script processing function.
-----------------------------------------------------------------------------*/

//
// Execute the state code of the actor.
//
void AActor::ProcessState( FLOAT DeltaSeconds )
{
	guard(AActor::ProcessState);
	FMainFrame* MainFrame = GetMainFrame();
	if
	(	MainFrame
	&&	MainFrame->Code
	&&	MainFrame->StateNode
	&&	(Role>=ROLE_Authority || (MainFrame->StateNode->StateFlags & STATE_Simulated))
	&&	!IsPendingKill() )
	{
		if( ++GScriptEntryTag==1 )
			uclock(GScriptCycles);

		// Create a work area for UnrealScript.
		BYTE Buffer[MAX_CONST_SIZE], *Addr;
		*(FLOAT*)Buffer = DeltaSeconds;

		// If a latent action is in progress, update it.
		if( MainFrame->LatentAction )
			(this->*GIntrinsics[MainFrame->LatentAction])( *MainFrame, Addr=Buffer );

		// Execute code.
		INT NumStates=0;
		MainFrame = GetMainFrame();
		while( !bDeleteMe && MainFrame && MainFrame->Code && MainFrame->StateNode && !MainFrame->LatentAction )
		{
			UState* OldStateNode = MainFrame->StateNode;
			MainFrame->Step( this, Addr=Buffer );
			MainFrame = GetMainFrame();
			if( !MainFrame || !MainFrame->StateNode )
				break;
			if( MainFrame->StateNode != OldStateNode )
			{
				if( ++NumStates > 4 )
				{
					//GetMainFrame().ScriptWarn( 0, "Pause going from %s to %s", xx, yy );
					break;
				}
			}
		}
		if( --GScriptEntryTag==0 )
			uunclock(GScriptCycles);
	}
#if PLATFORM_64BIT
	else
	{
		if( !MainFrame )
			UE1Android64ProcessStateOriginLogV76( "skip-no-mainframe-v76", this, DeltaSeconds, MainFrame );
		else if( UE1Android64StateDiagBadPtr(MainFrame) )
			UE1Android64ProcessStateOriginLogV76( "skip-bad-mainframe-v76", this, DeltaSeconds, MainFrame );
		else if( !MainFrame->StateNode )
			UE1Android64ProcessStateOriginLogV76( "skip-no-statenode-v76", this, DeltaSeconds, MainFrame );
		else if( !MainFrame->Code )
		{
			// UNREAL_ANDROID64_PLAYER_MOVE_BLOCK_PROBE_V83
			// A null Code pointer is normal for many UE1 event-driven states.
			// The old v76 logger flooded logcat with harmless skip-no-code rows and
			// hid the real movement/collision boundary we now need to inspect.
		}

	}
#endif
	unguardobj;
}

//
// Return whether a function should be executed remotely.
//
UBOOL AActor::ProcessRemoteFunction( UFunction* Function, void* Parms, FFrame* Stack )
{
	// Quick reject.
	UBOOL Absorb = Role<=ROLE_SimulatedProxy && !(Function->FunctionFlags & FUNC_Simulated);
	guardSlow(AActor::ProcessRemoteFunction);
	if( !(Function->FunctionFlags & FUNC_Net) || Level->NetMode==NM_Standalone )
		return Absorb;
	unguardSlow;

	// Check if the actor can potentially call remote functions.
	guard(AActor::ProcessRemoteFunction);
	AActor* Top = GetTopOwner();
	if
	(	Role==ROLE_Authority
	&&	(	!Top->IsA(APlayerPawn::StaticClass)
		||	!((APlayerPawn*)Top)->Player
		||	!((APlayerPawn*)Top)->Player->IsA(UNetConnection::StaticClass) ) )
		return Absorb;

	// Chase down the original version of the function.
	while( Function->GetSuperFunction() )
		Function = Function->GetSuperFunction();

	// Evaluate replication condition.
	if( Function->RepOffset != MAXWORD )
	{
		// See if UnrealScript replication condition is met.
		FFrame EvalStack( this, Function->GetOwnerClass(), Function->RepOffset, NULL );
		BYTE Buffer[MAX_CONST_SIZE], *Val=Buffer;
		EvalStack.Step( this, Val );

		// Replicate if condition is met.
		if( *(DWORD*)Val == 0 )
			return Absorb;
	}

	// Get the player.
	UNetConnection* Connection = NULL;
	FActorChannel*  Ch         = NULL;
	if( (Level->NetMode==NM_DedicatedServer) || (Level->NetMode==NM_ListenServer) )
	{
		Connection = (UNetConnection*)((APlayerPawn*)GetTopOwner())->Player;
		check(Connection);
		Ch = Connection->GetActorChannel( this );
		if( !Ch )
		{
			// It's time to create this channel so we can tell the other actor about it.
			//debugf( "RPC %s for uncreated actor %s", Function->GetName(), GetFullName() );
			Ch = (FActorChannel *)Connection->CreateChannel( CHTYPE_Actor, 1 );
			check(Ch);
			Ch->Actor = this;
		}
	}
	else if( Level->NetMode==NM_Client )
	{
		Connection = XLevel->NetDriver->ServerConnection;
		check(Connection);
		Ch = Connection->GetActorChannel( this );
		if( !Ch )
			return 1;
	}

	// Make sure initial replication has taken place.
	XLevel->NumRPC++;
	Ch->ReplicateActor( 0 );

	// Form the RPC preamble.
	FOutBunch Bunch( Ch );
	FName NodeName(Function->GetFName());
	Bunch << NodeName;

	// Form the RPC parameters.
	BYTE ParmMask=0, ParmBit=1;
	if( Stack )
		appMemset( Parms, 0, Function->ParmsSize );
	INT Count=0;
	TFieldIterator<UProperty> It(Function);
	for( ; It && (It->PropertyFlags & (CPF_Parm|CPF_ReturnParm))==CPF_Parm; ++It,ParmBit=ParmBit<<1 )
	{
		if( Stack )
		{
			BYTE* Dest=(BYTE*)Parms + It->Offset, *Addr=Dest;
			Stack->Step( Stack->Object, Addr );
			if( Addr != Dest )
				appMemcpy( Dest, Addr, It->GetSize() );
		}
		if( !It->Matches(Parms,NULL,0) )
			ParmMask |= ParmBit;
	}
	Bunch << ParmMask;
	ParmBit=1;
	for( It=TFieldIterator<UProperty>(Function); It && (It->PropertyFlags & (CPF_Parm|CPF_ReturnParm))==CPF_Parm; ++It,ParmBit=ParmBit<<1 )
		if( (ParmMask & ParmBit) || !ParmBit )
			Bunch.Overflowed |= Bunch.SendProperty( *It, 0, (BYTE*)Parms, NULL, 0 );

	// Send the bunch.
	if( !Bunch.Overflowed )
	{
		Ch->SendBunch( Bunch, !(Function->FunctionFlags & FUNC_NetReliable) );
	}
	else debugf( NAME_DevNet, "RPC bunch overflowed" );

	return 1;
	unguardf(( "(%s)", Function->GetName() ));
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
