import java.net.URI
import java.util.zip.ZipFile

plugins {
    id("com.android.application")
}

val ue1Version = "master"
val sdl2Version = "2.32.10"
val openAlSoftVersion = "1.23.1"
val overlayRevision = "rev27-ouya-api16"

val nativeRoot = layout.projectDirectory.dir("src/main/cpp")
val downloadsDir = layout.buildDirectory.dir("downloads")
val ue1Dir = nativeRoot.dir("UE1")
val thirdpartyDir = nativeRoot.dir("thirdparty")
val sdl2Dir = thirdpartyDir.dir("SDL2")
val openalDir = thirdpartyDir.dir("openal-soft")

fun downloadIfMissing(url: String, out: File) {
    if (out.isFile && out.length() > 0L) return
    out.parentFile.mkdirs()
    println("Downloading $url")
    URI(url).toURL().openStream().use { input ->
        out.outputStream().use { output -> input.copyTo(output) }
    }
}

fun extractZipStripRoot(zip: File, dest: File) {
    val prepared = dest.resolve(".prepared")
    if (prepared.isFile && prepared.readText().trim() == overlayRevision) return
    val tmp = dest.parentFile.resolve(dest.name + "-tmp")
    tmp.deleteRecursively()
    tmp.mkdirs()
    ZipFile(zip).use { zf ->
        val entries = zf.entries()
        while (entries.hasMoreElements()) {
            val e = entries.nextElement()
            if (e.isDirectory) continue
            val parts = e.name.split('/').filter { it.isNotEmpty() }
            if (parts.size <= 1) continue
            val rel = parts.drop(1).joinToString(File.separator)
            val out = tmp.resolve(rel)
            out.parentFile.mkdirs()
            zf.getInputStream(e).use { input -> out.outputStream().use { output -> input.copyTo(output) } }
        }
    }
    dest.deleteRecursively()
    tmp.renameTo(dest)
    dest.resolve(".prepared").writeText("$overlayRevision\n")
}

fun patchTextFile(file: File, action: (String) -> String) {
    val old = file.readText()
    val new = action(old)
    if (new != old) file.writeText(new)
}

fun requirePatched(file: File, marker: String) {
    if (!file.readText().contains(marker)) {
        throw GradleException("Required Android patch marker '$marker' missing in ${file.path}")
    }
}

fun patchUE1Source(root: File) {
    val source = root.resolve("Source")
    patchTextFile(source.resolve("CMakeLists.txt")) { input ->
        var s = input
        s = s.replace(
            "list(APPEND CMAKE_MODULE_PATH \${CMAKE_SOURCE_DIR}/cmake)",
            "list(APPEND CMAKE_MODULE_PATH \${CMAKE_CURRENT_SOURCE_DIR}/cmake)"
        )
        s = s.replace("set(CMAKE_CXX_STANDARD 14)", "set(CMAKE_CXX_STANDARD 17)")
        s = s.replace(
            "if(VITA)\n  set(BUILD_STATIC ON)\nendif()",
            "if(VITA OR ANDROID)\n  set(BUILD_STATIC ON)\nendif()"
        )
        s = s.replace(
            "if(TARGET_IS_64BIT)\n  if(TARGET_IS_X86 AND NOT MSVC)\n    message(STATUS \"Building x86 binary with x86_64 compiler\")\n    set(TARGET_IS_64BIT FALSE)\n    set(TARGET_ARCH \"x86\")\n    add_compile_options(-m32)\n  else()\n    message(FATAL_ERROR \"64-bit platforms are currently not supported. If you're building on Windows with MSVC, try -A Win32.\")\n  endif()\nendif()",
            "if(TARGET_IS_64BIT)\n  if(ANDROID)\n    message(STATUS \"Android arm64/x86_64 enabled; UE1 pointer-size audit remains experimental.\")\n  elseif(TARGET_IS_X86 AND NOT MSVC)\n    message(STATUS \"Building x86 binary with x86_64 compiler\")\n    set(TARGET_IS_64BIT FALSE)\n    set(TARGET_ARCH \"x86\")\n    add_compile_options(-m32)\n  else()\n    message(FATAL_ERROR \"64-bit platforms are currently not supported. If you're building on Windows with MSVC, try -A Win32.\")\n  endif()\nendif()"
        )
        s = s.replace(
            "if(TARGET_IS_WINDOWS)\n  add_definitions(-DPLATFORM_WIN32)\n  add_definitions(-DWIN32)\n  add_definitions(-DWINDOWS_IGNORE_PACKING_MISMATCH)\nelse()\n  # TODO\n  add_definitions(-DPLATFORM_POSIX)\n  if(VITA)\n    add_definitions(-DPLATFORM_PSVITA)\n  else()\n    add_definitions(-DPLATFORM_CASE_SENSITIVE_FS)\n    set(CMAKE_EXECUTABLE_SUFFIX \".bin\")\n  endif()\nendif()",
            "if(TARGET_IS_WINDOWS)\n  add_definitions(-DPLATFORM_WIN32)\n  add_definitions(-DWIN32)\n  add_definitions(-DWINDOWS_IGNORE_PACKING_MISMATCH)\nelse()\n  add_definitions(-DPLATFORM_POSIX)\n  if(VITA)\n    add_definitions(-DPLATFORM_PSVITA)\n  elseif(ANDROID)\n    add_definitions(-DPLATFORM_ANDROID -DUNREAL_ANDROID)\n    add_definitions(-DPLATFORM_CASE_SENSITIVE_FS)\n  else()\n    add_definitions(-DPLATFORM_CASE_SENSITIVE_FS)\n    set(CMAKE_EXECUTABLE_SUFFIX \".bin\")\n  endif()\nendif()"
        )
        s = s.replace(
            "set(CMAKE_SHARED_LIBRARY_PREFIX \"\")",
            "if(NOT ANDROID)\n  set(CMAKE_SHARED_LIBRARY_PREFIX \"\")\nendif()"
        )
        s
    }


    patchTextFile(source.resolve("Core/Src/UnClass.cpp")) { input ->
        input.replace(
            "Out.Logf( \"#define UCONST_%s %s\\r\\n\", ItC->GetName(), ItC->Value ),Consts++;",
            "Out.Logf( \"#define UCONST_%s %s\\r\\n\", ItC->GetName(), *ItC->Value ),Consts++;"
        )
    }


    // Android/SDL can return a null base path. UE1's old appBaseDir() path
    // feeds that pointer into appStrncpy(), so make the string helper tolerant
    // instead of relying on one fragile appBaseDir() text layout.
    patchTextFile(source.resolve("Core/Src/UnFile.cpp")) { input ->
        if (input.contains("UNREAL_ANDROID_NULLSAFE_STRNCPY_PATCH")) {
            input
        } else {
            val marker = "\tstrncpy( Dest, Src, MaxLen );\n\tDest[MaxLen-1]=0;\n\treturn Dest;"
            if (!input.contains(marker)) {
                throw GradleException("Could not find appStrncpy body in Core/Src/UnFile.cpp; Android null-safe string patch was not applied.")
            }
            input.replace(
                marker,
                "#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_NULLSAFE_STRNCPY_PATCH\n\tif( !Dest || MaxLen <= 0 )\n\t\treturn Dest;\n\tif( !Src )\n\t{\n\t\tDest[0] = 0;\n\t\treturn Dest;\n\t}\n#endif\n\tstrncpy( Dest, Src, MaxLen );\n\tDest[MaxLen-1]=0;\n\treturn Dest;"
            )
        }
    }



    // Mirror UE1's file log to Android logcat and print fatal errors before
    // the old engine deliberately breaks into the debugger. This lets us see
    // the real UGameEngine::Init() failure on device.
    patchTextFile(source.resolve("Core/Src/UnFile.cpp")) { input ->
        var s = input
        if (!s.contains("UNREAL_ANDROID_UNFILE_LOGCAT_INCLUDE")) {
            s = s.replace(
                "#include \"CorePrivate.h\"",
                "#include \"CorePrivate.h\"\n#ifdef PLATFORM_ANDROID\n#include <android/log.h> // UNREAL_ANDROID_UNFILE_LOGCAT_INCLUDE\n#endif"
            )
        }
        if (!s.contains("UNREAL_ANDROID_APPFPRINTF_LOGCAT")) {
            val regex = Regex("""CORE_API INT appFprintf\( FILE\* F, const char\* Fmt, \.\.\. \)\s*\{\s*char Temp\[32768\];\s*GET_VARARGS\(Temp,Fmt\);\s*return appFwrite\( Temp, 1, strlen\(Temp\), F \);\s*\}""")
            if (!regex.containsMatchIn(s)) {
                throw GradleException("Could not find appFprintf body in Core/Src/UnFile.cpp; Android logcat mirror patch was not applied.")
            }
            s = regex.replace(s, """CORE_API INT appFprintf( FILE* F, const char* Fmt, ... )
{
	char Temp[32768];
	GET_VARARGS(Temp,Fmt);
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_APPFPRINTF_LOGCAT
	__android_log_write( ANDROID_LOG_INFO, "UE1Log", Temp );
#endif
	return appFwrite( Temp, 1, strlen(Temp), F );
}""")
        }
        s
    }

    patchTextFile(source.resolve("IpDrv/Src/IpDrv.cpp")) { input ->
        var s = input
        s = s.replace(
            "String256,\n\t\t\t\"%s [%i.%i.%i.%i]:%i state: \",\n\t\t\tURL.Host,",
            "String256,\n\t\t\t\"%s [%i.%i.%i.%i]:%i state: \",\n\t\t\t*URL.Host,"
        )
        s = s.replace("debugf(NAME_Log,\"     Protocol: %s\", URL.Protocol  );", "debugf(NAME_Log,\"     Protocol: %s\", *URL.Protocol  );")
        s = s.replace("debugf(NAME_Log,\"         Host: %s\", URL.Host      );", "debugf(NAME_Log,\"         Host: %s\", *URL.Host      );")
        s = s.replace("debugf(NAME_Log,\"          Map: %s\", URL.Map       );", "debugf(NAME_Log,\"          Map: %s\", *URL.Map       );")
        s = s.replace("debugf(NAME_Log,\"       Portal: %s\", URL.Portal    );", "debugf(NAME_Log,\"       Portal: %s\", *URL.Portal    );")
        s = s.replace("debugf(NAME_Log,\"     Option %i: %s\", i, URL.Op(i) );", "debugf(NAME_Log,\"     Option %i: %s\", i, *URL.Op(i) );")
        s
    }

    patchTextFile(source.resolve("NOpenGLESDrv/CMakeLists.txt")) { input ->
        var s = input
        s = s.replace(
            "\${CMAKE_SOURCE_DIR}/../Thirdparty/glad_es/glad.c",
            "\${CMAKE_CURRENT_SOURCE_DIR}/../../Thirdparty/glad_es/glad.c"
        )
        s = s.replace(
            "\${CMAKE_SOURCE_DIR}/../Thirdparty/glad_es",
            "\${CMAKE_CURRENT_SOURCE_DIR}/../../Thirdparty/glad_es"
        )
        s = s.replace(
            "\${CMAKE_SOURCE_DIR}/../Thirdparty",
            "\${CMAKE_CURRENT_SOURCE_DIR}/../../Thirdparty"
        )
        s
    }
    patchTextFile(source.resolve("Unreal/CMakeLists.txt")) { input ->
        var s = input
        s = s.replace(
            "if(TARGET_IS_WINDOWS)\n  list(APPEND SRC_FILES \"Src/Res/LaunchRes.rc\")\n  add_executable(\${PROJECT_NAME} WIN32 \${SRC_FILES})\nelse()\n  add_executable(\${PROJECT_NAME} \${SRC_FILES})\nendif()",
            "if(ANDROID)\n  add_library(\${PROJECT_NAME} SHARED \${SRC_FILES})\nelseif(TARGET_IS_WINDOWS)\n  list(APPEND SRC_FILES \"Src/Res/LaunchRes.rc\")\n  add_executable(\${PROJECT_NAME} WIN32 \${SRC_FILES})\nelse()\n  add_executable(\${PROJECT_NAME} \${SRC_FILES})\nendif()"
        )
        s = s.replace(
            "if(USE_SDL)\n  target_link_libraries(\${PROJECT_NAME} \${SDL2_LIBRARY} NSDLDrv)\n  target_include_directories(\${PROJECT_NAME} PRIVATE \${SDL2_INCLUDE_DIR})\nelseif(TARGET_IS_WINDOWS)",
            "if(USE_SDL)\n  if(ANDROID AND BUILD_STATIC)\n    # NSDLDrv is already pulled in via LINK_PACKAGES under --whole-archive.\n    # Linking it here as well makes lld see each NSDLClient symbol twice.\n    target_link_libraries(\${PROJECT_NAME} \${SDL2_LIBRARY})\n  else()\n    target_link_libraries(\${PROJECT_NAME} \${SDL2_LIBRARY} NSDLDrv)\n  endif()\n  target_include_directories(\${PROJECT_NAME} PRIVATE \${SDL2_INCLUDE_DIR})\nelseif(TARGET_IS_WINDOWS)"
        )
        s = s.replace(
            "target_compile_definitions(\${PROJECT_NAME} PRIVATE UNREAL_EXPORTS UPACKAGE_NAME=\${PROJECT_NAME})",
            "target_compile_definitions(\${PROJECT_NAME} PRIVATE UNREAL_EXPORTS UPACKAGE_NAME=\${PROJECT_NAME})\n\nif(ANDROID)\n  target_link_libraries(\${PROJECT_NAME} android log GLESv2 EGL)\n  target_compile_definitions(\${PROJECT_NAME} PRIVATE PLATFORM_ANDROID UNREAL_ANDROID)\nendif()"
        )
        s
    }

    patchTextFile(source.resolve("Unreal/Src/SDLLaunch.cpp")) { input ->
        var s = input
        s = s.replace(
            "#include \"Engine.h\"",
            "#include \"Engine.h\"\n#ifdef PLATFORM_ANDROID\n#include <android/log.h>\n#include <errno.h>\n#include <sys/stat.h>\n#include <unistd.h>\n#include <string.h>\n#endif"
        )
        val marker = "#else\n\nvoid PlatformPreInit()\n{\n\n}\n\n#endif"
        val androidBlock = """
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
""".trimStart()
        s = s.replace(
            "#ifdef PLATFORM_WIN32\nINT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char* InCmdLine, INT nCmdShow )\n#else\nint main( int argc, const char** argv )\n#endif\n{\n#ifdef PLATFORM_WIN32\n\thInstance = hInInstance;\n#else\n\thInstance = NULL;\n\t// Remember arguments since we don't have GetCommandLine().\n\tappSetCmdLine( argc, argv );\n\tPlatformPreInit();\n#endif",
            "#ifdef PLATFORM_WIN32\nINT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char* InCmdLine, INT nCmdShow )\n#elif defined(PLATFORM_ANDROID)\nextern \"C\" __attribute__((visibility(\"default\"))) int SDL_main( int argc, char** argv )\n#else\nint main( int argc, const char** argv )\n#endif\n{\n#ifdef PLATFORM_WIN32\n\thInstance = hInInstance;\n#elif defined(PLATFORM_ANDROID)\n\thInstance = NULL;\n\t// SDLActivity looks up a C symbol named SDL_main with dlsym().\n\t// Consume Android-only path args before UE1 builds its command line.\n\tAndroidCaptureRootArg( argc, argv );\n\tappSetCmdLine( 1, (const char**)argv );\n\tPlatformPreInit();\n#else\n\thInstance = NULL;\n\t// Remember arguments since we don't have GetCommandLine().\n\tappSetCmdLine( argc, argv );\n\tPlatformPreInit();\n#endif"
        )
        s = s.replace(
            "\t// Init windowing.\n\tappChdir( appBaseDir() );",
            "\t// Init windowing.\n#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_SKIP_APPBASEDIR_CHDIR\n\t// PlatformPreInit already chdir()s to ANDROIDROOT/System.\n\t// Do not call appBaseDir() here: SDL_GetBasePath() may be null on Android.\n#else\n\tappChdir( appBaseDir() );\n#endif"
        )
        s = s.replace(marker, androidBlock)
        s
    }

    patchTextFile(source.resolve("Core/Src/UnPlat.cpp")) { input ->
        var s = input
        s = s.replace(
            "#include \"Core.h\"",
            "#include \"Core.h\"\n#ifdef PLATFORM_ANDROID\n#include <android/log.h> // UNREAL_ANDROID_LOGCAT_BRIDGE\n#endif"
        )
        s = s.replace(
            "void appDebugBreak()\n{\n\tguard(appDebugBreak);\n\n#ifdef PLATFORM_WIN32\n\t::DebugBreak();\n#else\n\t__builtin_trap();\n#endif\n\n\tunguard;\n}",
            "void appDebugBreak()\n{\n\tguard(appDebugBreak);\n\n#ifdef PLATFORM_WIN32\n\t::DebugBreak();\n#elif defined(PLATFORM_ANDROID)\n\tif( GErrorHist[0] )\n\t\t__android_log_write( ANDROID_LOG_ERROR, \"UE1\", GErrorHist );\n\tGIsRequestingExit = 1;\n\treturn;\n#else\n\t__builtin_trap();\n#endif\n\n\tunguard;\n}"
        )
        if (!s.contains("UNREAL_ANDROID_APPERROR_LOGCAT")) {
            val errorRegex = Regex("""void\s+appError\s*\(\s*const\s+char\s*\*\s*(\w+)\s*\)\s*\{""")
            val match = errorRegex.find(s)
                ?: throw GradleException("Could not find appError body in Core/Src/UnPlat.cpp; Android fatal-error logcat patch was not applied.")
            val errorVar = match.groupValues[1]
            val inserted = match.value + "\n#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_APPERROR_LOGCAT\n\t__android_log_print( ANDROID_LOG_ERROR, \"UE1\", \"appError: %s\", " + errorVar + " ? " + errorVar + " : \"(null)\" );\n\tif( GErrorHist[0] )\n\t\t__android_log_write( ANDROID_LOG_ERROR, \"UE1\", GErrorHist );\n#endif"
            s = s.replaceRange(match.range, inserted)
        }
        s
    }

    patchTextFile(source.resolve("NSDLDrv/Src/NSDLClient.cpp")) { input ->
        var s = input
        s = s.replace(
            "\tif ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER ) < 0 )",
            "#ifdef PLATFORM_ANDROID\n\tSDL_SetHint( SDL_HINT_ANDROID_TRAP_BACK_BUTTON, \"1\" );\n\tSDL_SetHint( SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, \"1\" );\n\tSDL_SetHint( SDL_HINT_RENDER_DRIVER, \"opengles2\" );\n#endif\n\n\tif ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER ) < 0 )"
        )
        s = s.replace(
            "\tif( SDL_NumJoysticks() > 0 )\n\t\tController = SDL_GameControllerOpen( 0 );",
            "\tfor( INT i = 0; i < SDL_NumJoysticks(); ++i )\n\t{\n\t\tif( SDL_IsGameController( i ) )\n\t\t{\n\t\t\tController = SDL_GameControllerOpen( i );\n\t\t\tif( Controller )\n\t\t\t{\n\t\t\t\tdebugf( NAME_Init, \"Opened SDL controller: %s\", SDL_GameControllerName( Controller ) );\n\t\t\t\tbreak;\n\t\t\t}\n\t\t}\n\t}"
        )
        s = s.replace("SDL_QuitSubSystem( SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER );", "SDL_QuitSubSystem( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER );")
        s
    }

    patchTextFile(source.resolve("Engine/Src/UnCanvas.cpp")) { input ->
        var s = input
        if (!s.contains("UNREAL_ANDROID_CANVAS_UI_SCALE_INCLUDE")) {
            s = s.replace(
                "#include \"UnRender.h\"",
                "#include \"UnRender.h\"\n#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_CANVAS_UI_SCALE_INCLUDE\n#include <stdlib.h>\n#include <stdio.h>\n#include <string.h>\n#endif"
            )
        }
        if (!s.contains("UNREAL_ANDROID_CANVAS_UI_SCALE_HELPER")) {
            s = s.replace(
                "/*-----------------------------------------------------------------------------\n\tUCanvas scaled sprites.\n-----------------------------------------------------------------------------*/",
                "#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_CANVAS_UI_SCALE_HELPER\nstatic FLOAT AndroidCanvasScale()\n{\n\tstatic FLOAT Scale = -1.0f;\n\tif( Scale < 0.0f )\n\t{\n\t\tScale = 2.0f;\n\n\t\tconst char* EnvScale = getenv( \"UE1_ANDROID_UI_SCALE\" );\n\t\tif( EnvScale && EnvScale[0] )\n\t\t{\n\t\t\tScale = (FLOAT)atof( EnvScale );\n\t\t}\n\t\telse\n\t\t{\n\t\t\tconst char* Root = getenv( \"UE1_ANDROID_ROOT\" );\n\t\t\tif( Root && Root[0] )\n\t\t\t{\n\t\t\t\tchar Path[1024];\n\t\t\t\tsnprintf( Path, sizeof(Path), \"%s/System/AndroidUI.ini\", Root );\n\t\t\t\tFILE* F = fopen( Path, \"r\" );\n\t\t\t\tif( F )\n\t\t\t\t{\n\t\t\t\t\tchar Line[256];\n\t\t\t\t\twhile( fgets( Line, sizeof(Line), F ) )\n\t\t\t\t\t{\n\t\t\t\t\t\tif( !strncmp( Line, \"UIScale=\", 8 ) )\n\t\t\t\t\t\t\tScale = (FLOAT)atof( Line + 8 );\n\t\t\t\t\t}\n\t\t\t\t\tfclose( F );\n\t\t\t\t}\n\t\t\t}\n\t\t}\n\n\t\tif( Scale < 1.0f )\n\t\t\tScale = 1.0f;\n\t\tif( Scale > 4.0f )\n\t\t\tScale = 4.0f;\n\t}\n\treturn Scale;\n}\n#endif\n\n/*-----------------------------------------------------------------------------\n\tUCanvas scaled sprites.\n-----------------------------------------------------------------------------*/"
            )
        }

        // Rev26: Do NOT scale the low-level UCanvas::DrawTile path globally.
        // Some non-HUD effects, especially projected coronas/lights, also use
        // this low-level path with already projected screen coordinates. Rev24
        // scaled those too, which made lights move with the UI. Only the
        // UnrealScript Canvas intrinsic below is scaled.
        if (!s.contains("UNREAL_ANDROID_CANVAS_UI_SCALE_EXEC_DRAW_TILE")) {
            val drawTileRegex = Regex("""\tif\(\s*Style!=STY_None\s*\)\s*DrawTile\s*\(\s*Tex,\s*OrgX\+CurX,\s*OrgX\+CurY,\s*XL,\s*YL,""")
            val newCall = """#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_CANVAS_UI_SCALE_EXEC_DRAW_TILE
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
#endif"""
            val match = drawTileRegex.find(s)
                ?: throw GradleException("Could not find UCanvas::execDrawTile call in Engine/Src/UnCanvas.cpp; Android UI scale patch was not applied.")
            s = s.replaceRange(match.range, newCall)
        }

        if (!s.contains("UNREAL_ANDROID_CANVAS_UI_SCALE_DRAW_CHAR")) {
            s = s.replace(
                "static inline void DrawChar( UCanvas* Canvas, FTextureInfo& Info, INT X, INT Y, INT XL, INT YL, INT U, INT V, INT UL, INT VL, FPlane Color )\n{\n\t// Reject.",
                "static inline void DrawChar( UCanvas* Canvas, FTextureInfo& Info, INT X, INT Y, INT XL, INT YL, INT U, INT V, INT UL, INT VL, FPlane Color )\n{\n#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_CANVAS_UI_SCALE_DRAW_CHAR\n\t// Text needs screen position and screen size scaled, but the font texture\n\t// UV range must remain original. Rev24 scaled X/Y but the renderer still\n\t// used UL/VL as the screen size, causing tiny letters with huge spacing.\n\tconst FLOAT AndroidUIScale = AndroidCanvasScale();\n\tif( AndroidUIScale != 1.0f )\n\t{\n\t\tX  = (INT)( X  * AndroidUIScale );\n\t\tY  = (INT)( Y  * AndroidUIScale );\n\t\tXL = (INT)( XL * AndroidUIScale );\n\t\tYL = (INT)( YL * AndroidUIScale );\n\t}\n#endif\n\t// Reject."
            )
        }
        if (!s.contains("UNREAL_ANDROID_CANVAS_UI_SCALE_DRAW_CHAR_SIZE_FIX")) {
            s = s.replace(
                "\tFrame->Viewport->RenDev->DrawTile( Frame, Info, X, Y, UL, VL, U, V, UL, VL, NULL, Canvas->Z, Color, FPlane(0,0,0,0), PF_NoSmooth | PF_Masked | PF_RenderHint );",
                "\tFrame->Viewport->RenDev->DrawTile( Frame, Info, X, Y, XL, YL, U, V, UL, VL, NULL, Canvas->Z, Color, FPlane(0,0,0,0), PF_NoSmooth | PF_Masked | PF_RenderHint ); // UNREAL_ANDROID_CANVAS_UI_SCALE_DRAW_CHAR_SIZE_FIX"
            )
        }
        if (!s.contains("UNREAL_ANDROID_CANVAS_UI_SCALE_LOGICAL_SIZE")) {
            s = s.replace(
                "\t// Copy size parameters from viewport.\n\tFrame = InFrame;\n\tX = ClipX = Frame->X;\n\tY = ClipY = Frame->Y;",
                "\t// Copy size parameters from viewport.\n\tFrame = InFrame;\n#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_CANVAS_UI_SCALE_LOGICAL_SIZE\n\tconst FLOAT AndroidUIScale = AndroidCanvasScale();\n\tif( AndroidUIScale != 1.0f )\n\t{\n\t\tX = ClipX = Frame->X / AndroidUIScale;\n\t\tY = ClipY = Frame->Y / AndroidUIScale;\n\t\tstatic int LoggedAndroidUIScale = 0;\n\t\tif( !LoggedAndroidUIScale )\n\t\t{\n\t\t\tdebugf( NAME_Log, \"Android UI scale: %f logical canvas %ix%i from frame %ix%i\", AndroidUIScale, (INT)ClipX, (INT)ClipY, Frame->X, Frame->Y );\n\t\t\tLoggedAndroidUIScale = 1;\n\t\t}\n\t}\n\telse\n#endif\n\t{\n\t\tX = ClipX = Frame->X;\n\t\tY = ClipY = Frame->Y;\n\t}"
            )
        }
        s
    }

    patchTextFile(source.resolve("NSDLDrv/Src/NSDLViewport.cpp")) { input ->
        var s = input

        if (!s.contains("UNREAL_ANDROID_ACTIVITY_FULLSCREEN_ONLY")) {
            s = s.replace(
                "\t\tif( DoOpenGL )\n\t\t{\n\t\t\tFlags |= SDL_WINDOW_OPENGL;\n\t\t}",
                "\t\tif( DoOpenGL )\n\t\t{\n\t\t\tFlags |= SDL_WINDOW_OPENGL;\n\t\t}\n#ifdef PLATFORM_ANDROID\n\t\t// UNREAL_ANDROID_ACTIVITY_FULLSCREEN_ONLY\n\t\t// The Android Activity/SurfaceView already owns fullscreen. Requesting\n\t\t// SDL_WINDOW_FULLSCREEN here can make Android allocate a rotated buffer\n\t\t// (for example 972x1920 transform=7) while the surface is 1920x1080.\n\t\tFlags |= SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE;\n#endif"
            )
        }

        if (!s.contains("UNREAL_ANDROID_GLES_PROFILE_ES")) {
            s = s.replace(
                "\t\t\tSDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, GLProfile );",
                "#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_GLES_PROFILE_ES\n\t\t\tGLProfile = SDL_GL_CONTEXT_PROFILE_ES;\n#endif\n\t\t\tSDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, GLProfile );"
            )
        }

        if (!s.contains("UNREAL_ANDROID_PRECREATE_DISPLAY_SIZE")) {
            s = s.replace(
                "\t\t// Create or update the window.\n\t\tif( !hWnd )",
                "#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_PRECREATE_DISPLAY_SIZE\n\t\t// Android/SDL may otherwise use UE1's historical 800x600 startup\n\t\t// size to decide orientation and SurfaceView bounds. Use the real\n\t\t// display size before SDL_CreateWindow, then resync with drawable size.\n\t\tSDL_DisplayMode AndroidMode;\n\t\tif( SDL_GetCurrentDisplayMode( 0, &AndroidMode ) == 0 && AndroidMode.w > 0 && AndroidMode.h > 0 )\n\t\t{\n\t\t\tINT AndroidW = AndroidMode.w;\n\t\t\tINT AndroidH = AndroidMode.h;\n\t\t\tif( AndroidH > AndroidW )\n\t\t\t\tExchange( AndroidW, AndroidH );\n\t\t\tif( NewX != AndroidW || NewY != AndroidH )\n\t\t\t\tdebugf( NAME_Log, \"Android pre-create display size: requested=%ix%i -> %ix%i\", NewX, NewY, AndroidW, AndroidH );\n\t\t\tNewX = Align( AndroidW, 4 );\n\t\t\tNewY = AndroidH;\n\t\t}\n#endif\n\n\t\t// Create or update the window.\n\t\tif( !hWnd )"
            )
        }

        if (!s.contains("UNREAL_ANDROID_SYNC_DRAWABLE_SIZE")) {
            s = s.replace(
                "\t\tSDL_ShowWindow( hWnd );\n\n\t\t// Get this window's display parameters.",
                "\t\tSDL_ShowWindow( hWnd );\n\n#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_SYNC_DRAWABLE_SIZE\n\t\t// Android may create a fullscreen surface whose real drawable size differs\n\t\t// from UE1's requested startup viewport. If SizeX/SizeY keep that old\n\t\t// value, GLES renders only into the lower-left part of the screen.\n\t\tint AndroidWindowW = 0, AndroidWindowH = 0;\n\t\tint AndroidDrawW = 0, AndroidDrawH = 0;\n\t\tSDL_GetWindowSize( hWnd, &AndroidWindowW, &AndroidWindowH );\n\t\tif( DoOpenGL )\n\t\t\tSDL_GL_GetDrawableSize( hWnd, &AndroidDrawW, &AndroidDrawH );\n\t\tif( AndroidDrawW <= 0 || AndroidDrawH <= 0 )\n\t\t{\n\t\t\tAndroidDrawW = AndroidWindowW;\n\t\t\tAndroidDrawH = AndroidWindowH;\n\t\t}\n\t\tif( AndroidDrawW > 0 && AndroidDrawH > 0 )\n\t\t{\n\t\t\tINT FixedX = Align( AndroidDrawW, 4 );\n\t\t\tINT FixedY = AndroidDrawH;\n\t\t\tif( NewX != FixedX || NewY != FixedY )\n\t\t\t{\n\t\t\t\tdebugf( NAME_Log, \"Android drawable size: window=%ix%i drawable=%ix%i viewport=%ix%i -> %ix%i\", AndroidWindowW, AndroidWindowH, AndroidDrawW, AndroidDrawH, NewX, NewY, FixedX, FixedY );\n\t\t\t\tNewX = FixedX;\n\t\t\t\tNewY = FixedY;\n\t\t\t}\n\t\t}\n#endif\n\n\t\t// Get this window's display parameters."
            )
        }

        if (!s.contains("UNREAL_ANDROID_SETCLIENT_DRAWABLE_SIZE")) {
            s = s.replace(
                "\t\tSDL_SetWindowSize( hWnd, NewX, NewY );\n\t\t// Resize output texture if required.",
                "#ifndef PLATFORM_ANDROID // UNREAL_ANDROID_NO_SETWINDOWSIZE_800X600\n\t\tSDL_SetWindowSize( hWnd, NewX, NewY );\n#endif\n#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_SETCLIENT_DRAWABLE_SIZE\n\t\tint AndroidWindowW = 0, AndroidWindowH = 0;\n\t\tint AndroidDrawW = 0, AndroidDrawH = 0;\n\t\tSDL_GetWindowSize( hWnd, &AndroidWindowW, &AndroidWindowH );\n\t\tif( GLCtx )\n\t\t\tSDL_GL_GetDrawableSize( hWnd, &AndroidDrawW, &AndroidDrawH );\n\t\tif( AndroidDrawW <= 0 || AndroidDrawH <= 0 )\n\t\t{\n\t\t\tAndroidDrawW = AndroidWindowW;\n\t\t\tAndroidDrawH = AndroidWindowH;\n\t\t}\n\t\tif( AndroidDrawW > 0 && AndroidDrawH > 0 )\n\t\t{\n\t\t\tNewX = Align( AndroidDrawW, 4 );\n\t\t\tNewY = AndroidDrawH;\n\t\t}\n#endif\n\t\t// Resize output texture if required."
            )
        }

        if (!s.contains("UNREAL_ANDROID_FULLSCREEN_NO_MODE_SWITCH")) {
            s = s.replace(
                "\tClient->FullscreenViewport = this;\n\tSetClientSize( NewX, NewY, false );\n\tSDL_SetWindowFullscreen( hWnd, SDL_WINDOW_FULLSCREEN );",
                "\tClient->FullscreenViewport = this;\n#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_FULLSCREEN_NO_MODE_SWITCH\n\t// Keep Android in Activity fullscreen only. A real SDL fullscreen mode switch\n\t// can produce a portrait-sized rotated buffer on some handhelds.\n\tint AndroidWindowW = 0, AndroidWindowH = 0;\n\tint AndroidDrawW = 0, AndroidDrawH = 0;\n\tSDL_GetWindowSize( hWnd, &AndroidWindowW, &AndroidWindowH );\n\tif( GLCtx )\n\t\tSDL_GL_GetDrawableSize( hWnd, &AndroidDrawW, &AndroidDrawH );\n\tif( AndroidDrawW <= 0 || AndroidDrawH <= 0 )\n\t{\n\t\tAndroidDrawW = AndroidWindowW;\n\t\tAndroidDrawH = AndroidWindowH;\n\t}\n\tif( AndroidDrawW > 0 && AndroidDrawH > 0 )\n\t{\n\t\tNewX = Align( AndroidDrawW, 4 );\n\t\tNewY = AndroidDrawH;\n\t}\n\tSetClientSize( NewX, NewY, false );\n#else\n\tSetClientSize( NewX, NewY, false );\n\tSDL_SetWindowFullscreen( hWnd, SDL_WINDOW_FULLSCREEN );\n#endif"
            )
        }

        if (!s.contains("UNREAL_ANDROID_FULLSCREEN_END_NOOP")) {
            s = s.replace(
                "\tSDL_SetWindowFullscreen( hWnd, 0 );\n\tSetClientSize( SavedX, SavedY, false );",
                "#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_FULLSCREEN_END_NOOP\n\t// Stay in Activity fullscreen on Android; just resync to current surface.\n\tint AndroidWindowW = 0, AndroidWindowH = 0;\n\tint AndroidDrawW = 0, AndroidDrawH = 0;\n\tSDL_GetWindowSize( hWnd, &AndroidWindowW, &AndroidWindowH );\n\tif( GLCtx )\n\t\tSDL_GL_GetDrawableSize( hWnd, &AndroidDrawW, &AndroidDrawH );\n\tif( AndroidDrawW <= 0 || AndroidDrawH <= 0 )\n\t{\n\t\tAndroidDrawW = AndroidWindowW;\n\t\tAndroidDrawH = AndroidWindowH;\n\t}\n\tif( AndroidDrawW > 0 && AndroidDrawH > 0 )\n\t\tSetClientSize( Align(AndroidDrawW,4), AndroidDrawH, false );\n#else\n\tSDL_SetWindowFullscreen( hWnd, 0 );\n\tSetClientSize( SavedX, SavedY, false );\n#endif"
            )
        }

        if (!s.contains("UNREAL_ANDROID_NO_SETWINDOWSIZE_800X600") && s.contains("UNREAL_ANDROID_SETCLIENT_DRAWABLE_SIZE")) {
            // Rev20 already-prepared source trees had the drawable-size block
            // but still kept SDL_SetWindowSize above it. Upgrade them in-place.
            s = s.replace(
                "\t\tSDL_SetWindowSize( hWnd, NewX, NewY );\n#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_SETCLIENT_DRAWABLE_SIZE",
                "#ifndef PLATFORM_ANDROID // UNREAL_ANDROID_NO_SETWINDOWSIZE_800X600\n\t\tSDL_SetWindowSize( hWnd, NewX, NewY );\n#endif\n#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_SETCLIENT_DRAWABLE_SIZE"
            )
        }

        if (!s.contains("UNREAL_ANDROID_START_IS_ESCAPE")) {
            s = s.replace(
                "\t/* BUTTON_START         */ IK_Joy7,",
                "#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_START_IS_ESCAPE\n\t/* BUTTON_START         */ IK_Escape,\n#else\n\t/* BUTTON_START         */ IK_Joy7,\n#endif"
            )
        }

        if (!s.contains("UNREAL_ANDROID_RIGHT_STICK_MOUSELOOK")) {
            s = s.replace(
                "\t/* AXIS_RIGHT_X         */ IK_JoyU,\n\t/* AXIS_RIGHT_Y         */ IK_JoyV,",
                "#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_RIGHT_STICK_MOUSELOOK\n\t/* AXIS_RIGHT_X         */ IK_MouseX,\n\t/* AXIS_RIGHT_Y         */ IK_MouseY,\n#else\n\t/* AXIS_RIGHT_X         */ IK_JoyU,\n\t/* AXIS_RIGHT_Y         */ IK_JoyV,\n#endif"
            )
        }

        if (!s.contains("UNREAL_ANDROID_MOUSELOOK_SLOWER")) {
            s = s.replace(
                "\t/* AXIS_RIGHT_X         */ +60.f,\n\t/* AXIS_RIGHT_Y         */ +60.f,",
                "#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_MOUSELOOK_SLOWER\n\t/* AXIS_RIGHT_X         */ +4.f,\n\t/* AXIS_RIGHT_Y         */ +4.f,\n#else\n\t/* AXIS_RIGHT_X         */ +60.f,\n\t/* AXIS_RIGHT_Y         */ +60.f,\n#endif"
            )
        }

        if (!s.contains("UNREAL_ANDROID_MOUSELOOK_AXIS_DEADZONE")) {
            s = s.replace(
                "\t\t\t\t\tif ( Key < IK_JoyX )",
                "\t\t\t\t\tif ( Key < IK_JoyX\n#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_MOUSELOOK_AXIS_DEADZONE\n\t\t\t\t\t\t&& Key != IK_MouseX && Key != IK_MouseY\n#endif\n\t\t\t\t\t)"
            )
            s = s.replace(
                "\t\t\t\t\t\telse if ( Key == IK_JoyR || Key == IK_JoyU || Key == IK_JoyV )\n\t\t\t\t\t\t\tDeadZone = Client->DeadZoneRUV * 32767.f;",
                "\t\t\t\t\t\telse if ( Key == IK_JoyR || Key == IK_JoyU || Key == IK_JoyV\n#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_MOUSELOOK_AXIS_DEADZONE_RUV\n\t\t\t\t\t\t\t|| Key == IK_MouseX || Key == IK_MouseY\n#endif\n\t\t\t\t\t\t)\n\t\t\t\t\t\t\tDeadZone = Client->DeadZoneRUV * 32767.f;"
            )
        }
        s
    }

}

val prepareSources = tasks.register("prepareSources") {
    group = "setup"
    description = "Downloads UE1, SDL2 and OpenAL Soft sources and applies the Android patch overlay."
    doLast {
        val dl = downloadsDir.get().asFile
        val ueZip = dl.resolve("UE1-${ue1Version}.zip")
        val sdlZip = dl.resolve("SDL2-${sdl2Version}.zip")
        val alZip = dl.resolve("openal-soft-${openAlSoftVersion}.zip")

        downloadIfMissing("https://github.com/fgsfdsfgs/UE1/archive/refs/heads/${ue1Version}.zip", ueZip)
        downloadIfMissing("https://www.libsdl.org/release/SDL2-${sdl2Version}.zip", sdlZip)
        downloadIfMissing("https://github.com/kcat/openal-soft/archive/refs/tags/${openAlSoftVersion}.zip", alZip)

        extractZipStripRoot(ueZip, ue1Dir.asFile)
        extractZipStripRoot(sdlZip, sdl2Dir.asFile)
        extractZipStripRoot(alZip, openalDir.asFile)
        patchUE1Source(ue1Dir.asFile)
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnFile.cpp"), "UNREAL_ANDROID_NULLSAFE_STRNCPY_PATCH")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnFile.cpp"), "UNREAL_ANDROID_APPFPRINTF_LOGCAT")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnPlat.cpp"), "UNREAL_ANDROID_APPERROR_LOGCAT")
        requirePatched(ue1Dir.asFile.resolve("Source/Unreal/Src/SDLLaunch.cpp"), "SDLActivity looks up a C symbol named SDL_main")
        requirePatched(ue1Dir.asFile.resolve("Source/Unreal/Src/SDLLaunch.cpp"), "UNREAL_ANDROID_SKIP_APPBASEDIR_CHDIR")
        requirePatched(ue1Dir.asFile.resolve("Source/Unreal/Src/SDLLaunch.cpp"), "UNREAL_ANDROID_NATIVE_ROOT_PATH")
        requirePatched(ue1Dir.asFile.resolve("Source/Unreal/Src/SDLLaunch.cpp"), "--ue1-root")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnPlat.cpp"), "UNREAL_ANDROID_LOGCAT_BRIDGE")
        requirePatched(ue1Dir.asFile.resolve("Source/Engine/Src/UnCanvas.cpp"), "UNREAL_ANDROID_CANVAS_UI_SCALE_HELPER")
        requirePatched(ue1Dir.asFile.resolve("Source/Engine/Src/UnCanvas.cpp"), "UNREAL_ANDROID_CANVAS_UI_SCALE_EXEC_DRAW_TILE")
        requirePatched(ue1Dir.asFile.resolve("Source/Engine/Src/UnCanvas.cpp"), "UNREAL_ANDROID_CANVAS_UI_SCALE_DRAW_CHAR")
        requirePatched(ue1Dir.asFile.resolve("Source/Engine/Src/UnCanvas.cpp"), "UNREAL_ANDROID_CANVAS_UI_SCALE_DRAW_CHAR_SIZE_FIX")
        requirePatched(ue1Dir.asFile.resolve("Source/Engine/Src/UnCanvas.cpp"), "UNREAL_ANDROID_CANVAS_UI_SCALE_LOGICAL_SIZE")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_SYNC_DRAWABLE_SIZE")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_SETCLIENT_DRAWABLE_SIZE")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_ACTIVITY_FULLSCREEN_ONLY")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_FULLSCREEN_NO_MODE_SWITCH")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_FULLSCREEN_END_NOOP")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_PRECREATE_DISPLAY_SIZE")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_NO_SETWINDOWSIZE_800X600")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_START_IS_ESCAPE")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_RIGHT_STICK_MOUSELOOK")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_MOUSELOOK_SLOWER")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_MOUSELOOK_AXIS_DEADZONE")
        projectDir.resolve(".cxx").deleteRecursively()
        layout.buildDirectory.dir("intermediates/cxx").get().asFile.deleteRecursively()
    }
}

android {
    namespace = "com.ast.unreal"
    compileSdk = 33

    signingConfigs {
        create("release") {
            storeFile = file("E:/Development/Android/UE1/YOUR_KEYSTORE.jks")
            storePassword = "DEIN_STORE_PASSWORT"
            keyAlias = "DEIN_KEY_ALIAS"
            keyPassword = "DEIN_KEY_PASSWORT"
        }
    }

    buildTypes {
        create("ouyaSignedDebug") {
            initWith(getByName("debug"))

            // Deine bestehende Release-Signatur verwenden
            signingConfig = signingConfigs.getByName("release")

            // Wichtig: Debug-Verhalten behalten
            isDebuggable = true
            isJniDebuggable = true
            isMinifyEnabled = false
            isShrinkResources = false

            // Kein Suffix, wenn diese APK die normale com.ast.unreal ersetzen soll
            applicationIdSuffix = null
            versionNameSuffix = "-OUYA-Signed"

            matchingFallbacks += listOf("debug")
        }
    }

    // Replaces deprecated gradle.properties option: android.defaults.buildfeatures.buildconfig=true
    buildFeatures {
        buildConfig = true
    }

    lint {
        checkReleaseBuilds = false
        abortOnError = false
        disable.add("ExpiredTargetSdkVersion")
    }

    defaultConfig {
        applicationId = "com.ast.unreal"
        minSdk = 16
        targetSdk = 19
        versionCode = 1
        versionName = "1.0.2"

        ndk {
            abiFilters += listOf("armeabi-v7a")
            // Optional later, after the UE1 pointer-size audit is complete:
            // abiFilters += listOf("arm64-v8a", "x86_64")
        }

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DANDROID_PLATFORM=android-16",
                    "-DUNREAL_ANDROID_OUYA=ON",
                    "-DANDROID_LEGACY_API16=ON"
                )
                cppFlags += listOf("-std=c++17", "-DPLATFORM_ANDROID", "-DUNREAL_ANDROID", "-DUNREAL_ANDROID_OUYA", "-DANDROID_LEGACY_API16", "-DWITH_GLES2", "-DWITH_SDL2", "-DWITH_OPENAL", "-fexceptions", "-frtti")
            }
        }
    }

    ndkVersion = "23.2.8568313"

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    sourceSets {
        getByName("main") {
            java.srcDirs(
                "src/main/java",
                "src/main/cpp/thirdparty/SDL2/android-project/app/src/main/java"
            )
            assets.srcDir("src/main/assets")
        }
    }

    packagingOptions {
        jniLibs {
            useLegacyPackaging = true
        }
        resources {
            excludes += setOf("/META-INF/{AL2.0,LGPL2.1}")
        }
    }
}

afterEvaluate {
    tasks.matching {
        it.name == "preBuild" ||
        it.name.startsWith("configureCMake") ||
        it.name.startsWith("externalNativeBuild") ||
        it.name.contains("JavaWithJavac")
    }.configureEach {
        dependsOn(prepareSources)
    }
}


// OUYA legacy: sources are already prepared/patched. Do not download/overwrite them again.
tasks.named("prepareSources") {
    enabled = false
}

