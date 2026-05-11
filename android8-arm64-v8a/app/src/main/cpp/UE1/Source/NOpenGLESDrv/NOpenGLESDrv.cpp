#include "SDL2/SDL.h"
#include "glad.h"
#include "glm/matrix.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
#include <android/log.h>
#endif

#include "NOpenGLESDrvPrivate.h"

/*-----------------------------------------------------------------------------
	GLSL shaders.
-----------------------------------------------------------------------------*/

static const char *FragShaderGLSL {
#include "FragmentShader.glsl.inc"
};

static const char *VertShaderGLSL {
#include "VertexShader.glsl.inc"
};

/*-----------------------------------------------------------------------------
	Global implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_PACKAGE(NOpenGLESDrv);
IMPLEMENT_CLASS(UNOpenGLESRenderDevice);

/*-----------------------------------------------------------------------------
	UNOpenGLESRenderDevice implementation.
-----------------------------------------------------------------------------*/

// MV matrix that puts the coordinate system in order
static constexpr glm::mat4 MtxModelView {
	+1.f, +0.f, +0.f, +0.f,
	+0.f, -1.f, +0.f, +0.f,
	+0.f, +0.f, -1.f, +0.f,
	+0.f, +0.f, +0.f, +1.f,
};

// in floats
static constexpr DWORD AttribSizes[AT_Count] = {
	3, 2, 2, 2, 2, 4, 4
};

// from XOpenGLDrv:
// PF_Masked requires index 0 to be transparent, but is set on the polygon instead of the texture,
// so we potentially need two copies of any palettized texture in the cache
// unlike in newer unreal versions the low cache bits are actually used, so we have use one of the
// actually unused higher bits for this purpose, thereby breaking 64-bit compatibility for now
#define MASKED_TEXTURE_TAG (1ULL << 60ULL)
#define LOWDETAIL_TEXTURE_TAG (1ULL << 61ULL)

// FColor is adjusted for endianness
#define ALPHA_MASK 0xff000000

// Android/UE1 legacy sprite effect handling.
// The explosion/smoke sprites in UnrealI are palettized DrawTile effects.
// Their transparent/neutral border is not always palette index 0, so treating
// them exactly like normal wall/floor textures makes the full billboard quad
// visible on GLES. Keep this limited to the concrete effect packages seen in
// logcat so torches/coronas and normal level textures are not affected.
static const char* UE1GLESGetTextureObjectName( const FTextureInfo& Info, char* Buffer, INT BufferSize )
{
	appStrncpy( Buffer, "", BufferSize - 1 );

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

static UBOOL UE1GLESTextureNameStartsWith( const FTextureInfo& Info, const char* Prefix )
{
	char Name[512];
	UE1GLESGetTextureObjectName( Info, Name, sizeof(Name) );
	return Name[0] && appStrnicmp( Name, Prefix, appStrlen(Prefix) ) == 0;
}

static UBOOL UE1GLESConfigBool( const char* Section, const char* Key, UBOOL DefaultValue )
{
	char Value[64];
	if( !GConfigCache.GetString( Section, Key, Value, sizeof(Value) ) )
		return DefaultValue;

	if( !appStricmp( Value, "True" ) || !appStricmp( Value, "1" ) || !appStricmp( Value, "Yes" ) || !appStricmp( Value, "On" ) )
		return true;
	if( !appStricmp( Value, "False" ) || !appStricmp( Value, "0" ) || !appStricmp( Value, "No" ) || !appStricmp( Value, "Off" ) )
		return false;

	return DefaultValue;
}

static FLOAT UE1GLESConfigFloat( const char* Section, const char* Key, FLOAT DefaultValue )
{
	char Value[64];
	if( !GConfigCache.GetString( Section, Key, Value, sizeof(Value) ) )
		return DefaultValue;
	return appAtof( Value );
}

static INT UE1GLESConfigInt( const char* Section, const char* Key, INT DefaultValue )
{
	char Value[64];
	if( !GConfigCache.GetString( Section, Key, Value, sizeof(Value) ) )
		return DefaultValue;
	return appAtoi( Value );
}

static void UE1GLESAndroidLogResolutionV71( const char* Text )
{
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "%s", Text );
#endif
	debugf( NAME_Log, "%s", Text );
}

static INT UE1GLESAndroidResolutionModeV71()
{
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	return UE1GLESConfigInt( "NSDLDrv.NSDLClient", "AndroidResolutionMode", 0 );
#else
	return 0;
#endif
}

static UBOOL UE1GLESAndroidFixedRenderSizeV71( INT& OutX, INT& OutY )
{
	const INT Mode = UE1GLESAndroidResolutionModeV71();
	if( Mode == 1 )
	{
		OutX = 1280;
		OutY = 720;
		return true;
	}
	if( Mode == 2 )
	{
		OutX = 1024;
		OutY = 768;
		return true;
	}
	OutX = 0;
	OutY = 0;
	return false;
}

static UBOOL UE1GLESIsMaineffectTexture( const FTextureInfo& Info )
{
	return UE1GLESTextureNameStartsWith( Info, "Texture UnrealI.Maineffect." );
}

static UBOOL UE1GLESIsSmokeBlackTexture( const FTextureInfo& Info )
{
	return UE1GLESTextureNameStartsWith( Info, "Texture UnrealI.SmokeBlack." );
}

static UBOOL UE1GLESIsHubWaterFall2Texture( const FTextureInfo& Info )
{
	return UE1GLESTextureNameStartsWith( Info, "FireTexture HubEffects.WaterFall2" );
}

static UBOOL UE1GLESIsHubSmoke1Texture( const FTextureInfo& Info )
{
	return UE1GLESTextureNameStartsWith( Info, "FireTexture HubEffects.Smoke1" );
}

static UBOOL UE1GLESIsHubWaterRings2Texture( const FTextureInfo& Info )
{
	return UE1GLESTextureNameStartsWith( Info, "WaveTexture HubEffects.WaterRings2" );
}


static INT UE1GLESWaterRings2Luma( const FTextureInfo& Info, const BYTE* Data, INT USize, INT VSize, INT X, INT Y )
{
	if( X < 0 ) X = 0;
	else if( X >= USize ) X = USize - 1;
	if( Y < 0 ) Y = 0;
	else if( Y >= VSize ) Y = VSize - 1;

	const BYTE Index = Data[Y * USize + X];
	const FColor C = Info.Palette[Index];
	return ( (INT)C.R + (INT)C.G + (INT)C.B ) / 3;
}

static UBOOL UE1GLESIsHubSplashOverlayTexture( const FTextureInfo& Info )
{
	// Keep WetTexture HubEffects.WaterRings OUT of this list: that texture is
	// used as the actual pool/water surface. Masking it removes the visible
	// water texture and leaves only a flat cyan base.
	//
	// Also keep FireTexture HubEffects.WaterFall2 OUT: the vertical waterfall
	// itself needs the original translucent FireTexture colour data. Masking
	// dark/green texels made it degenerate into plain cyan/blue streaks. The
	// regular PF_Translucent blend already treats black source pixels as neutral
	// enough for this texture.
	return UE1GLESIsHubSmoke1Texture( Info );
}

static UBOOL UE1GLESNeedsHubSplashMask( const FTextureInfo& Info )
{
	// Smoke1 is a translucent/fire overlay with black borders and still needs
	// alpha test. WaterFall2 is the vertical waterfall texture and must not be
	// alpha-masked. WaterRings2 is PF_Modulated and has a dedicated blend fix in
	// DrawComplexSurface.
	return UE1GLESIsHubSplashOverlayTexture( Info );
}

static UBOOL UE1GLESIsLegacyEffectSpriteTexture( const FTextureInfo& Info )
{
	return UE1GLESIsMaineffectTexture( Info )
		|| UE1GLESIsSmokeBlackTexture( Info )
		|| UE1GLESNeedsHubSplashMask( Info );
}

static BYTE UE1GLESPaletteEffectAlpha( const FTextureInfo& Info, BYTE Index, UBOOL Masked, UBOOL bMaineffect, UBOOL bSmokeBlack, UBOOL bHubWaterFall2, UBOOL bHubSmoke1, UBOOL bHubWaterRings2, BYTE WR2Corner0, BYTE WR2Corner1, BYTE WR2Corner2, BYTE WR2Corner3 )
{
	if( bMaineffect )
	{
		// Maineffect explosion frames use palette index 1 as the clear border in
		// the logged frames. UE1 software rendering effectively hides it; GLES
		// must do that explicitly.
		return Index > 1 ? 255 : 0;
	}

	if( bSmokeBlack )
	{
		// PF_Modulated smoke uses neutral grey as the non-effect area. In the
		// GL blend equation 128/128/128 is visually neutral; discard near-neutral
		// pixels so the 32x32 smoke billboard border does not show as a square.
		FColor C = Info.Palette[Index];
		INT DR = ( C.R > 128 ) ? ( C.R - 128 ) : ( 128 - C.R );
		INT DG = ( C.G > 128 ) ? ( C.G - 128 ) : ( 128 - C.G );
		INT DB = ( C.B > 128 ) ? ( C.B - 128 ) : ( 128 - C.B );
		INT D = Max( DR, Max( DG, DB ) );
		return D <= 10 ? 0 : 255;
	}

	if( bHubWaterFall2 || bHubSmoke1 )
	{
		// Hub splash/fall FireTextures are DrawComplexSurface translucent
		// surfaces, not DrawTile sprites. Their empty area is black/very dark
		// green rather than only palette index 0. Keep the actual brighter water
		// streaks, but discard the dark colour-key border that forms the rectangle.
		FColor C = Info.Palette[Index];
		INT MaxC = Max( C.R, Max( C.G, C.B ) );
		return ( Index <= 1 || MaxC <= 24 ) ? 0 : 255;
	}

	if( bHubWaterRings2 )
	{
		// WaterRings2 is PF_Modulated and must not be alpha-discarded: the actual
		// ripple is encoded as subtle brightness deviations. Keep all pixels and
		// make the neutral background invisible by colour remapping during upload.
		return 255;
	}

	if( Masked )
		return Index ? 255 : 0;

	return 255;
}

// lightmaps are 0-127
#define LIGHTMAP_SCALE 2

// and it also would be nice to overbright them
#define LIGHTMAP_OVERBRIGHT 1.4f

// max vertices in a single draw call
#define MAX_VERTS 32768

void UNOpenGLESRenderDevice::InternalClassInitializer( UClass* Class )
{
	guardSlow(UNOpenGLESRenderDevice::InternalClassInitializer);
	new(Class, "NoFiltering",    RF_Public)UBoolProperty( CPP_PROPERTY(NoFiltering),    "Options", CPF_Config );
	new(Class, "Overbright",     RF_Public)UBoolProperty( CPP_PROPERTY(Overbright),     "Options", CPF_Config );
	new(Class, "DetailTextures", RF_Public)UBoolProperty( CPP_PROPERTY(DetailTextures), "Options", CPF_Config );
	new(Class, "UseVAO",         RF_Public)UBoolProperty( CPP_PROPERTY(UseVAO),         "Options", CPF_Config );
	new(Class, "UseBGRA",        RF_Public)UBoolProperty( CPP_PROPERTY(UseBGRA),        "Options", CPF_Config );
	new(Class, "AutoFOV",        RF_Public)UBoolProperty( CPP_PROPERTY(AutoFOV),        "Options", CPF_Config );
	new(Class, "BrightnessScale",RF_Public)UFloatProperty( CPP_PROPERTY(BrightnessScale),"Options", CPF_Config );
	new(Class, "WorldGamma",    RF_Public)UFloatProperty( CPP_PROPERTY(WorldGamma),    "Options", CPF_Config );
	new(Class, "WorldShadowLift",RF_Public)UFloatProperty( CPP_PROPERTY(WorldShadowLift),"Options", CPF_Config );
	new(Class, "SwapInterval",   RF_Public)UIntProperty ( CPP_PROPERTY(SwapInterval),   "Options", CPF_Config );
	unguardSlow;
}

UNOpenGLESRenderDevice::UNOpenGLESRenderDevice()
{
	DetailTextures = true;
	Overbright = true;
	NoFiltering = false;
	UseVAO = false;
	UseBGRA = true;
	AutoFOV = true;
	// Keep the classic UE1 Brightness slider linear.  Android world-gamma is
	// intentionally separate so dark level geometry can be tuned without
	// changing sprite/effect/HUD brightness.
	BrightnessScale = 1.0f;
	WorldGamma = 1.0f;
	WorldShadowLift = 0.0f;
	RuntimeLowDetailTextures = false;
	CurrentBrightness = -1.f;
	CurrentBrightnessScale = -1.f;
	CurrentWorldGamma = -1.f;
	CurrentWorldShadowLift = -1.f;
	CurrentSceneNode.X = -1;
	CurrentSceneNode.Y = -1;
	CurrentSceneNode.XB = -1;
	CurrentSceneNode.YB = -1;
	CurrentSceneNode.SizeX = -1;
	CurrentSceneNode.SizeY = -1;
	CurrentSceneNode.DrawableX = -1;
	CurrentSceneNode.DrawableY = -1;
	CurrentSceneNode.FX = -1.f;
	CurrentSceneNode.FY = -1.f;
	CurrentSceneNode.FovAngle = -1.f; // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	AndroidSceneFBO = 0; // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	AndroidSceneColorTex = 0; // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	AndroidSceneDepthRB = 0; // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	AndroidSceneBlitProg = 0; // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	AndroidSceneBlitVS = 0; // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	AndroidSceneBlitFS = 0; // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	AndroidSceneBlitTextureLoc = -1; // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	AndroidSceneFBOSizeX = -1; // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	AndroidSceneFBOSizeY = -1; // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	AndroidSceneDrawableX = -1; // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	AndroidSceneDrawableY = -1; // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	AndroidSceneFBOActive = false; // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	SwapInterval = 1;
}

UBOOL UNOpenGLESRenderDevice::Init( UViewport* InViewport )
{
	guard(UNOpenGLESRenderDevice::Init)

	if( !gladLoadGLES2Loader( &SDL_GL_GetProcAddress ) )
	{
		debugf( NAME_Warning, "Could not load GLES2: %s", SDL_GetError() );
		return false;
	}

	debugf( NAME_Log, "Got OpenGL %s", glGetString( GL_VERSION ) );

	NoVolumetricBlend = true;
	SupportsFogMaps = true;
	SupportsDistanceFog = true;

	if( BrightnessScale < 0.25f )
		BrightnessScale = 0.25f;
	else if( BrightnessScale > 4.0f )
		BrightnessScale = 4.0f;
	if( WorldGamma < 0.5f )
		WorldGamma = 0.5f;
	else if( WorldGamma > 3.0f )
		WorldGamma = 3.0f;
	if( WorldShadowLift < 0.0f )
		WorldShadowLift = 0.0f;
	else if( WorldShadowLift > 0.35f )
		WorldShadowLift = 0.35f;

	RuntimeLowDetailTextures = GetConfiguredLowDetailTextures();

	UpdateSwapInterval();

	ComposeSize = 256 * 256 * 4;
	Compose = (BYTE*)appMalloc( ComposeSize, "GLComposeBuf" );
	verify( Compose );

	VtxDataSize = 18 * MAX_VERTS; // should be enough for all attributes
	VtxData = (FLOAT*)appMalloc( VtxDataSize * sizeof(FLOAT), "GLVtxDataBuf" );
	verify( VtxData );
	VtxDataEnd = VtxData + VtxDataSize;
	VtxDataPtr = VtxData;
	VtxPolyStart = VtxData;

	IdxDataSize = MAX_VERTS;
	IdxData = (GLushort*)appMalloc( IdxDataSize * sizeof(GLushort), "GLIdxDataBuf" );
	verify( IdxData );
	IdxDataEnd = IdxData + IdxDataSize;
	IdxDataPtr = IdxData;
	IdxCount = 0;

	if( UseVAO )
	{
		glGenBuffers( 1, &GLBuf );
		glBindBuffer( GL_ARRAY_BUFFER, GLBuf );
		glBufferData( GL_ARRAY_BUFFER, VtxDataSize * sizeof(FLOAT), (void*)VtxData, GL_DYNAMIC_DRAW );
	}

	if( UseBGRA )
	{
		// check if BGRA is actually supported
		if( !( GLAD_GL_APPLE_texture_format_BGRA8888 || GLAD_GL_EXT_texture_format_BGRA8888 || GLAD_GL_MESA_bgra ) )
		{
			debugf( "GLES2: BGRA8888 enabled, but not supported; disabling" );
			UseBGRA = false;
		}
		else
		{
			debugf( "GLES2: BGRA8888 supported" );
		}
	}

	// Set permanent state.
	glEnable( GL_DEPTH_TEST );
	glDepthMask( GL_TRUE );
	glBlendFunc( GL_ONE, GL_ZERO );
	glEnable( GL_BLEND );
	glEnableVertexAttribArray( 0 );

	// Precache some common shaders
	static const DWORD PrecacheShaders[] = {
		SF_VtxColor,
		SF_Texture0,
		SF_Texture0 | SF_VtxColor,
		SF_Texture0 | SF_AlphaTest,
		SF_Texture0 | SF_VtxColor | SF_AlphaTest,
		SF_Texture0 | SF_VtxColor | SF_VtxFog,
		SF_Texture0 | SF_VtxColor | SF_VtxFog | SF_AlphaTest,
		SF_Texture0 | SF_Texture1 | SF_Lightmap,
		SF_Texture0 | SF_Texture1 | SF_Lightmap | SF_AlphaTest,
		SF_Texture0 | SF_Texture1 | SF_Texture2 | SF_Lightmap | SF_Fogmap,
		SF_Texture0 | SF_Texture1 | SF_Texture2 | SF_Lightmap | SF_Fogmap | SF_AlphaTest,
	};
	for( DWORD i = 0; i < ARRAY_COUNT( PrecacheShaders ); ++i )
		CreateShader( PrecacheShaders[i] );

	CurrentPolyFlags = PF_Occlude;
	CurrentShaderFlags = 0;
	CurrentBrightness = -1.f;
	CurrentBrightnessScale = -1.f;
	CurrentWorldGamma = -1.f;
	CurrentWorldShadowLift = -1.f;
	Viewport = InViewport;
	RuntimeLowDetailTextures = GetConfiguredLowDetailTextures();

	return true;
	unguard;
}

void UNOpenGLESRenderDevice::Exit()
{
	guard(UNOpenGLESRenderDevice::Exit);

	debugf( NAME_Log, "Shutting down OpenGL ES2 renderer" );

	Flush();

	ReleaseAndroidSceneFBO(); // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	if( AndroidSceneBlitProg )
	{
		glDeleteProgram( AndroidSceneBlitProg );
		AndroidSceneBlitProg = 0;
	}

	if( Compose )
	{
		appFree( Compose );
		Compose = NULL;
	}
	ComposeSize = 0;

	unguard;
}

void UNOpenGLESRenderDevice::PostEditChange()
{
	guard(UNOpenGLESRenderDevice::PostEditChange)

	Super::PostEditChange();

	if( BrightnessScale < 0.25f )
		BrightnessScale = 0.25f;
	else if( BrightnessScale > 4.0f )
		BrightnessScale = 4.0f;
	if( WorldGamma < 0.5f )
		WorldGamma = 0.5f;
	else if( WorldGamma > 3.0f )
		WorldGamma = 3.0f;
	if( WorldShadowLift < 0.0f )
		WorldShadowLift = 0.0f;
	else if( WorldShadowLift > 0.35f )
		WorldShadowLift = 0.35f;

	UpdateSwapInterval();

	unguard;
}

void UNOpenGLESRenderDevice::Flush()
{
	guard(UNOpenGLESRenderDevice::Flush);

	if( TexAlloc.Num() )
	{
		debugf( NAME_Log, "Flushing %d/%d textures", TexAlloc.Num(), BindMap.Size() );
		ResetTexture( 0 );
		ResetTexture( 1 );
		ResetTexture( 2 );
		ResetTexture( 3 );
		glFinish();
		glDeleteTextures( TexAlloc.Num(), &TexAlloc(0) );
		TexAlloc.Empty();
		BindMap.Empty();
	}

	unguard;
}

UBOOL UNOpenGLESRenderDevice::Exec( const char* Cmd, FOutputDevice* Out )
{
	return false;
}

void UNOpenGLESRenderDevice::GetAndroidDrawableSize( INT& OutX, INT& OutY )
{
	guard(UNOpenGLESRenderDevice::GetAndroidDrawableSize);

	OutX = Viewport ? Viewport->SizeX : 0;
	OutY = Viewport ? Viewport->SizeY : 0;
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	if( Viewport )
	{
		SDL_Window* AndroidWindow = (SDL_Window*)Viewport->GetWindow();
		if( AndroidWindow )
		{
			int AndroidDrawW = 0, AndroidDrawH = 0;
			SDL_GL_GetDrawableSize( AndroidWindow, &AndroidDrawW, &AndroidDrawH );
			if( AndroidDrawW > 0 && AndroidDrawH > 0 )
			{
				OutX = AndroidDrawW;
				OutY = AndroidDrawH;
			}
		}
	}
#endif

	unguard;
}

UBOOL UNOpenGLESRenderDevice::ShouldUseAndroidSceneFBO()
{
	guard(UNOpenGLESRenderDevice::ShouldUseAndroidSceneFBO);

#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	if( !Viewport || Viewport->SizeX <= 0 || Viewport->SizeY <= 0 )
		return false;

	INT FixedX = 0, FixedY = 0;
	if( UE1GLESAndroidFixedRenderSizeV71( FixedX, FixedY ) )
		return FixedX > 0 && FixedY > 0;

	return false;
#else
	return false;
#endif

	unguard;
}

GLuint UNOpenGLESRenderDevice::CompileAndroidSceneBlitShader( GLenum Type, const char* Text )
{
	guard(UNOpenGLESRenderDevice::CompileAndroidSceneBlitShader);

	GLuint Id = glCreateShader( Type );
	const char* Src[] = { Text, NULL };
	glShaderSource( Id, 1, Src, NULL );
	glCompileShader( Id );

	GLint Status = 0;
	glGetShaderiv( Id, GL_COMPILE_STATUS, &Status );
	if( !Status )
	{
		char Tmp[2048] = { 0 };
		glGetShaderInfoLog( Id, sizeof(Tmp), NULL, Tmp );
		appErrorf( "Android resolution blit %s shader compilation failed:\n%s", ( Type == GL_FRAGMENT_SHADER ) ? "fragment" : "vertex", Tmp );
	}

	return Id;
	unguard;
}

void UNOpenGLESRenderDevice::EnsureAndroidSceneBlitProgram()
{
	guard(UNOpenGLESRenderDevice::EnsureAndroidSceneBlitProgram);

	if( AndroidSceneBlitProg )
		return;

	static const char* BlitVS =
		"#version 100\n"
		"attribute mediump vec2 aPosition;\n"
		"attribute mediump vec2 aTexCoord;\n"
		"varying mediump vec2 vTexCoord;\n"
		"void main()\n"
		"{\n"
		"\tvTexCoord = aTexCoord;\n"
		"\tgl_Position = vec4(aPosition, 0.0, 1.0);\n"
		"}\n";
	static const char* BlitFS =
		"#version 100\n"
		"varying mediump vec2 vTexCoord;\n"
		"uniform sampler2D uTexture;\n"
		"void main()\n"
		"{\n"
		"\tgl_FragColor = texture2D(uTexture, vTexCoord);\n"
		"}\n";

	AndroidSceneBlitVS = CompileAndroidSceneBlitShader( GL_VERTEX_SHADER, BlitVS );
	AndroidSceneBlitFS = CompileAndroidSceneBlitShader( GL_FRAGMENT_SHADER, BlitFS );
	AndroidSceneBlitProg = glCreateProgram();
	glAttachShader( AndroidSceneBlitProg, AndroidSceneBlitVS );
	glAttachShader( AndroidSceneBlitProg, AndroidSceneBlitFS );
	glBindAttribLocation( AndroidSceneBlitProg, 0, "aPosition" );
	glBindAttribLocation( AndroidSceneBlitProg, 1, "aTexCoord" );
	glLinkProgram( AndroidSceneBlitProg );

	GLint Status = 0;
	glGetProgramiv( AndroidSceneBlitProg, GL_LINK_STATUS, &Status );
	if( !Status )
	{
		char Tmp[2048] = { 0 };
		glGetProgramInfoLog( AndroidSceneBlitProg, sizeof(Tmp), NULL, Tmp );
		appErrorf( "Android resolution blit shader link failed:\n%s", Tmp );
	}

	AndroidSceneBlitTextureLoc = glGetUniformLocation( AndroidSceneBlitProg, "uTexture" );
	glDeleteShader( AndroidSceneBlitVS );
	glDeleteShader( AndroidSceneBlitFS );
	AndroidSceneBlitVS = 0;
	AndroidSceneBlitFS = 0;

	unguard;
}

void UNOpenGLESRenderDevice::ReleaseAndroidSceneFBO()
{
	guard(UNOpenGLESRenderDevice::ReleaseAndroidSceneFBO);

	if( AndroidSceneFBO )
	{
		glDeleteFramebuffers( 1, &AndroidSceneFBO );
		AndroidSceneFBO = 0;
	}
	if( AndroidSceneColorTex )
	{
		glDeleteTextures( 1, &AndroidSceneColorTex );
		AndroidSceneColorTex = 0;
	}
	if( AndroidSceneDepthRB )
	{
		glDeleteRenderbuffers( 1, &AndroidSceneDepthRB );
		AndroidSceneDepthRB = 0;
	}

	AndroidSceneFBOSizeX = -1;
	AndroidSceneFBOSizeY = -1;
	AndroidSceneDrawableX = -1;
	AndroidSceneDrawableY = -1;
	AndroidSceneFBOActive = false;
	CurrentSceneNode.X = -1;
	CurrentSceneNode.FX = -1.f;

	unguard;
}

UBOOL UNOpenGLESRenderDevice::EnsureAndroidSceneFBO()
{
	guard(UNOpenGLESRenderDevice::EnsureAndroidSceneFBO);

	if( !ShouldUseAndroidSceneFBO() )
	{
		if( AndroidSceneFBO || AndroidSceneColorTex || AndroidSceneDepthRB )
			ReleaseAndroidSceneFBO();
		AndroidSceneFBOActive = false;
		return false;
	}

	INT DrawableX = 0, DrawableY = 0;
	GetAndroidDrawableSize( DrawableX, DrawableY );
	INT RenderX = Viewport->SizeX;
	INT RenderY = Viewport->SizeY;
	INT FixedX = 0, FixedY = 0;
	if( UE1GLESAndroidFixedRenderSizeV71( FixedX, FixedY ) )
	{
		RenderX = FixedX;
		RenderY = FixedY;
	}
	if( DrawableX <= 0 || DrawableY <= 0 || RenderX <= 0 || RenderY <= 0 )
		return false;

	if( AndroidSceneFBO && AndroidSceneColorTex && AndroidSceneDepthRB &&
		AndroidSceneFBOSizeX == RenderX && AndroidSceneFBOSizeY == RenderY &&
		AndroidSceneDrawableX == DrawableX && AndroidSceneDrawableY == DrawableY )
	{
		AndroidSceneFBOActive = true;
		return true;
	}

	ReleaseAndroidSceneFBO();
	EnsureAndroidSceneBlitProgram();

	glGenTextures( 1, &AndroidSceneColorTex );
	glBindTexture( GL_TEXTURE_2D, AndroidSceneColorTex );
	// UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	// Fixed Android modes render into this real low-res colour target and are then
	// stretched fullscreen to the Android drawable.  Use nearest so 1280x720 and
	// 1024x768 are visibly different from Native and avoid extra filtering around
	// old modulated overlays such as WaterRings2.
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, RenderX, RenderY, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );

	glGenRenderbuffers( 1, &AndroidSceneDepthRB );
	glBindRenderbuffer( GL_RENDERBUFFER, AndroidSceneDepthRB );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, RenderX, RenderY );

	glGenFramebuffers( 1, &AndroidSceneFBO );
	glBindFramebuffer( GL_FRAMEBUFFER, AndroidSceneFBO );
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, AndroidSceneColorTex, 0 );
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, AndroidSceneDepthRB );

	GLenum Status = glCheckFramebufferStatus( GL_FRAMEBUFFER );
	if( Status != GL_FRAMEBUFFER_COMPLETE )
	{
		char WarnLine[256];
		appSprintf( WarnLine, "UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71 framebuffer incomplete: 0x%04x; using native backbuffer", (INT)Status );
		UE1GLESAndroidLogResolutionV71( WarnLine );
		ReleaseAndroidSceneFBO();
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		return false;
	}

	AndroidSceneFBOSizeX = RenderX;
	AndroidSceneFBOSizeY = RenderY;
	AndroidSceneDrawableX = DrawableX;
	AndroidSceneDrawableY = DrawableY;
	AndroidSceneFBOActive = true;
	TexInfo[0].CurrentCacheID = 0;
	TexInfo[1].CurrentCacheID = 0;
	TexInfo[2].CurrentCacheID = 0;
	TexInfo[3].CurrentCacheID = 0;
	ShaderInfo = NULL;
	CurrentSceneNode.X = -1;
	CurrentSceneNode.FX = -1.f;

	char LogLine[256];
	appSprintf( LogLine, "UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71 enabled: mode=%i render=%ix%i viewport=%ix%i drawable=%ix%i", UE1GLESAndroidResolutionModeV71(), AndroidSceneFBOSizeX, AndroidSceneFBOSizeY, Viewport->SizeX, Viewport->SizeY, AndroidSceneDrawableX, AndroidSceneDrawableY );
	UE1GLESAndroidLogResolutionV71( LogLine );

	return true;
	unguard;
}

void UNOpenGLESRenderDevice::BindAndroidSceneFBO()
{
	guard(UNOpenGLESRenderDevice::BindAndroidSceneFBO);

	if( EnsureAndroidSceneFBO() )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, AndroidSceneFBO );
		glViewport( 0, 0, AndroidSceneFBOSizeX, AndroidSceneFBOSizeY );
	}
	else
	{
		AndroidSceneFBOActive = false;
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
		INT DrawableX = Viewport ? Viewport->SizeX : 0;
		INT DrawableY = Viewport ? Viewport->SizeY : 0;
		GetAndroidDrawableSize( DrawableX, DrawableY );
		if( DrawableX > 0 && DrawableY > 0 )
			glViewport( 0, 0, DrawableX, DrawableY );
	}

	CurrentSceneNode.X = -1;
	CurrentSceneNode.FX = -1.f;

	unguard;
}

void UNOpenGLESRenderDevice::PresentAndroidSceneFBO()
{
	guard(UNOpenGLESRenderDevice::PresentAndroidSceneFBO);

	if( !AndroidSceneFBOActive || !AndroidSceneColorTex )
		return;

	INT DrawableX = 0, DrawableY = 0;
	GetAndroidDrawableSize( DrawableX, DrawableY );
	if( DrawableX <= 0 || DrawableY <= 0 )
		return;

	static INT LastPresentModeV71 = -999;
	static INT LastPresentRenderXV71 = -1;
	static INT LastPresentRenderYV71 = -1;
	static INT LastPresentDrawableXV71 = -1;
	static INT LastPresentDrawableYV71 = -1;
	if( LastPresentModeV71 != UE1GLESAndroidResolutionModeV71() ||
		LastPresentRenderXV71 != AndroidSceneFBOSizeX || LastPresentRenderYV71 != AndroidSceneFBOSizeY ||
		LastPresentDrawableXV71 != DrawableX || LastPresentDrawableYV71 != DrawableY )
	{
		char LogLine[256];
		appSprintf( LogLine, "UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71 present: mode=%i render=%ix%i viewport=%ix%i drawable=%ix%i", UE1GLESAndroidResolutionModeV71(), AndroidSceneFBOSizeX, AndroidSceneFBOSizeY, Viewport ? Viewport->SizeX : 0, Viewport ? Viewport->SizeY : 0, DrawableX, DrawableY );
		UE1GLESAndroidLogResolutionV71( LogLine );
		LastPresentModeV71 = UE1GLESAndroidResolutionModeV71();
		LastPresentRenderXV71 = AndroidSceneFBOSizeX;
		LastPresentRenderYV71 = AndroidSceneFBOSizeY;
		LastPresentDrawableXV71 = DrawableX;
		LastPresentDrawableYV71 = DrawableY;
	}

	EnsureAndroidSceneBlitProgram();

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glViewport( 0, 0, DrawableX, DrawableY );
	glDisable( GL_DEPTH_TEST );
	glDepthMask( GL_FALSE );
	glDisable( GL_BLEND );
	glBlendFunc( GL_ONE, GL_ZERO );
	glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, AndroidSceneColorTex );
	if( UseVAO )
		glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glUseProgram( AndroidSceneBlitProg );
	if( AndroidSceneBlitTextureLoc >= 0 )
		glUniform1i( AndroidSceneBlitTextureLoc, 0 );

	static const GLfloat BlitVerts[] = {
		-1.f, -1.f, 0.f, 0.f,
		+1.f, -1.f, 1.f, 0.f,
		-1.f, +1.f, 0.f, 1.f,
		+1.f, +1.f, 1.f, 1.f,
	};
	glEnableVertexAttribArray( 0 );
	glEnableVertexAttribArray( 1 );
	glVertexAttribPointer( 0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), BlitVerts );
	glVertexAttribPointer( 1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), BlitVerts + 2 );
	glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );

	// The fullscreen blit uses its own GL state. Force the regular UE1 state cache
	// to rebuild cleanly on the next frame. // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	TexInfo[0].CurrentCacheID = 0;
	TexInfo[1].CurrentCacheID = 0;
	TexInfo[2].CurrentCacheID = 0;
	TexInfo[3].CurrentCacheID = 0;
	CurrentShaderFlags = 0;
	CurrentPolyFlags = 0xffffffff;
	ShaderInfo = NULL;
	CurrentSceneNode.X = -1;
	CurrentSceneNode.FX = -1.f;

	if( UseVAO )
		glBindBuffer( GL_ARRAY_BUFFER, GLBuf );
	glDepthMask( GL_TRUE );
	glEnable( GL_DEPTH_TEST );

	unguard;
}

void UNOpenGLESRenderDevice::Lock( FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* InHitData, INT* InHitSize )
{
	guard(UNOpenGLESRenderDevice::Lock);

	glClearColor( ScreenClear.X, ScreenClear.Y, ScreenClear.Z, ScreenClear.W );
	glClearDepthf( 1.f );
	glDepthFunc( GL_LEQUAL );

	BindAndroidSceneFBO(); // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71

	UpdateRuntimeConfig();

	FLOAT TargetBrightness = GetConfiguredBrightness();

	if( CurrentBrightness != TargetBrightness )
	{
		CurrentBrightness = TargetBrightness;
		UniformsChanged[UF_Brightness] = true;
	}

	if( CurrentBrightnessScale != BrightnessScale )
	{
		CurrentBrightnessScale = BrightnessScale;
		UniformsChanged[UF_BrightnessScale] = true;
	}

	if( CurrentWorldGamma != WorldGamma )
	{
		CurrentWorldGamma = WorldGamma;
		UniformsChanged[UF_WorldGamma] = true;
	}

	if( CurrentWorldShadowLift != WorldShadowLift )
	{
		CurrentWorldShadowLift = WorldShadowLift;
		UniformsChanged[UF_WorldShadowLift] = true;
	}

	SetBlend( PF_Occlude );
	SetShader( CurrentShaderFlags );

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

void UNOpenGLESRenderDevice::Unlock( UBOOL Blit )
{
	guard(UNOpenGLESRenderDevice::Unlock);

	FlushTriangles();

	if( Blit )
		PresentAndroidSceneFBO(); // UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
	else if( AndroidSceneFBOActive )
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	glFlush();

	unguard;
}

void UNOpenGLESRenderDevice::DrawComplexSurface( FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet )
{
	guard(UNOpenGLESRenderDevice::DrawComplexSurface);

	check(Surface.Texture);

	DWORD RenderPolyFlags = Surface.PolyFlags;
	const UBOOL bHubWaterRings2 = UE1GLESIsHubWaterRings2Texture( *Surface.Texture );
	if( UE1GLESNeedsHubSplashMask( *Surface.Texture ) )
		RenderPolyFlags |= PF_Masked | PF_NoSmooth;

	SetSceneNode( Frame );
	SetBlend( RenderPolyFlags );
	if( bHubWaterRings2 && ( RenderPolyFlags & PF_Modulated ) )
	{
		// WaterRings2 is the legitimate ripple overlay at the waterfall impact.
		// Its white/near-white background should be neutral, while darker
		// texels draw the rings. The generic UE1 GLES PF_Modulated blend
		// uses GL_DST_COLOR,GL_SRC_COLOR, where 50% grey is neutral and
		// white brightens the whole polygon into a rectangle. For this one
		// texture use classic multiplicative modulation: dst = dst * src.
		FlushTriangles();
		glEnable( GL_BLEND );
		glBlendFunc( GL_ZERO, GL_SRC_COLOR );
	}
	SetTexture( 0, *Surface.Texture, ( RenderPolyFlags & PF_Masked ), 0.f );
	if( Surface.LightMap )
	{
		SetTexture( 1, *Surface.LightMap, 0, -0.5f );
		CurrentShaderFlags |= SF_Lightmap;
	}
	if( Surface.FogMap )
	{
		SetTexture( 2, *Surface.FogMap, 0, -0.5f );
		CurrentShaderFlags |= SF_Fogmap;
	}
	if( Surface.DetailTexture && DetailTextures && !RuntimeLowDetailTextures )
	{
		SetTexture( 3, *Surface.DetailTexture, 0, 0.f );
		CurrentShaderFlags |= SF_Detail;
	}
	SetShader( CurrentShaderFlags );

	FLOAT UDot = Facet.MapCoords.XAxis | Facet.MapCoords.Origin;
	FLOAT VDot = Facet.MapCoords.YAxis | Facet.MapCoords.Origin;
	for( FSavedPoly* Poly = Facet.Polys; Poly; Poly = Poly->Next )
	{
		// Android/GLES safety: never submit degenerate polygons.
		// Some 4:3/legacy paths can hand us 0-2 point polys; drawing them may
		// reuse stale vertex data and show up as a diagonal line across the screen.
		if( !Poly || Poly->NumPts < 3 )
			continue;

		BeginPoly();
		for( INT i = 0; i < Poly->NumPts; i++ )
		{
			FLOAT U = Facet.MapCoords.XAxis | Poly->Pts[i]->Point;
			FLOAT V = Facet.MapCoords.YAxis | Poly->Pts[i]->Point;
			AttribFloat3( &Poly->Pts[i]->Point.X );
			AttribFloat2( (U-UDot-TexInfo[0].UPan)*TexInfo[0].UMult, (V-VDot-TexInfo[0].VPan)*TexInfo[0].VMult );
			if( Surface.LightMap )
				AttribFloat2( (U-UDot-TexInfo[1].UPan)*TexInfo[1].UMult, (V-VDot-TexInfo[1].VPan)*TexInfo[1].VMult );
			if( Surface.FogMap )
				AttribFloat2( (U-UDot-TexInfo[2].UPan)*TexInfo[2].UMult, (V-VDot-TexInfo[2].VPan)*TexInfo[2].VMult );
			if( Surface.DetailTexture && DetailTextures && !RuntimeLowDetailTextures )
				AttribFloat2( (U-UDot-TexInfo[3].UPan)*TexInfo[3].UMult, (V-VDot-TexInfo[3].VPan)*TexInfo[3].VMult );
			PolyVertex();
		}
		EndPoly();
	}

	CurrentShaderFlags &= ~( SF_Lightmap|SF_Fogmap|SF_Detail );

	ResetTexture( 1 );
	ResetTexture( 2 );
	ResetTexture( 3 );

	if( bHubWaterRings2 && ( RenderPolyFlags & PF_Modulated ) )
	{
		// Flush while the WaterRings2 multiplicative blend is active, then
		// immediately restore the normal UE1 PF_Modulated GL state. The old
		// workaround only invalidated CurrentPolyFlags and left
		// GL_ZERO,GL_SRC_COLOR live until the next SetBlend() call. Some paths
		// do not re-enter SetBlend immediately, which can leak this special
		// state into later world/HUD draws and corrupt the whole view when
		// looking down at the waterfall pool.
		FlushTriangles();
		CurrentPolyFlags = 0xffffffff;
		SetBlend( RenderPolyFlags );
	}

	unguard;
}

void UNOpenGLESRenderDevice::DrawGouraudPolygon( FSceneNode* Frame, FTextureInfo& Texture, FTransTexture** Pts, INT NumPts, DWORD PolyFlags, FSpanBuffer* SpanBuffer )
{
	guard(UNOpenGLESRenderDevice::DrawGouraudPolygon);

	// Android/GLES safety: never submit degenerate gouraud polygons.
	// With fewer than 3 vertices, GLES may consume stale buffered data and draw
	// a long diagonal artifact, especially on non-16:9 viewports.
	if( !Pts || NumPts < 3 )
		return;

	const UBOOL IsFog = ( ( PolyFlags & ( PF_RenderFog|PF_Translucent|PF_Modulated ) ) == PF_RenderFog );
	const UBOOL IsModulated = ( PolyFlags & PF_Modulated );
	if( !IsModulated )
		CurrentShaderFlags |= SF_VtxColor;
	if( IsFog )
		CurrentShaderFlags |= SF_VtxFog;

	DWORD RenderPolyFlags = PolyFlags;
	if( UE1GLESIsMaineffectTexture( Texture ) )
		RenderPolyFlags |= PF_Masked | PF_NoSmooth;
	else if( UE1GLESIsSmokeBlackTexture( Texture ) )
		RenderPolyFlags |= PF_Masked | PF_NoSmooth;

	SetSceneNode( Frame );
	SetBlend( RenderPolyFlags );
	SetTexture( 0, Texture, ( RenderPolyFlags & PF_Masked ), 0 );
	SetShader( CurrentShaderFlags );

	BeginPoly();
	for( INT i=0; i<NumPts; i++ )
	{
		FTransTexture* P = Pts[i];
		AttribFloat3( &P->Point.X );
		AttribFloat2( P->U*TexInfo[0].UMult, P->V*TexInfo[0].VMult );
		if( !IsModulated )
			AttribFloat4( P->Light.X, P->Light.Y, P->Light.Z, 1.f );
		if( IsFog )
			AttribFloat4( &P->Fog.X );
		PolyVertex();
	}
	EndPoly();

	CurrentShaderFlags &= ~( SF_VtxColor|SF_VtxFog );

	unguard;
}

void UNOpenGLESRenderDevice::DrawTile( FSceneNode* Frame, FTextureInfo& Texture, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, FSpanBuffer* Span, FLOAT Z, FPlane Light, FPlane Fog, DWORD PolyFlags )
{
	guard(UNOpenGLESRenderDevice::DrawTile);

	FPlane VtxColor;
	if( !( PolyFlags & PF_Modulated ) )
	{
		VtxColor.X = Light.X;
		VtxColor.Y = Light.Y;
		VtxColor.Z = Light.Z;
		VtxColor.W = 1.f;
	}
	else
	{
		VtxColor.X = 1.f;
		VtxColor.Y = 1.f;
		VtxColor.Z = 1.f;
		VtxColor.W = 1.f;
	}

	CurrentShaderFlags |= SF_VtxColor;

	DWORD RenderPolyFlags = PolyFlags;
	if( UE1GLESIsMaineffectTexture( Texture ) )
		RenderPolyFlags |= PF_Masked | PF_NoSmooth;
	else if( UE1GLESIsSmokeBlackTexture( Texture ) )
		RenderPolyFlags |= PF_Masked | PF_NoSmooth;

	SetSceneNode( Frame );
	SetBlend( RenderPolyFlags );
	SetTexture( 0, Texture, ( RenderPolyFlags & PF_Masked ), 0.f );
	SetShader( CurrentShaderFlags );

	BeginPoly();
		AttribFloat3( RFX2 * Z * (X - Frame->FX2), RFY2 * Z * (Y - Frame->FY2), Z );
		AttribFloat2( U * TexInfo[0].UMult, V * TexInfo[0].VMult );
		AttribFloat4( &VtxColor.X );
		PolyVertex();
		AttribFloat3( RFX2 * Z * (X + XL - Frame->FX2), RFY2 * Z * (Y - Frame->FY2), Z );
		AttribFloat2( (U + UL) * TexInfo[0].UMult, V * TexInfo[0].VMult );
		AttribFloat4( &VtxColor.X );
		PolyVertex();
		AttribFloat3( RFX2 * Z * (X + XL - Frame->FX2), RFY2 * Z * (Y + YL - Frame->FY2), Z );
		AttribFloat2( (U + UL) * TexInfo[0].UMult, (V + VL) *TexInfo[0].VMult );
		AttribFloat4( &VtxColor.X );
		PolyVertex();
		AttribFloat3( RFX2 * Z * (X - Frame->FX2), RFY2 * Z * (Y + YL - Frame->FY2), Z );
		AttribFloat2( U * TexInfo[0].UMult, (V + VL) * TexInfo[0].VMult );
		AttribFloat4( &VtxColor.X );
		PolyVertex();
	EndPoly();

	CurrentShaderFlags &= ~SF_VtxColor;

	unguard;
}

void UNOpenGLESRenderDevice::Draw2DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 )
{

}

void UNOpenGLESRenderDevice::Draw2DPoint( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2 )
{

}

void UNOpenGLESRenderDevice::EndFlash( )
{
	guard(UNOpenGLESRenderDevice::EndFlash);

	if( ColorMod == FPlane( 0.f, 0.f, 0.f, 0.f ) )
		return;

	CurrentShaderFlags = SF_VtxColor;
	ResetTexture( 0 );
	ResetTexture( 1 );
	ResetTexture( 2 );
	ResetTexture( 3 );
	SetBlend( PF_Highlighted );
	SetShader( CurrentShaderFlags );

	const FLOAT Z = 1.f;
	const FLOAT RFX2 = RProjZ;
	const FLOAT RFY2 = RProjZ * Aspect;

	glDisable( GL_DEPTH_TEST );

	BeginPoly();
		AttribFloat3( RFX2 * -Z, RFY2 * -Z, Z );
		AttribFloat4( &ColorMod.R );
		PolyVertex();
		AttribFloat3( RFX2 * +Z, RFY2 * -Z, Z );
		AttribFloat4( &ColorMod.R );
		PolyVertex();
		AttribFloat3( RFX2 * +Z, RFY2 * +Z, Z );
		AttribFloat4( &ColorMod.R );
		PolyVertex();
		AttribFloat3( RFX2 * -Z, RFY2 * +Z, Z );
		AttribFloat4( &ColorMod.R );
		PolyVertex();
	EndPoly();

	glEnable( GL_DEPTH_TEST );

	CurrentShaderFlags &= ~SF_VtxColor;

	unguard;
}

void UNOpenGLESRenderDevice::PushHit( const BYTE* Data, INT Count )
{

}

void UNOpenGLESRenderDevice::PopHit( INT Count, UBOOL bForce )
{

}

void UNOpenGLESRenderDevice::GetStats( char* Result )
{
	guard(UNOpenGLESRenderDevice::GetStats)

	if( Result ) *Result = '\0';

	unguard;
}

void UNOpenGLESRenderDevice::ReadPixels( FColor* Pixels )
{
	guard(UNOpenGLESRenderDevice::ReadPixels);

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

void UNOpenGLESRenderDevice::ClearZ( FSceneNode* Frame )
{
	guard(UNOpenGLESRenderDevice::ClearZ);

	FlushTriangles();
	SetBlend( PF_Occlude );
	SetShader( CurrentShaderFlags );

	glClear( GL_DEPTH_BUFFER_BIT );

	unguard;
}

void UNOpenGLESRenderDevice::UpdateUniforms()
{
	guard(UNOpenGLESRenderDevice::UpdateUniforms);

	if( UniformsChanged[UF_Mtx] )
	{
		FlushTriangles();
		glUniformMatrix4fv( ShaderInfo->Uniforms[UF_Mtx], 1, GL_FALSE, &MtxMVP[0][0] );
		UniformsChanged[UF_Mtx] = false;
	}

	if( UniformsChanged[UF_Brightness] )
	{
		FlushTriangles();
		glUniform1f( ShaderInfo->Uniforms[UF_Brightness], CurrentBrightness );
		UniformsChanged[UF_Brightness] = false;
	}

	if( UniformsChanged[UF_BrightnessScale] )
	{
		FlushTriangles();
		if( ShaderInfo->Uniforms[UF_BrightnessScale] >= 0 )
			glUniform1f( ShaderInfo->Uniforms[UF_BrightnessScale], CurrentBrightnessScale );
		UniformsChanged[UF_BrightnessScale] = false;
	}

	if( UniformsChanged[UF_WorldGamma] )
	{
		FlushTriangles();
		if( ShaderInfo->Uniforms[UF_WorldGamma] >= 0 )
			glUniform1f( ShaderInfo->Uniforms[UF_WorldGamma], CurrentWorldGamma );
		UniformsChanged[UF_WorldGamma] = false;
	}

	if( UniformsChanged[UF_WorldShadowLift] )
	{
		FlushTriangles();
		if( ShaderInfo->Uniforms[UF_WorldShadowLift] >= 0 )
			glUniform1f( ShaderInfo->Uniforms[UF_WorldShadowLift], CurrentWorldShadowLift );
		UniformsChanged[UF_WorldShadowLift] = false;
	}

	for( INT i = UF_Texture0; i <= UF_Texture3; ++i )
	{
		if( UniformsChanged[i] && ShaderInfo->Uniforms[i] >= 0 )
		{
			glUniform1i( ShaderInfo->Uniforms[i], i - UF_Texture0 );
			UniformsChanged[i] = false;
		}
	}

	unguard;
}

GLuint UNOpenGLESRenderDevice::CompileShader( GLenum Type, const char* Text )
{
	guard(UNOpenGLESRenderDevice::CompileShader);

	GLuint Id = glCreateShader( Type );

	const char *Src[] = { Text, NULL };
	glShaderSource( Id, 1, Src, NULL );

	glCompileShader( Id );

	GLint Status = 0;
	glGetShaderiv( Id, GL_COMPILE_STATUS, &Status );
	if( !Status )
	{
		char Tmp[2048] = { 0 };
		glGetShaderInfoLog( Id, sizeof(Tmp), NULL, Tmp );
		appErrorf( "%s shader compilation failed:\n%s", ( Type == GL_FRAGMENT_SHADER ) ? "Fragment" : "Vertex", Tmp );
	}

	return Id;

	unguard;
}

UNOpenGLESRenderDevice::FCachedShader* UNOpenGLESRenderDevice::CreateShader( DWORD ShaderFlags )
{
	guard(UNOpenGLESRenderDevice::CreateShader);

	static const char* FlagNames[SF_Count] = {
		"SF_Texture0", "SF_Texture1", "SF_Texture2", "SF_Texture3",
		"SF_VtxColor", "SF_AlphaTest", "SF_Lightmap", "SF_Fogmap",
		"SF_Detail", "SF_VtxFog"
	};

	static const char* UniformNames[UF_Count] = {
		"uMtx", "uBrightness", "uBrightnessScale", "uWorldGamma", "uWorldShadowLift",
		"uTexture0", "uTexture1", "uTexture2", "uTexture3"
	};

	static const char* AttribNames[AT_Count] = {
		"aPosition", "aTexCoord0", "aTexCoord1", "aTexCoord2",
		"aTexCoord3", "aVtxColor", "aVtxFog"
	};

	static const DWORD AttribFlags[AT_Count] = {
		0, SF_Texture0, SF_Texture1, SF_Texture2, SF_Texture3, SF_VtxColor, SF_VtxFog
	};

	static const char* ShaderVersion = "#version 100\n";

	FCachedShader* NewShader = ShaderMap.Add( ShaderFlags, FCachedShader() );
	verify(NewShader);
	NewShader->Flags = ShaderFlags;

	FString VSText;
	FString FSText;

	VSText += ShaderVersion;
	FSText += ShaderVersion;

	for( DWORD Flag = 1, FlagNum = 0; Flag <= SF_Max; Flag <<= 1, ++FlagNum )
	{
		if( ShaderFlags & Flag )
		{
			VSText.Appendf( "#define %s %u\n", FlagNames[FlagNum], Flag );
			FSText.Appendf( "#define %s %u\n", FlagNames[FlagNum], Flag );
		}
	}

	if( Overbright )
		FSText.Appendf( "#define LIGHTMAP_OVERBRIGHT %f\n", LIGHTMAP_OVERBRIGHT );


	VSText += VertShaderGLSL;
	FSText += FragShaderGLSL;

	GLuint VS = CompileShader( GL_VERTEX_SHADER, *VSText );
	GLuint FS = CompileShader( GL_FRAGMENT_SHADER, *FSText );

	GLuint Prog = glCreateProgram();
	glAttachShader( Prog, VS );
	glAttachShader( Prog, FS );

	NewShader->NumFloats = 0;
	for( INT i = 0; i < AT_Count; ++i )
	{
		if( i == 0 || ( ShaderFlags & AttribFlags[i] ) )
		{
			glBindAttribLocation( Prog, i, AttribNames[i] );
			NewShader->Attribs[i] = true;
			NewShader->NumFloats += AttribSizes[i];
		}
		else
		{
			NewShader->Attribs[i] = false;
		}
	}

	glLinkProgram( Prog );

	GLint Status = 0;
	glGetProgramiv( Prog, GL_LINK_STATUS, &Status );
	if( !Status )
	{
		char Tmp[2048] = { 0 };
		glGetProgramInfoLog( Prog, sizeof(Tmp), NULL, Tmp );
		appErrorf( "Failed to link shader %08x:\n%s", ShaderFlags, Tmp );
	}

	glDeleteShader( VS );
	glDeleteShader( FS );

	NewShader->Prog = Prog;

	for( INT i = 0; i < UF_Count; ++i )
		NewShader->Uniforms[i] = glGetUniformLocation( NewShader->Prog, UniformNames[i] );

	return NewShader;
	unguard;
}

void UNOpenGLESRenderDevice::SetShader( DWORD ShaderFlags )
{
	guard(UNOpenGLESRenderDevice::SetShader);

	if( !ShaderInfo || ShaderInfo->Flags != ShaderFlags )
	{
		FlushTriangles();

		ShaderInfo = ShaderMap.Find( ShaderFlags );
		if( !ShaderInfo )
			ShaderInfo = CreateShader( ShaderFlags );
		verify( ShaderInfo );

		// TODO: probably don't do this every program change
		for( INT i = 0; i < UF_Count; ++i )
			UniformsChanged[i] = true;

		BYTE* Ptr = UseVAO ? nullptr : (BYTE*)VtxData;
		for( INT i = 0; i < AT_Count; ++i )
		{
			if( ShaderInfo->Attribs[i] )
			{
				glEnableVertexAttribArray( i );
				glVertexAttribPointer( i, AttribSizes[i], GL_FLOAT, GL_FALSE, ShaderInfo->NumFloats * sizeof(FLOAT), (void*)Ptr );
				Ptr += AttribSizes[i] * sizeof(FLOAT);
			}
			else
			{
				glDisableVertexAttribArray( i );
			}
		}

		glUseProgram( ShaderInfo->Prog );
	}

	UpdateUniforms();

	unguard;
}

void UNOpenGLESRenderDevice::SetSceneNode( FSceneNode* Frame )
{
	guard(UNOpenGLESRenderDevice::SetSceneNode);

	check(Viewport);

	if( !Frame )
	{
		// invalidate current saved data
		CurrentSceneNode.X = -1;
		CurrentSceneNode.FX = -1.f;
		CurrentSceneNode.SizeX = -1;
		CurrentSceneNode.DrawableX = -1;
		CurrentSceneNode.DrawableY = -1;
		return;
	}

	INT DrawableX = Viewport->SizeX;
	INT DrawableY = Viewport->SizeY;
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	SDL_Window* AndroidWindow = (SDL_Window*)Viewport->GetWindow();
	if( AndroidWindow )
	{
		int AndroidDrawW = 0, AndroidDrawH = 0;
		SDL_GL_GetDrawableSize( AndroidWindow, &AndroidDrawW, &AndroidDrawH );
		if( AndroidDrawW > 0 && AndroidDrawH > 0 )
		{
			DrawableX = AndroidDrawW;
			DrawableY = AndroidDrawH;
		}
	}
#endif

	if( Frame->X != CurrentSceneNode.X || Frame->Y != CurrentSceneNode.Y ||
			Frame->XB != CurrentSceneNode.XB || Frame->YB != CurrentSceneNode.YB ||
			Viewport->SizeX != CurrentSceneNode.SizeX || Viewport->SizeY != CurrentSceneNode.SizeY ||
			DrawableX != CurrentSceneNode.DrawableX || DrawableY != CurrentSceneNode.DrawableY )
	{
		FlushTriangles();

		INT GLX = Frame->XB;
		INT GLY = Viewport->SizeY - Frame->Y - Frame->YB;
		INT GLW = Frame->X;
		INT GLH = Frame->Y;
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
		if( AndroidSceneFBOActive && AndroidSceneFBOSizeX > 0 && AndroidSceneFBOSizeY > 0 &&
			Viewport->SizeX > 0 && Viewport->SizeY > 0 &&
			( AndroidSceneFBOSizeX != Viewport->SizeX || AndroidSceneFBOSizeY != Viewport->SizeY ) )
		{
			// UE1_ANDROID_REAL_RENDER_RESOLUTION_FBO_V71
			// Render SceneNode rectangles into the real low-res FBO. This is only a
			// safety path if the UE1 viewport and requested FBO diverge; normally they
			// are identical for 1280x720 and 1024x768.
			const FLOAT ScaleX = (FLOAT)AndroidSceneFBOSizeX / (FLOAT)Viewport->SizeX;
			const FLOAT ScaleY = (FLOAT)AndroidSceneFBOSizeY / (FLOAT)Viewport->SizeY;
			GLX = appRound( (FLOAT)Frame->XB * ScaleX );
			GLY = appRound( (FLOAT)( Viewport->SizeY - Frame->Y - Frame->YB ) * ScaleY );
			GLW = Max( 1, appRound( (FLOAT)Frame->X * ScaleX ) );
			GLH = Max( 1, appRound( (FLOAT)Frame->Y * ScaleY ) );
		}
		else if( !AndroidSceneFBOActive && DrawableX > 0 && DrawableY > 0 && Viewport->SizeX > 0 && Viewport->SizeY > 0 &&
			( DrawableX != Viewport->SizeX || DrawableY != Viewport->SizeY ) )
		{
			// Fallback only: if the FBO cannot be created, at least keep the selected
			// logical resolution fullscreen instead of drawing a small lower-left image.
			// UE1_ANDROID_RESOLUTION_STRETCH_FULL_V71
			const FLOAT ScaleX = (FLOAT)DrawableX / (FLOAT)Viewport->SizeX;
			const FLOAT ScaleY = (FLOAT)DrawableY / (FLOAT)Viewport->SizeY;
			GLX = appRound( (FLOAT)Frame->XB * ScaleX );
			GLY = appRound( (FLOAT)( Viewport->SizeY - Frame->Y - Frame->YB ) * ScaleY );
			GLW = Max( 1, appRound( (FLOAT)Frame->X * ScaleX ) );
			GLH = Max( 1, appRound( (FLOAT)Frame->Y * ScaleY ) );
		}
#endif
		glViewport( GLX, GLY, GLW, GLH );
		CurrentSceneNode.X = Frame->X;
		CurrentSceneNode.Y = Frame->Y;
		CurrentSceneNode.XB = Frame->XB;
		CurrentSceneNode.YB = Frame->YB;
		CurrentSceneNode.SizeX = Viewport->SizeX;
		CurrentSceneNode.SizeY = Viewport->SizeY;
		CurrentSceneNode.DrawableX = DrawableX;
		CurrentSceneNode.DrawableY = DrawableY;
	}

	if( Frame->FX != CurrentSceneNode.FX || Frame->FY != CurrentSceneNode.FY ||
			Viewport->Actor->FovAngle != CurrentSceneNode.FovAngle )
	{
		RProjZ = appTan( Viewport->Actor->FovAngle * PI / 360.0 );
		Aspect = Frame->FY / Frame->FX;
		RFX2 = 2.0f * RProjZ / Frame->FX;
		RFY2 = 2.0f * RProjZ * Aspect / Frame->FY;
		MtxProj = glm::frustum( -RProjZ, +RProjZ, -Aspect * RProjZ, +Aspect * RProjZ, 1.f, 65336.f );
		MtxMVP = MtxProj * MtxModelView;
		CurrentSceneNode.FX = Frame->FX;
		CurrentSceneNode.FY = Frame->FY;
		CurrentSceneNode.FovAngle = Viewport->Actor->FovAngle;
		UniformsChanged[UF_Mtx] = true;
	}

	unguard;
}

void UNOpenGLESRenderDevice::SetBlend( DWORD PolyFlags, UBOOL InverseOrder )
{
	guard(UNOpenGLESRenderDevice::SetBlend);

	// Adjust PolyFlags according to Unreal's precedence rules.
	if( !(PolyFlags & (PF_Translucent|PF_Modulated)) )
		PolyFlags |= PF_Occlude;
	else if( PolyFlags & PF_Translucent )
	{
#if !(defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__))
		PolyFlags &= ~PF_Masked;
#endif
	}

	// Detect changes in the blending modes.
	DWORD Xor = CurrentPolyFlags ^ PolyFlags;
	if( Xor & (PF_Translucent|PF_Modulated|PF_Invisible|PF_Occlude|PF_Masked|PF_Highlighted) )
	{
		FlushTriangles();
		if( Xor & (PF_Translucent|PF_Modulated|PF_Highlighted) )
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
			glDepthMask( (PolyFlags & PF_Occlude) != 0 );
		if( Xor & PF_Masked )
		{
			if( PolyFlags & PF_Masked )
				CurrentShaderFlags |= SF_AlphaTest;
			else
				CurrentShaderFlags &= ~SF_AlphaTest;
		}
	}

	CurrentPolyFlags = PolyFlags;

	unguard;
}

void UNOpenGLESRenderDevice::UpdateTextureFilter( const FTextureInfo& Info, DWORD PolyFlags, INT BaseMip )
{
	guard(UNOpenGLESRenderDevice::UpdateTextureFilter);

	if( UE1GLESIsLegacyEffectSpriteTexture( Info ) )
	{
		// Effect billboards must not sample across mip levels or wrap around
		// their opposite edge, otherwise the old UE1 transparent/neutral border
		// becomes visible as a square. Keep linear mag/min for smooth sprites,
		// but disable mipmap sampling for these concrete effects.
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		return;
	}

	const INT EffectiveNumMips = Max( Info.NumMips - BaseMip, 1 );

	// Set mip filtering if there are mips.
	if( ( PolyFlags & PF_NoSmooth ) || ( NoFiltering && Info.Palette ) ) // TODO: This is set per poly, not per texture.
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ( EffectiveNumMips > 1 ) ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}
	else
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, ( EffectiveNumMips > 1 ) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}

	// This is a light/fog map.
	if( !Info.Palette )
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	}

	unguard;
}

void UNOpenGLESRenderDevice::ResetTexture( INT TMU )
{
	guard(UNOpenGLESRenderDevice::ResetTexture);

	CurrentShaderFlags &= ~(1 << TMU);

	if( TexInfo[TMU].CurrentCacheID != 0 )
	{
		FlushTriangles();
		glActiveTexture( GL_TEXTURE0 + TMU );
		glBindTexture( GL_TEXTURE_2D, 0 );
		TexInfo[TMU].CurrentCacheID = 0;
	}

	unguard;
}

INT UNOpenGLESRenderDevice::GetTextureBaseMip( const FTextureInfo& Info, DWORD PolyFlags ) const
{
	guard(UNOpenGLESRenderDevice::GetTextureBaseMip);

	if( !RuntimeLowDetailTextures )
		return 0;

	// Keep lightmaps/fogmaps, realtime/fire/effect textures and masked sprites exact.
	// Low detail is only for ordinary palettized world textures with a real mip chain.
	if( !Info.Palette || Info.NumMips < 2 || ( PolyFlags & PF_Masked ) )
		return 0;
	if( Info.TextureFlags & ( TF_Realtime | TF_RealtimeChanged ) )
		return 0;
	if( UE1GLESIsLegacyEffectSpriteTexture( Info ) )
		return 0;

	return ( Info.Mips[1] && Info.Mips[1]->DataPtr ) ? 1 : 0;

	unguard;
}

UBOOL UNOpenGLESRenderDevice::GetConfiguredLowDetailTextures() const
{
	guard(UNOpenGLESRenderDevice::GetConfiguredLowDetailTextures);

	UBOOL Value = false;
	if( Viewport && Viewport->Client )
		Value = Viewport->Client->LowDetailTextures;

#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	Value = UE1GLESConfigBool( "NSDLDrv.NSDLClient", "LowDetailTextures", Value );
#endif

	return Value;

	unguard;
}

FLOAT UNOpenGLESRenderDevice::GetConfiguredBrightness() const
{
	guard(UNOpenGLESRenderDevice::GetConfiguredBrightness);

	FLOAT Value = 0.5f;
	if( Viewport && Viewport->Client )
		Value = Viewport->Client->Brightness;

#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	Value = UE1GLESConfigFloat( "NSDLDrv.NSDLClient", "Brightness", Value );
#endif

	return Clamp( Value, 0.0f, 1.0f );

	unguard;
}

FLOAT UNOpenGLESRenderDevice::GetConfiguredBrightnessScale() const
{
	guard(UNOpenGLESRenderDevice::GetConfiguredBrightnessScale);

	FLOAT Value = BrightnessScale;
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	Value = UE1GLESConfigFloat( "NOpenGLESDrv.NOpenGLESRenderDevice", "BrightnessScale", Value );
#endif
	return Clamp( Value, 0.25f, 4.0f );

	unguard;
}

FLOAT UNOpenGLESRenderDevice::GetConfiguredWorldGamma() const
{
	guard(UNOpenGLESRenderDevice::GetConfiguredWorldGamma);

	FLOAT Value = WorldGamma;
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	Value = UE1GLESConfigFloat( "NOpenGLESDrv.NOpenGLESRenderDevice", "WorldGamma", Value );
	// Gives a future/advanced menu one simple client-side value to write.
	Value = UE1GLESConfigFloat( "NSDLDrv.NSDLClient", "Gamma", Value );
#endif
	return Clamp( Value, 0.5f, 3.0f );

	unguard;
}

FLOAT UNOpenGLESRenderDevice::GetConfiguredWorldShadowLift() const
{
	guard(UNOpenGLESRenderDevice::GetConfiguredWorldShadowLift);

	FLOAT Value = WorldShadowLift;
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	Value = UE1GLESConfigFloat( "NOpenGLESDrv.NOpenGLESRenderDevice", "WorldShadowLift", Value );
#endif
	return Clamp( Value, 0.0f, 0.35f );

	unguard;
}

void UNOpenGLESRenderDevice::UpdateRuntimeConfig()
{
	guard(UNOpenGLESRenderDevice::UpdateRuntimeConfig);

	const FLOAT ConfigBrightness = GetConfiguredBrightness();
	if( Viewport && Viewport->Client && Viewport->Client->Brightness != ConfigBrightness )
		Viewport->Client->Brightness = ConfigBrightness;

	BrightnessScale = GetConfiguredBrightnessScale();
	WorldGamma = GetConfiguredWorldGamma();
	WorldShadowLift = GetConfiguredWorldShadowLift();

	const UBOOL NewLowDetailTextures = GetConfiguredLowDetailTextures();
	if( Viewport && Viewport->Client )
		Viewport->Client->LowDetailTextures = NewLowDetailTextures;

	if( RuntimeLowDetailTextures != NewLowDetailTextures )
	{
		RuntimeLowDetailTextures = NewLowDetailTextures;
		debugf( NAME_Log, "GLES2: Texture Detail runtime mode changed to %s", RuntimeLowDetailTextures ? "LOW" : "HIGH" );
		Flush();
	}

	unguard;
}

void UNOpenGLESRenderDevice::SetTexture( INT TMU, FTextureInfo& Info, DWORD PolyFlags, FLOAT PanBias )
{
	guard(UNOpenGLESRenderDevice::SetTexture);

	CurrentShaderFlags |= 1 << TMU;

	// Set panning.
	FTexInfo& Tex = TexInfo[TMU];
	Tex.UPan      = Info.Pan.X + PanBias*Info.UScale;
	Tex.VPan      = Info.Pan.Y + PanBias*Info.VScale;

	// Account for all the impact on scale normalization.
	Tex.UMult = 1.f / (Info.UScale * static_cast<FLOAT>(Info.USize));
	Tex.VMult = 1.f / (Info.VScale * static_cast<FLOAT>(Info.VSize));

	// Find in cache.
	const INT BaseMip = GetTextureBaseMip( Info, PolyFlags );
	QWORD NewCacheID = Info.CacheID;
	if( ( PolyFlags & PF_Masked ) && Info.Palette )
		NewCacheID |= MASKED_TEXTURE_TAG;
	if( BaseMip > 0 )
		NewCacheID |= LOWDETAIL_TEXTURE_TAG;
	UBOOL RealtimeChanged = ( Info.TextureFlags & TF_RealtimeChanged );
	if( NewCacheID == Tex.CurrentCacheID && !RealtimeChanged )
		return;

	FlushTriangles();

	// Make current.
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
	glBindTexture( GL_TEXTURE_2D, Bind->Id );

	Bind->BaseMip = BaseMip;
	Bind->MaxLevel = Max( Info.NumMips - BaseMip - 1, 0 );

	if( !OldBind || RealtimeChanged )
	{
		// New texture or it has changed, upload it.
		Info.TextureFlags &= ~TF_RealtimeChanged;
		UploadTexture( Info, ( PolyFlags & PF_Masked ), !OldBind, BaseMip );
		// TODO: This depends on PolyFlags, not Info.
		UpdateTextureFilter( Info, PolyFlags, BaseMip );
	}

	unguard;
}

void UNOpenGLESRenderDevice::UploadTexture( FTextureInfo& Info, UBOOL Masked, UBOOL NewTexture, INT BaseMip )
{
	guard(UNOpenGLESRenderDevice::UploadTexture);

	if( BaseMip < 0 || BaseMip >= Info.NumMips || !Info.Mips[BaseMip] )
		BaseMip = 0;

	if( !Info.Mips[BaseMip] )
	{
		debugf( NAME_Warning, "Encountered texture with invalid mips!" );
		return;
	}

	// We're gonna be using the compose buffer, so expand it to fit.
	INT NewComposeSize = Info.Mips[BaseMip]->USize * Info.Mips[BaseMip]->VSize * 4;
	if( NewComposeSize > ComposeSize )
	{
		Compose = (BYTE*)appRealloc( Compose, NewComposeSize, "GLComposeBuf" );
		verify( Compose );
	}

	const UBOOL bMaineffect = UE1GLESIsMaineffectTexture( Info );
	const UBOOL bSmokeBlack = UE1GLESIsSmokeBlackTexture( Info );
	const UBOOL bHubWaterFall2 = UE1GLESIsHubWaterFall2Texture( Info );
	const UBOOL bHubSmoke1 = UE1GLESIsHubSmoke1Texture( Info );
	const UBOOL bHubWaterRings2 = UE1GLESIsHubWaterRings2Texture( Info );
	const UBOOL bLegacyEffectSprite = bMaineffect || bSmokeBlack || bHubSmoke1 || bHubWaterRings2;

	// Upload all mips. In LOW texture detail mode, source mip 1 becomes GL level 0.
	for( INT SourceMipIndex = BaseMip, UploadMipIndex = 0; SourceMipIndex < Info.NumMips; ++SourceMipIndex, ++UploadMipIndex )
	{
		const FMipmap* Mip = Info.Mips[SourceMipIndex];
		if( !Mip || !Mip->DataPtr ) break;
		BYTE* UploadBuf;
		GLenum UploadFormat;
		// Convert texture if needed.
		if( Info.Palette )
		{
			// 8-bit indexed. We have to fix the alpha component since it's mostly garbage in non-detailmaps.
			UploadBuf = Compose;
			UploadFormat = GL_RGBA;
			const BYTE* Src = (const BYTE*)Mip->DataPtr;
			const DWORD* Pal = (const DWORD*)Info.Palette;
			const DWORD Count = Mip->USize * Mip->VSize;
			BYTE WR2Corner0 = 0, WR2Corner1 = 0, WR2Corner2 = 0, WR2Corner3 = 0;
			FColor WR2NeutralColor( 255, 255, 255, 255 );
			INT WR2NeutralLuma = 255;
			if( bHubWaterRings2 && Mip->USize > 0 && Mip->VSize > 0 )
			{
				const BYTE* P8 = (const BYTE*)Mip->DataPtr;
				WR2Corner0 = P8[0];
				WR2Corner1 = P8[Mip->USize - 1];
				WR2Corner2 = P8[(Mip->VSize - 1) * Mip->USize];
				WR2Corner3 = P8[(Mip->VSize - 1) * Mip->USize + (Mip->USize - 1)];

				// UE1_ANDROID_WATERRINGS_BORDER_NEUTRAL_V70
				// WaterRings2 is drawn with GL_ZERO/GL_SRC_COLOR, so the neutral
				// background must be exact white.  Earlier fixes picked the most
				// common colour from the whole animated frame and could accidentally
				// choose a ripple colour, whitening the waves away.  The frame border
				// is the stable neutral area; derive the neutral palette entry from
				// there and only remap near-neutral texels to white.
				INT BorderCounts[256];
				appMemset( BorderCounts, 0, sizeof(BorderCounts) );
				for( INT Y = 0; Y < Mip->VSize; ++Y )
				{
					for( INT X = 0; X < Mip->USize; ++X )
					{
						if( X != 0 && Y != 0 && X != Mip->USize - 1 && Y != Mip->VSize - 1 )
							continue;
						BorderCounts[P8[Y * Mip->USize + X]]++;
					}
				}

				INT BestIndex = WR2Corner0;
				INT BestCount = -1;
				INT BestLuma = -1;
				for( INT PaletteIndex = 0; PaletteIndex < 256; ++PaletteIndex )
				{
					if( BorderCounts[PaletteIndex] <= 0 )
						continue;
					FColor C = Info.Palette[PaletteIndex];
					INT Luma = ( (INT)C.R + (INT)C.G + (INT)C.B ) / 3;
					if( Luma < 96 )
						continue;
					if( BorderCounts[PaletteIndex] > BestCount || ( BorderCounts[PaletteIndex] == BestCount && Luma > BestLuma ) )
					{
						BestIndex = PaletteIndex;
						BestCount = BorderCounts[PaletteIndex];
						BestLuma = Luma;
					}
				}
				WR2NeutralColor = Info.Palette[BestIndex];
				WR2NeutralLuma = ( (INT)WR2NeutralColor.R + (INT)WR2NeutralColor.G + (INT)WR2NeutralColor.B ) / 3;
				if( WR2NeutralLuma < 96 )
				{
					WR2NeutralColor = FColor( 255, 255, 255, 255 );
					WR2NeutralLuma = 255;
				}
			}

			if( bLegacyEffectSprite )
			{
				// For the problematic UnrealI DrawTile effects, write RGBA byte-by-byte
				// and apply the per-texture border/neutral alpha rules above. This avoids
				// ARM endian surprises and keeps the change isolated to these effects.
				BYTE* Dst = (BYTE*)Compose;
				for( DWORD i = 0; i < Count; ++i, ++Src )
				{
					BYTE Index = *Src;
					FColor Color = Info.Palette[Index];

					if( bHubWaterRings2 )
					{
						// UE1_ANDROID_WATERRINGS_BORDER_NEUTRAL_V70
						// For multiplicative modulation, source white means "do not change".
						// Make only the neutral background exact white and remap darker
						// deviations around that border-derived neutral to visible ripple
						// darkening.  Alpha is intentionally kept opaque because the active
						// WaterRings2 blend reads RGB, not alpha.
						INT Luma = ( (INT)Color.R + (INT)Color.G + (INT)Color.B ) / 3;
						INT DR = ( (INT)Color.R > (INT)WR2NeutralColor.R ) ? ( (INT)Color.R - (INT)WR2NeutralColor.R ) : ( (INT)WR2NeutralColor.R - (INT)Color.R );
						INT DG = ( (INT)Color.G > (INT)WR2NeutralColor.G ) ? ( (INT)Color.G - (INT)WR2NeutralColor.G ) : ( (INT)WR2NeutralColor.G - (INT)Color.G );
						INT DB = ( (INT)Color.B > (INT)WR2NeutralColor.B ) ? ( (INT)Color.B - (INT)WR2NeutralColor.B ) : ( (INT)WR2NeutralColor.B - (INT)Color.B );
						INT MaxAbsDelta = Max( DR, Max( DG, DB ) );
						INT DarkDelta = WR2NeutralLuma - Luma;

						if( MaxAbsDelta <= 7 || DarkDelta <= 3 )
						{
							Color.R = 255;
							Color.G = 255;
							Color.B = 255;
						}
						else
						{
							INT Strength = Min( 192, Max( 8, DarkDelta * 5 ) );
							BYTE V = (BYTE)Max( 48, 255 - Strength );
							Color.R = V;
							Color.G = V;
							Color.B = V;
						}
					}
					*Dst++ = Color.R;
					*Dst++ = Color.G;
					*Dst++ = Color.B;
					*Dst++ = UE1GLESPaletteEffectAlpha( Info, Index, Masked, bMaineffect, bSmokeBlack, bHubWaterFall2, bHubSmoke1, bHubWaterRings2, WR2Corner0, WR2Corner1, WR2Corner2, WR2Corner3 );
				}
			}
			else if( Masked )
			{
				DWORD* Dst = (DWORD*)Compose;
				// index 0 is transparent
#if __INTEL_BYTE_ORDER__
				for( DWORD i = 0; i < Count; ++i, ++Src )
					*Dst++ = *Src ? ( Pal[*Src] | ALPHA_MASK ) : 0;
#else
				for( DWORD i = 0; i < Count; ++i, ++Src )
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
				for( DWORD i = 0; i < Count; ++i )
					*Dst++ = ( Pal[*Src++] | ALPHA_MASK );
#else
				for( DWORD i = 0; i < Count; ++i, ++Src )
				{
					FColor Color = Info.Palette[*Src];
					Color.A = 255;
					*Dst++ = (Color.R << 24) | (Color.G << 16) | (Color.B << 8) | Color.A;
				}
#endif
			}
		}
		else if( UseBGRA )
		{
			// BGRA8888 (or 7777) and we can upload it as-is.
			UploadBuf = Mip->DataPtr;
			UploadFormat = GL_BGRA_EXT;
		}
		else
		{
			// BGRA8888 (or 7777), but we must swap it because it's not supported natively.
			UploadBuf = Compose;
			UploadFormat = GL_RGBA;
			BYTE* Dst = (BYTE*)Compose;
			const BYTE* Src = (const BYTE*)Mip->DataPtr;
			const DWORD Count = Mip->USize * Mip->VSize;
			for( DWORD i = 0; i < Count; ++i, Src += 4 )
			{
				*Dst++ = Src[2];
				*Dst++ = Src[1];
				*Dst++ = Src[0];
				*Dst++ = Src[3];
			}
		}
		// Upload to GL.
		if( NewTexture )
			glTexImage2D( GL_TEXTURE_2D, UploadMipIndex, UploadFormat, Mip->USize, Mip->VSize, 0, UploadFormat, GL_UNSIGNED_BYTE, (void*)UploadBuf );
		else
			glTexSubImage2D( GL_TEXTURE_2D, UploadMipIndex, 0, 0, Mip->USize, Mip->VSize, UploadFormat, GL_UNSIGNED_BYTE, (void*)UploadBuf );
	}

	unguard;
}

void UNOpenGLESRenderDevice::UpdateSwapInterval()
{
	guard(UNOpenGLESRenderDevice::UpdateSwapInterval);

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
			UpdateSwapInterval();
		}
	}

	unguard;
}
