/*
  OUYA/API16 safe render-resolution menu bridge.

  The UnrealScript video menu already uses the classic UE1 commands:
    GetRes, GetCurrentRes and SetRes.

  OUYA previously stored the selected internal render resolution and asked for a
  restart. The current path applies SetRes live by updating the config keys,
  resizing the OUYA native buffer through SDL, and matching UE1's viewport to it.
*/
#ifndef NSDL_OUYA_RESOLUTION_MENU_H
#define NSDL_OUYA_RESOLUTION_MENU_H

#if PLATFORM_ANDROID && ANDROID_LEGACY_API16

extern "C" int SDL_OUYA_ApplyRenderResolutionLive(void* SdlWindow, int W, int H);
extern "C" void UE1AndroidConfigBeginBatchSave(void);
extern "C" void UE1AndroidConfigEndBatchSave(void);

static const char* NSDL_OUYA_RES_SECTION = "NOpenGLESDrv.NOpenGLESRenderDevice";

static UBOOL NSDL_OuyaIsSpaceChar( char C )
{
	return C == ' ' || C == '\t' || C == '\r' || C == '\n';
}

static const char* NSDL_OuyaSkipSpaces( const char* Text )
{
	while( Text && *Text && NSDL_OuyaIsSpaceChar( *Text ) )
		++Text;
	return Text ? Text : "";
}

static UBOOL NSDL_OuyaSupportedResolution( INT W, INT H )
{
	return ( W == 1280 && H == 720 ) ||
	       ( W == 960  && H == 540 ) ||
	       ( W == 1024 && H == 768 ) ||
	       ( W == 800  && H == 600 );
}

static void NSDL_OuyaNormalizeResolution( INT& W, INT& H )
{
	if( NSDL_OuyaSupportedResolution( W, H ) )
		return;

	W = 960;
	H = 540;
}

static UBOOL NSDL_OuyaParseResolution( const char* Text, INT& OutW, INT& OutH )
{
	if( !Text || !Text[0] )
		return 0;

	char Local[64];
	INT i = 0;

	for( ; Text[i] && i < 63; i++ )
		Local[i] = Text[i];
	Local[i] = 0;

	char* Sep = appStrchr( Local, 'x' );
	if( !Sep )
		Sep = appStrchr( Local, 'X' );

	if( !Sep )
		return 0;

	*Sep = 0;

	OutW = appAtoi( Local );
	OutH = appAtoi( Sep + 1 );

	return NSDL_OuyaSupportedResolution( OutW, OutH );
}

static void NSDL_OuyaGetConfiguredResolution( INT& W, INT& H )
{
	char Value[64];
	INT ParsedW = 0;
	INT ParsedH = 0;

	W = 960;
	H = 540;

	if( GConfigCache.GetString( NSDL_OUYA_RES_SECTION, "OuyaRenderResolution", Value, ARRAY_COUNT(Value) ) &&
		NSDL_OuyaParseResolution( Value, ParsedW, ParsedH ) )
	{
		W = ParsedW;
		H = ParsedH;
	}

	if( GConfigCache.GetString( NSDL_OUYA_RES_SECTION, "OuyaRenderWidth", Value, ARRAY_COUNT(Value) ) )
		W = appAtoi( Value );
	if( GConfigCache.GetString( NSDL_OUYA_RES_SECTION, "OuyaRenderHeight", Value, ARRAY_COUNT(Value) ) )
		H = appAtoi( Value );

	NSDL_OuyaNormalizeResolution( W, H );
}

static void NSDL_OuyaSaveConfiguredResolution( INT W, INT H )
{
	char Value[64];

	NSDL_OuyaNormalizeResolution( W, H );

	// UE1_ANDROID_OUYA_CONFIG_BATCH_SAVE_V5
	// SetRes changes five config keys.  Batch them and flush once to avoid
	// several synchronous Unreal.ini writes on the game thread.
	UE1AndroidConfigBeginBatchSave();

	appSprintf( Value, "%ix%i", W, H );
	GConfigCache.SetString( NSDL_OUYA_RES_SECTION, "OuyaRenderResolution", Value );

	appSprintf( Value, "%i", W );
	GConfigCache.SetString( NSDL_OUYA_RES_SECTION, "OuyaRenderWidth", Value );
	GConfigCache.SetString( "OUYA.Performance", "RenderWidth", Value );

	appSprintf( Value, "%i", H );
	GConfigCache.SetString( NSDL_OUYA_RES_SECTION, "OuyaRenderHeight", Value );
	GConfigCache.SetString( "OUYA.Performance", "RenderHeight", Value );

	UE1AndroidConfigEndBatchSave();
	GConfigCache.SaveAllConfigs();

	debugf( "OUYA resolution saved: %ix%i", W, H );
}

static void NSDL_OuyaNotifyResolutionApplied( UViewport* Viewport, INT W, INT H )
{
	char Message[255];

	appSprintf( Message, "OUYA render resolution switched to %ix%i.", W, H );
	debugf( "%s", Message );

	if( Viewport && Viewport->Actor )
		Viewport->Actor->eventClientMessage( Message );
}

static void NSDL_OuyaApplyResolutionLive( UViewport* Viewport, INT W, INT H, FOutputDevice* Out )
{
	if( !Viewport )
		return;

	// UE1_ANDROID_OUYA_LIVE_RESOLUTION_NATIVE_BUFFER_V6
	// The OUYA/API16 base does not use the Android8+ renderer-FBO path.
	// Its stable downscale path is SDL/ANativeWindow_setBuffersGeometry().
	// Therefore a live switch must update the native buffer first and only then
	// resize UE1's logical viewport to the same internal render size.
	void* SdlWindow = ((UNSDLViewport*)Viewport)->GetWindow();
	const INT Applied = SDL_OUYA_ApplyRenderResolutionLive( SdlWindow, W, H );

	((UNSDLViewport*)Viewport)->SetClientSize( W, H, 0 );

	if( Out )
		Out->Logf( "OUYA native buffer + viewport switched live to %ix%i", W, H );
	debugf( NAME_Log, "OUYA/API16 live native-buffer resolution applied: %ix%i result=%i", W, H, Applied );
	Viewport->Repaint();
}


static UBOOL NSDL_OuyaExecResolutionCommand( UViewport* Viewport, const char* InCmd, FOutputDevice* Out )
{
	const char* Cmd = InCmd;
	INT W;
	INT H;

	if( ParseCommand( &Cmd, "GetRes" ) )
	{
		if( Out )
			Out->Log( "960x540 1280x720 1024x768 800x600" );
		return 1;
	}
	else if( ParseCommand( &Cmd, "GetCurrentRes" ) )
	{
		NSDL_OuyaGetConfiguredResolution( W, H );
		if( Out )
		{
			if( W == 0 && H == 0 )
				Out->Log( "960x540" );
			else
				Out->Logf( "%ix%i", W, H );
		}
		return 1;
	}
	else if( ParseCommand( &Cmd, "SetRes" ) )
	{
		if( NSDL_OuyaParseResolution( Cmd, W, H ) )
		{
			NSDL_OuyaSaveConfiguredResolution( W, H );
			NSDL_OuyaApplyResolutionLive( Viewport, W, H, Out );
			if( Out )
				Out->Logf( "OUYA render resolution switched live to %ix%i.", W, H );
			NSDL_OuyaNotifyResolutionApplied( Viewport, W, H );
		}
		else
		{
			if( Out )
				Out->Log( "Unsupported OUYA render resolution. Use 960x540, 1280x720, 1024x768, or 800x600." );
			if( Viewport && Viewport->Actor )
				Viewport->Actor->eventClientMessage( "Unsupported OUYA resolution. Use 960x540, 1280x720, 1024x768, or 800x600." );
		}
		return 1;
	}
	else if( ParseCommand( &Cmd, "ToggleFullscreen" ) )
	{
		/* OUYA is always fullscreen. Do not tear down the stable SDL/EGL path. */
		if( Out )
			Out->Log( "OUYA always runs fullscreen. Select Resolution switches the internal renderer live." );
		if( Viewport && Viewport->Actor )
			Viewport->Actor->eventClientMessage( "OUYA is always fullscreen. Select Resolution switches live." );
		return 1;
	}

	return 0;
}

#endif /* PLATFORM_ANDROID && ANDROID_LEGACY_API16 */

#endif /* NSDL_OUYA_RESOLUTION_MENU_H */
