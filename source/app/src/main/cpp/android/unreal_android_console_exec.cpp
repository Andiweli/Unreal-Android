// UNREAL_ANDROID_NATIVE_CONSOLE_EXEC_V149
// Direct Android -> UE1 console command bridge.  Java shows the visible input
// line; this file executes the submitted text through the active UE1 viewport
// instead of trying to fake Tab + typed keys + Enter.
// V149: UE1 launch code has no public GEngine global in this tree, so CMake
// patches SDLLaunch.cpp to store the running UEngine* in GAndroidConsoleEngineV149.

#include <jni.h>
#include <android/log.h>
#include <string.h>

#include "Engine.h"

UEngine* GAndroidConsoleEngineV149 = NULL;

static UViewport* UE1AndroidFindConsoleViewportV149()
{
    UEngine* Engine = GAndroidConsoleEngineV149;
    if (!Engine || !Engine->Client)
        return NULL;

    // CurrentViewport() is the public UClient path and should be valid while the
    // game is running. Keep this helper conservative to avoid depending on
    // UClient internals.
    return Engine->Client->CurrentViewport();
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_ast_unreal_UnrealSDLActivity_nativeAndroidConsoleExecV148(JNIEnv* Env, jclass, jstring Command)
{
    if (!Command)
        return JNI_FALSE;

    const char* Raw = Env->GetStringUTFChars(Command, NULL);
    if (!Raw || !Raw[0])
    {
        if (Raw)
            Env->ReleaseStringUTFChars(Command, Raw);
        return JNI_FALSE;
    }

    char Cmd[1024];
    strncpy(Cmd, Raw, sizeof(Cmd) - 1);
    Cmd[sizeof(Cmd) - 1] = 0;
    Env->ReleaseStringUTFChars(Command, Raw);

    // Trim spaces.  Console commands are ASCII, so byte trimming is enough here.
    char* Start = Cmd;
    while (*Start == ' ' || *Start == '\t' || *Start == '\r' || *Start == '\n')
        ++Start;
    char* End = Start + strlen(Start);
    while (End > Start && (End[-1] == ' ' || End[-1] == '\t' || End[-1] == '\r' || End[-1] == '\n'))
        *--End = 0;

    if (!Start[0])
        return JNI_FALSE;

    UBOOL Handled = 0;
    UViewport* Viewport = UE1AndroidFindConsoleViewportV149();

    if (Viewport)
    {
        // This is the same high-level Exec path the in-game console ultimately uses.
        // It should cover commands such as STAT FPS, BEHINDVIEW, SLOMO, FLY, WALK,
        // IAMTHEONE/GOD, OPEN, etc., without relying on keyboard text events.
        Handled = Viewport->Exec(Start, GSystem);
    }

    if (!Handled && GAndroidConsoleEngineV149)
    {
        // Fallback for engine/global commands.
        Handled = GAndroidConsoleEngineV149->Exec(Start, GSystem);
    }

    __android_log_print(ANDROID_LOG_INFO, "UE1Controller",
            "UNREAL_ANDROID_NATIVE_CONSOLE_EXEC_V149 cmd='%s' viewport=%p handled=%d",
            Start, Viewport, (int)Handled);

    // Some UE1 commands may return false even when accepted through a deeper object,
    // so from Java's point of view the command is accepted when an active viewport
    // or engine exists.  The log still records the actual Exec return value.
    return (Viewport || GAndroidConsoleEngineV149) ? JNI_TRUE : JNI_FALSE;
}
