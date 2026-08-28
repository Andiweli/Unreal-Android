import java.net.URI
import java.util.zip.ZipFile

plugins {
    id("com.android.application")
}

val ue1Version = "51b0ecdad7e2d026485d7ec7cd0b5a77bd1ff026"
val sdl2Version = "2.32.10"
val openAlSoftVersion = "1.24.3" // C++17: compatible with NDK r23/API16 and includes Android 16KB-page support
val overlayRevision = "rev49-unified-normal-ouya-automotive-v1-signing-debugkey"
val androidVersionName = "2.2.0"

val nativeRoot = layout.projectDirectory.dir("src/main/cpp")
val downloadsDir = layout.buildDirectory.dir("downloads")
val ue1Dir = nativeRoot.dir("UE1")
val ue1PatchOverlayDir = layout.projectDirectory.dir("src/main/ue1_patch_overlay") // UNREAL_ANDROID_TOUCH_OVERLAY_SOURCE_OVERLAY_V125
val sdl2PatchOverlayDir = layout.projectDirectory.dir("src/main/sdl2_patch_overlay") // UNREAL_ANDROID_CHROMEOS_MOUSE_FRAMEPACED_OVERLAY_V210
val sdlApi16PatchOverlayDir = layout.projectDirectory.dir("src/main/sdl_api16_patch_overlay") // UNREAL_ANDROID_API16_SDL_OVERLAY_V212
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

fun applyUE1PatchOverlayV125(root: File) {
    // UNREAL_ANDROID_TOUCH_OVERLAY_SOURCE_OVERLAY_V125
    val overlay = ue1PatchOverlayDir.asFile
    if (!overlay.isDirectory) return
    copy {
        from(overlay)
        into(root)
    }
}

fun applySDL2PatchOverlayV210(root: File) {
    // UNREAL_ANDROID_CHROMEOS_MOUSE_FRAMEPACED_OVERLAY_V210
    val overlay = sdl2PatchOverlayDir.asFile
    if (!overlay.isDirectory) return
    copy {
        from(overlay)
        into(root)
    }
}

fun applySDLApi16PatchOverlayV212(root: File) {
    // UNREAL_ANDROID_API16_SDL_OVERLAY_V212
    val overlay = sdlApi16PatchOverlayDir.asFile
    if (!overlay.isDirectory) return
    copy {
        from(overlay)
        into(root)
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
        if (!s.contains("UNREAL_ANDROID_KEEP_SHARED_LIB_PREFIX")) {
            s = s.replace(
                "set(CMAKE_SHARED_LIBRARY_PREFIX \"\")",
                "# Android needs the normal lib prefix for libUnreal.so.\nif(NOT ANDROID) # UNREAL_ANDROID_KEEP_SHARED_LIB_PREFIX\n  set(CMAKE_SHARED_LIBRARY_PREFIX \"\")\nendif()"
            )
        }
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



    patchTextFile(source.resolve("Core/Src/UnFile.cpp")) { input ->
        input.replace(
            "unguardf(( \"%08X %i %s\", (INT)Ptr, NewSize, Tag ));",
            "unguardf(( \"%p %i %s\", Ptr, NewSize, Tag ));"
        )
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
        if (!s.contains("UNREAL_ANDROID_SDLLAUNCH_INCLUDES")) {
            s = s.replace(
                "#include \"Engine.h\"",
                "#include \"Engine.h\"\n#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_SDLLAUNCH_INCLUDES\n#include <android/log.h>\n#include <errno.h>\n#include <sys/stat.h>\n#include <unistd.h>\n#include <string.h>\n#endif"
            )
        }
        val marker = "#else\n\nvoid PlatformPreInit()\n{\n\n}\n\n#endif"
        val androidBlock = """
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
""".trimStart()
        s = s.replace(
            "#ifdef PLATFORM_WIN32\nINT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char* InCmdLine, INT nCmdShow )\n#else\nint main( int argc, const char** argv )\n#endif\n{\n#ifdef PLATFORM_WIN32\n\thInstance = hInInstance;\n#else\n\thInstance = NULL;\n\t// Remember arguments since we don't have GetCommandLine().\n\tappSetCmdLine( argc, argv );\n\tPlatformPreInit();\n#endif",
            "#ifdef PLATFORM_WIN32\nINT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char* InCmdLine, INT nCmdShow )\n#elif defined(PLATFORM_ANDROID)\nextern \"C\" __attribute__((visibility(\"default\"))) int SDL_main( int argc, char** argv )\n#else\nint main( int argc, const char** argv )\n#endif\n{\n#ifdef PLATFORM_WIN32\n\thInstance = hInInstance;\n#elif defined(PLATFORM_ANDROID)\n\thInstance = NULL;\n\tAndroidResetRuntimeStateForStart();\n\t// SDLActivity looks up a C symbol named SDL_main with dlsym().\n\t// Consume Android-only path args before UE1 builds its command line.\n\tAndroidCaptureRootArg( argc, argv );\n\tappSetCmdLine( 1, (const char**)argv );\n\tPlatformPreInit();\n\tAndroidFlushStaleStartupEvents();\n#else\n\thInstance = NULL;\n\t// Remember arguments since we don't have GetCommandLine().\n\tappSetCmdLine( argc, argv );\n\tPlatformPreInit();\n#endif"
        )
        if (!s.contains("UNREAL_ANDROID_SKIP_APPBASEDIR_CHDIR")) {
            s = s.replace(
                "\t// Init windowing.\n\tappChdir( appBaseDir() );",
                "\t// Init windowing.\n#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_SKIP_APPBASEDIR_CHDIR\n\t// PlatformPreInit already chdir()s to ANDROIDROOT/System.\n\t// Do not call appBaseDir() here: SDL_GetBasePath() may be null on Android.\n#else\n\tappChdir( appBaseDir() );\n#endif"
            )
        }
        s = s.replace(marker, androidBlock)
        s
    }

    patchTextFile(source.resolve("Core/Src/UnPlat.cpp")) { input ->
        var s = input
        if (!s.contains("UNREAL_ANDROID_LOGCAT_BRIDGE")) {
            s = s.replace(
                "#include \"Core.h\"",
                "#include \"Core.h\"\n#ifdef PLATFORM_ANDROID\n#include <android/log.h> // UNREAL_ANDROID_LOGCAT_BRIDGE\n#endif"
            )
        }
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
        s = s.replace(
            "CORE_API DWORD hWndMain=0, hWndProgressBar=0, hWndProgressText=0, hWndCallback=0;",
            "CORE_API UPTRINT hWndMain=0, hWndProgressBar=0, hWndProgressText=0, hWndCallback=0;"
        )
        s = s.replace(
            "\t\t\tParse( Str, \"PROGRESSBAR=\",  hWndProgressBar );\n\t\t\tParse( Str, \"PROGRESSTEXT=\", hWndProgressText );",
            "\t\t\tQWORD TmpBar=0, TmpText=0;\n\t\t\tif( Parse( Str, \"PROGRESSBAR=\",  TmpBar  ) ) hWndProgressBar  = (UPTRINT)TmpBar;\n\t\t\tif( Parse( Str, \"PROGRESSTEXT=\", TmpText ) ) hWndProgressText = (UPTRINT)TmpText;"
        )
        if (!s.contains("UNREAL_ANDROID_APP_FLUSH_CONFIG_FILES")) {
            val appExitMarker = "void appExit()\n{\n\tdebugf( NAME_Exit, \"appExit\" );\n\tappDumpAllocs( GSystem );\n\tappCloseLog();\n}"
            val appFlushBlock = """
CORE_API void appFlushConfigFiles() // UNREAL_ANDROID_APP_FLUSH_CONFIG_FILES
{
	guard(appFlushConfigFiles);

	if( !GIsStarted )
	{
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
		__android_log_print( ANDROID_LOG_WARN, "UE1Config", "Config flush skipped: engine not started" );
#endif
		return;
	}

	UBOOL Ok = GConfigCache.SaveAllConfigs();

#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	__android_log_print( ANDROID_LOG_INFO, "UE1Config", "GConfigCache flush result: %s", Ok ? "OK" : "FAILED" );
#endif
	debugf( "Config flush result: %s", Ok ? "OK" : "FAILED" );

	unguard;
}

void appExit()
{
	appFlushConfigFiles();
	debugf( NAME_Exit, "appExit" );
	appDumpAllocs( GSystem );
	appCloseLog();
}
""".trimStart()
            if (!s.contains(appExitMarker))
                throw GradleException("Could not find appExit in Core/Src/UnPlat.cpp; config flush implementation was not installed.")
            s = s.replace(appExitMarker, appFlushBlock)
        }
        s
    }

    patchTextFile(source.resolve("NSDLDrv/Src/NSDLClient.cpp")) { input ->
        var s = input
        if (!s.contains("UNREAL_ANDROID_SDL_INIT_HINTS")) {
            s = s.replace(
                "\tif ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER ) < 0 )",
                "#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_SDL_INIT_HINTS\n\tSDL_SetHint( SDL_HINT_ANDROID_TRAP_BACK_BUTTON, \"1\" );\n\tSDL_SetHint( SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, \"1\" );\n\tSDL_SetHint( SDL_HINT_RENDER_DRIVER, \"opengles2\" );\n#endif\n\n\tif ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER ) < 0 )"
            )
        }
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

        downloadIfMissing("https://github.com/fgsfdsfgs/UE1/archive/${ue1Version}.zip", ueZip)
        downloadIfMissing("https://www.libsdl.org/release/SDL2-${sdl2Version}.zip", sdlZip)
        downloadIfMissing("https://github.com/kcat/openal-soft/archive/refs/tags/${openAlSoftVersion}.zip", alZip)

        extractZipStripRoot(ueZip, ue1Dir.asFile)
        extractZipStripRoot(sdlZip, sdl2Dir.asFile)
        extractZipStripRoot(alZip, openalDir.asFile)
        patchUE1Source(ue1Dir.asFile)
        applyUE1PatchOverlayV125(ue1Dir.asFile) // UNREAL_ANDROID_TOUCH_OVERLAY_SOURCE_OVERLAY_V125
        applySDL2PatchOverlayV210(sdl2Dir.asFile) // UNREAL_ANDROID_CHROMEOS_MOUSE_FRAMEPACED_OVERLAY_V210
        applySDLApi16PatchOverlayV212(sdl2Dir.asFile) // UNREAL_ANDROID_API16_SDL_OVERLAY_V212
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_RETROTOUCH_V215")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_RETROTOUCH_MENU_OBJECT_V219")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_RETROTOUCH_RESET_API_V221")
        requirePatched(ue1Dir.asFile.resolve("Source/Unreal/Classes/UnrealOptionsMenu.uc"), "UNREAL_ANDROID_RETROTOUCH_READONLY_V219")
        requirePatched(layout.projectDirectory.file("src/main/java/com/ast/unreal/UnrealSDLActivity.java").asFile, "UNREAL_ANDROID_RETROTOUCH_V215")
        requirePatched(layout.projectDirectory.file("src/main/java/com/ast/unreal/UnrealRetroTouchBridge.java").asFile, "UNREAL_ANDROID_RETROTOUCH_V215")
        requirePatched(layout.projectDirectory.file("src/main/java/com/ast/unreal/UnrealSDLActivity.java").asFile, "UNREAL_ANDROID_RETROTOUCH_RESET_API_V221")
        requirePatched(layout.projectDirectory.file("src/main/java/com/ast/unreal/UnrealRetroTouchBridge.java").asFile, "UNREAL_ANDROID_RETROTOUCH_RESET_API_V221")
        // ChromeOS FIX1 guards. Keep the original 2.0.5 source preparation revision;
        // the UE1 overlay is applied on every prepareSources run.
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_CHROMEOS_MOUSE_CAPTURE_V205F1")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_CHROMEOS_MIXED_BINDINGS_V205F1")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_CHROMEOS_PHYSICAL_KEY_CAPTURE_V205F1")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_CHROMEOS_MOUSE_FRAMEPACED_FLOAT_V210")
        requirePatched(layout.projectDirectory.file("src/main/java/com/ast/unreal/UnrealSDLActivity.java").asFile, "UNREAL_ANDROID_CHROMEOS_MOUSE_ACTIVITY_ROUTE_V205F1")
        requirePatched(sdl2Dir.asFile.resolve("src/video/android/SDL_androidmouse.c"), "UNREAL_ANDROID_CHROMEOS_MOUSE_HIRES_EVENT_V210")
        requirePatched(sdl2Dir.asFile.resolve("android-project/app/src/main/java/org/libsdl/app/SDLActivity.java"), "UNREAL_ANDROID_API16_SDL_HID_GUARD_V212")
        requirePatched(sdl2Dir.asFile.resolve("android-project/app/src/main/java/org/libsdl/app/HIDDeviceManager.java"), "UNREAL_ANDROID_API16_HID_MANAGER_GUARD_V212")
        requirePatched(layout.projectDirectory.file("src/main/java/com/ast/unreal/UnrealDataPaths.java").asFile, "UNREAL_ANDROID_API16_DATAPATHS_V212")
        requirePatched(layout.projectDirectory.file("src/main/java/com/ast/unreal/MainActivity.java").asFile, "UNREAL_ANDROID_API16_ACTIVITY_V212")
        requirePatched(layout.projectDirectory.file("src/main/java/com/ast/unreal/UnrealSDLActivity.java").asFile, "UNREAL_ANDROID_API16_ACTIVITY_V212")
        requirePatched(layout.projectDirectory.file("src/main/java/com/ast/unreal/UnrealSDLActivity.java").asFile, "UNREAL_ANDROID_LIFECYCLE_PAUSE_V211")
        requirePatched(layout.projectDirectory.file("src/main/java/com/ast/unreal/ChromeOSInputLogger.java").asFile, "UNREAL_ANDROID_API16_CHROMEOS_DIAG_V212")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_LIFECYCLE_PAUSE_V211")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_OUYA_960_FBO_V212")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLClient.cpp"), "UNREAL_ANDROID_OUYA_960_FBO_V212")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Inc/NSDLDrv.h"), "UNREAL_ANDROID_OUYA_960_FBO_V212")
        requirePatched(layout.projectDirectory.file("src/main/java/com/ast/unreal/UnrealDataPaths.java").asFile, "UNREAL_ANDROID_OUYA_960_DEFAULT_V212")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/CMakeLists.txt"), "UNREAL_ANDROID_API16_SCRIPTVM_SAFE_FLAGS_V212")
        requirePatched(ue1Dir.asFile.resolve("Source/Engine/CMakeLists.txt"), "UNREAL_ANDROID_API16_ENGINE_EVENT_SAFE_FLAGS_V212")
        requirePatched(layout.projectDirectory.file("src/main/cpp/CMakeLists.txt").asFile, "UNREAL_ANDROID_API16_NATIVE_COMPAT_V212")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_GAMMA_LEVELS_1_0_TO_3_0_V16")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_GAMMA_DPAD_LEFT_RIGHT_V17")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_CONTROLLER_DIRECT_TOGGLES_V22")
        requirePatched(ue1Dir.asFile.resolve("Source/Engine/Src/UnActor.cpp"), "UNREAL_ANDROID_INFINITE_AMMO_FREEZE_V23")
        requirePatched(ue1Dir.asFile.resolve("Source/Engine/Src/UnLevTic.cpp"), "UNREAL_ANDROID_INFINITE_AMMO_POST_TICK_V23")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLViewport.cpp"), "UNREAL_ANDROID_TOUCH_CONTROLS_USEJOYSTICK_BRIDGE_V125")
        requirePatched(ue1Dir.asFile.resolve("Source/Engine/Src/UnCanvas.cpp"), "UNREAL_ANDROID_TOUCH_CONTROLS_MENU_TEXT_V125")
        requirePatched(ue1Dir.asFile.resolve("Source/Engine/Src/UnCanvas.cpp"), "UNREAL_ANDROID_CANVAS_NONFINITE_TILE_GUARD_V14")
        requirePatched(ue1Dir.asFile.resolve("Source/NOpenGLESDrv/NOpenGLESDrv.cpp"), "UNREAL_ANDROID_MALI_DRAWTILE_ISOLATE_V124")
        requirePatched(ue1Dir.asFile.resolve("Source/NOpenGLESDrv/NOpenGLESDrvPrivate.h"), "UNREAL_ANDROID_MALI_ENDPOLY_BOUNDS_V124")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnFile.cpp"), "UNREAL_ANDROID_NULLSAFE_STRNCPY_PATCH")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnFile.cpp"), "UNREAL_ANDROID_FLAVOR_DATA_FALLBACK_V212")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnFile.cpp"), "UNREAL_ANDROID_APPFPRINTF_LOGCAT")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Inc/UnFile.h"), "UNREAL_ANDROID_NDK_MATH_DECLARATIONS_V2")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnFile.cpp"), "UNREAL_ANDROID_NDK_GLOBAL_NEW_DELETE_V2")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Inc/NSDLDrv.h"), "AndroidNativeAxisCurve")
        requirePatched(ue1Dir.asFile.resolve("Source/NSDLDrv/Src/NSDLClient.cpp"), "AndroidResolutionMode")
        requirePatched(ue1Dir.asFile.resolve("Source/NOpenALDrv/NOpenALDrvPrivate.h"), "AndroidSetSuspended")
        requirePatched(ue1Dir.asFile.resolve("Source/Engine/Src/UnCon.cpp"), "UNREAL_ANDROID_CENTER_CONSOLE_BIGMESSAGE")
        requirePatched(ue1Dir.asFile.resolve("Source/IpDrv/Src/UdpLink.cpp"), "ANDROID_LAN_BEACON_LOOPBACK_FIX")
        requirePatched(ue1Dir.asFile.resolve("Source/NOpenGLESDrv/VertexShader.glsl.inc"), "UNREAL_ANDROID_MALI_SHADER_PRECISION_V121")
        requirePatched(ue1Dir.asFile.resolve("Source/NOpenGLESDrv/VertexShader.glsl.inc"), "UNREAL_ANDROID_MALI_WORLD_UV_PRECISION_V24")
        requirePatched(ue1Dir.asFile.resolve("Source/NOpenGLESDrv/FragmentShader.glsl.inc"), "UNREAL_ANDROID_MALI_WORLD_UV_PRECISION_V24")
        requirePatched(ue1Dir.asFile.resolve("Source/CMakeLists.txt"), "UNREAL_DUALABI_64BIT_PORT")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Inc/UnGcc.h"), "typedef uintptr_t UPTRINT")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnClass.cpp"), "#define XFER_OBJ")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnGUID.cpp"), "UNREAL_ANDROID_GUID_FIXED_WIDTH_V8")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnGUID.cpp"), "UNREAL_ANDROID_GUID_SIZE_ASSERT_V8")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnCorSc.cpp"), "stale 32-bit metadata")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnObj.cpp"), "IntrinsicSize")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnPlat.cpp"), "CORE_API UPTRINT hWndMain")
        requirePatched(ue1Dir.asFile.resolve("Source/Engine/Inc/UnCamera.h"), "#pragma pack (push,4)")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnPlat.cpp"), "UNREAL_ANDROID_APPERROR_LOGCAT")
        requirePatched(ue1Dir.asFile.resolve("Source/Unreal/Src/SDLLaunch.cpp"), "SDLActivity looks up a C symbol named SDL_main")
        requirePatched(ue1Dir.asFile.resolve("Source/Unreal/Src/SDLLaunch.cpp"), "UNREAL_ANDROID_SKIP_APPBASEDIR_CHDIR")
        requirePatched(ue1Dir.asFile.resolve("Source/Unreal/Src/SDLLaunch.cpp"), "UNREAL_ANDROID_NATIVE_ROOT_PATH")
        requirePatched(ue1Dir.asFile.resolve("Source/Unreal/Src/SDLLaunch.cpp"), "--ue1-root")
        requirePatched(ue1Dir.asFile.resolve("Source/Unreal/Src/SDLLaunch.cpp"), "extern \"C\" int UE1AndroidShouldIgnoreEarlyQuit()")
        requirePatched(ue1Dir.asFile.resolve("Source/Unreal/Src/SDLLaunch.cpp"), "AndroidResetRuntimeStateForStart")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnPlat.cpp"), "UNREAL_ANDROID_LOGCAT_BRIDGE")
        requirePatched(ue1Dir.asFile.resolve("Source/Core/Src/UnPlat.cpp"), "UNREAL_ANDROID_APP_FLUSH_CONFIG_FILES")
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
        requirePatched(ue1Dir.asFile.resolve("Source/Engine/Src/UnCon.cpp"), "UNREAL_ANDROID_MENU_VERSION_ABI_V4")
        projectDir.resolve(".cxx").deleteRecursively()
        layout.buildDirectory.dir("intermediates/cxx").get().asFile.deleteRecursively()
    }
}

android {
    namespace = "com.ast.unreal"
    compileSdk = 36

    tasks.withType<org.gradle.api.tasks.compile.JavaCompile>().configureEach {
        options.compilerArgs.addAll(
            listOf(
                "-Xlint:none"
            )
        )
    }

    // Replaces deprecated gradle.properties option: android.defaults.buildfeatures.buildconfig=true
    buildFeatures {
        buildConfig = true
    }

    buildTypes {
        getByName("debug") {
            // Android Studio green Play button: use the ordinary runnable debug build by default.
            isDefault = true
            // Uses Android Studio/AGP's automatic debug signing:
            // %USERPROFILE%\.android\debug.keystore
            signingConfig = signingConfigs.getByName("debug")
        }

        getByName("release") {
            // Keep every locally produced APK/AAB on the same long-standing
            // Android Studio debug certificate. No custom keystore/passwords required.
            signingConfig = signingConfigs.getByName("debug")
        }

        create("android8SignedDebug") {
            initWith(getByName("debug"))

            // Same automatic Android Studio debug certificate as normalDebug.
            signingConfig = signingConfigs.getByName("debug")

            // Keep debug behavior.
            isDebuggable = true
            isJniDebuggable = true
            isMinifyEnabled = false
            isShrinkResources = false

            // No suffix: this variant can replace the normal com.ast.unreal install.
            applicationIdSuffix = null
            versionNameSuffix = "-Android8-Signed"

            matchingFallbacks += listOf("debug")
        }
    }

    defaultConfig {
        applicationId = "com.ast.unreal"
        minSdk = 16
        targetSdk = 36
        versionCode = 11
        versionName = androidVersionName

        ndk {
            abiFilters += listOf("armeabi-v7a", "arm64-v8a")
        }

        externalNativeBuild {
            cmake {
                arguments += listOf(
                    "-DANDROID_STL=c++_static"
                )
                cppFlags += listOf(
                    "-std=c++17",
                    "-fexceptions",
                    "-frtti",
                    "-DUNREAL_ANDROID_VERSION_NAME=$androidVersionName"
                )
            }
        }
    }

    flavorDimensions += "platform"
    productFlavors {
        create("normal") {
            dimension = "platform"
            isDefault = true
            applicationId = "com.ast.unreal"
            minSdk = 16
            targetSdk = 36
            versionCode = 11
            versionName = androidVersionName
            externalNativeBuild {
                cmake {
                    arguments += listOf("-DUNREAL_ANDROID_API16_COMPAT=ON", "-DUNREAL_ANDROID_AUTOMOTIVE=OFF")
                }
            }
        }
        create("automotive") {
            dimension = "platform"
            applicationId = "com.ast.unrealandroid"
            minSdk = 23
            targetSdk = 36
            versionCode = 11
            versionName = androidVersionName
            externalNativeBuild {
                cmake {
                    arguments += listOf("-DUNREAL_ANDROID_API16_COMPAT=OFF", "-DUNREAL_ANDROID_AUTOMOTIVE=ON")
                }
            }
        }
    }

    ndkVersion = "23.2.8568313"

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

    packaging {
        jniLibs {
            useLegacyPackaging = true
        }
        resources {
            excludes += setOf("/META-INF/{AL2.0,LGPL2.1}")
        }
    }
}

dependencies {
    implementation(files("libs/retrotouch.aar")) // UNREAL_ANDROID_RETROTOUCH_V215 UNREAL_ANDROID_RETROTOUCH_BETA3_V221
}

// Keep Android Studio's green Play/Run path unambiguous.
// `assembleDebug` is an aggregate task when multiple product flavors provide a
// Debug variant. With automotiveDebug enabled, Studio can build both APKs and
// then has no single Debug APK to deploy. Automotive is distributed/tested via
// its release/signed variants, while normalDebug remains the only ordinary
// Debug variant and therefore the default deployable Run target.
androidComponents {
    beforeVariants(
        selector()
            .withBuildType("debug")
            .withFlavor("platform", "automotive")
    ) { variantBuilder ->
        variantBuilder.enable = false
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
