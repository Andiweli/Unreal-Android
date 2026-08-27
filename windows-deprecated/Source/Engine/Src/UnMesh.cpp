/*=============================================================================
	UnMesh.cpp: Unreal mesh animation functions
	Copyright 1997 Epic MegaGames, Inc. This software is a trade secret.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "EnginePrivate.h"
#include "UnRender.h"
#include "Amd3d.h"

/*-----------------------------------------------------------------------------
	UMesh object implementation.
-----------------------------------------------------------------------------*/

UMesh::UMesh()
{
	guard(UMesh::UMesh);

	// Scaling.
	Scale			= FVector(1,1,1);
	Origin			= FVector(0,0,0);
	RotOrigin		= FRotator(0,0,0);

	// Flags.
	AndFlags		= ~(DWORD)0;
	OrFlags			= 0;

	unguardobj;
}
void UMesh::Serialize( FArchive& Ar )
{
	guard(UMesh::Serialize);

	// Serialize parent.
	UPrimitive::Serialize(Ar);

	// Serialize this.
	Ar << Verts << Tris << AnimSeqs;
	Ar << Connects << BoundingBox << BoundingSphere << VertLinks << Textures;
	Ar << BoundingBoxes << BoundingSpheres;
	Ar << FrameVerts << AnimFrames;
	Ar << AndFlags << OrFlags;
	Ar << Scale << Origin << RotOrigin;
	Ar << CurPoly << CurVertex;

	unguard;
}
IMPLEMENT_CLASS(UMesh);

/*-----------------------------------------------------------------------------
	UMesh collision interface.
-----------------------------------------------------------------------------*/

//
// Get the rendering bounding box for this primitive, as owned by Owner.
//
FBox UMesh::GetRenderBoundingBox( const AActor* Owner, UBOOL Exact ) const
{
	guard(UMesh::GetRenderBoundingBox);
	FBox Bound;

	// Get frame indices.
	INT iFrame1 = 0, iFrame2 = 0;
	const FMeshAnimSeq *Seq = GetAnimSeq( Owner->AnimSequence );
	if( Seq && Owner->AnimFrame>=0.0 )
	{
		// Animating, so use bound enclosing two frames' bounds.
		INT iFrame = appFloor((Owner->AnimFrame+1.0) * Seq->NumFrames);
		iFrame1    = Seq->StartFrame + ((iFrame + 0) % Seq->NumFrames);
		iFrame2    = Seq->StartFrame + ((iFrame + 1) % Seq->NumFrames);
		Bound      = BoundingBoxes(iFrame1) + BoundingBoxes(iFrame2);
	}
	else
	{
		// Interpolating, so be pessimistic and use entire-mesh bound.
		Bound = BoundingBox;
	}

	// Transform Bound by owner's scale and origin.
	FLOAT DrawScale = Owner->bParticles ? 1.5 : Owner->DrawScale;
	Bound = FBox( Scale*DrawScale*(Bound.Min - Origin), Scale*DrawScale*(Bound.Max - Origin) ).ExpandBy(1.0);
	FCoords Coords = GMath.UnitCoords / RotOrigin / Owner->Rotation;
	Coords.Origin  = Owner->Location + Owner->PrePivot;
	return Bound.TransformBy( Coords.Transpose() );
	unguardobj;
}

//
// Get the rendering bounding sphere for this primitive, as owned by Owner.
//
FSphere UMesh::GetRenderBoundingSphere( const AActor* Owner, UBOOL Exact ) const
{
	guard(UMesh::GetRenderBoundingSphere);
	return FSphere(0);
	unguardobj;
}

//
// Primitive box line check.
//
UBOOL UMesh::LineCheck
(
	FCheckResult	&Result,
	AActor			*Owner,
	FVector			End,
	FVector			Start,
	FVector			Extent,
	DWORD           ExtraNodeFlags
)
{
	guard(UMesh::LineCheck);
	if( Extent != FVector(0,0,0) )
	{
		// Use cylinder.
		return UPrimitive::LineCheck( Result, Owner, End, Start, Extent, ExtraNodeFlags );
	}
	else
	{
		// Could use exact mesh collision.
		// 1. Reject with local bound.
		// 2. x-wise intersection test with all polygons.
		return UPrimitive::LineCheck( Result, Owner, End, Start, FVector(0,0,0), ExtraNodeFlags );
	}
	unguardobj;
}

/*-----------------------------------------------------------------------------
	UMesh animation interface.
-----------------------------------------------------------------------------*/

//
// Get the transformed point set corresponding to the animation frame 
// of this primitive owned by Owner. Returns the total outcode of the points.
//
void UMesh::GetFrame
(
	FVector*	ResultVerts,
	INT			Size,
	FCoords		Coords,
	AActor*		Owner
)
{
	guard(UMesh::GetFrame);

	// Create or get cache memory.
	FCacheItem* Item;
	UBOOL WasCached = 1;
	QWORD CacheID   = MakeCacheID( CID_TweenAnim, Owner, NULL );
	BYTE* Mem       = GCache.Get( CacheID, Item );
	if( Mem==NULL || *(UMesh**)Mem!=this )
	{
		if( Mem != NULL )
		{
			// Actor's mesh changed.
			Item->Unlock();
			GCache.Flush( CacheID );
		}
		Mem = GCache.Create( CacheID, Item, sizeof(UMesh*) + sizeof(FLOAT) + sizeof(FName) + FrameVerts * sizeof(FVector) );
		WasCached = 0;
	}
	UMesh*& CachedMesh  = *(UMesh**)Mem; Mem += sizeof(UMesh*);
	FLOAT&  CachedFrame = *(FLOAT *)Mem; Mem += sizeof(FLOAT );
	FName&  CachedSeq   = *(FName *)Mem; Mem += sizeof(FName);
	if( !WasCached )
	{
		CachedMesh  = this;
		CachedSeq   = NAME_None;
		CachedFrame = 0.0;
	}

	// Get stuff.
	FLOAT    DrawScale      = Owner->bParticles ? 1.0 : Owner->DrawScale;
	FVector* CachedVerts    = (FVector*)Mem;
	Coords                  = Coords * (Owner->Location + Owner->PrePivot) * Owner->Rotation * RotOrigin * FScale(Scale * DrawScale,0.0,SHEER_None);
	const FMeshAnimSeq* Seq = GetAnimSeq( Owner->AnimSequence );

	// Transform all points into screenspace.
	if( Owner->AnimFrame>=0.0 || !WasCached )
	{
		// Compute interpolation numbers.
		FLOAT Alpha=0.0;
		INT iFrameOffset1=0, iFrameOffset2=0;
		if( Seq )
		{
			FLOAT Frame   = ::Max(Owner->AnimFrame,0.f) * Seq->NumFrames;
			INT iFrame    = appFloor(Frame);
			Alpha         = Frame - iFrame;
			iFrameOffset1 = (Seq->StartFrame + ((iFrame + 0) % Seq->NumFrames)) * FrameVerts;
			iFrameOffset2 = (Seq->StartFrame + ((iFrame + 1) % Seq->NumFrames)) * FrameVerts;
		}

		// Interpolate two frames.
		FMeshVert* MeshVertex1 = &Verts( iFrameOffset1 );
		FMeshVert* MeshVertex2 = &Verts( iFrameOffset2 );
		for( INT i=0; i<FrameVerts; i++ )
		{
			FVector V1( MeshVertex1[i].X, MeshVertex1[i].Y, MeshVertex1[i].Z );
			FVector V2( MeshVertex2[i].X, MeshVertex2[i].Y, MeshVertex2[i].Z );
			CachedVerts[i] = V1 + (V2-V1)*Alpha;
			*ResultVerts = (CachedVerts[i] - Origin).TransformPointBy(Coords);
			*(BYTE**)&ResultVerts += Size;
		}
	}
	else
	{
		// Compute tweening numbers.
		FLOAT StartFrame = Seq ? (-1.0 / Seq->NumFrames) : 0.0;
		INT iFrameOffset = Seq ? Seq->StartFrame * FrameVerts : 0;
		FLOAT Alpha = 1.0 - Owner->AnimFrame / CachedFrame;
		if( CachedSeq!=Owner->AnimSequence || Alpha<0.0 || Alpha>1.0)
		{
			CachedSeq   = Owner->AnimSequence;
			CachedFrame = StartFrame;
			Alpha       = 0.0;
		}

		// Tween all points.
		FMeshVert* MeshVertex = &Verts( iFrameOffset );
		for( INT i=0; i<FrameVerts; i++ )
		{
			FVector V2( MeshVertex[i].X, MeshVertex[i].Y, MeshVertex[i].Z );
			CachedVerts[i] += (V2 - CachedVerts[i]) * Alpha;
			*ResultVerts = (CachedVerts[i] - Origin).TransformPointBy(Coords);
			*(BYTE**)&ResultVerts += Size;
		}

		// Update cached frame.
		CachedFrame = Owner->AnimFrame;
	}
	Item->Unlock();
	unguardobj;
}

/*-----------------------------------------------------------------------------
	UMesh constructor.
-----------------------------------------------------------------------------*/

//
// UMesh constructor.
//
UMesh::UMesh( INT NumPolys, INT NumVerts, INT NumFrames )
{
	guard(UMesh::UMesh);

	// Set counts.
	FrameVerts	= NumVerts;
	AnimFrames	= NumFrames;

	// Allocate all stuff.
	Tris			.Add(NumPolys);
	Verts			.Add(NumVerts * NumFrames);
	Connects		.Add(NumVerts);
	BoundingBoxes	.Add(NumFrames);
	BoundingSpheres	.Add(NumFrames);

	// Init textures.
	for( int i=0; i<Textures.Num(); i++ )
		Textures(i) = NULL;

	unguardobj;
}

/*-----------------------------------------------------------------------------
	The end.
-----------------------------------------------------------------------------*/
