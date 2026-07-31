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
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_SDLLAUNCH_INCLUDES
#include <android/log.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
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
#define VGL_MEM_THRESHOLD ( 4 * 1024 * 1024 )

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
	
	vglSetParamBufferSize(6 * 1024 * 1024);
	vglSetCircularPoolSize(3 * 1024 * 1024);
	vglInitWithCustomThreshold( 0, 960, 544, VGL_MEM_THRESHOLD, 0, 0, 0, SCE_GXM_MULTISAMPLE_4X );
}

#elif defined(PLATFORM_ANDROID)

#define MAX_PATH 1024
static char GAndroidRootPath[MAX_PATH] = "";
static Uint32 GAndroidIgnoreQuitUntilTicks = 0;
static INT GAndroidRunSerial = 0;

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
static Uint32 GAndroidIgnoreQuitUntilTicks = 0;
static INT GAndroidRunSerial = 0;

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
static Uint32 GAndroidIgnoreQuitUntilTicks = 0;
static INT GAndroidRunSerial = 0;

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
		// Update the world.
		DOUBLE NewTime = appSeconds();
		Engine->Tick( NewTime - OldTime );
		OldTime = NewTime;

		// Enforce optional maximum tick rate.
		INT MaxTickRate = Engine->GetMaxTickRate();
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
	// Consume Android-only path args before UE1 builds its command line.
	AndroidCaptureRootArg( argc, argv );
	appSetCmdLine( 1, (const char**)argv );
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
		UEngine* Engine = InitEngine();
		if( !GIsRequestingExit )
			MainLoop( Engine );
		ExitEngine( Engine );
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
	appExit();
	GIsStarted = 0;
	return 0;
}
