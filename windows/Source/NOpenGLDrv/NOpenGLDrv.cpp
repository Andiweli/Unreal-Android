#include "SDL2/SDL.h"
#include "glad.h"

#include "NOpenGLDrvPrivate.h"

/*-----------------------------------------------------------------------------
	Global implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_PACKAGE(NOpenGLDrv);
IMPLEMENT_CLASS(UNOpenGLRenderDevice);

/*-----------------------------------------------------------------------------
	UNOpenGLRenderDevice implementation.
-----------------------------------------------------------------------------*/

// from XOpenGLDrv:
// PF_Masked requires index 0 to be transparent, but is set on the polygon instead of the texture,
// so we potentially need two copies of any palettized texture in the cache
// unlike in newer unreal versions the low cache bits are actually used, so we have use one of the
// actually unused higher bits for this purpose, thereby breaking 64-bit compatibility for now
#define MASKED_TEXTURE_TAG (1ULL << 60)
#define LOWDETAIL_TEXTURE_TAG (1ULL << 61) // UE1_WINDOWS_TEXTURE_DETAIL_V16

// FColor is adjusted for endianness
#define ALPHA_MASK 0xff000000

// Windows/OpenGL port of the Android/GLES legacy-effect fixes.
// Several original Unreal effect textures use a non-zero or neutral palette
// entry as their transparent billboard border. The generic OpenGL path uploads
// all non-masked palette entries as fully opaque, which makes explosions,
// smoke and waterfall splash/ripple effects show square rectangles.
static const char* UE1OpenGLGetTextureObjectName( const FTextureInfo& Info, char* Buffer, INT BufferSize )
{
	if( BufferSize > 0 )
		Buffer[0] = 0;

	const BYTE CacheBase = (BYTE)( Info.CacheID & 0xff );
	if( CacheBase == CID_RenderTexture )
	{
		const INT ObjIndex = (INT)( Info.CacheID >> 32 );
		UObject* Obj = GObj.GetIndexedObject( ObjIndex );
		if( Obj )
			Obj->GetFullName( Buffer );
	}

	return Buffer;
}

static UBOOL UE1OpenGLTextureNameStartsWith( const FTextureInfo& Info, const char* Prefix )
{
	char Name[512];
	UE1OpenGLGetTextureObjectName( Info, Name, ARRAY_COUNT(Name) );
	return Name[0] && appStrnicmp( Name, Prefix, appStrlen(Prefix) ) == 0;
}

static UBOOL UE1OpenGLIsMaineffectTexture( const FTextureInfo& Info )
{
	return UE1OpenGLTextureNameStartsWith( Info, "Texture UnrealI.Maineffect." );
}

static UBOOL UE1OpenGLIsSmokeBlackTexture( const FTextureInfo& Info )
{
	return UE1OpenGLTextureNameStartsWith( Info, "Texture UnrealI.SmokeBlack." );
}

static UBOOL UE1OpenGLIsHubSmoke1Texture( const FTextureInfo& Info )
{
	return UE1OpenGLTextureNameStartsWith( Info, "FireTexture HubEffects.Smoke1" );
}

static UBOOL UE1OpenGLIsHubWaterRings2Texture( const FTextureInfo& Info )
{
	return UE1OpenGLTextureNameStartsWith( Info, "WaveTexture HubEffects.WaterRings2" );
}

static UBOOL UE1OpenGLNeedsHubSplashMask( const FTextureInfo& Info )
{
	// Keep WaterRings/WaterFall out: WaterRings is the actual water surface,
	// WaterFall2 needs its original translucent colour data. Smoke1 is the
	// rectangular splash overlay with dark border and benefits from alpha test.
	return UE1OpenGLIsHubSmoke1Texture( Info );
}

static UBOOL UE1OpenGLIsLegacyEffectSpriteTexture( const FTextureInfo& Info )
{
	return UE1OpenGLIsMaineffectTexture( Info )
		|| UE1OpenGLIsSmokeBlackTexture( Info )
		|| UE1OpenGLNeedsHubSplashMask( Info )
		|| UE1OpenGLIsHubWaterRings2Texture( Info );
}

static UBOOL UE1OpenGLPreserveMaskedTranslucent( const FTextureInfo& Info )
{
	return UE1OpenGLIsMaineffectTexture( Info )
		|| UE1OpenGLIsSmokeBlackTexture( Info )
		|| UE1OpenGLNeedsHubSplashMask( Info );
}

static BYTE UE1OpenGLPaletteEffectAlpha( const FTextureInfo& Info, BYTE Index, UBOOL Masked, UBOOL bMaineffect, UBOOL bSmokeBlack, UBOOL bHubSmoke1, UBOOL bHubWaterRings2 )
{
	if( bMaineffect )
	{
		// Maineffect explosion frames use palette index 1 as clear border.
		return Index > 1 ? 255 : 0;
	}

	if( bSmokeBlack )
	{
		// PF_Modulated smoke uses neutral grey as the non-effect area.
		FColor C = Info.Palette[Index];
		INT DR = ( C.R > 128 ) ? ( C.R - 128 ) : ( 128 - C.R );
		INT DG = ( C.G > 128 ) ? ( C.G - 128 ) : ( 128 - C.G );
		INT DB = ( C.B > 128 ) ? ( C.B - 128 ) : ( 128 - C.B );
		INT D = Max( DR, Max( DG, DB ) );
		return D <= 10 ? 0 : 255;
	}

	if( bHubSmoke1 )
	{
		// Hub splash FireTexture: black/very-dark border is empty area.
		FColor C = Info.Palette[Index];
		INT MaxC = Max( C.R, Max( C.G, C.B ) );
		return ( Index <= 1 || MaxC <= 24 ) ? 0 : 255;
	}

	if( bHubWaterRings2 )
	{
		// WaterRings2 is fixed through a dedicated multiplicative blend path.
		return 255;
	}

	if( Masked )
		return Index ? 255 : 0;

	return 255;
}

static DWORD UE1OpenGLPaletteDWORDWithAlpha( const FTextureInfo& Info, BYTE Index, BYTE Alpha )
{
#if __INTEL_BYTE_ORDER__
	const DWORD* Pal = (const DWORD*)Info.Palette;
	return ( Pal[Index] & ~ALPHA_MASK ) | ( (DWORD)Alpha << 24 );
#else
	FColor Color = Info.Palette[Index];
	Color.A = Alpha;
	return ( Color.R << 24 ) | ( Color.G << 16 ) | ( Color.B << 8 ) | Color.A;
#endif
}

// lightmaps are 0-127
#define LIGHTMAP_SCALE 2

// and it also would be nice to overbright them
#define LIGHTMAP_OVERBRIGHT 1.4f

// UE1_WINDOWS_TEXTURE_DETAIL_V16_BEGIN
static UBOOL UE1OpenGLParseBool( const char* Text, UBOOL Fallback )
{
	if( !Text || !Text[0] )
		return Fallback;
	if( !appStricmp( Text, "True" ) || !appStricmp( Text, "1" ) || !appStricmp( Text, "Yes" ) || !appStricmp( Text, "On" ) )
		return true;
	if( !appStricmp( Text, "False" ) || !appStricmp( Text, "0" ) || !appStricmp( Text, "No" ) || !appStricmp( Text, "Off" ) )
		return false;
	return Fallback;
}

static UBOOL UE1OpenGLConfigBool( const char* Section, const char* Key, UBOOL Fallback )
{
	char Value[64] = "";
	if( GConfigCache.GetString( Section, Key, Value, ARRAY_COUNT(Value), NULL ) )
		return UE1OpenGLParseBool( Value, Fallback );
	return Fallback;
}
// UE1_WINDOWS_TEXTURE_DETAIL_V16_END

#define GL_CHECK_EXT(ext) GLAD_GL_ ## ext
#define GL_CHECK_VER(maj, min) (((maj) * 10 + (min)) <= (GLVersion.major * 10 + GLVersion.minor))

void UNOpenGLRenderDevice::InternalClassInitializer( UClass* Class )
{
	guardSlow(UNOpenGLRenderDevice::InternalClassInitializer);
	new(Class, "NoFiltering",         RF_Public)UBoolProperty( CPP_PROPERTY(NoFiltering),         "Options", CPF_Config );
	new(Class, "UseHwPalette",        RF_Public)UBoolProperty( CPP_PROPERTY(UseHwPalette),        "Options", CPF_Config );
	new(Class, "UseBGRA",             RF_Public)UBoolProperty( CPP_PROPERTY(UseBGRA),             "Options", CPF_Config );
	new(Class, "DetailTextures",      RF_Public)UBoolProperty( CPP_PROPERTY(DetailTextures),      "Options", CPF_Config );
	new(Class, "UseMultiTexture",     RF_Public)UBoolProperty( CPP_PROPERTY(UseMultiTexture),     "Options", CPF_Config );
	new(Class, "AutoFOV",             RF_Public)UBoolProperty( CPP_PROPERTY(AutoFOV),             "Options", CPF_Config );
	new(Class, "UseWindowBrightness", RF_Public)UBoolProperty( CPP_PROPERTY(UseWindowBrightness), "Options", CPF_Config );
	new(Class, "SwapInterval",        RF_Public)UIntProperty ( CPP_PROPERTY(SwapInterval),        "Options", CPF_Config );
	unguardSlow;
}

UNOpenGLRenderDevice::UNOpenGLRenderDevice()
{
	NoFiltering = false;
	UseHwPalette = true;
	UseBGRA = true;
	DetailTextures = true;
	UseMultiTexture = true;
	AutoFOV = true;
	UseWindowBrightness = true;
	CurrentBrightness = -1.f;
	SwapInterval = 1;
	RuntimeLowDetailTextures = false; // UE1_WINDOWS_TEXTURE_DETAIL_V16
}

UBOOL UNOpenGLRenderDevice::Init( UViewport* InViewport )
{
	guard(UNOpenGLRenderDevice::Init)

	if( !gladLoadGLLoader( &SDL_GL_GetProcAddress ) )
	{
		debugf( NAME_Warning, "Could not load GL: %s", SDL_GetError() );
		return false;
	}

	SupportsFogMaps = true;
	SupportsDistanceFog = true;

	RuntimeLowDetailTextures = GetConfiguredLowDetailTextures(); // UE1_WINDOWS_TEXTURE_DETAIL_V16_INIT
	UpdateSwapInterval();

	if( UseHwPalette && !GL_CHECK_EXT( EXT_paletted_texture ) )
	{
		debugf( NAME_Warning, "EXT_paletted_texture not available, disabling UseHwPalette" );
		UseHwPalette = false;
	}

	if( UseBGRA && !GL_CHECK_VER( 1, 2 ) && !GL_CHECK_EXT( EXT_bgra ) )
	{
		debugf( NAME_Warning, "EXT_bgra not available, disabling UseBGRA" );
		UseBGRA = false;
	}

	if( UseMultiTexture && ( !GL_CHECK_EXT( ARB_multitexture ) || !GL_CHECK_EXT( EXT_texture_env_combine ) ) )
	{
		debugf( NAME_Warning, "ARB_multitexture or EXT_texture_env_combine is not available, disabling UseMultiTexture" );
		UseMultiTexture = false;
	}

	if( UseMultiTexture )
	{
		GLint TMUnits;
		glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &TMUnits );
		if ( TMUnits < 4 )
		{
			debugf( NAME_Warning, "Not enough texture units (%i, expected 4), disabling UseMultiTexture", TMUnits );
			UseMultiTexture = false;
		}
	}

	debugf( NAME_Log, "Got OpenGL %d.%d", GLVersion.major, GLVersion.minor );

	ComposeSize = 256 * 256 * 4;
	Compose = (BYTE*)appMalloc( ComposeSize, "GLComposeBuf" );
	verify( Compose );

	// Set modelview matrix to flip stuff into our coordinate system.
	const FLOAT Matrix[16] =
	{
		+1, +0, +0, +0,
		+0, -1, +0, +0,
		+0, +0, -1, +0,
		+0, +0, +0, +1,
	};
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
	glMultMatrixf( Matrix );

	// Set permanent state.
	glEnable( GL_DEPTH_TEST );
	glShadeModel( GL_SMOOTH );
	glAlphaFunc( GL_GREATER, 0.5 );
	glDisable( GL_ALPHA_TEST );
	glDepthMask( GL_TRUE );
	glBlendFunc( GL_ONE, GL_ZERO );
	glEnable( GL_BLEND );
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

	CurrentPolyFlags = PF_Occlude;
	Viewport = InViewport;

	return true;
	unguard;
}

void UNOpenGLRenderDevice::Exit()
{
	guard(UNOpenGLRenderDevice::Exit);

	debugf( NAME_Log, "Shutting down OpenGL renderer" );

	Flush();

	if( Compose )
	{
		appFree( Compose );
		Compose = NULL;
	}
	ComposeSize = 0;

	unguard;
}

void UNOpenGLRenderDevice::PostEditChange()
{
	guard(UNOpenGLRenderDevice::PostEditChange)

	Super::PostEditChange();

	RuntimeLowDetailTextures = GetConfiguredLowDetailTextures(); // UE1_WINDOWS_TEXTURE_DETAIL_V16_INIT
	UpdateSwapInterval();

	unguard;
}

void UNOpenGLRenderDevice::Flush()
{
	guard(UNOpenGLRenderDevice::Flush);

	if( TexAlloc.Num() )
	{
		debugf( NAME_Log, "Flushing %d/%d textures", TexAlloc.Num(), BindMap.Size() );
		for ( INT i = 0; i < MaxTexUnits; ++i )
		{
			ResetTexture( i );
		}
		glFinish();
		glDeleteTextures( TexAlloc.Num(), &TexAlloc(0) );
		TexAlloc.Empty();
		BindMap.Empty();
	}

	unguard;
}

UBOOL UNOpenGLRenderDevice::Exec( const char* Cmd, FOutputDevice* Out )
{
	return false;
}

void UNOpenGLRenderDevice::Lock( FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* InHitData, INT* InHitSize )
{
	guard(UNOpenGLRenderDevice::Lock);

	UpdateRuntimeLowDetailTextures(); // UE1_WINDOWS_TEXTURE_DETAIL_V16_LOCK

	BindCycles = ImageCycles = ComplexCycles = GouraudCycles = TileCycles = 0;

	glClearColor( ScreenClear.X, ScreenClear.Y, ScreenClear.Z, ScreenClear.W );
	glClearDepth( 1.0 );
	glDepthFunc( GL_LEQUAL );

	if( UseWindowBrightness )
	{
		FLOAT TargetBrightness = CurrentBrightness;
		if ( Viewport && Viewport->Client )
			TargetBrightness = Viewport->Client->Brightness;
		else if ( CurrentBrightness < 0.f )
			TargetBrightness = 0.5f;
		if ( CurrentBrightness != TargetBrightness )
		{
			CurrentBrightness = TargetBrightness;
			const FLOAT Gamma = 0.5 + 1.5 * CurrentBrightness;
			SDL_Window* Window = (SDL_Window*)Viewport->GetWindow();
			SDL_SetWindowBrightness( Window, Gamma );
		}
	}

	SetBlend( PF_Occlude );

	GLbitfield ClearBits = GL_DEPTH_BUFFER_BIT;
	if( RenderLockFlags & LOCKR_ClearScreen )
		ClearBits |= GL_COLOR_BUFFER_BIT;
	glClear( ClearBits );

	if( FlashScale != FPlane(0.5f, 0.5f, 0.5f, 0.0f) || FlashFog != FPlane(0.0f, 0.0f, 0.0f, 0.0f) )
		ColorMod = FPlane( FlashFog.X, FlashFog.Y, FlashFog.Z, 1.f - Min( FlashScale.X * 2.f, 1.f ) );
	else
		ColorMod = FPlane( 0.f, 0.f, 0.f, 0.f );

	if( AutoFOV && Viewport && Viewport->Actor && Viewport->Actor->DesiredFOV == 90.0f )
	{
		const FLOAT Aspect = (FLOAT)Viewport->SizeX / (FLOAT)Viewport->SizeY;
		const FLOAT Fov = (FLOAT)( appAtan( appTan( 90.0 * PI / 360.0 ) * ( Aspect / ( 4.0 / 3.0 ) ) ) * 360.0 ) / PI;
		Viewport->Actor->DesiredFOV = Fov;
	}

	unguard;
}

void UNOpenGLRenderDevice::Unlock( UBOOL Blit )
{
	guard(UNOpenGLRenderDevice::Unlock);

	glFlush();

	unguard;
}

void UNOpenGLRenderDevice::DrawComplexSurface( FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet )
{
	guard(UNOpenGLRenderDevice::DrawComplexSurface);

	check(Surface.Texture);

	SetSceneNode( Frame );

	uclock(ComplexCycles);

	if( UseMultiTexture )
	{
		// Draw with multitexture.
		DrawComplexSurfaceMultiTex( Frame, Surface, Facet );
	}
	else
	{
		// Draw with single texture unit.
		DrawComplexSurfaceSingleTex(Frame, Surface, Facet);
	}

	uunclock(ComplexCycles);

	unguard;
}

void UNOpenGLRenderDevice::DrawComplexSurfaceMultiTex( FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet )
{
	const FLOAT UDot = Facet.MapCoords.XAxis | Facet.MapCoords.Origin;
	const FLOAT VDot = Facet.MapCoords.YAxis | Facet.MapCoords.Origin;

	DWORD RenderPolyFlags = Surface.PolyFlags;
	const UBOOL bHubWaterRings2 = UE1OpenGLIsHubWaterRings2Texture( *Surface.Texture );
	if( UE1OpenGLNeedsHubSplashMask( *Surface.Texture ) )
		RenderPolyFlags |= PF_Masked | PF_NoSmooth;
	const UBOOL bSpecialWaterRings2Blend = bHubWaterRings2 && ( RenderPolyFlags & PF_Modulated );

	SetBlend( RenderPolyFlags, false, UE1OpenGLPreserveMaskedTranslucent( *Surface.Texture ) );
	if( bSpecialWaterRings2Blend )
	{
		// WaterRings2 has a white/near-white neutral area. The normal UE1
		// PF_Modulated blend brightens that into a rectangle; classic multiply
		// keeps the neutral background invisible and only dark texels draw rings.
		glEnable( GL_BLEND );
		glBlendFunc( GL_ZERO, GL_SRC_COLOR );
	}
	SetTexture( 0, *Surface.Texture, ( RenderPolyFlags & PF_Masked ), 0.0 );

	if( Surface.LightMap )
	{
		SetTexture( 1, *Surface.LightMap, 0, -0.5f );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
		glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE );
		glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0f );
		glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE );
		glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
		glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );
	}

	if( Surface.DetailTexture && DetailTextures && !( Viewport && Viewport->Client && Viewport->Client->LowDetailTextures ) ) // UE1_WINDOWS_LOWDETAIL_DETAILFILTER_V17
	{
		SetTexture( 2, *Surface.DetailTexture, 0, 0.f );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
		glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE );
		glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE, 2.0f );
		glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE );
		glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
		glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );
	}

	if( Surface.FogMap )
	{
		SetTexture( 3, *Surface.FogMap, 0, -0.5f );
		glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
		glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD );
		glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE );
		glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_PREVIOUS );
		glTexEnvi( GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA );
	}

	glColor4f( 1.f, 1.f, 1.f, 1.f );
	for( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next )
	{
		if( !Poly || Poly->NumPts < 3 )
			continue;
		glBegin( GL_TRIANGLE_FAN );
		for( INT i=0; i<Poly->NumPts; i++ )
		{
			const FLOAT U = Facet.MapCoords.XAxis | Poly->Pts[i]->Point;
			const FLOAT V = Facet.MapCoords.YAxis | Poly->Pts[i]->Point;
			for( INT t=0; t<MaxTexUnits; ++t)
			{
				if( TexInfo[t].CurrentCacheID != 0 )
				{
					glMultiTexCoord2f( GL_TEXTURE0+t, (U-UDot-TexInfo[t].UPan)*TexInfo[t].UMult, (V-VDot-TexInfo[t].VPan)*TexInfo[t].VMult );
				}
			}
			glVertex3fv( &Poly->Pts[i]->Point.X );
		}
		glEnd();
	}

	for( INT t=1; t<MaxTexUnits; ++t)
	{
		ResetTexture( t );
	}

	if( bSpecialWaterRings2Blend )
	{
		CurrentPolyFlags = 0xffffffff;
		SetBlend( RenderPolyFlags );
	}
}

void UNOpenGLRenderDevice::DrawComplexSurfaceSingleTex( FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet )
{
	const FLOAT UDot = Facet.MapCoords.XAxis | Facet.MapCoords.Origin;
	const FLOAT VDot = Facet.MapCoords.YAxis | Facet.MapCoords.Origin;

	DWORD RenderPolyFlags = Surface.PolyFlags;
	const UBOOL bHubWaterRings2 = UE1OpenGLIsHubWaterRings2Texture( *Surface.Texture );
	if( UE1OpenGLNeedsHubSplashMask( *Surface.Texture ) )
		RenderPolyFlags |= PF_Masked | PF_NoSmooth;
	const UBOOL bSpecialWaterRings2Blend = bHubWaterRings2 && ( RenderPolyFlags & PF_Modulated );

	// Draw texture.
	SetBlend( RenderPolyFlags, false, UE1OpenGLPreserveMaskedTranslucent( *Surface.Texture ) );
	if( bSpecialWaterRings2Blend )
	{
		glEnable( GL_BLEND );
		glBlendFunc( GL_ZERO, GL_SRC_COLOR );
	}
	SetTexture( 0, *Surface.Texture, ( RenderPolyFlags & PF_Masked ), 0.f );
	glColor4f( 1.f, 1.f, 1.f, 1.f );
	for( FSavedPoly* Poly = Facet.Polys; Poly; Poly = Poly->Next )
	{
		if( !Poly || Poly->NumPts < 3 )
			continue;
		glBegin( GL_TRIANGLE_FAN );
		for( INT i = 0; i < Poly->NumPts; i++ )
		{
			const FLOAT U = Facet.MapCoords.XAxis | Poly->Pts[i]->Point;
			const FLOAT V = Facet.MapCoords.YAxis | Poly->Pts[i]->Point;
			glTexCoord2f( (U-UDot-TexInfo[0].UPan)*TexInfo[0].UMult, (V-VDot-TexInfo[0].VPan)*TexInfo[0].VMult );
			glVertex3f( Poly->Pts[i]->Point.X, Poly->Pts[i]->Point.Y, Poly->Pts[i]->Point.Z );
		}
		glEnd();
	}

	if( bSpecialWaterRings2Blend )
	{
		CurrentPolyFlags = 0xffffffff;
		SetBlend( RenderPolyFlags );
	}

	// Draw lightmap.
	if( Surface.LightMap )
	{
		SetBlend( PF_Modulated );
		if( RenderPolyFlags & PF_Masked )
			glDepthFunc( GL_EQUAL );
		SetTexture( 0, *Surface.LightMap, 0, -0.5 );
		glColor4f( 1.f, 1.f, 1.f, 1.f );
		for( FSavedPoly* Poly = Facet.Polys; Poly; Poly = Poly->Next )
		{
			if( !Poly || Poly->NumPts < 3 )
				continue;
			glBegin( GL_TRIANGLE_FAN );
			for( INT i = 0; i < Poly->NumPts; i++ )
			{
				const FLOAT U = Facet.MapCoords.XAxis | Poly->Pts[i]->Point;
				const FLOAT V = Facet.MapCoords.YAxis | Poly->Pts[i]->Point;
				glTexCoord2f( (U-UDot-TexInfo[0].UPan)*TexInfo[0].UMult, (V-VDot-TexInfo[0].VPan)*TexInfo[0].VMult );
				glVertex3f( Poly->Pts[i]->Point.X, Poly->Pts[i]->Point.Y, Poly->Pts[i]->Point.Z );
			}
			glEnd();
		}
		if( RenderPolyFlags & PF_Masked )
			glDepthFunc( GL_LEQUAL );
	}

	// Draw detail texture overlaid.
	if( Surface.DetailTexture && DetailTextures && !( Viewport && Viewport->Client && Viewport->Client->LowDetailTextures ) ) // UE1_WINDOWS_LOWDETAIL_DETAILFILTER_V17
	{
		SetBlend( PF_Modulated );
		if( RenderPolyFlags & PF_Masked )
			glDepthFunc( GL_EQUAL );
		SetTexture( 0, *Surface.DetailTexture, 0, 0.f );

		for( FSavedPoly* Poly = Facet.Polys; Poly; Poly = Poly->Next )
		{
			if( !Poly || Poly->NumPts < 3 )
				continue;
			glBegin( GL_TRIANGLE_FAN );
			for( INT i = 0; i < Poly->NumPts; i++ )
			{
				const FLOAT U = Facet.MapCoords.XAxis | Poly->Pts[i]->Point;
				const FLOAT V = Facet.MapCoords.YAxis | Poly->Pts[i]->Point;
				glTexCoord2f( (U-UDot-TexInfo[0].UPan)*TexInfo[0].UMult, (V-VDot-TexInfo[0].VPan)*TexInfo[0].VMult );
				glVertex3f( Poly->Pts[i]->Point.X, Poly->Pts[i]->Point.Y, Poly->Pts[i]->Point.Z );
			}
			glEnd();
		}
		if( RenderPolyFlags & PF_Masked )
			glDepthFunc( GL_LEQUAL );
	}

	// Draw fog.
	if( Surface.FogMap )
	{
		SetBlend( PF_Highlighted );
		if( RenderPolyFlags & PF_Masked )
			glDepthFunc( GL_EQUAL );
		SetTexture( 0, *Surface.FogMap, 0, -0.5 );
		for( FSavedPoly* Poly = Facet.Polys; Poly; Poly = Poly->Next )
		{
			if( !Poly || Poly->NumPts < 3 )
				continue;
			glBegin( GL_TRIANGLE_FAN );
			for( INT i = 0; i < Poly->NumPts; i++ )
			{
				const FLOAT U = Facet.MapCoords.XAxis | Poly->Pts[i]->Point;
				const FLOAT V = Facet.MapCoords.YAxis | Poly->Pts[i]->Point;
				glTexCoord2f( (U-UDot-TexInfo[0].UPan)*TexInfo[0].UMult, (V-VDot-TexInfo[0].VPan)*TexInfo[0].VMult );
				glVertex3f( Poly->Pts[i]->Point.X, Poly->Pts[i]->Point.Y, Poly->Pts[i]->Point.Z );
			}
			glEnd();
		}
		if( RenderPolyFlags & PF_Masked )
			glDepthFunc( GL_LEQUAL );
	}
}

void UNOpenGLRenderDevice::DrawGouraudPolygon( FSceneNode* Frame, FTextureInfo& Texture, FTransTexture** Pts, INT NumPts, DWORD PolyFlags, FSpanBuffer* SpanBuffer )
{
		guard(UNOpenGLRenderDevice::DrawGouraudPolygon);

		if( !Pts || NumPts < 3 )
			return;

		DWORD RenderPolyFlags = PolyFlags;
		if( UE1OpenGLIsMaineffectTexture( Texture ) || UE1OpenGLIsSmokeBlackTexture( Texture ) )
			RenderPolyFlags |= PF_Masked | PF_NoSmooth;

		SetSceneNode( Frame );
		uclock(GouraudCycles);
		SetBlend( RenderPolyFlags, false, UE1OpenGLPreserveMaskedTranslucent( Texture ) );
		SetTexture( 0, Texture, ( RenderPolyFlags & PF_Masked ), 0 );
		ResetTexture( 1 );
		ResetTexture( 2 );
		ResetTexture( 3 );

		const UBOOL IsModulated = ( RenderPolyFlags & PF_Modulated );

		if( IsModulated )
			glColor4f( 1.f, 1.f, 1.f, 1.f );

		glBegin( GL_TRIANGLE_FAN  );
		for( INT i=0; i<NumPts; i++ )
		{
			FTransTexture* P = Pts[i];
			if( !IsModulated )
				glColor4f( P->Light.X, P->Light.Y, P->Light.Z, 1.f );
			glTexCoord2f( P->U*TexInfo[0].UMult, P->V*TexInfo[0].VMult );
			glVertex3f( P->Point.X, P->Point.Y, P->Point.Z );
		}
		glEnd();

		if( (RenderPolyFlags & (PF_RenderFog|PF_Translucent|PF_Modulated)) == PF_RenderFog )
		{
			ResetTexture( 0 );
			SetBlend( PF_Highlighted );
			glBegin( GL_TRIANGLE_FAN );
			for( INT i = 0; i < NumPts; i++ )
			{
				FTransTexture* P = Pts[i];
				glColor4f( P->Fog.X, P->Fog.Y, P->Fog.Z, P->Fog.W );
				glVertex3f( P->Point.X, P->Point.Y, P->Point.Z );
			}
			glEnd();
		}

		uunclock(GouraudCycles);
		unguard;
}

void UNOpenGLRenderDevice::DrawTile( FSceneNode* Frame, FTextureInfo& Texture, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, FSpanBuffer* Span, FLOAT Z, FPlane Light, FPlane Fog, DWORD PolyFlags )
{
	guard(UNOpenGLRenderDevice::DrawTile);

	DWORD RenderPolyFlags = PolyFlags;
	if( UE1OpenGLIsMaineffectTexture( Texture ) || UE1OpenGLIsSmokeBlackTexture( Texture ) )
		RenderPolyFlags |= PF_Masked | PF_NoSmooth;

	SetSceneNode( Frame );
	uclock(TileCycles);
	SetBlend( RenderPolyFlags, false, UE1OpenGLPreserveMaskedTranslucent( Texture ) );
	SetTexture( 0, Texture, ( RenderPolyFlags & PF_Masked ), 0.f );
	ResetTexture( 1 );
	ResetTexture( 2 );
	ResetTexture( 3 );

	if( RenderPolyFlags & PF_Modulated )
		glColor4f( 1.f, 1.f, 1.f, 1.f );
	else
		glColor4f( Light.X, Light.Y, Light.Z, 1.f );

	glBegin( GL_TRIANGLE_FAN );
		glTexCoord2f( (U   )*TexInfo[0].UMult, (V   )*TexInfo[0].VMult );
		glVertex3f( RFX2*Z*(X   -Frame->FX2), RFY2*Z*(Y   -Frame->FY2), Z );
		glTexCoord2f( (U+UL)*TexInfo[0].UMult, (V   )*TexInfo[0].VMult );
		glVertex3f( RFX2*Z*(X+XL-Frame->FX2), RFY2*Z*(Y   -Frame->FY2), Z );
		glTexCoord2f( (U+UL)*TexInfo[0].UMult, (V+VL)*TexInfo[0].VMult );
		glVertex3f( RFX2*Z*(X+XL-Frame->FX2), RFY2*Z*(Y+YL-Frame->FY2), Z );
		glTexCoord2f( (U   )*TexInfo[0].UMult, (V+VL)*TexInfo[0].VMult );
		glVertex3f( RFX2*Z*(X   -Frame->FX2), RFY2*Z*(Y+YL-Frame->FY2), Z );
	glEnd();

	uunclock(TileCycles);
	unguard;
}

void UNOpenGLRenderDevice::Draw2DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 )
{

}

void UNOpenGLRenderDevice::Draw2DPoint( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2 )
{

}

void UNOpenGLRenderDevice::EndFlash( )
{
	guard(UNOpenGLESRenderDevice::EndFlash);

	if( ColorMod == FPlane( 0.f, 0.f, 0.f, 0.f ) )
		return;

	ResetTexture( 0 );
	ResetTexture( 1 );
	ResetTexture( 2 );
	ResetTexture( 3 );
	SetBlend( PF_Highlighted );

	const FLOAT Z = 1.f;
	const FLOAT RFX2 = RProjZ;
	const FLOAT RFY2 = RProjZ * Aspect;

	glDisable( GL_DEPTH_TEST );

	glColor4fv( &ColorMod.R );
	glBegin( GL_TRIANGLE_FAN );
		glVertex3f( RFX2 * -Z, RFY2 * -Z, Z );
		glVertex3f( RFX2 * +Z, RFY2 * -Z, Z );
		glVertex3f( RFX2 * +Z, RFY2 * +Z, Z );
		glVertex3f( RFX2 * -Z, RFY2 * +Z, Z );
	glEnd();

	glEnable( GL_DEPTH_TEST );

	unguard;
}

void UNOpenGLRenderDevice::PushHit( const BYTE* Data, INT Count )
{

}

void UNOpenGLRenderDevice::PopHit( INT Count, UBOOL bForce )
{

}

void UNOpenGLRenderDevice::GetStats( char* Result )
{
	guard(UNOpenGLRenderDevice::GetStats)

//	if( Result ) *Result = '\0';
	appSprintf
	(
		Result,
		"OpenGL stats: Bind=%04.1f Image=%04.1f Complex=%04.1f Gouraud=%04.1f Tile=%04.1f",
		GSecondsPerCycle*1000 * BindCycles,
		GSecondsPerCycle*1000 * ImageCycles,
		GSecondsPerCycle*1000 * ComplexCycles,
		GSecondsPerCycle*1000 * GouraudCycles,
		GSecondsPerCycle*1000 * TileCycles
	);

	unguard;
}

void UNOpenGLRenderDevice::ReadPixels( FColor* Pixels )
{
	guard(UNOpenGLRenderDevice::ReadPixels);

	glPixelStorei( GL_UNPACK_ALIGNMENT, 0 );
	glReadPixels( 0, 0, Viewport->SizeX, Viewport->SizeY, GL_RGBA, GL_UNSIGNED_BYTE, (void*)Pixels );

	// Swap RGBA -> BGRA and flip vertically.
	for( INT i=0; i<Viewport->SizeY/2; i++ )
	{
		for( INT j=0; j<Viewport->SizeX; j++ )
		{
			Exchange( Pixels[j+i*Viewport->SizeX].R, Pixels[j+(Viewport->SizeY-1-i)*Viewport->SizeX].B );
			Exchange( Pixels[j+i*Viewport->SizeX].G, Pixels[j+(Viewport->SizeY-1-i)*Viewport->SizeX].G );
			Exchange( Pixels[j+i*Viewport->SizeX].B, Pixels[j+(Viewport->SizeY-1-i)*Viewport->SizeX].R );
		}
	}

	unguard;
}

void UNOpenGLRenderDevice::ClearZ( FSceneNode* Frame )
{
	guard(UNOpenGLRenderDevice::ClearZ);

	SetBlend( PF_Occlude );
	glClear( GL_DEPTH_BUFFER_BIT );

	unguard;
}

void UNOpenGLRenderDevice::SetSceneNode( FSceneNode* Frame )
{
	guard(UNOpenGLRenderDevice::SetSceneNode);

	check(Viewport);

	if( !Frame )
	{
		// invalidate current saved data
		CurrentSceneNode.X = -1;
		CurrentSceneNode.FX = -1.f;
		CurrentSceneNode.SizeX = -1;
		return;
	}

	if( Frame->X != CurrentSceneNode.X || Frame->Y != CurrentSceneNode.Y ||
			Frame->XB != CurrentSceneNode.XB || Frame->YB != CurrentSceneNode.YB ||
			Viewport->SizeX != CurrentSceneNode.SizeX || Viewport->SizeY != CurrentSceneNode.SizeY )
	{
		glViewport( Frame->XB, Viewport->SizeY - Frame->Y - Frame->YB, Frame->X, Frame->Y );
		CurrentSceneNode.X = Frame->X;
		CurrentSceneNode.Y = Frame->Y;
		CurrentSceneNode.XB = Frame->XB;
		CurrentSceneNode.YB = Frame->YB;
		CurrentSceneNode.SizeX = Viewport->SizeX;
		CurrentSceneNode.SizeY = Viewport->SizeY;
	}

	if( Frame->FX != CurrentSceneNode.FX || Frame->FY != CurrentSceneNode.FY ||
			Viewport->Actor->FovAngle != CurrentSceneNode.FovAngle )
	{
		RProjZ = appTan( Viewport->Actor->FovAngle * PI / 360.0 );
		Aspect = Frame->FY / Frame->FX;
		RFX2 = 2.0f * RProjZ / Frame->FX;
		RFY2 = 2.0f * RProjZ * Aspect / Frame->FY;
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
		glFrustum( -RProjZ, +RProjZ, -Aspect * RProjZ, +Aspect * RProjZ, 1.0, 65336.0 );
		CurrentSceneNode.FX = Frame->FX;
		CurrentSceneNode.FY = Frame->FY;
		CurrentSceneNode.FovAngle = Viewport->Actor->FovAngle;
	}

	unguard;
}

void UNOpenGLRenderDevice::SetBlend( DWORD PolyFlags, UBOOL InverseOrder, UBOOL PreserveTranslucentMask )
{
	guard(UNOpenGLRenderDevice::SetBlend);

	// Adjust PolyFlags according to Unreal's precedence rules.
	if( !(PolyFlags & (PF_Translucent|PF_Modulated)) )
		PolyFlags |= PF_Occlude;
	else if( ( PolyFlags & PF_Translucent ) && !PreserveTranslucentMask )
		PolyFlags &= ~PF_Masked;

	// Detect changes in the blending modes.
	DWORD Xor = CurrentPolyFlags ^ PolyFlags;
	if( Xor & (PF_Translucent|PF_Modulated|PF_Invisible|PF_Occlude|PF_Masked|PF_Highlighted) )
	{
		if( Xor&(PF_Translucent|PF_Modulated|PF_Highlighted) )
		{
			glEnable( GL_BLEND );
			if( PolyFlags & PF_Translucent )
			{
				glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_COLOR );
			}
			else if( PolyFlags & PF_Modulated )
			{
				glBlendFunc( GL_DST_COLOR, GL_SRC_COLOR );
			}
			else if( PolyFlags & PF_Highlighted )
			{
				glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
			}
			else
			{
				glDisable( GL_BLEND );
				glBlendFunc( GL_ONE, GL_ZERO );
			}
		}
		if( Xor & PF_Invisible )
		{
			UBOOL Show = !( PolyFlags & PF_Invisible );
			glColorMask( Show, Show, Show, Show );
		}
		if( Xor & PF_Occlude )
		{
			glDepthMask( (PolyFlags & PF_Occlude) != 0 );
		}
		if( Xor & PF_Masked )
		{
			if( PolyFlags & PF_Masked )
				glEnable( GL_ALPHA_TEST );
			else
				glDisable( GL_ALPHA_TEST );
		}
	}

	CurrentPolyFlags = PolyFlags;

	unguard;
}

// UE1_WINDOWS_TEXTURE_DETAIL_V16_FUNCTIONS_BEGIN
void UNOpenGLRenderDevice::UpdateTextureFilter( const FTextureInfo& Info, DWORD PolyFlags, INT BaseMip )
{
	guard(UNOpenGLRenderDevice::UpdateTextureFilter);

	if( UE1OpenGLIsLegacyEffectSpriteTexture( Info ) )
	{
		// Effect billboards/splash overlays must not sample wrapped edges or
		// lower mips, otherwise their old palette border appears as a square.
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		return;
	}

	const INT EffectiveNumMips = Max( Info.NumMips - BaseMip, 1 );

	if( ( PolyFlags & PF_NoSmooth ) || ( NoFiltering && Info.Palette ) )
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ( EffectiveNumMips > 1 ) ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}
	else
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ( EffectiveNumMips > 1 ) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}

	if( !Info.Palette )
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	}

	unguard;
}

INT UNOpenGLRenderDevice::GetTextureBaseMip( const FTextureInfo& Info, DWORD PolyFlags ) const
{
	guard(UNOpenGLRenderDevice::GetTextureBaseMip);

	if( !RuntimeLowDetailTextures )
		return 0;

	// LOW texture detail is intentionally conservative on Windows/OpenGL:
	// ordinary palettized world textures only. Keep lightmaps/fogmaps,
	// masked/translucent/modulated/highlighted sprites, realtime/fire textures
	// and textures without a valid mip chain exact.
	if( !Info.Palette || Info.NumMips < 2 || ( PolyFlags & ( PF_Masked | PF_Translucent | PF_Modulated | PF_Highlighted ) ) )
		return 0;
	if( Info.TextureFlags & ( TF_Realtime | TF_RealtimeChanged ) )
		return 0;
	if( UE1OpenGLIsLegacyEffectSpriteTexture( Info ) )
		return 0;

	return ( Info.Mips[1] && Info.Mips[1]->DataPtr ) ? 1 : 0;

	unguard;
}

UBOOL UNOpenGLRenderDevice::GetConfiguredLowDetailTextures() const
{
	guard(UNOpenGLRenderDevice::GetConfiguredLowDetailTextures);

	UBOOL Value = false;
	if( Viewport && Viewport->Client )
		Value = Viewport->Client->LowDetailTextures;

#if defined(PLATFORM_WIN32) || defined(_WIN32)
	Value = UE1OpenGLConfigBool( "NSDLDrv.NSDLClient", "LowDetailTextures", Value );
	Value = UE1OpenGLConfigBool( "WinDrv.WindowsClient", "LowDetailTextures", Value );
#endif

	return Value;

	unguard;
}

void UNOpenGLRenderDevice::UpdateRuntimeLowDetailTextures()
{
	guard(UNOpenGLRenderDevice::UpdateRuntimeLowDetailTextures);

	const UBOOL NewLowDetailTextures = GetConfiguredLowDetailTextures();
	if( Viewport && Viewport->Client )
		Viewport->Client->LowDetailTextures = NewLowDetailTextures;

	if( RuntimeLowDetailTextures != NewLowDetailTextures )
	{
		RuntimeLowDetailTextures = NewLowDetailTextures;
		debugf( NAME_Log, "OpenGL: Texture Detail runtime mode changed to %s", RuntimeLowDetailTextures ? "LOW" : "HIGH" );
		Flush();
	}

	unguard;
}
// UE1_WINDOWS_TEXTURE_DETAIL_V16_FUNCTIONS_END
void UNOpenGLRenderDevice::ResetTexture( INT TMU )
{
	guard(UNOpenGLRenderDevice::ResetTexture);

	if( TexInfo[TMU].CurrentCacheID != 0 )
	{
		uclock(BindCycles);
		glActiveTexture( GL_TEXTURE0 + TMU );
		glBindTexture( GL_TEXTURE_2D, 0 );
		glDisable( GL_TEXTURE_2D );
		TexInfo[TMU].CurrentCacheID = 0;
		uunclock(BindCycles);
	}

	unguard;
}

void UNOpenGLRenderDevice::SetTexture( INT TMU, FTextureInfo& Info, DWORD PolyFlags, FLOAT PanBias )
{
	guard(UNOpenGLRenderDevice::SetTexture);

	// Set panning.
	FTexInfo& Tex = TexInfo[TMU];
	Tex.UPan      = Info.Pan.X + PanBias*Info.UScale;
	Tex.VPan      = Info.Pan.Y + PanBias*Info.VScale;

	// Account for all the impact on scale normalization.
	Tex.UMult = 1.f / (Info.UScale * static_cast<FLOAT>(Info.USize));
	Tex.VMult = 1.f / (Info.VScale * static_cast<FLOAT>(Info.VSize));

	const INT BaseMip = GetTextureBaseMip( Info, PolyFlags );

	// Find in cache.
	QWORD NewCacheID = Info.CacheID;
	if( ( PolyFlags & PF_Masked ) && Info.Palette )
		NewCacheID |= MASKED_TEXTURE_TAG;
	if( BaseMip > 0 )
		NewCacheID |= LOWDETAIL_TEXTURE_TAG;
	UBOOL RealtimeChanged = ( Info.TextureFlags & TF_RealtimeChanged );
	if( NewCacheID == Tex.CurrentCacheID && !RealtimeChanged )
		return;

	// Make current.
	uclock(BindCycles);
	Tex.CurrentCacheID = NewCacheID;
	FCachedTexture* Bind = BindMap.Find( NewCacheID );
	FCachedTexture* OldBind = Bind;
	if( !Bind )
	{
		// New texture.
		Bind = BindMap.Add( NewCacheID, FCachedTexture() );
		Bind->BaseMip = BaseMip;
		Bind->MaxLevel = Max( Info.NumMips - BaseMip - 1, 0 );
		glGenTextures( 1, &Bind->Id );
		TexAlloc.AddItem( Bind->Id );
	}

	glActiveTexture( GL_TEXTURE0 + TMU );
	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, Bind->Id );
	uunclock(BindCycles);

	Bind->BaseMip = BaseMip;
	Bind->MaxLevel = Max( Info.NumMips - BaseMip - 1, 0 );

	if( !OldBind || RealtimeChanged )
	{
		// New texture or it has changed, upload it.
		Info.TextureFlags &= ~TF_RealtimeChanged;
		UploadTexture( Info, ( PolyFlags & PF_Masked ), !OldBind, BaseMip );
		UpdateTextureFilter( Info, PolyFlags, BaseMip );
	}

	unguard;
}
void UNOpenGLRenderDevice::EnsureComposeSize( const DWORD NewSize )
{
	if( NewSize > ComposeSize )
	{
		Compose = (BYTE*)appRealloc( Compose, NewSize, "GLComposeBuf" );
	}
	verify( Compose );
}

void UNOpenGLRenderDevice::ConvertTextureMipI8( const FTextureInfo& Info, const FMipmap* Mip, const UBOOL Masked, BYTE*& UploadBuf, GLenum& UploadFormat, GLenum& InternalFormat )
{
	// 8-bit indexed. We have to fix the alpha component since it's mostly garbage.
	DWORD i;
	const UBOOL bMaineffect = UE1OpenGLIsMaineffectTexture( Info );
	const UBOOL bSmokeBlack = UE1OpenGLIsSmokeBlackTexture( Info );
	const UBOOL bHubSmoke1 = UE1OpenGLIsHubSmoke1Texture( Info );
	const UBOOL bHubWaterRings2 = UE1OpenGLIsHubWaterRings2Texture( Info );
	const UBOOL bLegacyEffectSprite = bMaineffect || bSmokeBlack || bHubSmoke1 || bHubWaterRings2;

	if( UseHwPalette )
	{
		// GL has support for palettized textures, use it. Still have to fix alpha.
		EnsureComposeSize( 256 * 4 );
		DWORD* DstPal = (DWORD*)Compose;
		UploadBuf = Mip->DataPtr;
		InternalFormat = GL_COLOR_INDEX8_EXT;
		UploadFormat = GL_COLOR_INDEX8_EXT;
		for( i = 0; i < 256; ++i )
		{
			const BYTE Alpha = bLegacyEffectSprite
				? UE1OpenGLPaletteEffectAlpha( Info, (BYTE)i, Masked, bMaineffect, bSmokeBlack, bHubSmoke1, bHubWaterRings2 )
				: ( ( Masked && i == 0 ) ? 0 : 255 );
			*DstPal++ = UE1OpenGLPaletteDWORDWithAlpha( Info, (BYTE)i, Alpha );
		}
		glColorTableEXT( GL_TEXTURE_2D, GL_RGBA8, 256, GL_RGBA, GL_UNSIGNED_BYTE, (const void*)Compose );
	}
	else
	{
		// No support for palettized textures. Expand to RGBA8888 and fix alpha.
		const BYTE* Src = (const BYTE*)Mip->DataPtr;
		const DWORD* Pal = (const DWORD*)Info.Palette;
		const DWORD Count = Mip->USize * Mip->VSize;
		EnsureComposeSize( Count * 4 );
		UploadBuf = Compose;
		UploadFormat = GL_RGBA;
		InternalFormat = GL_RGBA8;

		if( bLegacyEffectSprite )
		{
			BYTE* Dst = (BYTE*)Compose;
			for( i = 0; i < Count; ++i, ++Src )
			{
				const BYTE Index = *Src;
				FColor Color = Info.Palette[Index];
				*Dst++ = Color.R;
				*Dst++ = Color.G;
				*Dst++ = Color.B;
				*Dst++ = UE1OpenGLPaletteEffectAlpha( Info, Index, Masked, bMaineffect, bSmokeBlack, bHubSmoke1, bHubWaterRings2 );
			}
		}
		else if( Masked )
		{
			DWORD* Dst = (DWORD*)Compose;
			// index 0 is transparent
#if __INTEL_BYTE_ORDER__
			for( i = 0; i < Count; ++i, ++Src )
				*Dst++ = *Src ? ( Pal[*Src] | ALPHA_MASK ) : 0;
#else
			for( i = 0; i < Count; ++i, ++Src )
			{
				FColor Color = Info.Palette[*Src];
				Color.A = *Src ? 255 : 0;
				*Dst++ = (Color.R << 24) | (Color.G << 16) | (Color.B << 8) | Color.A;
			}
#endif
		}
		else
		{
			DWORD* Dst = (DWORD*)Compose;
			// index 0 is whatever
#if __INTEL_BYTE_ORDER__
			for( i = 0; i < Count; ++i )
				*Dst++ = ( Pal[*Src++] | ALPHA_MASK );
#else
			for( i = 0; i < Count; ++i, ++Src )
			{
				FColor Color = Info.Palette[*Src];
				Color.A = 255;
				*Dst++ = (Color.R << 24) | (Color.G << 16) | (Color.B << 8) | Color.A;
			}
#endif
		}
	}
}

void UNOpenGLRenderDevice::ConvertTextureMipBGRA7777( const FMipmap* Mip, BYTE*& UploadBuf, GLenum& UploadFormat, GLenum& InternalFormat )
{
	// BGRA8888. This is actually a BGRA7777 lightmap, so we need to scale it.
	const BYTE* Src = (const BYTE*)Mip->DataPtr;
	const DWORD Count = Mip->USize * Mip->VSize;
	EnsureComposeSize( Count * 4 );
	BYTE* Dst = (BYTE*)Compose;
	UploadBuf = Compose;
	InternalFormat = GL_RGBA8;
	if( UseBGRA )
	{
		UploadFormat = GL_BGRA;
		for( DWORD i = 0; i < Count; ++i )
		{
			*Dst++ = (*Src++) << 1;
			*Dst++ = (*Src++) << 1;
			*Dst++ = (*Src++) << 1;
			*Dst++ = (*Src++) << 1;
		}
	}
	else
	{
		// Swap BGRA -> RGBA
		UploadFormat = GL_RGBA;
		for( DWORD i = 0; i < Count; ++i, Src += 4 )
		{
			*Dst++ = Src[2] << 1;
			*Dst++ = Src[1] << 1;
			*Dst++ = Src[0] << 1;
			*Dst++ = Src[3] << 1;
		}
	}
}

void UNOpenGLRenderDevice::UploadTexture( FTextureInfo& Info, UBOOL Masked, UBOOL NewTexture, INT BaseMip )
{
	guard(UNOpenGLRenderDevice::UploadTexture);

	if( BaseMip < 0 || BaseMip >= Info.NumMips || !Info.Mips[BaseMip] )
		BaseMip = 0;

	if( !Info.Mips[BaseMip] )
	{
		debugf( NAME_Warning, "Encountered texture with invalid mips!" );
		return;
	}

	// Upload all mips. In LOW texture detail mode, source mip 1 becomes GL level 0.
	uclock(ImageCycles);
	for( INT SourceMipIndex = BaseMip, UploadMipIndex = 0; SourceMipIndex < Info.NumMips; ++SourceMipIndex, ++UploadMipIndex )
	{
		const FMipmap* Mip = Info.Mips[SourceMipIndex];
		BYTE* UploadBuf;
		GLenum UploadFormat;
		GLenum InternalFormat;
		if( !Mip || !Mip->DataPtr )
			break;
		// Convert texture if needed.
		if( Info.Palette )
			ConvertTextureMipI8( Info, Mip, Masked, UploadBuf, UploadFormat, InternalFormat );
		else
			ConvertTextureMipBGRA7777( Mip, UploadBuf, UploadFormat, InternalFormat );
		// Upload to GL.
		if( NewTexture )
			glTexImage2D( GL_TEXTURE_2D, UploadMipIndex, InternalFormat, Mip->USize, Mip->VSize, 0, UploadFormat, GL_UNSIGNED_BYTE, (void*)UploadBuf );
		else
			glTexSubImage2D( GL_TEXTURE_2D, UploadMipIndex, 0, 0, Mip->USize, Mip->VSize, UploadFormat, GL_UNSIGNED_BYTE, (void*)UploadBuf );
	}
	uunclock(ImageCycles);

	unguard;
}
void UNOpenGLRenderDevice::UpdateSwapInterval()
{
	guard(UNOpenGLRenderDevice::UpdateSwapInterval);

	if( SwapInterval < -1 )
	{
		SwapInterval = -1;
	}

	if( SDL_GL_SetSwapInterval( SwapInterval ) < 0 )
	{
		debugf( NAME_Warning, "Failed to set swap interval %d: %s", SwapInterval, SDL_GetError() );
		if( SwapInterval < 0 )
		{
			// Adaptive VSync not supported, try normal VSync.
			SwapInterval = 1;
	RuntimeLowDetailTextures = false; // UE1_WINDOWS_TEXTURE_DETAIL_V16
			RuntimeLowDetailTextures = GetConfiguredLowDetailTextures(); // UE1_WINDOWS_TEXTURE_DETAIL_V16_INIT
	UpdateSwapInterval();
		}
	}

	unguard;
}
