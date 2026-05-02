#include "SDL2/SDL.h"
#ifdef PLATFORM_WIN32
#include <windows.h>
#endif
#ifdef PLATFORM_PSVITA
#include <vitasdk.h>
#include <vitaGL.h>
#include <unistd.h>
#endif

#include "Engine.h"
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

extern CORE_API FGlobalPlatform GTempPlatform;
extern DLL_IMPORT UBOOL GTickDue;
extern "C" {HINSTANCE hInstance;}
extern "C" {char GCC_HIDDEN THIS_PACKAGE[64]="Launch";}

// FExecHook.
class FExecHook : public FExec
{
	UBOOL Exec( const char* Cmd, FOutputDevice* Out )
	{
		return 0;
	}
};

FExecHook GLocalHook;
DLL_EXPORT FExec* GThisExecHook = &GLocalHook;

#ifdef PLATFORM_PSVITA

//
// PSVita-specific globals.
//

#define MAX_PATH 1024
#define SYSTEM_PATH "data/unreal/System/"

// 200MB libc heap, 512K main thread stack, 16MB for loading game DLLs
// the rest goes to vitaGL
extern "C" { SceUInt32 sceUserMainThreadStackSize = 512 * 1024; }
extern "C" { unsigned int _pthread_stack_default_user = 512 * 1024; }
extern "C" { unsigned int _newlib_heap_size_user = 200 * 1024 * 1024; }
#define VGL_MEM_THRESHOLD ( 16 * 1024 * 1024 )

static char GRootPath[MAX_PATH] = "app0:/";

//
// PSVita-specific functions.
//

static bool FindRootPath( char* Out, int OutLen )
{
	static const char *Drives[] = { "uma0", "imc0", "ux0" };

	// check if an unreal folder exists on one of the drives
	// default to the last one (ux0)
	for ( unsigned int i = 0; i < sizeof(Drives) / sizeof(*Drives); ++i )
	{
		snprintf( Out, OutLen, "%s:/" SYSTEM_PATH, Drives[i] );
		SceUID Dir = sceIoDopen( Out );
		if ( Dir >= 0 )
		{
			sceIoDclose( Dir );
			return true;
		}
	}

	// not found
	return false;
}

static INT PowerCallback( INT NotifyID, INT NotifyCnt, INT PowerInfo, void* Common )
{
	if ( PowerInfo & ( SCE_POWER_CB_APP_RESUME | SCE_POWER_CB_APP_RESUMING ) )
	{
		debugf( "PowerCallback: resuming..." );
		appHandleSuspendResume( false );
	}
	else if ( PowerInfo & ( SCE_POWER_CB_BUTTON_PS_PRESS | SCE_POWER_CB_APP_SUSPEND | SCE_POWER_CB_SYSTEM_SUSPEND ) )
	{
		debugf( "PowerCallback: suspending..." );
		appHandleSuspendResume( true );
	}

	return 0;
}

static INT CallbackThread( DWORD Argc, void* Argv )
{
	const INT CbID = sceKernelCreateCallback( "Power Callback", 0, PowerCallback, nullptr );
	scePowerRegisterCallback( CbID );
	while( true )
		sceKernelDelayThreadCB( 10000000 );
	return 0;
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	fprintf( stderr, "FATAL ERROR: %s\n", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Fatal Error", Msg, nullptr );
	sceKernelExitProcess( 0 );
	abort();
}

static void PlatformPreInit()
{
	sceTouchSetSamplingState( SCE_TOUCH_PORT_BACK, SCE_TOUCH_SAMPLING_STATE_STOP );
	scePowerSetArmClockFrequency( 444 );
	scePowerSetBusClockFrequency( 222 );
	scePowerSetGpuClockFrequency( 222 );
	scePowerSetGpuXbarClockFrequency( 166 );
	sceSysmoduleLoadModule( SCE_SYSMODULE_NET );

	if ( !FindRootPath( GRootPath, sizeof(GRootPath) ) )
		EarlyError( "Could not find Unreal directory" );

	if ( chdir( GRootPath ) < 0 )
		EarlyError( "Could not chdir to Unreal directory" );

	SceUID Th = sceKernelCreateThread( "CallbackThread", CallbackThread, 0x10000100, 0x10000, 0, 0, nullptr );
	if( Th >= 0 )
		sceKernelStartThread( Th, 0, nullptr );

	vglInitWithCustomThreshold( 0, 960, 544, VGL_MEM_THRESHOLD, 0, 0, 0, SCE_GXM_MULTISAMPLE_2X );
	vglSetSemanticBindingMode( VGL_MODE_POSTPONED );
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";
static volatile UBOOL GAndroidAppSuspended = false;
static Uint32 GAndroidIgnoreQuitUntilTicks = 0;
static INT GAndroidRunSerial = 0;

extern "C" void UE1AndroidSetAudioSuspended( int Suspended ) __attribute__((weak));

extern "C" int UE1AndroidShouldIgnoreEarlyQuit()
{
	if( !GAndroidIgnoreQuitUntilTicks )
		return 0;

	const Uint32 Now = SDL_GetTicks();
	return SDL_TICKS_PASSED( Now, GAndroidIgnoreQuitUntilTicks ) ? 0 : 1;
}

static void AndroidResetRuntimeStateForStart()
{
	GAndroidRunSerial++;
	GIsRequestingExit = 0;
	GIsCriticalError  = 0;
	GIsRunning        = 0;
	GIsGuarded        = 0;
	GAndroidAppSuspended = false;
	GAndroidIgnoreQuitUntilTicks = SDL_GetTicks() + 2500;

	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "SDL_main start #%d: runtime flags reset", GAndroidRunSerial );
}

static void AndroidFlushStaleStartupEvents()
{
	SDL_PumpEvents();
	SDL_FlushEvent( SDL_QUIT );
	SDL_FlushEvent( SDL_APP_TERMINATING );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "SDL_main start #%d: stale startup events flushed", GAndroidRunSerial );
}

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void AndroidSetAppSuspended( UBOOL bSuspended )
{
	if( GAndroidAppSuspended == bSuspended )
		return;

	GAndroidAppSuspended = bSuspended;

	if( bSuspended )
		appFlushConfigFiles();

	appHandleSuspendResume( bSuspended );

	if( UE1AndroidSetAudioSuspended )
		UE1AndroidSetAudioSuspended( bSuspended ? 1 : 0 );

	// Harmless for SDL audio backends and useful as an additional safety net.
	SDL_PauseAudio( bSuspended ? 1 : 0 );

	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android lifecycle audio/game %s", bSuspended ? "suspended" : "resumed" );
}

static int AndroidEventWatch( void* UserData, SDL_Event* Event )
{
	switch( Event->type )
	{
		case SDL_APP_WILLENTERBACKGROUND:
		case SDL_APP_DIDENTERBACKGROUND:
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "SDL app background event: %u", Event->type );
			AndroidSetAppSuspended( true );
			break;

		case SDL_APP_WILLENTERFOREGROUND:
		case SDL_APP_DIDENTERFOREGROUND:
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "SDL app foreground event: %u", Event->type );
			AndroidSetAppSuspended( false );
			break;

		case SDL_APP_TERMINATING:
		case SDL_APP_LOWMEMORY:
			__android_log_print( ANDROID_LOG_WARN, "UE1Android", "SDL app terminating/lowmemory event: %u", Event->type );
			appFlushConfigFiles();
			AndroidSetAppSuspended( true );
			break;

		case SDL_QUIT:
			if( UE1AndroidShouldIgnoreEarlyQuit() )
			{
				__android_log_print( ANDROID_LOG_WARN, "UE1Android", "Ignoring early SDL_QUIT during startup guard" );
				break;
			}
			__android_log_print( ANDROID_LOG_WARN, "UE1Android", "SDL_QUIT accepted" );
			appFlushConfigFiles();
			AndroidSetAppSuspended( true );
			break;

		default:
			break;
	}

	return 1;
}

static void PlatformPreInit()
{
	// Keep all Unreal runtime data inside the app-specific external files dir:
	// Android/data/com.ast.unreal/files/Unreal/
	appAndroidInitFileSystem();

	snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", appAndroidUnrealRootDir() );
	GAndroidRootPath[sizeof(GAndroidRootPath) - 1] = 0;

	const char* SystemPath = appAndroidUnrealSystemDir();

	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal System directory could not be created." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );

	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android system: %s", SystemPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}

	SDL_AddEventWatch( AndroidEventWatch, NULL );
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Do not read ANDROIDROOT from appCmdLine(): UE1 treats unknown KEY=value
	// tokens as map URLs. Derive the root natively from SDL instead.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", EnvRoot );
	}
	else
	{
		const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
		if( ExternalBase && ExternalBase[0] )
		{
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		}
	}

	if( !GAndroidRootPath[0] )
		EarlyError( "Could not resolve Android external app data path." );

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Do not read ANDROIDROOT from appCmdLine(): UE1 treats unknown KEY=value
	// tokens as map URLs. Derive the root natively from SDL instead.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", EnvRoot );
	}
	else
	{
		const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
		if( ExternalBase && ExternalBase[0] )
		{
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		}
	}

	if( !GAndroidRootPath[0] )
		EarlyError( "Could not resolve Android external app data path." );

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Do not read ANDROIDROOT from appCmdLine(): UE1 treats unknown KEY=value
	// tokens as map URLs. Derive the root natively from SDL instead.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", EnvRoot );
	}
	else
	{
		const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
		if( ExternalBase && ExternalBase[0] )
		{
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		}
	}

	if( !GAndroidRootPath[0] )
		EarlyError( "Could not resolve Android external app data path." );

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Do not read ANDROIDROOT from appCmdLine(): UE1 treats unknown KEY=value
	// tokens as map URLs. Derive the root natively from SDL instead.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", EnvRoot );
	}
	else
	{
		const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
		if( ExternalBase && ExternalBase[0] )
		{
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		}
	}

	if( !GAndroidRootPath[0] )
		EarlyError( "Could not resolve Android external app data path." );

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Do not read ANDROIDROOT from appCmdLine(): UE1 treats unknown KEY=value
	// tokens as map URLs. Derive the root natively from SDL instead.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", EnvRoot );
	}
	else
	{
		const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
		if( ExternalBase && ExternalBase[0] )
		{
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		}
	}

	if( !GAndroidRootPath[0] )
		EarlyError( "Could not resolve Android external app data path." );

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Do not read ANDROIDROOT from appCmdLine(): UE1 treats unknown KEY=value
	// tokens as map URLs. Derive the root natively from SDL instead.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", EnvRoot );
	}
	else
	{
		const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
		if( ExternalBase && ExternalBase[0] )
		{
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		}
	}

	if( !GAndroidRootPath[0] )
		EarlyError( "Could not resolve Android external app data path." );

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal and /sdcard/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal and /sdcard/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal and /sdcard/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal and /sdcard/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";

static bool AndroidDirExists( const char* Path )
{
	struct stat St;
	return stat( Path, &St ) == 0 && S_ISDIR( St.st_mode );
}

static bool AndroidLooksLikeUnrealRoot( const char* Root )
{
	if( !Root || !Root[0] )
		return false;

	char Path[MAX_PATH];
	snprintf( Path, sizeof(Path), "%s/System/Core.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/Engine.u", Root );
	if( access( Path, R_OK ) != 0 )
		return false;
	snprintf( Path, sizeof(Path), "%s/System/UnrealI.u", Root );
	if( access( Path, R_OK ) != 0 )
	{
		snprintf( Path, sizeof(Path), "%s/System/UnrealShare.u", Root );
		if( access( Path, R_OK ) != 0 )
			return false;
	}
	snprintf( Path, sizeof(Path), "%s/Maps", Root );
	return AndroidDirExists( Path );
}

static void AndroidTryRootCandidate( const char* Root )
{
	if( GAndroidRootPath[0] || !Root || !Root[0] )
		return;
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "checking data root candidate: %s", Root );
	if( AndroidLooksLikeUnrealRoot( Root ) )
	{
		snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s", Root );
		__android_log_print( ANDROID_LOG_INFO, "UE1Android", "using data root candidate: %s", GAndroidRootPath );
	}
}

static void AndroidCaptureRootArg( int argc, char** argv )
{
	for( int i = 1; i < argc; ++i )
	{
		if( !argv[i] )
			continue;
		if( !strcmp( argv[i], "--ue1-root" ) && i + 1 < argc && argv[i + 1] )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i + 1], 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i + 1] );
			return;
		}
		if( !strncmp( argv[i], "--ue1-root=", 11 ) )
		{
			setenv( "UE1_ANDROID_ROOT", argv[i] + 11, 1 );
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "root from Java: %s", argv[i] + 11 );
			return;
		}
	}
}

[[noreturn]] static void EarlyError( const char* Msg )
{
	__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "%s", Msg );
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, "Unreal Android", Msg, nullptr );
	abort();
}

static void PlatformPreInit()
{
	// UNREAL_ANDROID_NATIVE_ROOT_PATH
	// Java checks Android/data first, then public /Unreal and SD-root candidates.
	// It passes the selected path with --ue1-root; AndroidCaptureRootArg consumes it
	// before UE1 sees argv.
	const char* EnvRoot = getenv( "UE1_ANDROID_ROOT" );
	if( EnvRoot && EnvRoot[0] )
		AndroidTryRootCandidate( EnvRoot );

	const char* ExternalBase = SDL_AndroidGetExternalStoragePath();
	if( ExternalBase && ExternalBase[0] )
	{
		char Candidate[MAX_PATH];
		snprintf( Candidate, sizeof(Candidate), "%s/Unreal", ExternalBase );
		AndroidTryRootCandidate( Candidate );
	}

	AndroidTryRootCandidate( "/storage/emulated/0/Unreal" );
	AndroidTryRootCandidate( "/sdcard/Unreal" );
	AndroidTryRootCandidate( "/storage/sdcard0/Unreal" );
	AndroidTryRootCandidate( "/mnt/sdcard/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive/Unreal" );
	AndroidTryRootCandidate( "/mnt/usbdrive0/Unreal" );
	AndroidTryRootCandidate( "/mnt/usb_storage/Unreal" );

	if( !GAndroidRootPath[0] )
	{
		if( ExternalBase && ExternalBase[0] )
			snprintf( GAndroidRootPath, sizeof(GAndroidRootPath), "%s/Unreal", ExternalBase );
		else
			EarlyError( "Could not resolve Android external app data path." );
	}

	char SystemPath[MAX_PATH];
	snprintf( SystemPath, sizeof(SystemPath), "%s/System", GAndroidRootPath );
	if( !AndroidDirExists( SystemPath ) )
		EarlyError( "Unreal data was not found. Checked Android/data, /storage/emulated/0/Unreal, /sdcard/Unreal, /storage/sdcard0/Unreal and /mnt/usbdrive/Unreal. Copy your v200/v205 files to Android/data/com.ast.unreal/files/Unreal/ for maximum Android compatibility." );

	setenv( "HOME", GAndroidRootPath, 1 );
	setenv( "UE1_ANDROID_ROOT", GAndroidRootPath, 1 );
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "Android root: %s", GAndroidRootPath );

	if( chdir( SystemPath ) < 0 )
	{
		char Err[MAX_PATH + 128];
		snprintf( Err, sizeof(Err), "Could not chdir to %s: errno=%d", SystemPath, errno );
		EarlyError( Err );
	}
}

#else

void PlatformPreInit()
{

}

#endif
























































































//
// Handle an error.
//
void HandleError()
{
	GIsGuarded=0;
	GIsCriticalError=1;
	debugf( NAME_Exit, "Shutting down after catching exception" );
	GObj.ShutdownAfterError();
	debugf( NAME_Exit, "Exiting due to exception" );
	GErrorHist[ARRAY_COUNT(GErrorHist)-1]=0;
	SDL_ShowSimpleMessageBox( SDL_MESSAGEBOX_ERROR, LocalizeError("Critical"), GErrorHist, SDL_GetKeyboardFocus() );
}

//
// Initialize.
//
UEngine* InitEngine()
{
	guard(InitEngine);

	// Platform init.
	appInit();
	GDynMem.Init( 65536 );

	// Init subsystems.
	GSceneMem.Init( 32768 );

	// First-run menu.
	UBOOL FirstRun=0;
	GetConfigBool( "FirstRun", "FirstRun", FirstRun );

	// Create the global engine object.
	UClass* EngineClass;
	if( !GIsEditor )
	{
		// Create game engine.
		EngineClass = GObj.LoadClass( UGameEngine::StaticClass, NULL, "ini:Engine.Engine.GameEngine", NULL, LOAD_NoFail | LOAD_KeepImports, NULL );
	}
	else if( ParseParam( appCmdLine(),"MAKE" ) )
	{
		// Create editor engine.
		EngineClass = GObj.LoadClass( UEngine::StaticClass, NULL, "ini:Engine.Engine.EditorEngine", NULL, LOAD_NoFail | LOAD_DisallowFiles | LOAD_KeepImports, NULL );
	}
	else
	{
		// Editor.
		EngineClass = GObj.LoadClass( UEngine::StaticClass, NULL, "ini:Engine.Engine.EditorEngine", NULL, LOAD_NoFail | LOAD_KeepImports, NULL );
	}

	// Init engine.
	UEngine* Engine = ConstructClassObject<UEngine>( EngineClass );
	Engine->Init();

	return Engine;

	unguard;
}


#if defined(PLATFORM_ANDROID) && defined(ANDROID_LEGACY_API16)
static UBOOL UE1OuyaPerfConfigBool( const char* Key, UBOOL DefaultValue )
{
	char Value[64];
	if( GConfigCache.GetString( "OUYA.Performance", Key, Value, sizeof(Value) ) )
	{
		if( !appStricmp(Value, "True") || !appStricmp(Value, "Yes") || !appStricmp(Value, "On") || !appStrcmp(Value, "1") )
			return 1;
		if( !appStricmp(Value, "False") || !appStricmp(Value, "No") || !appStricmp(Value, "Off") || !appStrcmp(Value, "0") )
			return 0;
	}
	return DefaultValue;
}

static INT UE1OuyaPerfConfigInt( const char* Key, INT DefaultValue, INT MinValue, INT MaxValue )
{
	char Value[64];
	INT Result = DefaultValue;
	if( GConfigCache.GetString( "OUYA.Performance", Key, Value, sizeof(Value) ) )
		Result = appAtoi( Value );
	return Clamp( Result, MinValue, MaxValue );
}

static FLOAT UE1OuyaPerfConfigFloat( const char* Key, FLOAT DefaultValue, FLOAT MinValue, FLOAT MaxValue )
{
	char Value[64];
	FLOAT Result = DefaultValue;
	if( GConfigCache.GetString( "OUYA.Performance", Key, Value, sizeof(Value) ) )
		Result = appAtof( Value );
	return Clamp( Result, MinValue, MaxValue );
}

static void UE1OuyaPerfLogLine( const char* Fmt, ... )
{
	char Buffer[1024];
	va_list Args;
	va_start( Args, Fmt );
	vsnprintf( Buffer, sizeof(Buffer), Fmt, Args );
	va_end( Args );
	Buffer[sizeof(Buffer)-1] = 0;
	__android_log_write( ANDROID_LOG_INFO, "UE1Perf", Buffer );
	debugf( NAME_Log, "%s", Buffer );
}
#endif

//
// Unreal's main message loop.  All windows in Unreal receive messages
// somewhere below this function on the stack.
//
void MainLoop( UEngine* Engine )
{
	guard(MainLoop);

	GIsRunning = 1;
	DOUBLE OldTime = appSeconds();
	while( GIsRunning && !GIsRequestingExit )
	{
#if defined(PLATFORM_ANDROID)
		if( GAndroidAppSuspended )
		{
			SDL_PumpEvents();
			appSleep( 0.05 );
			OldTime = appSeconds();
			continue;
		}
#endif
		// Update the world.
#if defined(PLATFORM_ANDROID) && defined(ANDROID_LEGACY_API16)
		static DOUBLE PerfWindowStart = 0.0;
		static FLOAT PerfWindowTickMS = 0.0f;
		static INT PerfWindowFrames = 0;
		static DOUBLE PerfConfigLastRefresh = 0.0;
		static UBOOL bPerfLog = 0;
		static FLOAT PerfSpikeMS = 40.0f;
		static INT CachedFrameLimit = 0;
		const DOUBLE LoopStartTime = appSeconds();
		if( PerfConfigLastRefresh <= 0.0 || LoopStartTime - PerfConfigLastRefresh >= 0.5 )
		{
			bPerfLog = UE1OuyaPerfConfigBool( "EnablePerfLog", bPerfLog );
			PerfSpikeMS = UE1OuyaPerfConfigFloat( "PerfSpikeMs", PerfSpikeMS, 5.0f, 500.0f );
			CachedFrameLimit = UE1OuyaPerfConfigInt( "FrameLimit", CachedFrameLimit, 0, 120 );
			PerfConfigLastRefresh = LoopStartTime;
		}
		DOUBLE NewTime = LoopStartTime;
#else
		DOUBLE NewTime = appSeconds();
#endif
		Engine->Tick( NewTime - OldTime );
#if defined(PLATFORM_ANDROID) && defined(ANDROID_LEGACY_API16)
		const DOUBLE TickEndTime = appSeconds();
		const FLOAT TickMS = (FLOAT)( ( TickEndTime - LoopStartTime ) * 1000.0 );
		if( bPerfLog )
		{
			PerfWindowTickMS += TickMS;
			PerfWindowFrames++;
			if( PerfWindowStart <= 0.0 )
				PerfWindowStart = LoopStartTime;
			if( TickMS >= PerfSpikeMS )
				UE1OuyaPerfLogLine( "tick spikeMS=%.2f frameDeltaMS=%.2f", TickMS, (FLOAT)( ( NewTime - OldTime ) * 1000.0 ) );
			if( TickEndTime - PerfWindowStart >= 5.0 )
			{
				const FLOAT AvgTick = PerfWindowFrames > 0 ? PerfWindowTickMS / PerfWindowFrames : 0.0f;
				UE1OuyaPerfLogLine( "tick avg5s frames=%i avgTickMS=%.2f", PerfWindowFrames, AvgTick );
				PerfWindowStart = TickEndTime;
				PerfWindowTickMS = 0.0f;
				PerfWindowFrames = 0;
			}
		}
#endif
		OldTime = NewTime;

		// Enforce optional maximum tick rate.
		INT MaxTickRate = Engine->GetMaxTickRate();
#if defined(PLATFORM_ANDROID) && defined(ANDROID_LEGACY_API16)
		if( MaxTickRate <= 0 )
			MaxTickRate = CachedFrameLimit;
#endif
		if( MaxTickRate )
		{
			DOUBLE Delta = (1.0/MaxTickRate) - (appSeconds()-OldTime);
			if( Delta > 0.0 )
				appSleep( Delta );
		}
	}
	GIsRunning = 0;
	unguard;
}

//
// Exit the engine.
//
void ExitEngine( UEngine* Engine )
{
	guard(ExitEngine);

	GObj.Exit();
	GMem.Exit();
	GDynMem.Exit();
	GSceneMem.Exit();
	GCache.Exit(1);
	appDumpAllocs( &GTempPlatform );

	unguard;
}

#ifdef PLATFORM_WIN32
INT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char* InCmdLine, INT nCmdShow )
#elif defined(PLATFORM_ANDROID)
extern "C" __attribute__((visibility("default"))) int SDL_main( int argc, char** argv )
#else
int main( int argc, const char** argv )
#endif
{
#ifdef PLATFORM_WIN32
	hInstance = hInInstance;
#elif defined(PLATFORM_ANDROID)
	hInstance = NULL;
	AndroidResetRuntimeStateForStart();
	// SDLActivity looks up a C symbol named SDL_main with dlsym().
	// Keep Unreal's command-line helper using const char** internally.
	appSetCmdLine( argc, (const char**)argv );
	PlatformPreInit();
	AndroidFlushStaleStartupEvents();
#else
	hInstance = NULL;
	// Remember arguments since we don't have GetCommandLine().
	appSetCmdLine( argc, argv );
	PlatformPreInit();
#endif

	GIsStarted = 1;

	// Set package name.
	appStrcpy( THIS_PACKAGE, appPackage() );

	// Init mode.
	GIsServer = 1;
	GIsClient = !ParseParam(appCmdLine(),"SERVER") && !ParseParam(appCmdLine(),"MAKE");
	GIsEditor = ParseParam(appCmdLine(),"EDITOR") || ParseParam(appCmdLine(),"MAKE");

	// Init windowing.
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_SKIP_APPBASEDIR_CHDIR
	// PlatformPreInit already chdir()s to ANDROIDROOT/System.
	// Do not call appBaseDir() here: SDL_GetBasePath() may be null on Android.
#else
	appChdir( appBaseDir() );
#endif

	// Init log.
	// TODO: GLog
	GExecHook = GThisExecHook;

	// Begin.
#ifndef _DEBUG
	try
	{
#endif
			// Start main loop.
			GIsGuarded=1;
			GSystem = &GTempPlatform;
#ifdef PLATFORM_ANDROID
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "InitEngine begin" );
#endif
			UEngine* Engine = InitEngine();
#ifdef PLATFORM_ANDROID
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "InitEngine end: Engine=%p GIsRequestingExit=%d GIsCriticalError=%d", Engine, GIsRequestingExit, GIsCriticalError );
#endif
			if( !GIsRequestingExit )
			{
#ifdef PLATFORM_ANDROID
				__android_log_print( ANDROID_LOG_INFO, "UE1Android", "MainLoop begin" );
#endif
				MainLoop( Engine );
#ifdef PLATFORM_ANDROID
				__android_log_print( ANDROID_LOG_INFO, "UE1Android", "MainLoop end: GIsRequestingExit=%d", GIsRequestingExit );
#endif
			}
#ifdef PLATFORM_ANDROID
			else
			{
				__android_log_print( ANDROID_LOG_ERROR, "UE1Android", "MainLoop skipped because GIsRequestingExit=%d GIsCriticalError=%d", GIsRequestingExit, GIsCriticalError );
			}
#endif
			appFlushConfigFiles();
			ExitEngine( Engine );
#ifdef PLATFORM_ANDROID
			__android_log_print( ANDROID_LOG_INFO, "UE1Android", "ExitEngine returned" );
#endif
			GIsGuarded=0;
#ifndef _DEBUG
	}
	catch( ... )
	{
		// Crashed.
		try {HandleError();} catch( ... ) {}
	}
#endif

	// Shut down.
	GExecHook=NULL;
#ifdef PLATFORM_ANDROID
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "SDL_main shutting down via appExit" );
#endif
	appExit();
	GIsStarted = 0;
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_HARD_EXIT_AFTER_MENU_QUIT
	// When Unreal is closed through the in-game Beenden/Exit command, SDL_main
	// returns while the Android process can stay warm. SDLActivity and UE1 then
	// keep stale Java/native state and the next launcher tap may immediately
	// fall back to the Android launcher. Task-manager close works because it
	// kills the process. Do the same after a clean engine shutdown, but only
	// after config flushing, ExitEngine() and appExit() have completed.
	__android_log_print( ANDROID_LOG_INFO, "UE1Android", "SDL_main finished after in-game quit; terminating Android process for clean next launch" );
	SDL_Quit();
	fflush( NULL );
	_exit( 0 );
#endif
	return 0;
}
