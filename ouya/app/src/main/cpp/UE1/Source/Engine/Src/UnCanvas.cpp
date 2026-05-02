/*=============================================================================
	UnCanvas.cpp: Unreal canvas rendering.
	Copyright 1997 Epic MegaGames, Inc. This software is a trade secret.

Revision history:
	* Created by Tim Sweeney
=============================================================================*/

#include "EnginePrivate.h"
#include "UnRender.h"
#if defined(ANDROID_LEGACY_API16) && defined(PLATFORM_ANDROID)
extern "C" __attribute__((used,visibility("default"))) const char UE1_OUYA_AFTER_INCLUDE_BINARY_ANCHOR[] =
    "UE1_OUYA_AFTER_INCLUDE_BINARY_ANCHOR | UNREAL_ANDROID_OUYA_ACTIVE | UnCanvas.cpp";
#endif

#if defined(ANDROID_LEGACY_API16) && defined(PLATFORM_ANDROID)
#if defined(__ANDROID__)
#include <android/log.h>
#endif

// OUYA_CANVAS_HALFSCALE_HELPER_DISABLED
static UBOOL GUE1OuyaCanvasUiScaleLogged = 0;

static inline void UE1OuyaCanvasScaleRect(FSceneNode* Frame, FLOAT& X, FLOAT& Y, FLOAT& XL, FLOAT& YL)
{
    if( !Frame )
        return;

    const FLOAT Scale = 1.0f;
    const FLOAT ScreenW = Frame->FX;
    const FLOAT ScreenH = Frame->FY;
    const FLOAT OffsetX = ScreenW * (1.0f - Scale) * 0.5f;
    const FLOAT OffsetY = ScreenH * (1.0f - Scale) * 0.5f;

    X  = OffsetX + X * Scale;
    Y  = OffsetY + Y * Scale;
    XL = XL * Scale;
    YL = YL * Scale;

#if defined(__ANDROID__)
    if( !GUE1OuyaCanvasUiScaleLogged )
    {
GUE1OuyaCanvasUiScaleLogged = 1;
    }
#endif
}
#endif

#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_CANVAS_UI_SCALE_INCLUDE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif

#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_CANVAS_UI_SCALE_HELPER
static FLOAT AndroidCanvasScale()
{
#if defined(ANDROID_LEGACY_API16) && defined(PLATFORM_ANDROID)
    // OUYA_ANDROID_CANVAS_SCALE_FORCED_1_0
    static UBOOL LoggedOuyaCanvasScale = 0;
    if( !LoggedOuyaCanvasScale )
    {
        debugf( NAME_Log, "OUYA AndroidCanvasScale forced 1.0" );
        LoggedOuyaCanvasScale = 1;
    }
    return 1.0f;
#else
    static FLOAT Scale = -1.0f;
    if( Scale < 0.0f )
    {
        Scale = 2.0f;

        const char* EnvScale = getenv( "UE1_ANDROID_UI_SCALE" );
        if( EnvScale && EnvScale[0] )
        {
            Scale = (FLOAT)atof( EnvScale );
        }

        if( Scale < 1.0f )
            Scale = 1.0f;
        if( Scale > 4.0f )
            Scale = 4.0f;
    }
    return Scale;
#endif
}
#endif


#ifdef PLATFORM_ANDROID // UE1_ANDROID_AUDIOVIDEO_MENU_DRAW_FIX
static UBOOL AndroidCanvasNearlyEqual( FLOAT A, FLOAT B, FLOAT Tolerance )
{
	return ( A >= B - Tolerance ) && ( A <= B + Tolerance );
}

static UBOOL AndroidCanvasActiveMenuIs( UCanvas* Canvas, const char* ClassName )
{
	if( !Canvas || !ClassName || !Canvas->Viewport || !Canvas->Viewport->Actor || !Canvas->Viewport->Actor->myHUD || !Canvas->Viewport->Actor->myHUD->MainMenu )
		return 0;
	return Canvas->Viewport->Actor->myHUD->MainMenu->GetClass()
		&& !appStricmp( Canvas->Viewport->Actor->myHUD->MainMenu->GetClass()->GetName(), ClassName );
}
#endif
/*-----------------------------------------------------------------------------
	UCanvas scaled sprites.
-----------------------------------------------------------------------------*/

void UCanvas::DrawTile
(
	UTexture*		Texture,
	FLOAT			X,
	FLOAT			Y,
	FLOAT			XL,
	FLOAT			YL,
	FLOAT			U,
	FLOAT			V,
	FLOAT			UL,
	FLOAT			VL,
	FSpanBuffer*	SpanBuffer,
	FLOAT			Z,
	FPlane			Color,
	FPlane			Fog,
	DWORD			PolyFlags
)
{
	guard(UCanvas::DrawTile);
	check(Texture);

	// Compute clipping region.
	FLOAT ClipY0 = /*SpanBuffer ? SpanBuffer->StartY :*/ 0;
	FLOAT ClipY1 = /*SpanBuffer ? SpanBuffer->EndY   :*/ Frame->FY;

	// Reject.
	if( XL<=0.f || YL<=0.f || X+XL<=0.f || Y+YL<=ClipY0 || X>=Frame->FX || Y>=ClipY1 )
		return;

	// Clip.
	if( X<0.f )
		{FLOAT C=X*UL/XL; U-=C; UL+=C; XL+=X; X=0.f;}
	if( Y<0.f )
		{FLOAT C=Y*VL/YL; V-=C; VL+=C; YL+=Y; Y=0.f;}
	if( XL>Frame->FX-X )
		{UL+=(Frame->FX-X-XL)*UL/XL; XL=Frame->FX-X;}
	if( YL>Frame->FY-Y )
		{VL+=(Frame->FY-Y-YL)*VL/YL; YL=Frame->FY-Y;}

	// Draw it.
	FTextureInfo Info;
	Texture->GetInfo( Info, Viewport->CurrentTime );
	U *= Info.UScale; UL *= Info.UScale;
	V *= Info.VScale; VL *= Info.VScale;
	#if defined(ANDROID_LEGACY_API16) && defined(PLATFORM_ANDROID)
    // OUYA_CANVAS_HALFSCALE_DRAWTILE_DISABLED
    FLOAT DrawX  = X;
    FLOAT DrawY  = Y;
    FLOAT DrawXL = XL;
    FLOAT DrawYL = YL;
    UE1OuyaCanvasScaleRect( Frame, DrawX, DrawY, DrawXL, DrawYL );
    Viewport->RenDev->DrawTile( Frame, Info, DrawX, DrawY, DrawXL, DrawYL, U, V, UL, VL, SpanBuffer, Z, Color, Fog, PolyFlags );
#else
    Viewport->RenDev->DrawTile( Frame, Info, X, Y, XL, YL, U, V, UL, VL, SpanBuffer, Z, Color, Fog, PolyFlags );
#endif

	unguard;
}

void UCanvas::DrawPattern
(
	UTexture*		Texture,
	FLOAT			X,
	FLOAT			Y,
	FLOAT			XL,
	FLOAT			YL,
	FLOAT			Scale,
	FLOAT			OrgX,
	FLOAT			OrgY,
	FSpanBuffer*	SpanBuffer,
	FLOAT			Z,
	FPlane			Color,
	FPlane			Fog,
	DWORD			PolyFlags
)
{
	guard(UCanvas::DrawPattern);
	DrawTile( Texture, X, Y, XL, YL, (X-OrgX)*Scale + Texture->USize, (Y-OrgY)*Scale + Texture->VSize, XL*Scale, YL*Scale, SpanBuffer, Z, Color, Fog, PolyFlags );
	unguard;
}

//
// Draw a scaled sprite.  Takes care of clipping.
// XSize and YSize are in pixels.
//
void UCanvas::DrawIcon
(
	UTexture*			Texture,
	FLOAT				ScreenX, 
	FLOAT				ScreenY, 
	FLOAT				XSize, 
	FLOAT				YSize, 
	FSpanBuffer*		SpanBuffer,
	FLOAT				Z,
	FPlane				Color,
	FPlane				Fog,
	DWORD				PolyFlags
)
{
	guard(UCanvas::DrawIcon);
	DrawTile( Texture, ScreenX, ScreenY, XSize, YSize, 0, 0, Texture->USize, Texture->VSize, SpanBuffer, Z, Color, Fog, PolyFlags );
	unguard;
}

/*-----------------------------------------------------------------------------
	UCanvas text drawing.
-----------------------------------------------------------------------------*/

//
// Calculate the length of a string built from a font, starting at a specified
// position and counting up to the specified number of characters (-1 = infinite).
//
void UCanvas::StrLen
(
	UFont*		Font,
	INT&		XL, 
	INT&		YL, 
	const char*	Text,
	INT			iStart,
	INT			NumChars
)
{
	guard(UCanvas::StrLen);

	XL = YL = 0;
	for( const char* c=Text+iStart; *c && NumChars>0; c++,NumChars-- )
	{
		if( *c < Font->Characters.Num() )
		{
			XL += Font->Characters(*c).USize + SpaceX;
			YL = ::Max(YL,Font->Characters(*c).VSize);
		}
	}
	YL += SpaceY;

	unguard;
}

//
// Calculate the size of a string built from a font, word wrapped
// to a specified region.
//
void UCanvas::WrappedStrLen
(
	UFont*		Font,
	INT&		XL, 
	INT&		YL, 
	INT			MaxWidth, 
	const char*	Text
)
{
	guard(UCanvas::WrappedStrLen);
	check(Font);

	int iLine=0;
	int TestXL,TestYL;
	XL = YL = 0;

	// Process each output line.
	while( Text[iLine] )
	{
		// Process each word until the current line overflows.
		int iWord, iTestWord=iLine;
		do
		{
			iWord = iTestWord;
			if( !Text[iTestWord] )
				break;
			while( Text[iTestWord] && Text[iTestWord]!=' ' )
				iTestWord++;
			while( Text[iTestWord]==' ' )
				iTestWord++;
			StrLen( Font, TestXL, TestYL, Text, iLine, iTestWord-iLine );
		} while( TestXL <= MaxWidth );
		
		if( iWord == iLine )
		{
			// The text didn't fit word-wrapped onto this line, so chop it.
			int iTestWord = iLine;
			do
			{
				iWord = iTestWord;
				if( !Text[iTestWord] )
					break;
				iTestWord++;
				StrLen( Font, TestXL, TestYL, Text, iLine, iTestWord-iLine );
			} while( TestXL <= MaxWidth );
			
			// Word wrap failed because window is too small to hold a single character.
			if( iWord == iLine )
				return;
		}

		// Sucessfully split this line.
		StrLen( Font, TestXL, TestYL, Text, iLine, iWord-iLine );
		check(TestXL<=MaxWidth);
		YL += TestYL;
		if( TestXL > XL )
			XL = TestXL;

		// Go to the next line.
		while( Text[iWord]==' ' )
			iWord++;
		
		iLine = iWord;
	}
	unguard;
}

//
// Font printing.
//
static inline void DrawChar( UCanvas* Canvas, FTextureInfo& Info, INT X, INT Y, INT XL, INT YL, INT U, INT V, INT UL, INT VL, FPlane Color )
{
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_CANVAS_UI_SCALE_DRAW_CHAR
	// Text needs screen position and screen size scaled, but the font texture
	// UV range must remain original. Rev24 scaled X/Y but the renderer still
	// used UL/VL as the screen size, causing tiny letters with huge spacing.
	const FLOAT AndroidUIScale = AndroidCanvasScale();
	if( AndroidUIScale != 1.0f )
	{
		X  = (INT)( X  * AndroidUIScale );
		Y  = (INT)( Y  * AndroidUIScale );
		XL = (INT)( XL * AndroidUIScale );
		YL = (INT)( YL * AndroidUIScale );
	}
#endif
	// Reject.
	FSceneNode* Frame=Canvas->Frame;
	if( X+XL<=0.0 || Y+YL<=0 || X>=Frame->FX || Y>=Frame->FY )
		return;

	// Clip.
	if( X<0.f )
		{FLOAT C=X*UL/XL; U-=C; UL+=C; XL+=X; X=0.f;}
	if( Y<0.f )
		{FLOAT C=Y*VL/YL; V-=C; VL+=C; YL+=Y; Y=0.f;}
	if( XL>Frame->FX-X )
		{UL+=(Frame->FX-X-XL)*UL/XL; XL=Frame->FX-X;}
	if( YL>Frame->FY-Y )
		{VL+=(Frame->FY-Y-YL)*VL/YL; YL=Frame->FY-Y;}

	// Draw.
	#if defined(ANDROID_LEGACY_API16) && defined(PLATFORM_ANDROID)
    // OUYA_CANVAS_HALFSCALE_DRAWCHAR_DISABLED
    FLOAT DrawX  = (FLOAT)X;
    FLOAT DrawY  = (FLOAT)Y;
    FLOAT DrawXL = (FLOAT)XL;
    FLOAT DrawYL = (FLOAT)YL;
    UE1OuyaCanvasScaleRect( Frame, DrawX, DrawY, DrawXL, DrawYL );
    Frame->Viewport->RenDev->DrawTile( Frame, Info, DrawX, DrawY, DrawXL, DrawYL, U, V, UL, VL, NULL, Canvas->Z, Color, FPlane(0,0,0,0), PF_NoSmooth | PF_Masked | PF_RenderHint );
#else
    Frame->Viewport->RenDev->DrawTile( Frame, Info, X, Y, XL, YL, U, V, UL, VL, NULL, Canvas->Z, Color, FPlane(0,0,0,0), PF_NoSmooth | PF_Masked | PF_RenderHint );
#endif // UNREAL_ANDROID_CANVAS_UI_SCALE_DRAW_CHAR_SIZE_FIX
}
void VARARGS UCanvas::Printf
(
	UFont*		Font,
	INT			X,
	INT			Y,
	const char* Fmt,
	...
)
{
	char Text[4096];
	GET_VARARGS( Text, Fmt );

	guard(UCanvas::Printf);
	check(Font);

	FTextureInfo Info;
	Font->GetInfo( Info, Viewport->CurrentTime );
	FPlane DrawColor = Color.Plane();
	for( BYTE* c=(BYTE*)&Text[0]; *c; c++ )
	{
		//const char* C=LocalizeGeneral("Copyright","Core");
		//while( *C ) 
		//	debugf("%i",*C);
		FFontCharacter& Char = Font->Characters( *c );
		DrawChar( this, Info, OrgX+X, OrgY+Y, Char.USize, Char.VSize, Char.StartU, Char.StartV, Char.USize, Char.VSize, DrawColor );
		X += Char.USize + SpaceX;
	}
	unguard;
}

//
// Wrapped printf.
//
void VARARGS UCanvas::WrappedPrintf( UFont* Font, UBOOL Center, const char* Fmt, ... )
{
	char Text[4096];
	GET_VARARGS(Text,Fmt);

	guard(UCanvas::WrappedPrintf);
	check(Font);

	int iLine=0;
	int TestXL, TestYL;

	// Process each output line.
	while( Text[iLine] )
	{
		// Process each word until the current line overflows.
		int iWord, iTestWord=iLine;
		do
		{
			iWord = iTestWord;
			if( !Text[iTestWord] )
				break;
			while( Text[iTestWord] && Text[iTestWord]!=' ' ) 
				iTestWord++;
			while( Text[iTestWord]==' ' )
				iTestWord++;
			StrLen( Font, TestXL, TestYL, Text, iLine, iTestWord-iLine );
		} while( TestXL <= ClipX );

		// If the text didn't fit word-wrapped onto this line, chop it.
		if( iWord==iLine )
		{
			int iTestWord = iLine;
			do
			{
				iWord = iTestWord;
				if( !Text[iTestWord] )
					break;
				iTestWord++;
				StrLen( Font, TestXL, TestYL, Text, iLine, iTestWord-iLine );
			} while( TestXL <= ClipX );
			if( iWord==iLine ) 
			{
				// Word wrap failed.
				return;
			}
		}

		// Sucessfully split this line, now draw it.
		char Temp[256];
		appStrcpy( Temp, &Text[iLine] );
		Temp[iWord-iLine]=0;
		StrLen( Font, TestXL, TestYL, Text, iLine, iWord-iLine );
		check(TestXL<=ClipX);
		Printf( Font, Center ? CurX+(ClipX-TestXL)/2 : CurX, CurY, "%s", Temp );
		CurY += TestYL;

		// Go to the next line.
		while( Text[iWord]==' ' )
			iWord++;
		iLine = iWord;
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	UCanvas object functions.
-----------------------------------------------------------------------------*/

void UCanvas::Init( UViewport* InViewport )
{
	guard(UCanvas::UCanvas);
	Viewport = InViewport;
	unguard;
}
void UCanvas::Update( FSceneNode* InFrame )
{
	guard(UCanvas::Update);

	// Call UnrealScript to reset.
	ProcessEvent( FindFunctionChecked("Reset"), NULL );

	// Copy size parameters from viewport.
	Frame = InFrame;
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_CANVAS_UI_SCALE_LOGICAL_SIZE
	const FLOAT AndroidUIScale = AndroidCanvasScale();
	if( AndroidUIScale != 1.0f )
	{
		X = ClipX = Frame->X / AndroidUIScale;
		Y = ClipY = Frame->Y / AndroidUIScale;
		static int LoggedAndroidUIScale = 0;
		if( !LoggedAndroidUIScale )
		{
			debugf( NAME_Log, "Android UI scale: %f logical canvas %ix%i from frame %ix%i", AndroidUIScale, (INT)ClipX, (INT)ClipY, Frame->X, Frame->Y );
			LoggedAndroidUIScale = 1;
		}
	}
	else
#endif
	{
		X = ClipX = Frame->X;
		Y = ClipY = Frame->Y;
	}

	unguard;
}

/*-----------------------------------------------------------------------------
	UCanvas intrinsics.
-----------------------------------------------------------------------------*/

void UCanvas::execDrawText( FFrame& Stack, BYTE*& Result )
{
	guard(UCanvas::execDrawText);
	P_GET_STRING(Text);
	P_GET_UBOOL_OPT(CR,1);
	P_FINISH;
	if( !Font )
	{
		Stack.ScriptWarn( 0, "DrawText: No font" );
		return;
	}

	//debugf( "DrawText: '%s' %i", Text, CR );
#ifdef PLATFORM_ANDROID // UE1_ANDROID_AUDIOVIDEO_MENU_DRAW_FIX
	if( AndroidCanvasActiveMenuIs( this, "UnrealVideoMenu" ) )
	{
	// Runtime cleanup for the stock UnrealVideoMenu without rebuilding Unreal.u:
	// - shift the AUDIO/VIDEO value column slightly right for Resolution/Texture Detail;
	// - suppress the orphaned High/Low that UnrealVideoMenu.DrawMenu still draws for
	//   hidden Sound Quality after MenuLength was trimmed to 6.
	FLOAT AndroidMenuSpacing = 0.04f * ClipY;
	if( AndroidMenuSpacing < 16.0f ) AndroidMenuSpacing = 16.0f;
	if( AndroidMenuSpacing > 32.0f ) AndroidMenuSpacing = 32.0f;

	FLOAT AndroidMenuStartX = 0.5f * ClipX - 120.0f;
	if( AndroidMenuStartX < 40.0f ) AndroidMenuStartX = 40.0f;

	FLOAT AndroidMenuStartY = 0.5f * ( ClipY - 6.0f * AndroidMenuSpacing - 128.0f );
	if( AndroidMenuStartY < 36.0f ) AndroidMenuStartY = 36.0f;

	const FLOAT AndroidValueX = AndroidMenuStartX + 152.0f;
	INT AndroidSpaceX=0, AndroidSpaceY=0;
	StrLen( Font, AndroidSpaceX, AndroidSpaceY, " ", 0, 1 );
	if( AndroidSpaceX < 1 )
		AndroidSpaceX = 1;

	const UBOOL AndroidIsHighLow = ( !appStricmp( Text, "High" ) || !appStricmp( Text, "Low" ) );
	if( AndroidIsHighLow )
	{
		const FLOAT AndroidRowTolerance = Max( 4.0f, AndroidMenuSpacing * 0.35f );
		const FLOAT AndroidXLeft  = AndroidValueX - 32.0f;
		const FLOAT AndroidXRight = AndroidValueX + 128.0f;
		if( CurX >= AndroidXLeft && CurX <= AndroidXRight
		&&  AndroidCanvasNearlyEqual( CurY, AndroidMenuStartY + AndroidMenuSpacing * 6.0f, AndroidRowTolerance ) )
		{
			return;
		}
	}

	if( AndroidCanvasNearlyEqual( CurX, AndroidValueX, 2.5f ) )
	{
		if( AndroidCanvasNearlyEqual( CurY, AndroidMenuStartY + AndroidMenuSpacing * 2.0f, 2.5f ) )
		{
			if( Text[0] == '[' || Text[0] == ' ' )
				CurX += AndroidSpaceX;
		}
		else if( AndroidCanvasNearlyEqual( CurY, AndroidMenuStartY + AndroidMenuSpacing * 3.0f, 2.5f ) )
		{
			if( AndroidIsHighLow )
				CurX += AndroidSpaceX * 2;
		}
	}
	}
#endif

#ifdef PLATFORM_ANDROID // UE1_ANDROID_MULTIPLAYER_JOIN_MENU_DRAW_FIX
	if( AndroidCanvasActiveMenuIs( this, "UnrealJoinGameMenu" ) )
	{
		FLOAT JoinSpacing = 0.06f * ClipY;
		if( JoinSpacing < 11.0f ) JoinSpacing = 11.0f;
		if( JoinSpacing > 32.0f ) JoinSpacing = 32.0f;

		FLOAT JoinStartX = 0.5f * ClipX - 120.0f;
		if( JoinStartX < 12.0f ) JoinStartX = 12.0f;

		FLOAT JoinStartY = 0.5f * ( ClipY - 3.0f * JoinSpacing - 128.0f );
		if( JoinStartY < 32.0f ) JoinStartY = 32.0f;

		const FLOAT JoinValueX = JoinStartX + 100.0f;
		const FLOAT JoinTolerance = Max( 3.0f, JoinSpacing * 0.25f );

		if( ( !appStricmp( Text, "Choose From Favorites" ) || !appStricmp( Text, "Go to the Epic Unreal server list" ) )
		&&  AndroidCanvasNearlyEqual( CurX, JoinStartX, 3.0f ) )
		{
			return;
		}

		if( AndroidCanvasNearlyEqual( CurY, JoinStartY + JoinSpacing * 2.0f, JoinTolerance )
		 || AndroidCanvasNearlyEqual( CurY, JoinStartY + JoinSpacing * 3.0f, JoinTolerance ) )
		{
			if( AndroidCanvasNearlyEqual( CurX, JoinStartX, 3.0f )
			 || AndroidCanvasNearlyEqual( CurX, JoinValueX, 4.0f ) )
			{
				CurY -= JoinSpacing;
			}
		}
	}
#endif
	if( Style!=STY_None )
		WrappedPrintf( Font, bCenter, "%s", Text );
	INT XL, YL;
	WrappedStrLen( Font, XL, YL, ClipX, Text );
	CurX += XL;
	CurYL = Max(CurYL,(FLOAT)YL);
	if( CR )
	{
		CurX  = 0;
		CurY += CurYL;
		CurYL = 0;
	}

	unguardexec;
}
AUTOREGISTER_INTRINSIC( UCanvas, 465, execDrawText );

void UCanvas::execDrawTile( FFrame& Stack, BYTE*& Result )
{
	guard(UCanvas::execDrawTile);
	P_GET_OBJECT(UTexture,Tex);
	P_GET_FLOAT(XL);
	P_GET_FLOAT(YL);
	P_GET_FLOAT(U);
	P_GET_FLOAT(V);
	P_GET_FLOAT(UL);
	P_GET_FLOAT(VL);
	P_FINISH;
	if( !Tex )
	{
		Stack.ScriptWarn( 0, "DrawTile: Missing Texture" );
		return;
	}
	//debugf( "DrawTile: %s %f %f %f %f %f %f", Tex->GetPathName(), XL, YL, U0, V0, U1, V1 );
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_CANVAS_UI_SCALE_EXEC_DRAW_TILE
    FLOAT AndroidDrawX  = OrgX + CurX;
    FLOAT AndroidDrawY  = OrgY + CurY;
    FLOAT AndroidDrawXL = XL;
    FLOAT AndroidDrawYL = YL;
    const FLOAT AndroidUIScale = AndroidCanvasScale();
    if( AndroidUIScale != 1.0f )
    {
        AndroidDrawX  *= AndroidUIScale;
        AndroidDrawY  *= AndroidUIScale;
        AndroidDrawXL *= AndroidUIScale;
        AndroidDrawYL *= AndroidUIScale;
    }
    if( Style!=STY_None ) DrawTile
    (
        Tex,
        AndroidDrawX,
        AndroidDrawY,
        AndroidDrawXL,
        AndroidDrawYL,
#else
    if( Style!=STY_None ) DrawTile
    (
        Tex,
        OrgX+CurX,
        OrgY+CurY,
        XL,
        YL,
#endif
		U,
		V,
		UL,
		VL,
		NULL,
		Z,
		Color.Plane(),
		FPlane(0,0,0,0),
		PF_TwoSided | (Style==STY_Masked ? PF_Masked : Style==STY_Translucent ? PF_Translucent : Style==STY_Modulated ? PF_Modulated : 0) | (bNoSmooth ? PF_NoSmooth : 0)
	);
	CurX += XL + SpaceX;
	CurYL = Max(CurYL,YL);
	unguardexec;
}
AUTOREGISTER_INTRINSIC( UCanvas, 466, execDrawTile );

IMPLEMENT_CLASS(UCanvas);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
