#include <string.h>
#include <ctype.h>
#include <math.h>
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
#include <jni.h>
#include <android/log.h>
#endif

#include "NSDLDrv.h"
#include "UnRender.h"

IMPLEMENT_CLASS( UNSDLViewport );

#if PLATFORM_ANDROID
extern "C" int UE1AndroidShouldIgnoreEarlyQuit();
#endif


#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
// Optional Android-native controller backend. Java captures Android KeyEvent and
// MotionEvent input, C++ normalizes it to the old stable UE1 Joy* keys. SDL stays
// available as fallback when AndroidNativeController=False. ANDROID_NATIVE_CONTROLLER_BACKEND_V93 ANDROID_NATIVE_CONTROLLER_LINEAR_AXIS_RAMP_V97 ANDROID_NATIVE_CONTROLLER_LEFT_STICK_ANALOG_AXIS_V98 ANDROID_NATIVE_CONTROLLER_LEFTY_DIRECTION_SENSITIVITY_V99 ANDROID_NATIVE_CONTROLLER_LEFT_STICK_SMOOTHER_LINEAR_V100
static volatile INT GAndroidNativeControllerRuntimeEnabled = 1;
static const INT GAndroidNativeControllerQueueSize = 256;
static const SWORD GAndroidNativeAxisReleaseThreshold = 4096;

struct FAndroidNativeControllerEvent
{
	INT Type; // 1=key, 2=motion
	INT DeviceId;
	INT VendorId;
	INT ProductId;
	INT Source;
	INT KeyCode;
	INT ScanCode;
	INT Action;
	INT RepeatCount;
	FLOAT AxisX;
	FLOAT AxisY;
	FLOAT AxisZ;
	FLOAT AxisRZ;
	FLOAT AxisLTrigger;
	FLOAT AxisRTrigger;
	FLOAT AxisBrake;
	FLOAT AxisGas;
	FLOAT AxisHatX;
	FLOAT AxisHatY;
};

static SDL_mutex* GAndroidNativeControllerMutex = NULL;
static FAndroidNativeControllerEvent GAndroidNativeControllerQueue[GAndroidNativeControllerQueueSize];
static INT GAndroidNativeControllerQueueHead = 0;
static INT GAndroidNativeControllerQueueCount = 0;
static UBOOL GAndroidNativeButtonPressed[IK_MAX];
static UBOOL GAndroidNativeAxisDirPressed[SDL_CONTROLLER_AXIS_MAX][2];
static UBOOL GAndroidNativeHatPressed[2][2];
static UBOOL GAndroidNativeControllerWasInNormalMenu = 0; // ANDROID_CONTROLLER_NATIVE_MENU_TAP_V93
static UBOOL GAndroidNativeControllerWasInKeyMenuing = 0; // ANDROID_CONTROLLER_KEYMENUING_DEDUP_V93
static UBOOL GAndroidNativeKeyMenuingAxisArmed = 1; // ANDROID_CONTROLLER_KEYMENUING_DEDUP_V93
static FLOAT GAndroidNativeKeyMenuingIgnoreMotionUntil = 0.0f; // ANDROID_CONTROLLER_KEYMENUING_DEDUP_V93
static UBOOL GAndroidNativeKeyMenuingCaptureDone = 0; // ANDROID_CONTROLLER_KEYMENUING_DEDUP_V93

static UBOOL UE1AndroidCleanDispatchKeyCaptureV86( UNSDLViewport* Viewport, INT Key );

static SDL_mutex* UE1AndroidNativeControllerMutex()
{
	if( !GAndroidNativeControllerMutex )
		GAndroidNativeControllerMutex = SDL_CreateMutex();
	return GAndroidNativeControllerMutex;
}

static void UE1AndroidNativeControllerPushEvent( const FAndroidNativeControllerEvent& Event )
{
	SDL_mutex* Mutex = UE1AndroidNativeControllerMutex();
	if( Mutex )
		SDL_LockMutex( Mutex );

	if( GAndroidNativeControllerQueueCount >= GAndroidNativeControllerQueueSize )
	{
		GAndroidNativeControllerQueueHead = ( GAndroidNativeControllerQueueHead + 1 ) % GAndroidNativeControllerQueueSize;
		GAndroidNativeControllerQueueCount--;
	}
	const INT Tail = ( GAndroidNativeControllerQueueHead + GAndroidNativeControllerQueueCount ) % GAndroidNativeControllerQueueSize;
	GAndroidNativeControllerQueue[Tail] = Event;
	GAndroidNativeControllerQueueCount++;

	if( Mutex )
		SDL_UnlockMutex( Mutex );
}

static INT UE1AndroidNativeControllerDrainEvents( FAndroidNativeControllerEvent* OutEvents, INT MaxEvents )
{
	if( !OutEvents || MaxEvents <= 0 )
		return 0;

	SDL_mutex* Mutex = UE1AndroidNativeControllerMutex();
	if( Mutex )
		SDL_LockMutex( Mutex );

	const INT Count = Min( GAndroidNativeControllerQueueCount, MaxEvents );
	for( INT i=0; i<Count; ++i )
	{
		const INT Index = ( GAndroidNativeControllerQueueHead + i ) % GAndroidNativeControllerQueueSize;
		OutEvents[i] = GAndroidNativeControllerQueue[Index];
	}
	GAndroidNativeControllerQueueHead = ( GAndroidNativeControllerQueueHead + Count ) % GAndroidNativeControllerQueueSize;
	GAndroidNativeControllerQueueCount -= Count;

	if( Mutex )
		SDL_UnlockMutex( Mutex );

	return Count;
}

static void UE1AndroidNativeControllerResetState()
{
	appMemset( GAndroidNativeButtonPressed, 0, sizeof(GAndroidNativeButtonPressed) );
	appMemset( GAndroidNativeAxisDirPressed, 0, sizeof(GAndroidNativeAxisDirPressed) );
	appMemset( GAndroidNativeHatPressed, 0, sizeof(GAndroidNativeHatPressed) );
	SDL_mutex* Mutex = UE1AndroidNativeControllerMutex();
	if( Mutex )
		SDL_LockMutex( Mutex );
	GAndroidNativeControllerQueueHead = 0;
	GAndroidNativeControllerQueueCount = 0;
	if( Mutex )
		SDL_UnlockMutex( Mutex );
}

static BYTE UE1AndroidNativeDirectionalAxisKey( INT Axis, INT Direction )
{
	if( Direction < 0 )
	{
		switch( Axis )
		{
			case SDL_CONTROLLER_AXIS_LEFTX:  return IK_UnknownD8; // LJoyLeft
			case SDL_CONTROLLER_AXIS_LEFTY:  return IK_UnknownDA; // LJoyUp
			case SDL_CONTROLLER_AXIS_RIGHTX: return IK_Joy14;     // RJoyLeft
			case SDL_CONTROLLER_AXIS_RIGHTY: return IK_Joy16;     // RJoyUp
		}
	}
	else if( Direction > 0 )
	{
		switch( Axis )
		{
			case SDL_CONTROLLER_AXIS_LEFTX:  return IK_UnknownD9; // LJoyRight
			case SDL_CONTROLLER_AXIS_LEFTY:  return IK_UnknownDF; // LJoyDown
			case SDL_CONTROLLER_AXIS_RIGHTX: return IK_Joy15;     // RJoyRight
			case SDL_CONTROLLER_AXIS_RIGHTY: return IK_UnknownEA; // RJoyDown
		}
	}
	return IK_None;
}

static BYTE UE1AndroidNativeKeyCodeToUE1Key( INT KeyCode, UBOOL bIsInUI )
{
	// Android KeyEvent constants, kept numeric to avoid depending on Java headers.
	switch( KeyCode )
	{
		case 96:  return bIsInUI ? IK_Enter  : IK_Joy1;  // BUTTON_A
		case 97:  return bIsInUI ? IK_Escape : IK_Joy2;  // BUTTON_B
		case 99:  return bIsInUI ? IK_N      : IK_Joy3;  // BUTTON_X
		case 100: return bIsInUI ? IK_Y      : IK_Joy4;  // BUTTON_Y
		case 102: return IK_Joy10; // L1 / ShoulderL
		case 103: return IK_Joy11; // R1 / ShoulderR
		case 104: return IK_Joy12; // L2 / TriggerL digital fallback
		case 105: return IK_Joy13; // R2 / TriggerR digital fallback
		case 106: return IK_Joy8;  // THUMBL / LJoyPush
		case 107: return IK_Joy9;  // THUMBR / RJoyPush
		case 108: return IK_Escape; // START keeps current Android menu behaviour
		case 109: return bIsInUI ? IK_Escape : IK_Joy5; // SELECT / BACK
		case 19:  return bIsInUI ? IK_Up    : IK_JoyPovUp;
		case 20:  return bIsInUI ? IK_Down  : IK_JoyPovDown;
		case 21:  return bIsInUI ? IK_Left  : IK_JoyPovLeft;
		case 22:  return bIsInUI ? IK_Right : IK_JoyPovRight;
	}
	return IK_None;
}

static FLOAT UE1AndroidNativeLinearRampV97( FLOAT T )
{
	// ANDROID_NATIVE_CONTROLLER_LINEAR_AXIS_RAMP_V97
	// Simple, predictable analogue response: after the configured deadzone the
	// remaining physical stick travel maps linearly from 0.0 to 1.0.  This keeps
	// tiny stick movement slow, medium tilt medium, and full tilt full speed
	// without the over-heavy v96 progressive table.
	return Clamp( T, 0.0f, 1.0f );
}

static FLOAT UE1AndroidNativeApplyDeadzoneCurve( FLOAT Value, FLOAT Deadzone, FLOAT Curve, UBOOL bSigned )
{
	// ANDROID_NATIVE_CONTROLLER_DEADZONE_CURVE_V94
	// ANDROID_NATIVE_CONTROLLER_PROGRESSIVE_AXIS_CURVE_V94
	// ANDROID_NATIVE_CONTROLLER_RIGHT_STICK_SOFT_START_V95
	// ANDROID_NATIVE_CONTROLLER_TRUE_PROGRESSIVE_AXIS_V96
	// ANDROID_NATIVE_CONTROLLER_LINEAR_AXIS_RAMP_V97
	Deadzone = Clamp( Deadzone, 0.0f, 0.85f );
	// Keep the config field for compatibility, but v97 intentionally uses a
	// linear ramp.  Earlier high exponent curves made the stick too sluggish.
	(void)Curve;

	FLOAT Clamped = bSigned ? Clamp( Value, -1.0f, 1.0f ) : Clamp( Value, 0.0f, 1.0f );
	const FLOAT Sign = Clamped < 0.0f ? -1.0f : 1.0f;
	FLOAT Magnitude = (FLOAT)fabsf( Clamped );
	if( Magnitude <= Deadzone )
		return 0.0f;

	Magnitude = ( Magnitude - Deadzone ) / Max( 1.0f - Deadzone, 0.0001f );
	Magnitude = UE1AndroidNativeLinearRampV97( Magnitude );

	return bSigned ? Sign * Magnitude : Magnitude;
}

static UBOOL UE1AndroidNativeBindingStartsWithAliasV96( const char* Binding, const char* Alias )
{
	if( !Binding || !Alias || !*Alias )
		return 0;
	while( *Binding == ' ' )
		++Binding;
	const INT Len = appStrlen( Alias );
	if( appStrnicmp( Binding, Alias, Len ) )
		return 0;
	return Binding[Len] == 0 || Binding[Len] == ' ' || Binding[Len] == '|' || Binding[Len] == '\t';
}

static UBOOL UE1AndroidNativeBindingIsAnalogAxisAliasV96( const char* Binding )
{
	// Directional friendly stick keys such as RJoyLeft should not behave like
	// keyboard keys for movement/look aliases.  Keyboard-style Press/Hold makes
	// them instantly full speed.  These aliases are therefore driven as analogue
	// IST_Axis events below.
	return UE1AndroidNativeBindingStartsWithAliasV96( Binding, "MoveForward" )
		|| UE1AndroidNativeBindingStartsWithAliasV96( Binding, "MoveBackward" )
		|| UE1AndroidNativeBindingStartsWithAliasV96( Binding, "TurnLeft" )
		|| UE1AndroidNativeBindingStartsWithAliasV96( Binding, "TurnRight" )
		|| UE1AndroidNativeBindingStartsWithAliasV96( Binding, "StrafeLeft" )
		|| UE1AndroidNativeBindingStartsWithAliasV96( Binding, "StrafeRight" )
		|| UE1AndroidNativeBindingStartsWithAliasV96( Binding, "LookUp" )
		|| UE1AndroidNativeBindingStartsWithAliasV96( Binding, "LookDown" );
}

static SWORD UE1AndroidNativeFloatToAxis( FLOAT Value, FLOAT Deadzone, FLOAT Curve )
{
	const FLOAT Filtered = UE1AndroidNativeApplyDeadzoneCurve( Value, Deadzone, Curve, 1 );
	return (SWORD)Clamp( (INT)( Filtered * 32767.0f ), -32767, 32767 );
}

static SWORD UE1AndroidNativeTriggerToAxis( FLOAT A, FLOAT B, FLOAT Deadzone )
{
	const FLOAT RawValue = Max( Clamp( A, 0.0f, 1.0f ), Clamp( B, 0.0f, 1.0f ) );
	const FLOAT Filtered = UE1AndroidNativeApplyDeadzoneCurve( RawValue, Deadzone, 1.0f, 0 );
	return (SWORD)Clamp( (INT)( Filtered * 32767.0f ), 0, 32767 );
}

static UBOOL UE1AndroidNativeMotionIsNeutralForKeyCaptureV90( const FAndroidNativeControllerEvent& Event )
{
	// While entering Customize Controls, Android may still deliver the DPad/stick
	// motion that was used to navigate/confirm the row. Do not arm axis/hat capture
	// until every analogue source is back in a small neutral zone.
	const FLOAT Threshold = 0.25f;
	return Abs(Event.AxisX) < Threshold
		&& Abs(Event.AxisY) < Threshold
		&& Abs(Event.AxisZ) < Threshold
		&& Abs(Event.AxisRZ) < Threshold
		&& Abs(Event.AxisLTrigger) < Threshold
		&& Abs(Event.AxisRTrigger) < Threshold
		&& Abs(Event.AxisBrake) < Threshold
		&& Abs(Event.AxisGas) < Threshold
		&& Abs(Event.AxisHatX) < Threshold
		&& Abs(Event.AxisHatY) < Threshold;
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_ast_unreal_UnrealSDLActivity_nativeAndroidControllerIsEnabled( JNIEnv*, jclass )
{
	return GAndroidNativeControllerRuntimeEnabled ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_ast_unreal_UnrealSDLActivity_nativeAndroidControllerKey(
	JNIEnv* Env, jclass, jint DeviceId, jint VendorId, jint ProductId, jint KeyCode, jint ScanCode, jint Action, jint RepeatCount, jint Source, jstring DeviceName )
{
	if( !GAndroidNativeControllerRuntimeEnabled )
		return JNI_FALSE;
	if( Action != 0 && Action != 1 ) // KeyEvent.ACTION_DOWN / ACTION_UP
		return JNI_FALSE;

	FAndroidNativeControllerEvent Event;
	appMemset( &Event, 0, sizeof(Event) );
	Event.Type = 1;
	Event.DeviceId = DeviceId;
	Event.VendorId = VendorId;
	Event.ProductId = ProductId;
	Event.Source = Source;
	Event.KeyCode = KeyCode;
	Event.ScanCode = ScanCode;
	Event.Action = Action;
	Event.RepeatCount = RepeatCount;
	UE1AndroidNativeControllerPushEvent( Event );
	return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_ast_unreal_UnrealSDLActivity_nativeAndroidControllerMotion(
	JNIEnv* Env, jclass, jint DeviceId, jint VendorId, jint ProductId, jint Source, jstring DeviceName,
	jfloat AxisX, jfloat AxisY, jfloat AxisZ, jfloat AxisRZ, jfloat AxisLTrigger, jfloat AxisRTrigger, jfloat AxisBrake, jfloat AxisGas, jfloat AxisHatX, jfloat AxisHatY )
{
	if( !GAndroidNativeControllerRuntimeEnabled )
		return JNI_FALSE;

	FAndroidNativeControllerEvent Event;
	appMemset( &Event, 0, sizeof(Event) );
	Event.Type = 2;
	Event.DeviceId = DeviceId;
	Event.VendorId = VendorId;
	Event.ProductId = ProductId;
	Event.Source = Source;
	Event.AxisX = AxisX;
	Event.AxisY = AxisY;
	Event.AxisZ = AxisZ;
	Event.AxisRZ = AxisRZ;
	Event.AxisLTrigger = AxisLTrigger;
	Event.AxisRTrigger = AxisRTrigger;
	Event.AxisBrake = AxisBrake;
	Event.AxisGas = AxisGas;
	Event.AxisHatX = AxisHatX;
	Event.AxisHatY = AxisHatY;
	UE1AndroidNativeControllerPushEvent( Event );
	return JNI_TRUE;
}

extern "C" JNIEXPORT void JNICALL Java_com_ast_unreal_UnrealSDLActivity_nativeAndroidControllerDeviceChanged(
	JNIEnv* Env, jclass, jint DeviceId, jint VendorId, jint ProductId, jint Source, jstring DeviceName, jint EventType )
{
	const char* Name = DeviceName ? Env->GetStringUTFChars( DeviceName, NULL ) : NULL;
	__android_log_print( ANDROID_LOG_INFO, "UE1Controller", "ANDROID_NATIVE_CONTROLLER_BACKEND_V93 ANDROID_NATIVE_CONTROLLER_LINEAR_AXIS_RAMP_V97 device event=%d id=%d vendor=%d product=%d source=0x%x name=%s", EventType, DeviceId, VendorId, ProductId, Source, Name ? Name : "" );
	if( Name )
		Env->ReleaseStringUTFChars( DeviceName, Name );
}

extern "C" JNIEXPORT void JNICALL Java_com_ast_unreal_UnrealSDLActivity_nativeAndroidControllerReset( JNIEnv*, jclass )
{
	// v88: Activity resume/focus can reuse the same process while Android has
	// already cancelled the physical key stream. Reset the native debounce state
	// so the next real BUTTON_DOWN is not swallowed as a duplicate.
	UE1AndroidNativeControllerResetState(); // ANDROID_CONTROLLER_NATIVE_RESET_V92
}


struct FAndroidGammaMode
{
	FLOAT Value;
	const char* Label;
};

static const FAndroidGammaMode GAndroidGammaModes[] =
{
	{ 1.00f, "Default"    },
	{ 1.10f, "Default +1" },
	{ 1.20f, "Default +2" },
	{ 1.30f, "Default +3" },
	{ 1.40f, "Default +4" },
};

static FLOAT UE1AndroidGetConfiguredGammaModeValue()
{
	guard(UE1AndroidGetConfiguredGammaModeValue);

	char Value[64];
	if( GConfigCache.GetString( "NSDLDrv.NSDLClient", "Gamma", Value, sizeof(Value) ) )
		return Clamp( appAtof( Value ), 0.5f, 3.0f );

	if( GConfigCache.GetString( "NOpenGLESDrv.NOpenGLESRenderDevice", "WorldGamma", Value, sizeof(Value) ) )
		return Clamp( appAtof( Value ), 0.5f, 3.0f );

	return 1.0f;

	unguard;
}

static UBOOL UE1AndroidIsFixedMenuResolution( INT X, INT Y )
{
	guard(UE1AndroidIsFixedMenuResolution);
	return ( X == 1280 && Y == 720 ) || ( X == 1024 && Y == 768 );
	unguard;
}

static void UE1AndroidGetNativeDrawableSize( SDL_Window* Window, SDL_GLContext Context, UNSDLClient* Client, INT& OutX, INT& OutY )
{
	guard(UE1AndroidGetNativeDrawableSize);

	OutX = 0;
	OutY = 0;

	int WindowW = 0, WindowH = 0;
	int DrawW = 0, DrawH = 0;
	if( Window )
	{
		SDL_GetWindowSize( Window, &WindowW, &WindowH );
		if( Context )
			SDL_GL_GetDrawableSize( Window, &DrawW, &DrawH );
	}

	if( DrawW <= 0 || DrawH <= 0 )
	{
		DrawW = WindowW;
		DrawH = WindowH;
	}

	if( ( DrawW <= 0 || DrawH <= 0 ) && Client )
	{
		DrawW = Client->GetDefaultDisplayMode().w;
		DrawH = Client->GetDefaultDisplayMode().h;
	}

	if( DrawW > 0 && DrawH > 0 )
	{
		if( DrawH > DrawW )
			Exchange( DrawW, DrawH );
		OutX = Align( DrawW, 4 );
		OutY = DrawH;
	}

	unguard;
}

static void UE1AndroidApplyConfiguredResolution( UNSDLClient* Client, SDL_Window* Window, SDL_GLContext Context, INT& X, INT& Y )
{
	guard(UE1AndroidApplyConfiguredResolution);

	if( Client )
	{
		if( Client->AndroidResolutionMode == 1 )
		{
			X = 1280;
			Y = 720;
			return;
		}
		if( Client->AndroidResolutionMode == 2 )
		{
			X = 1024;
			Y = 768;
			return;
		}
	}

	INT NativeX = 0, NativeY = 0;
	UE1AndroidGetNativeDrawableSize( Window, Context, Client, NativeX, NativeY );
	if( NativeX > 0 && NativeY > 0 )
	{
		X = NativeX;
		Y = NativeY;
	}

	unguard;
}

static const char* UE1AndroidResolutionModeName( INT Mode )
{
	switch( Mode )
	{
		case 1: return "1280x720";
		case 2: return "1024x768";
		default: return "Native";
	}
}

static INT UE1AndroidResolutionModeFromName( const char* Text, INT X, INT Y )
{
	if( Text && !appStricmp( Text, "Native" ) )
		return 0;
	if( X == 1280 && Y == 720 )
		return 1;
	if( X == 1024 && Y == 768 )
		return 2;
	return 0;
}

static INT UE1AndroidGetNearestGammaModeIndex()
{
	guard(UE1AndroidGetNearestGammaModeIndex);

	const FLOAT Current = UE1AndroidGetConfiguredGammaModeValue();
	INT Nearest = 0;
	FLOAT BestDiff = 999.0f;
	for( INT i=0; i<(INT)(sizeof(GAndroidGammaModes)/sizeof(GAndroidGammaModes[0])); ++i )
	{
		const FLOAT Diff = ( Current > GAndroidGammaModes[i].Value ) ? ( Current - GAndroidGammaModes[i].Value ) : ( GAndroidGammaModes[i].Value - Current );
		if( Diff < BestDiff )
		{
			BestDiff = Diff;
			Nearest = i;
		}
	}
	return Nearest;

	unguard;
}

static void UE1AndroidBuildGammaMenuLabel( char* OutLabel, INT OutSize )
{
	guard(UE1AndroidBuildGammaMenuLabel);

	if( !OutLabel || OutSize <= 0 )
		return;

	const INT ModeIndex = UE1AndroidGetNearestGammaModeIndex();

	// Match the classic UE1 menu layout: option name on the left,
	// current value in the AUDIO/VIDEO value column. The current Android 8
	// menu font/layout needs two more spaces here to line up with Texture Detail.
	appSprintf( OutLabel, "Toggle Gamma              %s", GAndroidGammaModes[ModeIndex].Label );
	OutLabel[OutSize-1] = 0;

	unguard;
}

static UBOOL UE1AndroidStringLooksLikeGammaFullscreenMenuItem( const char* Text )
{
	guard(UE1AndroidStringLooksLikeGammaFullscreenMenuItem);

	return Text
		&& ( appStrfind( Text, "Toggle Fullscreen" ) != NULL
		  || appStrfind( Text, "Toggle Gamma" ) != NULL );

	unguard;
}

static UBOOL UE1AndroidSetGammaMenuLabelOnObject( UObject* Object, UProperty* MenuListProperty, const char* Label )
{
	guard(UE1AndroidSetGammaMenuLabelOnObject);

	if( !Object || !MenuListProperty || !Label || MenuListProperty->ArrayDim <= 0 )
		return 0;

	const INT ElementSize = MenuListProperty->GetElementSize();
	if( ElementSize <= 1 )
		return 0;

	// UnrealVideoMenu uses one-based menu indices in UnrealScript.
	// In the stock Android package the fullscreen item is MenuList(2).
	// A previously recompiled test package may have shifted it to MenuList(3),
	// so patch only the element that still contains Toggle Fullscreen/Gamma.
	static const INT CandidateIndices[] = { 2, 3 };
	BYTE* Base = (BYTE*)Object + MenuListProperty->Offset;
	for( INT Candidate=0; Candidate<(INT)(sizeof(CandidateIndices)/sizeof(CandidateIndices[0])); ++Candidate )
	{
		const INT Index = CandidateIndices[Candidate];
		if( Index < 0 || Index >= MenuListProperty->ArrayDim )
			continue;

		char* MenuText = (char*)(Base + Index * ElementSize);
		if( UE1AndroidStringLooksLikeGammaFullscreenMenuItem( MenuText ) )
		{
			appStrncpy( MenuText, Label, ElementSize );
			MenuText[ElementSize-1] = 0;
			return 1;
		}
	}

	// Fallback for the normal, unmodified Unreal.u: keep the change restricted to
	// the known fullscreen slot directly below Brightness.
	if( MenuListProperty->ArrayDim > 2 )
	{
		char* MenuText = (char*)(Base + 2 * ElementSize);
		appStrncpy( MenuText, Label, ElementSize );
		MenuText[ElementSize-1] = 0;
		return 1;
	}

	return 0;

	unguard;
}

static void UE1AndroidRefreshGammaMenuLabel()
{
	guard(UE1AndroidRefreshGammaMenuLabel);

	UClass* MenuClass = ::FindObject<UClass>( ANY_PACKAGE, "UnrealVideoMenu" );
	if( !MenuClass )
		return;

	UProperty* MenuListProperty = FindField<UProperty>( MenuClass, "MenuList" );
	if( !MenuListProperty )
		return;

	char Label[64];
	UE1AndroidBuildGammaMenuLabel( Label, sizeof(Label) );

	UE1AndroidSetGammaMenuLabelOnObject( MenuClass->GetDefaultObject(), MenuListProperty, Label );

	for( TObjectIterator<UObject> It; It; ++It )
	{
		if( It->IsA( MenuClass ) )
			UE1AndroidSetGammaMenuLabelOnObject( *It, MenuListProperty, Label );
	}

	unguard;
}

static void UE1AndroidToggleGammaMode( UNSDLViewport* Viewport, FOutputDevice* Out )
{
	guard(UE1AndroidToggleGammaMode);

	const INT Nearest = UE1AndroidGetNearestGammaModeIndex();
	const INT NewIndex = ( Nearest + 1 ) % (INT)(sizeof(GAndroidGammaModes)/sizeof(GAndroidGammaModes[0]));
	const FAndroidGammaMode& NewMode = GAndroidGammaModes[NewIndex];

	char NewValue[32];
	appSprintf( NewValue, "%.2f", NewMode.Value );

	// NSDLDrv.NSDLClient/Gamma is the runtime value read by the Android GLES renderer.
	// Mirroring to WorldGamma keeps the renderer section readable and avoids stale advanced config values.
	GConfigCache.SetString( "NSDLDrv.NSDLClient", "Gamma", NewValue );
	GConfigCache.SetString( "NOpenGLESDrv.NOpenGLESRenderDevice", "WorldGamma", NewValue );
	GConfigCache.SaveAllConfigs();
	UE1AndroidRefreshGammaMenuLabel();

	char Message[128];
	appSprintf( Message, "Gamma: %s (%.2f)", NewMode.Label, NewMode.Value );

	if( Viewport && Viewport->Actor )
		Viewport->Actor->eventClientMessage( Message );
	if( Out )
		Out->Log( Message );

	debugf( NAME_Log, "Android gamma mode toggled via ToggleFullscreen: %s", Message );

	unguard;
}


static AMenu* UE1AndroidGetActiveMenu( UNSDLViewport* Viewport )
{
	guard(UE1AndroidGetActiveMenu);

	if( !Viewport || !Viewport->Actor || !Viewport->Actor->myHUD )
		return NULL;
	return Viewport->Actor->myHUD->MainMenu;

	unguard;
}

static UBOOL UE1AndroidMenuIsExactClass( AMenu* Menu, const char* ClassName )
{
	guard(UE1AndroidMenuIsExactClass);

	if( !Menu || !ClassName || !Menu->GetClass() )
		return 0;
	return !appStricmp( Menu->GetClass()->GetName(), ClassName );

	unguard;
}

struct FAndroidPlayerClassChoice
{
	const char* PrimaryClass;
	const char* FallbackClass;
	const char* Label;
};

static const FAndroidPlayerClassChoice GAndroidPlayerClassChoices[] =
{
	{ "Unreal.FemaleOne",      "UnrealI.FemaleOne",      "Female1"    },
	{ "Unreal.FemaleTwo",      "UnrealI.FemaleTwo",      "Female2"    },
	{ "Unreal.UnrealSpectator", "UnrealI.UnrealSpectator", "Spectator"  },
	{ "Unreal.SkaarjPlayer",    "UnrealI.SkaarjPlayer",    "Sktrooper1" },
	{ "Unreal.MaleThree",      "UnrealI.MaleThree",      "Male3"      },
	{ "Unreal.MaleTwo",        "UnrealI.MaleTwo",        "Male2"      },
	{ "Unreal.MaleOne",        "UnrealI.MaleOne",        "Male 1"     },
};

static UClass* UE1AndroidLoadPlayerClassChoice( const FAndroidPlayerClassChoice& Choice )
{
	guard(UE1AndroidLoadPlayerClassChoice);

	UClass* Result = LoadClass<APlayerPawn>( NULL, Choice.PrimaryClass, NULL, LOAD_NoWarn | LOAD_KeepImports, NULL );
	if( !Result && Choice.FallbackClass )
		Result = LoadClass<APlayerPawn>( NULL, Choice.FallbackClass, NULL, LOAD_NoWarn | LOAD_KeepImports, NULL );
	return Result;

	unguard;
}

static INT UE1AndroidFindCurrentPlayerClassChoice( APlayerPawn* PlayerOwner )
{
	guard(UE1AndroidFindCurrentPlayerClassChoice);

	char ConfigClass[128] = "";
	GConfigCache.GetString( "DefaultPlayer", "Class", ConfigClass, sizeof(ConfigClass) );

	for( INT i=0; i<(INT)(sizeof(GAndroidPlayerClassChoices)/sizeof(GAndroidPlayerClassChoices[0])); ++i )
	{
		const FAndroidPlayerClassChoice& Choice = GAndroidPlayerClassChoices[i];
		if( ConfigClass[0]
		&& ( !appStricmp( ConfigClass, Choice.PrimaryClass ) || !appStricmp( ConfigClass, Choice.FallbackClass )
		  || appStrfind( ConfigClass, appStrchr( Choice.PrimaryClass, '.' ) ? appStrchr( Choice.PrimaryClass, '.' ) + 1 : Choice.PrimaryClass ) != NULL ) )
		{
			return i;
		}
	}

	if( PlayerOwner && PlayerOwner->GetClass() )
	{
		const char* CurrentName = PlayerOwner->GetClass()->GetName();
		for( INT i=0; i<(INT)(sizeof(GAndroidPlayerClassChoices)/sizeof(GAndroidPlayerClassChoices[0])); ++i )
		{
			const char* ShortName = appStrchr( GAndroidPlayerClassChoices[i].PrimaryClass, '.' );
			ShortName = ShortName ? ShortName + 1 : GAndroidPlayerClassChoices[i].PrimaryClass;
			if( !appStricmp( CurrentName, ShortName ) )
				return i;
		}
	}

	return 0;

	unguard;
}

static UBOOL UE1AndroidApplyPlayerClassChoice( UNSDLViewport* Viewport, AMenu* Menu, INT ChoiceIndex )
{
	guard(UE1AndroidApplyPlayerClassChoice);

	if( !Viewport || !Menu || !Menu->PlayerOwner )
		return 0;

	const INT Count = (INT)(sizeof(GAndroidPlayerClassChoices)/sizeof(GAndroidPlayerClassChoices[0]));
	if( ChoiceIndex < 0 ) ChoiceIndex = Count - 1;
	if( ChoiceIndex >= Count ) ChoiceIndex = 0;

	const FAndroidPlayerClassChoice& Choice = GAndroidPlayerClassChoices[ChoiceIndex];
	UClass* PlayerClass = UE1AndroidLoadPlayerClassChoice( Choice );
	if( !PlayerClass )
	{
		debugf( NAME_Log, "Android player setup class not found: %s / %s", Choice.PrimaryClass, Choice.FallbackClass ? Choice.FallbackClass : "" );
		return 0;
	}

	APlayerPawn* DefaultPawn = (APlayerPawn*)PlayerClass->GetDefaultObject();
	if( !DefaultPawn )
		return 0;

	// Keep the menu preview and the current player pawn visually in sync.
	Menu->Mesh = DefaultPawn->Mesh;
	Menu->Skin = DefaultPawn->Skin;
	Menu->PlayerOwner->Mesh = DefaultPawn->Mesh;
	Menu->PlayerOwner->Skin = DefaultPawn->Skin;

	// Persist the class for the next travel/join and also add it to the runtime URL.
	const char* ClassPath = PlayerClass->GetPathName();
	GConfigCache.SetString( "DefaultPlayer", "Class", ClassPath );
	GConfigCache.SaveAllConfigs();

	if( Viewport->Actor && Viewport->Actor->GetLevel() && Viewport->Actor->GetLevel()->Engine )
	{
		char URLClass[160];
		appSprintf( URLClass, "Class=%s", ClassPath );
		((UGameEngine*)Viewport->Actor->GetLevel()->Engine)->LastURL.AddOption( URLClass );
	}

	char URLSkin[160];
	if( DefaultPawn->Skin )
		appSprintf( URLSkin, "Skin=%s", DefaultPawn->Skin->GetPathName() );
	else
		appSprintf( URLSkin, "Skin=" );
	if( Viewport->Actor && Viewport->Actor->GetLevel() && Viewport->Actor->GetLevel()->Engine )
		((UGameEngine*)Viewport->Actor->GetLevel()->Engine)->LastURL.AddOption( URLSkin );

	char Message[128];
	appSprintf( Message, "Class: %s", Choice.Label );
	if( Viewport->Actor )
		Viewport->Actor->eventClientMessage( Message );
	debugf( NAME_Log, "Android player setup class changed: %s", ClassPath );
	return 1;

	unguard;
}

static UBOOL UE1AndroidHandlePlayerSetupClassInput( UNSDLViewport* Viewport, INT iKey, EInputAction Action )
{
	guard(UE1AndroidHandlePlayerSetupClassInput);

	if( Action != IST_Press || ( iKey != IK_Left && iKey != IK_Right ) )
		return 0;

	AMenu* Menu = UE1AndroidGetActiveMenu( Viewport );
	if( !UE1AndroidMenuIsExactClass( Menu, "UnrealPlayerMenu" ) )
		return 0;
	if( Menu->Selection != 4 )
		return 0;

	INT Index = UE1AndroidFindCurrentPlayerClassChoice( Menu->PlayerOwner );
	if( iKey == IK_Right )
		Index++;
	else
		Index--;
	return UE1AndroidApplyPlayerClassChoice( Viewport, Menu, Index );

	unguard;
}

static void UE1AndroidPostProcessJoinMenuInput( UNSDLViewport* Viewport, INT iKey, EInputAction Action )
{
	guard(UE1AndroidPostProcessJoinMenuInput);

	if( Action != IST_Press || ( iKey != IK_Up && iKey != IK_Down ) )
		return;

	AMenu* Menu = UE1AndroidGetActiveMenu( Viewport );
	if( !UE1AndroidMenuIsExactClass( Menu, "UnrealJoinGameMenu" ) )
		return;

	if( Menu->Selection == 2 )
		Menu->Selection = ( iKey == IK_Up ) ? 1 : 3;
	else if( Menu->Selection < 1 )
		Menu->Selection = 1;
	else if( Menu->Selection > 4 )
		Menu->Selection = 4;

	unguard;
}

static void UE1AndroidSetFixedStringArrayElement( UObject* Object, UProperty* Property, INT Index, const char* Text )
{
	guard(UE1AndroidSetFixedStringArrayElement);

	if( !Object || !Property || !Text || Index < 0 || Index >= Property->ArrayDim )
		return;

	const INT ElementSize = Property->GetElementSize();
	if( ElementSize <= 1 )
		return;

	char* Value = (char*)((BYTE*)Object + Property->Offset + Index * ElementSize);
	appStrncpy( Value, Text, ElementSize );
	Value[ElementSize-1] = 0;

	unguard;
}

static const char* UE1AndroidGetFixedStringArrayElement( UObject* Object, UProperty* Property, INT Index )
{
	guard(UE1AndroidGetFixedStringArrayElement);

	if( !Object || !Property || Index < 0 || Index >= Property->ArrayDim )
		return "";

	const INT ElementSize = Property->GetElementSize();
	if( ElementSize <= 1 )
		return "";

	return (const char*)((BYTE*)Object + Property->Offset + Index * ElementSize);

	unguard;
}

static void UE1AndroidSetIntPropertyValue( UObject* Object, UProperty* Property, INT Value )
{
	guard(UE1AndroidSetIntPropertyValue);

	if( !Object || !Property )
		return;

	*(INT*)((BYTE*)Object + Property->Offset) = Value;

	unguard;
}

static void UE1AndroidClampMenuSelection( UObject* Object, UProperty* SelectionProperty, INT MaxSelection )
{
	guard(UE1AndroidClampMenuSelection);

	if( !Object || !SelectionProperty || MaxSelection <= 0 )
		return;

	INT* Selection = (INT*)((BYTE*)Object + SelectionProperty->Offset);
	if( *Selection > MaxSelection )
		*Selection = MaxSelection;
	else if( *Selection < 1 )
		*Selection = 1;

	unguard;
}

static void UE1AndroidPatchMenuObjectLengthAndTail( UObject* Object, UProperty* MenuLengthProperty, UProperty* SelectionProperty, UProperty* MenuListProperty, UProperty* HelpMessageProperty, INT NewLength, INT FirstHiddenIndex )
{
	guard(UE1AndroidPatchMenuObjectLengthAndTail);

	if( !Object )
		return;

	UE1AndroidSetIntPropertyValue( Object, MenuLengthProperty, NewLength );
	UE1AndroidClampMenuSelection( Object, SelectionProperty, NewLength );

	// UnrealScript menu arrays are effectively one-based in these classes.
	// Clear the now-hidden tail so stale text/help cannot reappear if a menu
	// draw briefly happens before MenuLength is observed.
	UE1AndroidSetFixedStringArrayElement( Object, MenuListProperty, FirstHiddenIndex, "" );
	UE1AndroidSetFixedStringArrayElement( Object, HelpMessageProperty, FirstHiddenIndex, "" );

	unguard;
}

static UBOOL UE1AndroidPatchMenuClassLengthAndTail( const char* ClassName, INT NewLength, INT FirstHiddenIndex )
{
	guard(UE1AndroidPatchMenuClassLengthAndTail);

	UClass* MenuClass = ::FindObject<UClass>( ANY_PACKAGE, ClassName );
	if( !MenuClass )
		return 0;

	UProperty* MenuLengthProperty = FindField<UProperty>( MenuClass, "MenuLength" );
	UProperty* SelectionProperty   = FindField<UProperty>( MenuClass, "Selection" );
	UProperty* MenuListProperty    = FindField<UProperty>( MenuClass, "MenuList" );
	UProperty* HelpMessageProperty = FindField<UProperty>( MenuClass, "HelpMessage" );

	if( !MenuLengthProperty )
		return 0;

	UE1AndroidPatchMenuObjectLengthAndTail( MenuClass->GetDefaultObject(), MenuLengthProperty, SelectionProperty, MenuListProperty, HelpMessageProperty, NewLength, FirstHiddenIndex );

	for( TObjectIterator<UObject> It; It; ++It )
	{
		if( It->IsA( MenuClass ) )
			UE1AndroidPatchMenuObjectLengthAndTail( *It, MenuLengthProperty, SelectionProperty, MenuListProperty, HelpMessageProperty, NewLength, FirstHiddenIndex );
	}

	debugf( NAME_Log, "Android runtime menu trim applied: %s MenuLength=%i hiddenIndex=%i", ClassName, NewLength, FirstHiddenIndex );
	return 1;

	unguard;
}

static void UE1AndroidPatchVideoMenuCosmeticsOnObject( UObject* Object, UProperty* MenuListProperty )
{
	guard(UE1AndroidPatchVideoMenuCosmeticsOnObject);

	if( !Object || !MenuListProperty )
		return;

	// UnrealVideoMenu default slot 3 is "Select Resolution". On Android the
	// action is still the same, but the shorter label better matches the old
	// low-width menu layout and leaves more room for the value column.
	UE1AndroidSetFixedStringArrayElement( Object, MenuListProperty, 3, "Resolution" );

	unguard;
}

static UBOOL UE1AndroidPatchVideoMenuCosmetics()
{
	guard(UE1AndroidPatchVideoMenuCosmetics);

	UClass* MenuClass = ::FindObject<UClass>( ANY_PACKAGE, "UnrealVideoMenu" );
	if( !MenuClass )
		return 0;

	UProperty* MenuListProperty = FindField<UProperty>( MenuClass, "MenuList" );
	if( !MenuListProperty )
		return 0;

	UE1AndroidPatchVideoMenuCosmeticsOnObject( MenuClass->GetDefaultObject(), MenuListProperty );

	for( TObjectIterator<UObject> It; It; ++It )
	{
		if( It->IsA( MenuClass ) )
			UE1AndroidPatchVideoMenuCosmeticsOnObject( *It, MenuListProperty );
	}

	debugf( NAME_Log, "Android runtime video menu cosmetic labels applied" );
	return 1;

	unguard;
}

static void UE1AndroidPatchJoinMenuOnObject( UObject* Object, UProperty* MenuLengthProperty, UProperty* SelectionProperty, UProperty* MenuListProperty, UProperty* HelpMessageProperty )
{
	guard(UE1AndroidPatchJoinMenuOnObject);

	if( !Object )
		return;

	UE1AndroidSetIntPropertyValue( Object, MenuLengthProperty, 4 );
	UE1AndroidClampMenuSelection( Object, SelectionProperty, 4 );

	// Keep the original hardcoded UnrealJoinGameMenu selection indices intact:
	// 1=Find Local Servers, 3=Open, 4=Optimized for. The draw filter in
	// UnCanvas shifts rows 3/4 up visually over the hidden favorites row.
	UE1AndroidSetFixedStringArrayElement( Object, MenuListProperty,    2, "" );
	UE1AndroidSetFixedStringArrayElement( Object, HelpMessageProperty, 2, "" );
	UE1AndroidSetFixedStringArrayElement( Object, MenuListProperty,    3, "Connect to Server via IP" );
	UE1AndroidSetFixedStringArrayElement( Object, HelpMessageProperty, 3, "Type in a server IP address and connect directly." );
	UE1AndroidSetFixedStringArrayElement( Object, MenuListProperty,    5, "" );
	UE1AndroidSetFixedStringArrayElement( Object, HelpMessageProperty, 5, "" );

	AMenu* Menu = Cast<AMenu>( Object );
	if( Menu && Menu->Selection == 2 )
		Menu->Selection = 3;

	unguard;
}

static UBOOL UE1AndroidPatchJoinMenu()
{
	guard(UE1AndroidPatchJoinMenu);

	UClass* MenuClass = ::FindObject<UClass>( ANY_PACKAGE, "UnrealJoinGameMenu" );
	if( !MenuClass )
		return 0;

	UProperty* MenuLengthProperty = FindField<UProperty>( MenuClass, "MenuLength" );
	UProperty* SelectionProperty   = FindField<UProperty>( MenuClass, "Selection" );
	UProperty* MenuListProperty    = FindField<UProperty>( MenuClass, "MenuList" );
	UProperty* HelpMessageProperty = FindField<UProperty>( MenuClass, "HelpMessage" );
	if( !MenuLengthProperty || !MenuListProperty )
		return 0;

	UE1AndroidPatchJoinMenuOnObject( MenuClass->GetDefaultObject(), MenuLengthProperty, SelectionProperty, MenuListProperty, HelpMessageProperty );
	for( TObjectIterator<UObject> It; It; ++It )
	{
		if( It->GetClass() == MenuClass )
			UE1AndroidPatchJoinMenuOnObject( *It, MenuLengthProperty, SelectionProperty, MenuListProperty, HelpMessageProperty );
	}

	debugf( NAME_Log, "Android runtime multiplayer JOIN menu patch applied" );
	return 1;

	unguard;
}

static void UE1AndroidPatchPlayerMenuOnObject( UObject* Object, UProperty* MenuLengthProperty, UProperty* SelectionProperty, UProperty* MenuListProperty, UProperty* HelpMessageProperty )
{
	guard(UE1AndroidPatchPlayerMenuOnObject);

	if( !Object )
		return;

	UE1AndroidSetIntPropertyValue( Object, MenuLengthProperty, 4 );
	UE1AndroidClampMenuSelection( Object, SelectionProperty, 4 );
	UE1AndroidSetFixedStringArrayElement( Object, MenuListProperty,    4, "Class:" );
	UE1AndroidSetFixedStringArrayElement( Object, HelpMessageProperty, 4, "Change your player class using the left and right arrows." );

	unguard;
}

static UBOOL UE1AndroidPatchPlayerSetupMenu()
{
	guard(UE1AndroidPatchPlayerSetupMenu);

	UClass* MenuClass = ::FindObject<UClass>( ANY_PACKAGE, "UnrealPlayerMenu" );
	if( !MenuClass )
		return 0;

	UProperty* MenuLengthProperty = FindField<UProperty>( MenuClass, "MenuLength" );
	UProperty* SelectionProperty   = FindField<UProperty>( MenuClass, "Selection" );
	UProperty* MenuListProperty    = FindField<UProperty>( MenuClass, "MenuList" );
	UProperty* HelpMessageProperty = FindField<UProperty>( MenuClass, "HelpMessage" );
	if( !MenuLengthProperty || !MenuListProperty )
		return 0;

	UE1AndroidPatchPlayerMenuOnObject( MenuClass->GetDefaultObject(), MenuLengthProperty, SelectionProperty, MenuListProperty, HelpMessageProperty );
	for( TObjectIterator<UObject> It; It; ++It )
	{
		// Exact class only. UnrealMeshMenu inherits from UnrealPlayerMenu and already
		// has its own 5-row Start Game layout.
		if( It->GetClass() == MenuClass )
			UE1AndroidPatchPlayerMenuOnObject( *It, MenuLengthProperty, SelectionProperty, MenuListProperty, HelpMessageProperty );
	}

	debugf( NAME_Log, "Android runtime player setup class row patch applied" );
	return 1;

	unguard;
}

static void UE1AndroidRefreshRuntimeMenuPatches()

{
	guard(UE1AndroidRefreshRuntimeMenuPatches);

	static UBOOL VideoMenuPatched    = 0;
	static UBOOL OptionsMenuPatched  = 0;
	static UBOOL VideoMenuCosmetics  = 0;
	static UBOOL JoinMenuPatched     = 0;
	static UBOOL ServerMenuPatched   = 0;
	static UBOOL PlayerMenuPatched   = 0;

	// AUDIO/VIDEO: hide the bottom Sound Quality entry. MenuLength=6 keeps
	// Brightness, Toggle Gamma, Select Resolution, Texture Detail, Music Volume
	// and Sound Volume. The old Sound Quality Low/High value is drawn before
	// the help panel, so moving the help panel up via MenuLength also covers the
	// obsolete value row on the stock UnrealVideoMenu.
	if( !VideoMenuPatched )
		VideoMenuPatched = UE1AndroidPatchMenuClassLengthAndTail( "UnrealVideoMenu", 6, 7 );
	if( !VideoMenuCosmetics )
		VideoMenuCosmetics = UE1AndroidPatchVideoMenuCosmetics();

	// MULTIPLAYER / JOIN: hide favorites and the obsolete Epic server list,
	// rename Open to direct IP connect, while preserving the stock hardcoded
	// selection indices used by UnrealJoinGameMenu.ProcessSelection().
	if( !JoinMenuPatched )
		JoinMenuPatched = UE1AndroidPatchJoinMenu();

	// MULTIPLAYER / START GAME: hide the bottom Launch Dedicated Server entry.
	if( !ServerMenuPatched )
		ServerMenuPatched = UE1AndroidPatchMenuClassLengthAndTail( "UnrealServerMenu", 4, 5 );

	// MULTIPLAYER / PLAYER SETUP: restore the missing Class row. Runtime input
	// handling below cycles the class on left/right without rebuilding Unreal.u.
	if( !PlayerMenuPatched )
		PlayerMenuPatched = UE1AndroidPatchPlayerSetupMenu();

	// OPTIONS: hide the bottom Advanced Options entry cleanly. It is already the
	// final item, so MenuLength=14 is enough and does not disturb other hardcoded
	// option selections such as HUD Configuration or View Bob.
	if( !OptionsMenuPatched )
		OptionsMenuPatched = UE1AndroidPatchMenuClassLengthAndTail( "UnrealOptionsMenu", 14, 15 );

	unguard;
}
#endif

// UE1_ANDROID_SOFT_KEYBOARD_PATCH_BEGIN
#if PLATFORM_ANDROID
static UBOOL GAndroidSoftKeyboardActive = 0;
static FLOAT GAndroidSoftKeyboardTimeout = 0.0f;
static FLOAT GAndroidSoftKeyboardLastKick = -1000.0f;
static FLOAT GAndroidSoftKeyboardPulseUntil = 0.0f;
static UNSDLViewport* GAndroidSoftKeyboardPulseViewport = NULL;

static UBOOL AndroidViewportIsMenuing( UNSDLViewport* Viewport )
{
	return Viewport &&
		Viewport->Console &&
		((UObject*)Viewport->Console)->GetMainFrame() &&
		((UObject*)Viewport->Console)->GetMainFrame()->StateNode &&
		(
			((UObject*)Viewport->Console)->GetMainFrame()->StateNode->GetFName() == "Menuing" ||
			((UObject*)Viewport->Console)->GetMainFrame()->StateNode->GetFName() == "Typing" ||
			((UObject*)Viewport->Console)->GetMainFrame()->StateNode->GetFName() == "Console"
		);
}

static void AndroidSetSoftKeyboardRect( UNSDLViewport* Viewport )
{
	if( !Viewport )
		return;

	SDL_Rect TextRect;
	TextRect.x = 0;
	TextRect.y = ( Viewport->SizeY > 180 ) ? ( Viewport->SizeY - 180 ) : 0;
	TextRect.w = ( Viewport->SizeX > 0 ) ? Viewport->SizeX : 1;
	TextRect.h = 180;
	SDL_SetTextInputRect( &TextRect );
}

static void AndroidKeepSoftKeyboardAlive( UNSDLViewport* Viewport, FLOAT KeepAliveSeconds )
{
	if( !Viewport )
		return;

	AndroidSetSoftKeyboardRect( Viewport );

	if( !SDL_IsTextInputActive() )
	{
		SDL_StartTextInput();
	}

	GAndroidSoftKeyboardActive = 1;
	GAndroidSoftKeyboardTimeout = appSeconds() + KeepAliveSeconds;
}

static void AndroidKickSoftKeyboard( UNSDLViewport* Viewport, FLOAT KeepAliveSeconds, UBOOL bForceRestart )
{
	if( !Viewport )
		return;

	const FLOAT Now = appSeconds();

	AndroidSetSoftKeyboardRect( Viewport );

	if( bForceRestart )
	{
		if( SDL_IsTextInputActive() )
		{
			SDL_StopTextInput();
		}
		SDL_StartTextInput();
		GAndroidSoftKeyboardLastKick = Now;
		debugf( NAME_Log, "Android soft keyboard force requested" );
	}
	else if( !SDL_IsTextInputActive() )
	{
		SDL_StartTextInput();
		GAndroidSoftKeyboardLastKick = Now;
	}

	GAndroidSoftKeyboardActive = 1;
	GAndroidSoftKeyboardTimeout = Now + KeepAliveSeconds;
}

static void AndroidRequestSoftKeyboard( UNSDLViewport* Viewport, FLOAT KeepAliveSeconds )
{
	if( !Viewport )
		return;

	const FLOAT Now = appSeconds();
	GAndroidSoftKeyboardPulseViewport = Viewport;
	GAndroidSoftKeyboardPulseUntil = Now + 1.5f;

	// Force once immediately. This reopens the IME even if SDL still reports text input as active.
	AndroidKickSoftKeyboard( Viewport, KeepAliveSeconds, 1 );
}

static void AndroidShowSoftKeyboard( UNSDLViewport* Viewport, FLOAT KeepAliveSeconds )
{
	// Compatibility wrapper for older patch hooks.
	AndroidRequestSoftKeyboard( Viewport, KeepAliveSeconds );
}

static void AndroidHideSoftKeyboard()
{
	if( GAndroidSoftKeyboardActive || SDL_IsTextInputActive() )
	{
		SDL_StopTextInput();
	}
	GAndroidSoftKeyboardActive = 0;
	GAndroidSoftKeyboardTimeout = 0.0f;
	GAndroidSoftKeyboardPulseUntil = 0.0f;
	GAndroidSoftKeyboardPulseViewport = NULL;
}

static void AndroidUpdateSoftKeyboardTimeout( FLOAT CurTime )
{
	if( GAndroidSoftKeyboardPulseViewport && GAndroidSoftKeyboardPulseUntil > CurTime )
	{
		// One extra gentle kick after the menu processed the click/A button and entered text mode.
		if( CurTime - GAndroidSoftKeyboardLastKick > 0.45f )
		{
			AndroidKickSoftKeyboard( GAndroidSoftKeyboardPulseViewport, 90.0f, 0 );
		}
	}
	else
	{
		GAndroidSoftKeyboardPulseUntil = 0.0f;
		GAndroidSoftKeyboardPulseViewport = NULL;
	}

	if( GAndroidSoftKeyboardActive && GAndroidSoftKeyboardTimeout > 0.0f && CurTime > GAndroidSoftKeyboardTimeout )
	{
		AndroidHideSoftKeyboard();
	}
}
#endif
// UE1_ANDROID_SOFT_KEYBOARD_PATCH_END

/*-----------------------------------------------------------------------------
	UNSDLViewport implementation.
-----------------------------------------------------------------------------*/

//
// SDL_BUTTON_ -> EInputKey translation map.
//
const BYTE UNSDLViewport::MouseButtonMap[6] =
{
	/* invalid           */ IK_None,
	/* SDL_BUTTON_LEFT   */ IK_LeftMouse,
	/* SDL_BUTTON_MIDDLE */ IK_MiddleMouse,
	/* SDL_BUTTON_RIGHT  */ IK_RightMouse,
	/* SDL_BUTTON_X1     */ IK_None,
	/* SDL_BUTTON_X2     */ IK_None
};

//
// SDL_CONTROLLER_BUTTON_ -> EInputKey translation map.
//
const BYTE UNSDLViewport::JoyButtonMap[SDL_CONTROLLER_BUTTON_MAX] =
{
	/* BUTTON_A             */ IK_Joy1,
	/* BUTTON_B             */ IK_Joy2,
	/* BUTTON_X             */ IK_Joy3,
	/* BUTTON_Y             */ IK_Joy4,
	/* BUTTON_BACK          */ IK_Joy5,
	/* BUTTON_GUIDE         */ IK_Joy6,
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_START_IS_ESCAPE
	/* BUTTON_START         */ IK_Escape,
#else
	/* BUTTON_START         */ IK_Joy7,
#endif
	/* BUTTON_LEFTSTICK     */ IK_Joy8,
	/* BUTTON_RIGHTSTICK    */ IK_Joy9,
	/* BUTTON_LEFTSHOULDER  */ IK_Joy10,
	/* BUTTON_RIGHTSHOULDER */ IK_Joy11,
	/* BUTTON_DPAD_UP       */ IK_JoyPovUp,
	/* BUTTON_DPAD_DOWN     */ IK_JoyPovDown,
	/* BUTTON_DPAD_LEFT     */ IK_JoyPovLeft,
	/* BUTTON_DPAD_RIGHT    */ IK_JoyPovRight,
};

//
// SDL_CONTROLLER_BUTTON_ -> EInputKey translation map for UI controls.
//
const BYTE UNSDLViewport::JoyButtonMapUI[SDL_CONTROLLER_BUTTON_MAX] =
{
	/* BUTTON_A             */ IK_Enter,
	/* BUTTON_B             */ IK_Escape,
	/* BUTTON_X             */ IK_N,
	/* BUTTON_Y             */ IK_Y,
	/* BUTTON_BACK          */ IK_Escape,
	/* BUTTON_GUIDE         */ IK_Escape,
	/* BUTTON_START         */ IK_Escape,
	/* BUTTON_LEFTSTICK     */ IK_Joy8,
	/* BUTTON_RIGHTSTICK    */ IK_Joy9,
	/* BUTTON_LEFTSHOULDER  */ IK_Joy10,
	/* BUTTON_RIGHTSHOULDER */ IK_Joy11,
	/* BUTTON_DPAD_UP       */ IK_Up,
	/* BUTTON_DPAD_DOWN     */ IK_Down,
	/* BUTTON_DPAD_LEFT     */ IK_Left,
	/* BUTTON_DPAD_RIGHT    */ IK_Right,
};

//
// SDL_CONTROLLER_BUTTON_ -> EInputKey translation map.
//
const BYTE UNSDLViewport::JoyAxisMap[SDL_CONTROLLER_AXIS_MAX] =
{
	/* AXIS_LEFT_X          */ IK_JoyX,
	/* AXIS_LEFT_Y          */ IK_JoyY,
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_RIGHT_STICK_MOUSELOOK
	/* AXIS_RIGHT_X         */ IK_MouseX,
	/* AXIS_RIGHT_Y         */ IK_MouseY,
#else
	/* AXIS_RIGHT_X         */ IK_JoyU,
	/* AXIS_RIGHT_Y         */ IK_JoyV,
#endif
	/* AXIS_LTRIGGER        */ IK_Joy12,
	/* AXIS_RTRIGGER        */ IK_Joy13,
};

//
// Additional scale to apply per SDL axis.
//
const FLOAT UNSDLViewport::JoyAxisDefaultScale[SDL_CONTROLLER_AXIS_MAX] =
{
	/* AXIS_LEFT_X          */ +60.f,
	/* AXIS_LEFT_Y          */ -60.f,
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_MOUSELOOK_SLOWER
	/* AXIS_RIGHT_X         */ +4.f,
	/* AXIS_RIGHT_Y         */ +4.f,
#else
	/* AXIS_RIGHT_X         */ +60.f,
	/* AXIS_RIGHT_Y         */ +60.f,
#endif
	/* AXIS_LTRIGGER        */ +60.f,
	/* AXIS_RTRIGGER        */ +60.f,
};

//
// SDL_Scancode -> EInputKey translation map.
//
BYTE UNSDLViewport::KeyMap[512];
void UNSDLViewport::InitKeyMap()
{
	#define INIT_KEY_RANGE( AStart, AEnd, BStart, BEnd ) \
		for( DWORD Key = AStart; Key <= AEnd; ++Key ) KeyMap[Key] = BStart + ( Key - AStart )

	appMemset( KeyMap, 0, sizeof( KeyMap ) );

	// TODO: IK_LControl, IK_LShift, etc exist, what are they for?
	KeyMap[SDL_SCANCODE_LSHIFT] = IK_Shift;
	KeyMap[SDL_SCANCODE_RSHIFT] = IK_Shift;
	KeyMap[SDL_SCANCODE_LCTRL] = IK_Ctrl;
	KeyMap[SDL_SCANCODE_RCTRL] = IK_Ctrl;
	KeyMap[SDL_SCANCODE_LALT] = IK_Alt;
	KeyMap[SDL_SCANCODE_RALT] = IK_Alt;
	KeyMap[SDL_SCANCODE_GRAVE] = IK_Tilde;
	KeyMap[SDL_SCANCODE_ESCAPE] = IK_Escape;
	KeyMap[SDL_SCANCODE_SPACE] = IK_Space;
	KeyMap[SDL_SCANCODE_RETURN] = IK_Enter;
	KeyMap[SDL_SCANCODE_BACKSPACE] = IK_Backspace;
	KeyMap[SDL_SCANCODE_CAPSLOCK] = IK_CapsLock;
	KeyMap[SDL_SCANCODE_TAB] = IK_Tab;
	KeyMap[SDL_SCANCODE_DELETE] = IK_Delete;
	KeyMap[SDL_SCANCODE_INSERT] = IK_Insert;
	KeyMap[SDL_SCANCODE_HOME] = IK_Home;
	KeyMap[SDL_SCANCODE_END] = IK_End;
	KeyMap[SDL_SCANCODE_PAGEUP] = IK_PageUp;
	KeyMap[SDL_SCANCODE_PAGEDOWN] = IK_PageDown;
	KeyMap[SDL_SCANCODE_PRINTSCREEN] = IK_PrintScrn;
	KeyMap[SDL_SCANCODE_EQUALS] = IK_Equals;
	KeyMap[SDL_SCANCODE_SEMICOLON] = IK_Semicolon;
	KeyMap[SDL_SCANCODE_BACKSLASH] = IK_Backslash;
	KeyMap[SDL_SCANCODE_SLASH] = IK_Slash;
	KeyMap[SDL_SCANCODE_LEFTBRACKET] = IK_LeftBracket;
	KeyMap[SDL_SCANCODE_RIGHTBRACKET] = IK_RightBracket;
	KeyMap[SDL_SCANCODE_COMMA] = IK_Comma;
	KeyMap[SDL_SCANCODE_PERIOD] = IK_Period;
	KeyMap[SDL_SCANCODE_LEFT] = IK_Left;
	KeyMap[SDL_SCANCODE_UP] = IK_Up;
	KeyMap[SDL_SCANCODE_RIGHT] = IK_Right;
	KeyMap[SDL_SCANCODE_DOWN] = IK_Down;
	KeyMap[SDL_SCANCODE_0] = IK_0;
	KeyMap[SDL_SCANCODE_KP_0] = IK_NumPad0;
	KeyMap[SDL_SCANCODE_KP_PERIOD] = IK_NumPadPeriod;

	INIT_KEY_RANGE( SDL_SCANCODE_1,    SDL_SCANCODE_9,    IK_1,       IK_9 );
	INIT_KEY_RANGE( SDL_SCANCODE_A,    SDL_SCANCODE_Z,    IK_A,       IK_Z );
	INIT_KEY_RANGE( SDL_SCANCODE_KP_1, SDL_SCANCODE_KP_9, IK_NumPad1, IK_NumPad9 );
	INIT_KEY_RANGE( SDL_SCANCODE_F1,   SDL_SCANCODE_F12,  IK_F1,      IK_F12 );
	INIT_KEY_RANGE( SDL_SCANCODE_F13,  SDL_SCANCODE_F24,  IK_F13,     IK_F24 );

	#undef INIT_KEY_RANGE
}

//
// Static init.
//
void UNSDLViewport::InternalClassInitializer( UClass* Class )
{
	guard(UNSDLViewport::InternalClassInitializer);
	// Fill in keymap.
	InitKeyMap();
	unguard;
}

//
// Constructor.
//
UNSDLViewport::UNSDLViewport( ULevel* InLevel, UNSDLClient* InClient )
:	UViewport( InLevel, InClient )
,	Client( InClient )
{
	guard(UNSDLViewport::UNSDLViewport);

	// Set color bytes based on screen resolution.
	SDL_DisplayMode Mode;
	SDL_GetDesktopDisplayMode( InClient->DefaultDisplay, &Mode );
	ColorBytes = SDL_BYTESPERPIXEL( Mode.format );
	Caps = 0;
	if( ColorBytes == 2 && SDL_PIXELLAYOUT( Mode.format ) == SDL_PACKEDLAYOUT_565 )
	{
		Caps |= CC_RGB565;
	}

	// Inherit default display until we have a window.
	DisplayIndex = InClient->DefaultDisplay;
	DisplaySize.w = InClient->GetDefaultDisplayMode().w;
	DisplaySize.h = InClient->GetDefaultDisplayMode().h;

	// Init input.
	if( GIsEditor )
		Input->Init( this, GSystem );

	Destroyed = false;
	QuitRequested = false;

	unguard;
}

// UObject interface.
void UNSDLViewport::Destroy()
{
	guard(UNSDLViewport::Destroy);
	if( Client->FullscreenViewport == this )
	{
		Client->FullscreenViewport = NULL;
	}
	UViewport::Destroy();
	unguard;
}

//
// Set the mouse cursor according to Unreal or UnrealEd's mode, or to
// an hourglass if a slow task is active. Not implemented.
//
void UNSDLViewport::SetModeCursor()
{
	guard(UNSDLViewport::SetModeCursor);
	unguard;
}

//
// Update user viewport interface.
//
void UNSDLViewport::UpdateWindow()
{
	guard(UNSDLViewport::UpdateViewportWindow);

	// If not a window, exit.
	if( hWnd==NULL || OnHold )
		return;

	// Set viewport window's name to show resolution.
	char WindowName[80];
	if( !GIsEditor || (Actor->ShowFlags&SHOW_PlayerCtrl) )
	{
		appSprintf( WindowName, LocalizeGeneral("Product","Core") );
	}
	else switch( Actor->RendMap )
	{
		case REN_Wire:		strcpy(WindowName,LocalizeGeneral("ViewPersp")); break;
		case REN_OrthXY:	strcpy(WindowName,LocalizeGeneral("ViewXY")); break;
		case REN_OrthXZ:	strcpy(WindowName,LocalizeGeneral("ViewXZ")); break;
		case REN_OrthYZ:	strcpy(WindowName,LocalizeGeneral("ViewYZ")); break;
		default:			strcpy(WindowName,LocalizeGeneral("ViewOther")); break;
	}

	// Set window title.
	if( SizeX && SizeY )
	{
		appSprintf(WindowName+strlen(WindowName)," (%i x %i)",SizeX,SizeY);
		if( this == Client->CurrentViewport() )
			strcat( WindowName, " *" );
	}
	SDL_SetWindowTitle( hWnd, WindowName );

	unguard;
}

//
// Open a viewport window.
//
void UNSDLViewport::OpenWindow( void* InParentWindow, UBOOL Temporary, INT NewX, INT NewY, INT OpenX, INT OpenY )
{
	guard(UNSDLViewport::OpenWindow);
	check(Actor);
	check(!OnHold);
	UBOOL DoRepaint=0, DoSetActive=0;
	UBOOL DoOpenGL=0;
	UBOOL NoHard=ParseParam( appCmdLine(), "nohard" );
	SDL_GLprofile GLProfile = SDL_GL_CONTEXT_PROFILE_COMPATIBILITY;
	NewX = Align(NewX,4);

	if( !Temporary && !GIsEditor && !NoHard )
	{
		// HACK: Just check if we're about to load OpenGLDrv. Not sure how else you would know to add the GL flag.
		char Temp[256] = "";
		GetConfigString( "Engine.Engine", "GameRenderDevice", Temp, ARRAY_COUNT(Temp) );
		appStrupr( Temp );
		if( !appStrstr( Temp, "OPENGL" ) )
		{
			GetConfigString( "Engine.Engine", "WindowedRenderDevice", Temp, ARRAY_COUNT(Temp) );
			appStrupr( Temp );
			if( appStrstr( Temp, "OPENGL" ) )
				DoOpenGL = 1;
		}
		else
		{
			DoOpenGL = 1;
		}
		if( DoOpenGL && appStrstr( Temp, "GLES" ) )
			GLProfile = SDL_GL_CONTEXT_PROFILE_ES;
	}

	// User window of launcher if no parent window was specified.
	if( !InParentWindow )
	{
		QWORD ParentPtr;
		Parse( appCmdLine(), "HWND=", ParentPtr );
		InParentWindow = (void*)ParentPtr;
	}

	if( Temporary )
	{
		// Create in-memory data.
		ColorBytes = 2;
		ScreenPointer = (BYTE*)appMalloc( 2 * NewX * NewY, "TemporaryViewportData" );	
		hWnd = NULL;
		debugf( NAME_Log, "Opened temporary viewport" );
	}
	else
	{
		// Get flags.
		DWORD Flags = 0;
		if( InParentWindow && (Actor->ShowFlags & SHOW_ChildWindow) )
		{
			Flags = SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS;
		}
		else
		{
			Flags = SDL_WINDOW_HIDDEN;
		}
		if( DoOpenGL )
		{
			Flags |= SDL_WINDOW_OPENGL;
		}
#ifdef PLATFORM_ANDROID
		// UNREAL_ANDROID_ACTIVITY_FULLSCREEN_ONLY
		// The Android Activity/SurfaceView already owns fullscreen. Requesting
		// SDL_WINDOW_FULLSCREEN here can make Android allocate a rotated buffer
		// (for example 972x1920 transform=7) while the surface is 1920x1080.
		Flags |= SDL_WINDOW_BORDERLESS | SDL_WINDOW_RESIZABLE;
#endif

		// Set OpenGL attributes if needed.
		if( DoOpenGL )
		{
			if( GLProfile == SDL_GL_CONTEXT_PROFILE_ES )
			{
				// Request GLES2.
				SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
				SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
			}
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_GLES_PROFILE_ES
			GLProfile = SDL_GL_CONTEXT_PROFILE_ES;
#endif
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, GLProfile );
		}

		// Set position and size.
		if( OpenX==-1 )
			OpenX = SDL_WINDOWPOS_UNDEFINED;
		if( OpenY==-1 )
			OpenY = SDL_WINDOWPOS_UNDEFINED;

		// If switching renderers, destroy the old window.
		if( hWnd && ( DoOpenGL != !!( SDL_GetWindowFlags( hWnd ) & SDL_WINDOW_OPENGL ) ) )
		{
			CloseWindow();
		}

#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_PRECREATE_DISPLAY_SIZE
		// Native follows the real Android drawable. Fixed entries keep their logical
		// render size and are fullscreen-scaled by NOpenGLESDrv.
		// UE1_ANDROID_RESOLUTION_MENU_NATIVE_FIXED_CLEAN_V85
		if( !UE1AndroidIsFixedMenuResolution( NewX, NewY ) && Client->AndroidResolutionMode == 0 )
		{
			INT NativeX = 0, NativeY = 0;
			UE1AndroidGetNativeDrawableSize( hWnd, GLCtx, Client, NativeX, NativeY );
			if( NativeX <= 0 || NativeY <= 0 )
			{
				SDL_DisplayMode AndroidMode;
				if( SDL_GetCurrentDisplayMode( 0, &AndroidMode ) == 0 && AndroidMode.w > 0 && AndroidMode.h > 0 )
				{
					NativeX = AndroidMode.w;
					NativeY = AndroidMode.h;
					if( NativeY > NativeX )
						Exchange( NativeX, NativeY );
					NativeX = Align( NativeX, 4 );
				}
			}
			if( NativeX > 0 && NativeY > 0 )
			{
				if( NewX != NativeX || NewY != NativeY )
					debugf( NAME_Log, "Android pre-create display size: requested=%ix%i -> Native %ix%i", NewX, NewY, NativeX, NativeY );
				NewX = NativeX;
				NewY = NativeY;
			}
		}
#endif

		// Create or update the window.
		if( !hWnd )
		{
			// Creating new viewport.
			hWnd = SDL_CreateWindow( "", OpenX, OpenY, NewX, NewY, Flags );
			if( !hWnd && DoOpenGL )
			{
				// Try without GL.
				debugf( NAME_Warning, "Could not create OpenGL window: %s. Trying without OpenGL.", SDL_GetError() );
				Flags &= ~SDL_WINDOW_OPENGL;
				DoOpenGL = 0;
				hWnd = SDL_CreateWindow( "", OpenX, OpenY, NewX, NewY, Flags );
			}
			if( !hWnd )
			{
				appErrorf( "Could not create SDL window: %s", SDL_GetError() );
			}

			// Set parent window.
			if( InParentWindow && (Actor->ShowFlags & SHOW_ChildWindow) )
			{
				SDL_SetWindowModalFor( hWnd, (SDL_Window*)InParentWindow );
			}

			debugf( NAME_Log, "Opened viewport" );
			DoSetActive = DoRepaint = 1;
		}
		else
		{
			// Resizing existing viewport.
			SetClientSize( NewX, NewY, false );
		}

		// Create GL context or SDL renderer if needed.
		if( DoOpenGL )
		{
			if( !GLCtx )
			{
				GLCtx = SDL_GL_CreateContext( hWnd );
				if( !GLCtx )
				{
					appErrorf( "Could not create GL context: %s", SDL_GetError() );
				}
			}
			SDL_GL_MakeCurrent( hWnd, GLCtx );
		}
		else
		{
			SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "nearest" );
			SDLRen = SDL_CreateRenderer( hWnd, -1, 0 );
			if( !SDLRen )
			{
				// Fallback to software.
				debugf( NAME_Warning, "Could not create SDL renderer: %s. Trying software.", SDL_GetError() );
				SDLRen = SDL_CreateRenderer( hWnd, -1, SDL_RENDERER_SOFTWARE );
				if( !SDLRen )
				{
					appErrorf( "Could not create SDL renderer: %s", SDL_GetError() );
				}
			}
			// Create framebuffer texture.
			SDLTexFormat = SDL_PIXELFORMAT_ARGB8888;
			ColorBytes = SDL_BYTESPERPIXEL( SDLTexFormat );
			Caps = ( SDL_PIXELLAYOUT( SDLTexFormat ) == SDL_PACKEDLAYOUT_565 ) ? CC_RGB565 : 0;
			SDLTex = SDL_CreateTexture( SDLRen, SDLTexFormat, SDL_TEXTUREACCESS_STREAMING, NewX, NewY );
			if( !SDLTex )
			{
				appErrorf( "Could not create framebuffer texture: %s", SDL_GetError() );
			}
		}

		SDL_ShowWindow( hWnd );

#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_SYNC_DRAWABLE_SIZE
		// Native tracks the drawable. Fixed menu resolutions deliberately keep
		// SizeX/SizeY lower and are fullscreen-scaled by the GLES FBO.
		// UE1_ANDROID_RESOLUTION_MENU_NATIVE_FIXED_CLEAN_V85
		if( Client->AndroidResolutionMode == 0 && !UE1AndroidIsFixedMenuResolution( NewX, NewY ) )
		{
			INT FixedX = 0, FixedY = 0;
			UE1AndroidGetNativeDrawableSize( hWnd, DoOpenGL ? GLCtx : NULL, Client, FixedX, FixedY );
			if( FixedX > 0 && FixedY > 0 && ( NewX != FixedX || NewY != FixedY ) )
			{
				debugf( NAME_Log, "Android drawable size: viewport=%ix%i -> Native %ix%i", NewX, NewY, FixedX, FixedY );
				NewX = FixedX;
				NewY = FixedY;
			}
		}
#endif

		// Get this window's display parameters.
		SDL_DisplayMode DisplayMode;
		DisplayIndex = SDL_GetWindowDisplayIndex( hWnd );
		if( SDL_GetWindowDisplayMode( hWnd, &DisplayMode ) == 0 )
		{
			DisplaySize.w = DisplayMode.w;
			DisplaySize.h = DisplayMode.h;
		}
	}

	SizeX = NewX;
	SizeY = NewY;

	if( !RenDev && Temporary )
		Client->TryRenderDevice( this, "SoftDrv.SoftwareRenderDevice", 0 );
	if( !RenDev && !GIsEditor && !NoHard )
		Client->TryRenderDevice( this, "ini:Engine.Engine.GameRenderDevice", Client->StartupFullscreen );
	if( !RenDev )
		Client->TryRenderDevice( this, "ini:Engine.Engine.WindowedRenderDevice", 0 );
	check(RenDev);

	if( !Temporary )
		UpdateWindow();
	if( DoRepaint )
		Repaint();

	unguard;
}

//
// Close a viewport window.  Assumes that the viewport has been opened with
// OpenViewportWindow.  Does not affect the viewport's object, only the
// platform-specific information associated with it.
//
void UNSDLViewport::CloseWindow()
{
	guard(UNSDLViewport::CloseWindow);

#if PLATFORM_ANDROID
	AndroidHideSoftKeyboard();
#endif

	if( hWnd )
	{
		if( SDLTex )
		{
			SDL_DestroyTexture( SDLTex );
			SDLTex = NULL;
		}
		if( SDLRen )
		{
			SDL_DestroyRenderer( SDLRen );
			SDLRen = NULL;
		}
		if( GLCtx )
		{
			SDL_GL_DeleteContext( GLCtx );
			GLCtx = NULL;
		}
		SDL_DestroyWindow( hWnd );
		hWnd = NULL;
	}

	unguard;
}

//
// Lock the viewport window and set the approprite Screen and RealScreen fields
// of Viewport.  Returns 1 if locked successfully, 0 if failed.  Note that a
// lock failing is not a critical error; it's a sign that a DirectDraw mode
// has ended or the user has closed a viewport window.
//
UBOOL UNSDLViewport::Lock( FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize )
{
	guard(UNSDLViewport::LockWindow);
	uclock(Client->DrawCycles);

	// Make sure window is lockable.
	if( !hWnd )
	{
		return 0;
	}

	if( OnHold || !SizeX || !SizeY )
	{
		appErrorf( "Failed locking viewport" );
		return 0;
	}

	if( SDLRen && SDLTex )
	{
		// Obtain pointer to screen.
		Stride = SizeX;
		ScreenPointer = NULL;
		SDL_LockTexture( SDLTex, NULL, (void **)&ScreenPointer, &Stride );
		Stride /= ColorBytes;
		check(ScreenPointer);
	}

	// Success.
	uunclock(Client->DrawCycles);

	return UViewport::Lock( FlashScale, FlashFog, ScreenClear, RenderLockFlags, HitData, HitSize );

	unguard;
}

//
// Unlock the viewport window.  If Blit=1, blits the viewport's frame buffer.
//
void UNSDLViewport::Unlock( UBOOL Blit )
{
	guard(UNSDLViewport::Unlock);

	Client->DrawCycles=0;
	uclock(Client->DrawCycles);

	// Unlock base.
	UViewport::Unlock( Blit );

	// Blit, if desired.
	if( Blit && hWnd && !OnHold )
	{
		if( GLCtx )
		{
			// Flip OpenGL buffers.
			SDL_GL_SwapWindow( hWnd );
		}
		else if( SDLRen && SDLTex )
		{
			// Blitting with SDLRenderer.
			SDL_UnlockTexture( SDLTex );
			SDL_RenderCopy( SDLRen, SDLTex, NULL, NULL );
			SDL_RenderPresent( SDLRen );
		}
	}

	uunclock(Client->DrawCycles);

	unguard;
}

//
// Make this viewport the current one.
// If Viewport=0, makes no viewport the current one.
//
void UNSDLViewport::MakeCurrent()
{
	guard(UNSDLViewport::MakeCurrent);
	Current = 1;
	for( INT i=0; i<Client->Viewports.Num(); i++ )
	{
		UViewport* OldViewport = Client->Viewports(i);
		if( OldViewport->Current && OldViewport != this )
		{
			OldViewport->Current = 0;
			OldViewport->UpdateWindow();
		}
	}
	if( GLCtx )
	{
		SDL_GL_MakeCurrent( hWnd, GLCtx );
	}
	UpdateWindow();
	unguard;
}

//
// Repaint the viewport.
//
void UNSDLViewport::Repaint()
{
	guard(UNSDLViewport::Repaint);
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	UE1AndroidRefreshRuntimeMenuPatches();
#endif
	if( !OnHold && RenDev && SizeX && SizeY )
		Client->Engine->Draw( this, 0 );
	unguard;
}

//
// Set the client size (viewport view size) of a viewport.
//
void UNSDLViewport::SetClientSize( INT NewX, INT NewY, UBOOL UpdateProfile )
{
	guard(UNSDLViewport::SetClientSize);

	if( hWnd )
	{
#ifndef PLATFORM_ANDROID // UNREAL_ANDROID_NO_SETWINDOWSIZE_800X600
		SDL_SetWindowSize( hWnd, NewX, NewY );
#endif
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_SETCLIENT_DRAWABLE_SIZE
		// Native follows the drawable; fixed menu resolutions keep their logical
		// SizeX/SizeY and are expanded to the surface by NOpenGLESDrv.
		// UE1_ANDROID_RESOLUTION_MENU_NATIVE_FIXED_CLEAN_V85
		UE1AndroidApplyConfiguredResolution( Client, hWnd, GLCtx, NewX, NewY );
#endif
		// Resize output texture if required.
		if( SDLRen && SDLTex )
		{
			SDL_DestroyTexture( SDLTex );
			SDLTex = SDL_CreateTexture( SDLRen, SDLTexFormat, SDL_TEXTUREACCESS_STREAMING, NewX, NewY );
			if( !SDLTex )
			{
				appErrorf( "Could not create framebuffer texture: %s", SDL_GetError() );
			}
		}
	}

	SizeX = NewX;
	SizeY = NewY;

	// Optionally save this size in the profile.
	if( UpdateProfile )
	{
		Client->ViewportX = NewX;
		Client->ViewportY = NewY;
		Client->SaveConfig();
	}

	unguard;
}

//
// Return the viewport's window.
//
void* UNSDLViewport::GetWindow()
{
	return (void*)hWnd;
}

//
// Try to make this viewport fullscreen, matching the fullscreen
// mode of the nearest x-size to the current window. If already in
// fullscreen, returns to non-fullscreen.
//
void UNSDLViewport::MakeFullscreen( INT NewX, INT NewY, UBOOL UpdateProfile )
{
	guard(UNSDLViewport::MakeFullscreen);

	// If someone else is fullscreen, stop them.
	if( Client->FullscreenViewport )
		Client->EndFullscreen();

	// Save this window.
	SavedX = SizeX;
	SavedY = SizeY;

	// Fullscreen rendering. For now no borderless.
	Client->FullscreenViewport = this;
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_FULLSCREEN_NO_MODE_SWITCH
	// Keep Android in Activity fullscreen only. Resolution selection is a logical
	// render-size switch: Native tracks the drawable, fixed entries stay fixed.
	// UE1_ANDROID_RESOLUTION_MENU_NATIVE_FIXED_CLEAN_V85
	UE1AndroidApplyConfiguredResolution( Client, hWnd, GLCtx, NewX, NewY );
	SetClientSize( NewX, NewY, false );
#else
	SetClientSize( NewX, NewY, false );
	SDL_SetWindowFullscreen( hWnd, SDL_WINDOW_FULLSCREEN );
#endif

	if( UpdateProfile )
	{
		Client->ViewportX = NewX;
		Client->ViewportY = NewY;
		Client->SaveConfig();
	}

	SetMouseCapture(1, 1, 0);

	unguard;
}

//
//
//
void UNSDLViewport::EndFullscreen()
{
	guard(UNSDLViewport::EndFullscreen);

#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_FULLSCREEN_END_NOOP
	// Stay in Activity fullscreen on Android; just resync to selected logical size.
	// Native tracks the drawable; fixed modes remain fixed.
	INT NewX = SizeX;
	INT NewY = SizeY;
	UE1AndroidApplyConfiguredResolution( Client, hWnd, GLCtx, NewX, NewY ); // UE1_ANDROID_RESOLUTION_MENU_NATIVE_FIXED_CLEAN_V85
	SetClientSize( NewX, NewY, false );
#else
	SDL_SetWindowFullscreen( hWnd, 0 );
	SetClientSize( SavedX, SavedY, false );
#endif

	unguard;
}

//
// Update input for viewport.
//
void UNSDLViewport::UpdateInput( UBOOL Reset )
{
	guard(UNSDLViewport::UpdateInput);

	if( Reset )
	{
		appMemset( (void*)JoyAxis, 0, sizeof(JoyAxis) );
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
		UE1AndroidNativeControllerResetState();
#endif
	}

	unguard;
}

//
// If the cursor is currently being captured, stop capturing, clipping, and 
// hiding it, and move its position back to where it was when it was initially
// captured.
//
void UNSDLViewport::SetMouseCapture( UBOOL Capture, UBOOL Clip, UBOOL OnlyFocus )
{
	guard(UNSDLViewport::SetMouseCapture);

	// If only focus, reject.
	if( OnlyFocus )
		if( hWnd != SDL_GetMouseFocus() )
			return;

	// If capturing, windows requires clipping in order to keep focus.
	Clip |= Capture;

	// Handle capturing.
	SDL_SetRelativeMouseMode( (SDL_bool)Capture );

	unguard;
}

#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
static UBOOL UE1AndroidCleanDispatchKeyCaptureV86( UNSDLViewport* Viewport, INT Key );
#endif

UBOOL UNSDLViewport::CauseInputEvent( INT iKey, EInputAction Action, FLOAT Delta )
{
	guard(UWindowsViewport::CauseInputEvent);

	// Route to engine if a valid key
	if( iKey > 0 )
	{
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
		// UnrealPlayerMenu originally exposes only Name/Team/Skin. The restored
		// Class row is handled natively because the compiled Unreal.u cannot be
		// changed safely on-device.
		if( UE1AndroidHandlePlayerSetupClassInput( this, iKey, Action ) )
			return 1;

		// v85: UI-only key-capture bridge. Do not alter gameplay input.
		// It gives Android controller aliases such as TriggerL/TriggerR their
		// friendly labels while Customize Controls is waiting for a key, and it
		// directly saves NextWeapon because old compiled Unreal.u may still miss
		// the entry-20 ProcessPending path.
		if( Action == IST_Press && UE1AndroidCleanDispatchKeyCaptureV86( this, iKey ) )
			return 1;
#endif
		UBOOL Result = Client->Engine->InputEvent( this, (EInputKey)iKey, Action, Delta );
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
		UE1AndroidPostProcessJoinMenuInput( this, iKey, Action );
#endif
		return Result;
	}
	else
		return 0;

	unguard;
}

#ifdef __ANDROID__
// ANDROID_SOFT_KEYBOARD_V5_MENUTYPING_HELPER
//
// Android soft keyboard policy:
// UE1 menu text input uses Console state 'MenuTyping'.
// Keep the Android IME active only while that state is active.
static UBOOL UE1Android_IsConsoleMenuTyping( UObject* ConsoleObject )
{
	if( !ConsoleObject )
		return 0;

	if( !ConsoleObject->GetMainFrame() )
		return 0;

	if( !ConsoleObject->GetMainFrame()->StateNode )
		return 0;

	return ConsoleObject->GetMainFrame()->StateNode->GetFName() == "MenuTyping";
}

static void UE1Android_UpdateSoftKeyboardForMenuTyping( UObject* ConsoleObject, INT ViewSizeX, INT ViewSizeY )
{
	static UBOOL bKeyboardWanted = 0;
	static Uint32 LastStartTicks = 0;

	const UBOOL bWantKeyboard = UE1Android_IsConsoleMenuTyping( ConsoleObject );
	const Uint32 NowTicks = SDL_GetTicks();

	if( bWantKeyboard )
	{
		SDL_Rect TextRect;
		TextRect.x = 0;
		TextRect.y = ( ViewSizeY > 0 ) ? ( ( ViewSizeY * 2 ) / 3 ) : 0;
		TextRect.w = ( ViewSizeX > 0 ) ? ViewSizeX : 1;
		TextRect.h = ( ViewSizeY > 3 ) ? ( ViewSizeY / 3 ) : 1;
		SDL_SetTextInputRect( &TextRect );

		// Keep-alive: some Android keyboards ignore a one-shot StartTextInput
		// if focus was not fully settled yet.
		if( !bKeyboardWanted || !SDL_IsTextInputActive() || ( NowTicks - LastStartTicks ) > 750 )
		{
			SDL_StartTextInput();
			LastStartTicks = NowTicks;
		}

		bKeyboardWanted = 1;
	}
	else
	{
		if( bKeyboardWanted || SDL_IsTextInputActive() )
		{
			SDL_StopTextInput();
		}

		bKeyboardWanted = 0;
		LastStartTicks = 0;
	}
}
#endif

UBOOL UNSDLViewport::TickInput()
{
	guard(UNSDLViewport::TickInput);

	SDL_Event Ev;
	INT Tmp;
	const FLOAT CurTime = appSeconds();
	const FLOAT DeltaTime = CurTime - InputUpdateTime;
#ifdef __ANDROID__
	// ANDROID_SOFT_KEYBOARD_V5_MENUTYPING_TICK
	UE1Android_UpdateSoftKeyboardForMenuTyping( (UObject*)Console, SizeX, SizeY );
#endif
#if PLATFORM_ANDROID
	AndroidUpdateSoftKeyboardTimeout( CurTime );
#endif
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	const UBOOL bAndroidNativeController = Client && Client->UseJoystick && Client->AndroidNativeController;
	GAndroidNativeControllerRuntimeEnabled = bAndroidNativeController ? 1 : 0;
	const UBOOL bAndroidNativeNormalMenu = bAndroidNativeController && Console &&
		((UObject*)Console)->GetMainFrame() &&
		((UObject*)Console)->GetMainFrame()->StateNode &&
		((UObject*)Console)->GetMainFrame()->StateNode->GetFName() == "Menuing";
	const UBOOL bAndroidNativeKeyMenuing = bAndroidNativeController && Console &&
		((UObject*)Console)->GetMainFrame() &&
		((UObject*)Console)->GetMainFrame()->StateNode &&
		((UObject*)Console)->GetMainFrame()->StateNode->GetFName() == "KeyMenuing";
	if( bAndroidNativeController )
	{
		if( bAndroidNativeNormalMenu && !GAndroidNativeControllerWasInNormalMenu )
		{
			// When entering normal menus, discard held native gameplay state.
			// Menu input is handled as short taps below, so stale press/release pairs
			// cannot make the next menu button require a second physical press.
			UE1AndroidNativeControllerResetState(); // ANDROID_CONTROLLER_NATIVE_MENU_TAP_V93
			appMemset( JoyAxis, 0, sizeof(JoyAxis) );
		}
		if( bAndroidNativeKeyMenuing && !GAndroidNativeControllerWasInKeyMenuing )
		{
			// Entering Customize Controls: the DPad/stick/confirm input used to select
			// the row can still be present in Android's motion stream. Do not let that
			// stale navigation input become the new binding.
			UE1AndroidNativeControllerResetState(); // ANDROID_CONTROLLER_KEYMENUING_DEDUP_V93
			appMemset( JoyAxis, 0, sizeof(JoyAxis) );
			GAndroidNativeKeyMenuingAxisArmed = 1;
			GAndroidNativeKeyMenuingCaptureDone = 0;
			GAndroidNativeKeyMenuingIgnoreMotionUntil = CurTime + 0.12f;
		}
		if( !bAndroidNativeKeyMenuing )
		{
			GAndroidNativeKeyMenuingAxisArmed = 1;
			GAndroidNativeKeyMenuingCaptureDone = 0;
			GAndroidNativeKeyMenuingIgnoreMotionUntil = 0.0f;
		}
		GAndroidNativeControllerWasInNormalMenu = bAndroidNativeNormalMenu;
		GAndroidNativeControllerWasInKeyMenuing = bAndroidNativeKeyMenuing;
	}
	else
	{
		GAndroidNativeControllerWasInNormalMenu = 0;
		GAndroidNativeControllerWasInKeyMenuing = 0;
		GAndroidNativeKeyMenuingAxisArmed = 1;
		GAndroidNativeKeyMenuingCaptureDone = 0;
		GAndroidNativeKeyMenuingIgnoreMotionUntil = 0.0f;
	}
#endif

	while( SDL_PollEvent( &Ev ) )
	{
		switch( Ev.type )
		{
			case SDL_QUIT:
#if PLATFORM_ANDROID
				if( UE1AndroidShouldIgnoreEarlyQuit() )
				{
					debugf( NAME_Warning, "Ignoring early SDL_QUIT during Android startup guard" );
					break;
				}
#endif
				// signal to client and remember set a flag just in case
				QuitRequested = true;
				return true;
			case SDL_TEXTINPUT:
#if PLATFORM_ANDROID
				AndroidKeepSoftKeyboardAlive( this, 90.0f );
#endif
				for( const char *p = Ev.text.text; *p && p < Ev.text.text + sizeof( Ev.text.text ); ++p )
				{
					if( *p < 0 )
						break;
					if( isprint( *p ) || *p == '\r' )
						Client->Engine->Key( this, (EInputKey)*p );
				}
				break;
			case SDL_KEYDOWN:
#if PLATFORM_ANDROID
				if( Ev.key.keysym.sym == SDLK_RETURN || Ev.key.keysym.sym == SDLK_ESCAPE )
				{
					AndroidHideSoftKeyboard();
				}
#endif
				if( Ev.key.keysym.sym == SDLK_RETURN && (Ev.key.keysym.mod & KMOD_ALT) )
				{
					Exec("ToggleFullscreen", this);
					break;
				}
			case SDL_KEYUP:
				CauseInputEvent( KeyMap[Ev.key.keysym.scancode], ( Ev.type == SDL_KEYDOWN ) ? IST_Press : IST_Release );
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
#if PLATFORM_ANDROID
				if( ( Ev.type == SDL_MOUSEBUTTONDOWN || Ev.type == SDL_MOUSEBUTTONUP ) && AndroidViewportIsMenuing( this ) )
				{
					// v4.2 selective: disabled broad keyboard trigger from pointer/button event.
					// original: AndroidRequestSoftKeyboard( this, 90.0f );
				}
#endif
				CauseInputEvent( MouseButtonMap[Ev.button.button], ( Ev.type == SDL_MOUSEBUTTONDOWN ) ? IST_Press : IST_Release );
				break;
			case SDL_MOUSEWHEEL:
				if( Ev.wheel.y )
				{
					CauseInputEvent( IK_MouseW, IST_Axis, Ev.wheel.y );
					if( Ev.wheel.y < 0 )
					{
						CauseInputEvent( IK_MouseWheelDown, IST_Press );
						CauseInputEvent( IK_MouseWheelDown, IST_Release );
					}
					else if( Ev.wheel.y > 0 )
					{
						CauseInputEvent( IK_MouseWheelUp, IST_Press );
						CauseInputEvent( IK_MouseWheelUp, IST_Release );
					}
				}
				break;
			case SDL_CONTROLLERBUTTONDOWN:
			case SDL_CONTROLLERBUTTONUP:
				{
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
					if( bAndroidNativeController )
						break; // ANDROID_NATIVE_CONTROLLER_BACKEND_V93 ANDROID_NATIVE_CONTROLLER_LINEAR_AXIS_RAMP_V97: Java path owns controller buttons
#endif
					// HACK: Swap to alternate bindings when in menus, but not when waiting for keypress in the keybind menu.
					const UBOOL bIsInUI = Console &&
						((UObject*)Console)->GetMainFrame() &&
						((UObject*)Console)->GetMainFrame()->StateNode &&
						((UObject*)Console)->GetMainFrame()->StateNode->GetFName() == "Menuing";
					const BYTE* JoyMap = bIsInUI ? JoyButtonMapUI : JoyButtonMap;
#if PLATFORM_ANDROID
					if( bIsInUI && Ev.type == SDL_CONTROLLERBUTTONDOWN && Ev.cbutton.button == SDL_CONTROLLER_BUTTON_A )
					{
						// v4.2 selective: disabled broad keyboard trigger from pointer/button event.
						// original: AndroidRequestSoftKeyboard( this, 90.0f );
					}
#endif
					CauseInputEvent( JoyMap[Ev.cbutton.button], ( Ev.type == SDL_CONTROLLERBUTTONDOWN ) ? IST_Press : IST_Release );
				}
				break;
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				{
					if( bAndroidNativeController )
						break; // ANDROID_NATIVE_CONTROLLER_BACKEND_V93 ANDROID_NATIVE_CONTROLLER_LINEAR_AXIS_RAMP_V97

					// Raw joystick events are only a fallback for devices that SDL does not
					// expose as GameController. If a GameController is active, SDL may emit
					// both controller and joystick events for the same physical button. Feeding
					// both into UE1 makes menus skip entries and confirms one level too deep.
					if( Client && Client->GetController() )
						break;

					const UBOOL bIsInUI = Console &&
						((UObject*)Console)->GetMainFrame() &&
						((UObject*)Console)->GetMainFrame()->StateNode &&
						((UObject*)Console)->GetMainFrame()->StateNode->GetFName() == "Menuing";
					const BYTE* JoyMap = bIsInUI ? JoyButtonMapUI : JoyButtonMap;
					BYTE Key = IK_None;

					if( Ev.jbutton.button < SDL_CONTROLLER_BUTTON_MAX )
					{
						Key = JoyMap[Ev.jbutton.button];
					}
					else
					{
						switch( Ev.jbutton.button )
						{
							case 6:
							case 7:
								Key = IK_Escape;
								break;
							case 4:
							case 8:
								Key = bIsInUI ? IK_Escape : IK_Joy5;
								break;
							default:
								Key = IK_None;
								break;
						}
					}

					if( Key != IK_None )
						CauseInputEvent( Key, ( Ev.type == SDL_JOYBUTTONDOWN ) ? IST_Press : IST_Release );
				}
				break;

			case SDL_JOYHATMOTION:
				if( bAndroidNativeController )
					break; // ANDROID_NATIVE_CONTROLLER_BACKEND_V93 ANDROID_NATIVE_CONTROLLER_LINEAR_AXIS_RAMP_V97

				// Same duplicate-event guard as above. D-Pad from GameController is handled
				// via SDL_CONTROLLERBUTTONDOWN/UP and mapped to IK_Up/IK_Down in UI.
				if( Client && Client->GetController() )
					break;

				CauseInputEvent( IK_JoyPovUp,    ( Ev.jhat.value & SDL_HAT_UP    ) ? IST_Press : IST_Release );
				CauseInputEvent( IK_JoyPovDown,  ( Ev.jhat.value & SDL_HAT_DOWN  ) ? IST_Press : IST_Release );
				CauseInputEvent( IK_JoyPovLeft,  ( Ev.jhat.value & SDL_HAT_LEFT  ) ? IST_Press : IST_Release );
				CauseInputEvent( IK_JoyPovRight, ( Ev.jhat.value & SDL_HAT_RIGHT ) ? IST_Press : IST_Release );
				break;
#endif
			case SDL_CONTROLLERAXISMOTION:
				{
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
					if( bAndroidNativeController )
						break; // ANDROID_NATIVE_CONTROLLER_BACKEND_V93 ANDROID_NATIVE_CONTROLLER_LINEAR_AXIS_RAMP_V97: Java MotionEvent path owns axes
#endif
					const BYTE Key = JoyAxisMap[Ev.caxis.axis];
					const INT PrevValue = JoyAxis[Ev.caxis.axis];
					INT NewValue = Ev.caxis.value;
					INT DeadZone = 0;
					if ( Key < IK_JoyX
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_MOUSELOOK_AXIS_DEADZONE
						&& Key != IK_MouseX && Key != IK_MouseY
#endif
					)
					{
						// Treat the axis like a trigger.
						if ( PrevValue < JoyAxisPressThreshold && NewValue >= JoyAxisPressThreshold )
							CauseInputEvent( Key, IST_Press );
						else if ( PrevValue >= JoyAxisPressThreshold && NewValue < JoyAxisPressThreshold )
							CauseInputEvent( Key, IST_Release );
					}
					else
					{
						// Apply deadzone.
						if ( Key >= IK_JoyX && Key <= IK_JoyZ )
							DeadZone = Client->DeadZoneXYZ * 32767.f;
						else if ( Key == IK_JoyR || Key == IK_JoyU || Key == IK_JoyV
#ifdef PLATFORM_ANDROID // UNREAL_ANDROID_MOUSELOOK_AXIS_DEADZONE_RUV
							|| Key == IK_MouseX || Key == IK_MouseY
#endif
						)
							DeadZone = Client->DeadZoneRUV * 32767.f;
						if ( Abs(NewValue) < DeadZone )
							NewValue = 0;
					}
					JoyAxis[Ev.caxis.axis] = NewValue;
				}
				break;
			case SDL_MOUSEMOTION:
				if( !Client->FullscreenViewport && !SDL_GetRelativeMouseMode() )
				{
					// If cursor isn't captured, just do MousePosition.
					Client->Engine->MousePosition( this, 0, Ev.motion.x, Ev.motion.y );
				}
				else
				{
					DWORD ViewportButtonFlags = 0;
					if( Ev.motion.state & SDL_BUTTON_LMASK ) ViewportButtonFlags |= MOUSE_Left;
					if( Ev.motion.state & SDL_BUTTON_RMASK ) ViewportButtonFlags |= MOUSE_Right;
					if( Ev.motion.state & SDL_BUTTON_MMASK ) ViewportButtonFlags |= MOUSE_Middle;
					if( Ev.motion.xrel || Ev.motion.yrel )
					{
						Client->Engine->MouseDelta( this, ViewportButtonFlags, Ev.motion.xrel, -Ev.motion.yrel );
						if( Ev.motion.xrel ) CauseInputEvent( IK_MouseX, IST_Axis, Ev.motion.xrel );
						if( Ev.motion.yrel ) CauseInputEvent( IK_MouseY, IST_Axis, -Ev.motion.yrel );
					}
				}
				break;
			default:
				break;
		}
	}

#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	if( bAndroidNativeController )
	{
		FAndroidNativeControllerEvent NativeEvents[128];
		const INT NativeCount = UE1AndroidNativeControllerDrainEvents( NativeEvents, ARRAY_COUNT(NativeEvents) );
		const UBOOL bIsInUI = Console &&
			((UObject*)Console)->GetMainFrame() &&
			((UObject*)Console)->GetMainFrame()->StateNode &&
			((UObject*)Console)->GetMainFrame()->StateNode->GetFName() == "Menuing";
		const UBOOL bIsKeyMenuing = Console &&
			((UObject*)Console)->GetMainFrame() &&
			((UObject*)Console)->GetMainFrame()->StateNode &&
			((UObject*)Console)->GetMainFrame()->StateNode->GetFName() == "KeyMenuing";

		for( INT EventIndex=0; EventIndex<NativeCount; ++EventIndex )
		{
			const FAndroidNativeControllerEvent& NE = NativeEvents[EventIndex];
			if( NE.Type == 1 )
			{
				const BYTE Key = UE1AndroidNativeKeyCodeToUE1Key( NE.KeyCode, bIsInUI );
				if( Key != IK_None && Key > 0 && Key < IK_MAX )
				{
					const UBOOL bPressed = NE.Action == 0;

					if( bIsKeyMenuing )
					{
						// Customize Controls must only accept one explicit physical DOWN.
						// Release/repeat/stale confirm events are swallowed here so they cannot
						// become ghost bindings after reopening the menu.
						// ANDROID_CONTROLLER_KEYMENUING_DEDUP_V93
						if( bPressed && NE.RepeatCount == 0 && !GAndroidNativeKeyMenuingCaptureDone && CurTime >= GAndroidNativeKeyMenuingIgnoreMotionUntil )
						{
							if( UE1AndroidCleanDispatchKeyCaptureV86( this, Key ) )
							{
								GAndroidNativeKeyMenuingCaptureDone = 1;
								UE1AndroidNativeControllerResetState();
								appMemset( JoyAxis, 0, sizeof(JoyAxis) );
							}
						}
						continue;
					}

					if( bIsInUI )
					{
						// Normal UE1 menus are more reliable with discrete tap events.
						// The stateful press/release path is still used for gameplay, but
						// Menuing gets an immediate press+release.
						// ANDROID_CONTROLLER_NATIVE_MENU_TAP_V93
						GAndroidNativeButtonPressed[Key] = 0;
						if( bPressed )
						{
							CauseInputEvent( Key, IST_Press );
							CauseInputEvent( Key, IST_Release );
						}
						continue;
					}

					// Ignore Android key repeats outside menus; UE1 holds movement/action state until release.
					if( bPressed && NE.RepeatCount > 0 )
						continue;
					if( GAndroidNativeButtonPressed[Key] != bPressed )
					{
						GAndroidNativeButtonPressed[Key] = bPressed;
						CauseInputEvent( Key, bPressed ? IST_Press : IST_Release );
					}
				}
			}
			else if( NE.Type == 2 )
			{
				if( bIsKeyMenuing )
				{
					// In Customize Controls, do not feed analog/DPad motion through the
					// normal press/release path. Capture exactly one explicit threshold
					// crossing and swallow everything else. This fixes both ghost bindings
					// and the previous "press twice" behaviour.
					// ANDROID_CONTROLLER_KEYMENUING_DEDUP_V93
					if( CurTime < GAndroidNativeKeyMenuingIgnoreMotionUntil || GAndroidNativeKeyMenuingCaptureDone )
						continue;

					BYTE CaptureKey = IK_None;
					const FLOAT CaptureThreshold = 0.55f;
					const FLOAT NativeTriggerDeadzone = Client ? Clamp( Client->AndroidNativeTriggerDeadzone, 0.0f, 0.85f ) : 0.12f;
					const SWORD TriggerLeft = UE1AndroidNativeTriggerToAxis( NE.AxisLTrigger, NE.AxisBrake, NativeTriggerDeadzone );
					const SWORD TriggerRight = UE1AndroidNativeTriggerToAxis( NE.AxisRTrigger, NE.AxisGas, NativeTriggerDeadzone );

					if( TriggerLeft >= JoyAxisPressThreshold )
						CaptureKey = IK_Joy12;
					else if( TriggerRight >= JoyAxisPressThreshold )
						CaptureKey = IK_Joy13;
					else
					{
						struct FAxisCapturePair { INT Axis; FLOAT Value; };
						const FAxisCapturePair AxisPairs[4] =
						{
							{ SDL_CONTROLLER_AXIS_LEFTX,  NE.AxisX  },
							{ SDL_CONTROLLER_AXIS_LEFTY,  NE.AxisY  },
							{ SDL_CONTROLLER_AXIS_RIGHTX, NE.AxisZ  },
							{ SDL_CONTROLLER_AXIS_RIGHTY, NE.AxisRZ }
						};
						for( INT AxisIndex=0; AxisIndex<4 && CaptureKey == IK_None; ++AxisIndex )
						{
							if( AxisPairs[AxisIndex].Value <= -CaptureThreshold )
								CaptureKey = UE1AndroidNativeDirectionalAxisKey( AxisPairs[AxisIndex].Axis, -1 );
							else if( AxisPairs[AxisIndex].Value >= CaptureThreshold )
								CaptureKey = UE1AndroidNativeDirectionalAxisKey( AxisPairs[AxisIndex].Axis, +1 );
						}
					}

					if( CaptureKey == IK_None )
					{
						if( NE.AxisHatX <= -0.50f )
							CaptureKey = IK_JoyPovLeft;
						else if( NE.AxisHatX >= 0.50f )
							CaptureKey = IK_JoyPovRight;
						else if( NE.AxisHatY <= -0.50f )
							CaptureKey = IK_JoyPovUp;
						else if( NE.AxisHatY >= 0.50f )
							CaptureKey = IK_JoyPovDown;
					}

					if( CaptureKey != IK_None )
					{
						if( UE1AndroidCleanDispatchKeyCaptureV86( this, CaptureKey ) )
						{
							GAndroidNativeKeyMenuingCaptureDone = 1;
							UE1AndroidNativeControllerResetState();
							appMemset( JoyAxis, 0, sizeof(JoyAxis) );
						}
					}
					continue;
				}

				if( bIsInUI )
				{
					// In normal menus, treat Android HAT/DPad motion as discrete taps.
					// Do not update gameplay JoyAxis values while browsing menus.
					const FLOAT MenuHatValues[2] = { NE.AxisHatX, NE.AxisHatY };
					for( INT HatAxis=0; HatAxis<2; ++HatAxis )
					{
						for( INT DirIndex=0; DirIndex<2; ++DirIndex )
						{
							const INT Direction = DirIndex == 0 ? -1 : 1;
							const FLOAT DirValue = Direction < 0 ? -MenuHatValues[HatAxis] : MenuHatValues[HatAxis];
							const UBOOL bWasPressed = GAndroidNativeHatPressed[HatAxis][DirIndex];
							const UBOOL bNowPressed = bWasPressed ? ( DirValue >= 0.25f ) : ( DirValue >= 0.50f );
							if( bWasPressed != bNowPressed )
							{
								GAndroidNativeHatPressed[HatAxis][DirIndex] = bNowPressed;
								if( bNowPressed )
								{
									BYTE HatKey = IK_None;
									if( HatAxis == 0 )
										HatKey = Direction < 0 ? IK_Left : IK_Right;
									else
										HatKey = Direction < 0 ? IK_Up : IK_Down;
									if( HatKey != IK_None )
									{
										CauseInputEvent( HatKey, IST_Press );
										CauseInputEvent( HatKey, IST_Release );
									}
								}
							}
						}
					}
					continue;
				}

				SWORD NativeAxisValues[SDL_CONTROLLER_AXIS_MAX];
				FLOAT NativeLeftDeadzone = Client ? Clamp( Client->AndroidNativeLeftStickDeadzone, 0.0f, 0.85f ) : 0.06f;
				// ANDROID_NATIVE_CONTROLLER_LEFT_STICK_SMOOTHER_LINEAR_V100
				// Keep the left stick genuinely linear for walking/strafing, but remove
				// most of the deadzone step that made the first movement feel uneven.
				// Existing v97/v98/v99 installs may still have 0.10 or 0.14 saved, so
				// treat those exact defaults as the new smoother 0.06 default while
				// preserving deliberate custom values.
				if( ( NativeLeftDeadzone > 0.099f && NativeLeftDeadzone < 0.101f ) ||
					( NativeLeftDeadzone > 0.139f && NativeLeftDeadzone < 0.141f ) )
					NativeLeftDeadzone = 0.06f;
				const FLOAT NativeRightDeadzone = Client ? Clamp( Client->AndroidNativeRightStickDeadzone, 0.0f, 0.85f ) : 0.14f;
				const FLOAT NativeTriggerDeadzone = Client ? Clamp( Client->AndroidNativeTriggerDeadzone, 0.0f, 0.85f ) : 0.12f;
				const FLOAT NativeAxisCurve = Client ? Clamp( Client->AndroidNativeAxisCurve, 0.50f, 5.00f ) : 1.00f; // ANDROID_NATIVE_CONTROLLER_LINEAR_AXIS_RAMP_V97
				NativeAxisValues[SDL_CONTROLLER_AXIS_LEFTX] = UE1AndroidNativeFloatToAxis( NE.AxisX, NativeLeftDeadzone, NativeAxisCurve );
				NativeAxisValues[SDL_CONTROLLER_AXIS_LEFTY] = UE1AndroidNativeFloatToAxis( NE.AxisY, NativeLeftDeadzone, NativeAxisCurve );
				NativeAxisValues[SDL_CONTROLLER_AXIS_RIGHTX] = UE1AndroidNativeFloatToAxis( NE.AxisZ, NativeRightDeadzone, NativeAxisCurve );
				NativeAxisValues[SDL_CONTROLLER_AXIS_RIGHTY] = UE1AndroidNativeFloatToAxis( NE.AxisRZ, NativeRightDeadzone, NativeAxisCurve );
				NativeAxisValues[SDL_CONTROLLER_AXIS_TRIGGERLEFT] = UE1AndroidNativeTriggerToAxis( NE.AxisLTrigger, NE.AxisBrake, NativeTriggerDeadzone );
				NativeAxisValues[SDL_CONTROLLER_AXIS_TRIGGERRIGHT] = UE1AndroidNativeTriggerToAxis( NE.AxisRTrigger, NE.AxisGas, NativeTriggerDeadzone );

				for( INT Axis=0; Axis<SDL_CONTROLLER_AXIS_MAX; ++Axis )
				{
					const BYTE Key = JoyAxisMap[Axis];
					const INT PrevValue = JoyAxis[Axis];
					INT NewValue = NativeAxisValues[Axis];
					INT DeadZone = 0;

					if( Axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT || Axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT || Key < IK_JoyX )
					{
						if( PrevValue < JoyAxisPressThreshold && NewValue >= JoyAxisPressThreshold )
							CauseInputEvent( Key, IST_Press );
						else if( PrevValue >= JoyAxisPressThreshold && NewValue < JoyAxisPressThreshold )
							CauseInputEvent( Key, IST_Release );
					}
					else
					{
						if( Key >= IK_JoyX && Key <= IK_JoyZ )
							DeadZone = Client->DeadZoneXYZ * 32767.f;
						else if( Key == IK_JoyR || Key == IK_JoyU || Key == IK_JoyV || Key == IK_MouseX || Key == IK_MouseY )
							DeadZone = Client->DeadZoneRUV * 32767.f;
						if( Abs(NewValue) < DeadZone )
							NewValue = 0;
					}

					if( Axis == SDL_CONTROLLER_AXIS_LEFTX || Axis == SDL_CONTROLLER_AXIS_LEFTY || Axis == SDL_CONTROLLER_AXIS_RIGHTX || Axis == SDL_CONTROLLER_AXIS_RIGHTY )
					{
						for( INT DirIndex=0; DirIndex<2; ++DirIndex )
						{
							const INT Direction = DirIndex == 0 ? -1 : 1;
							const BYTE DirKey = UE1AndroidNativeDirectionalAxisKey( Axis, Direction );
							if( DirKey == IK_None )
								continue;
							const INT DirValue = Direction < 0 ? -NewValue : NewValue;
							const char* DirBinding = ( Input && DirKey > 0 && DirKey < IK_MAX && Input->Bindings[DirKey].Length() ) ? *Input->Bindings[DirKey] : NULL;
							const UBOOL bAnalogAxisAlias = UE1AndroidNativeBindingIsAnalogAxisAliasV96( DirBinding );
							const UBOOL bWasPressed = GAndroidNativeAxisDirPressed[Axis][DirIndex];
							if( bAnalogAxisAlias )
							{
								// ANDROID_NATIVE_CONTROLLER_TRUE_PROGRESSIVE_AXIS_V96
								// Movement/look aliases are fed as analogue axis events in the per-frame
								// loop below.  Do not create a held digital key here, because that is what
								// made a tiny stick tilt turn/run at full keyboard speed.
								if( bWasPressed )
								{
									GAndroidNativeAxisDirPressed[Axis][DirIndex] = 0;
									CauseInputEvent( DirKey, IST_Release );
								}
								continue;
							}
							const UBOOL bNowPressed = bWasPressed ? ( DirValue >= GAndroidNativeAxisReleaseThreshold ) : ( DirValue >= JoyAxisPressThreshold );
							if( bWasPressed != bNowPressed )
							{
								GAndroidNativeAxisDirPressed[Axis][DirIndex] = bNowPressed;
								CauseInputEvent( DirKey, bNowPressed ? IST_Press : IST_Release );
							}
						}
					}

					JoyAxis[Axis] = (SWORD)NewValue;
				}

				const FLOAT HatValues[2] = { NE.AxisHatX, NE.AxisHatY };
				for( INT HatAxis=0; HatAxis<2; ++HatAxis )
				{
					for( INT DirIndex=0; DirIndex<2; ++DirIndex )
					{
						const INT Direction = DirIndex == 0 ? -1 : 1;
						const FLOAT DirValue = Direction < 0 ? -HatValues[HatAxis] : HatValues[HatAxis];
						const UBOOL bWasPressed = GAndroidNativeHatPressed[HatAxis][DirIndex];
						const UBOOL bNowPressed = bWasPressed ? ( DirValue >= 0.25f ) : ( DirValue >= 0.50f );
						if( bWasPressed != bNowPressed )
						{
							GAndroidNativeHatPressed[HatAxis][DirIndex] = bNowPressed;
							BYTE HatKey = IK_None;
							if( HatAxis == 0 )
								HatKey = Direction < 0 ? ( bIsInUI ? IK_Left : IK_JoyPovLeft ) : ( bIsInUI ? IK_Right : IK_JoyPovRight );
							else
								HatKey = Direction < 0 ? ( bIsInUI ? IK_Up : IK_JoyPovUp ) : ( bIsInUI ? IK_Down : IK_JoyPovDown );
							if( HatKey != IK_None )
								CauseInputEvent( HatKey, bNowPressed ? IST_Press : IST_Release );
						}
					}
				}
			}
		}
	}
#endif

	// Constantly hammer the input system with axis events for axes that are not zero.
	for ( INT i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i )
	{
		const BYTE Key = JoyAxisMap[i];
		const SWORD Value = JoyAxis[i];
		if ( Value && Key && Key >= IK_JoyX )
		{
			const FLOAT FltValue = Clamp( Value / 32767.f, -1.f, 1.f );
			FLOAT AxisOutputValue = FltValue;
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
			if( bAndroidNativeController && i == SDL_CONTROLLER_AXIS_LEFTY )
			{
				// ANDROID_NATIVE_CONTROLLER_LEFTY_DIRECTION_SENSITIVITY_V99
				// Keep the Android/SDL left-Y sign for the real analogue JoyY path.
				// v98 inverted this output and made forward/backward reversed on the
				// tested controller.  Directional LJoyUp/LJoyDown capture still uses
				// the raw stored JoyAxis value above, so no extra capture inversion is
				// needed here.
			}
#endif
			FLOAT Scale = ( Key >= IK_JoyX && Key <= IK_JoyZ ) ? Client->ScaleXYZ : Client->ScaleRUV;
			Scale *= JoyAxisDefaultScale[i] * DeltaTime;
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
			if( bAndroidNativeController && ( i == SDL_CONTROLLER_AXIS_LEFTX || i == SDL_CONTROLLER_AXIS_LEFTY ) )
			{
				// ANDROID_NATIVE_CONTROLLER_LEFT_STICK_SMOOTHER_LINEAR_V100
				// Still a straight linear ramp, just slightly more responsive and with a
				// much smaller deadzone above.  Right stick/mouselook is deliberately not
				// touched because v97+ right-stick behaviour tested well.
				Scale *= 1.35f;
			}
			if( bAndroidNativeController && ( i == SDL_CONTROLLER_AXIS_RIGHTX || i == SDL_CONTROLLER_AXIS_RIGHTY ) )
			{
				// ANDROID_NATIVE_CONTROLLER_LINEAR_AXIS_RAMP_V97
				// Keep the already-deadzoned axis linear here.  MouseSensitivity should
				// scale the whole curve, not turn tiny stick movement into instant max speed.
				const FLOAT MouseSensitivity = Actor ? Actor->MouseSensitivity : 3.0f;
				const FLOAT MouseFactor = Clamp( MouseSensitivity / 4.0f, 0.20f, 3.00f );
				const FLOAT NativeRightStickScale = Client ? Clamp( Client->AndroidNativeRightStickScale, 0.05f, 2.0f ) : 0.50f;
				Scale *= NativeRightStickScale * MouseFactor;
			}

			if( bAndroidNativeController && ( i == SDL_CONTROLLER_AXIS_RIGHTX || i == SDL_CONTROLLER_AXIS_RIGHTY ) )
			{
				UBOOL bSentDirectionalAxis = 0;
				for( INT DirIndex=0; DirIndex<2; ++DirIndex )
				{
					const INT Direction = DirIndex == 0 ? -1 : 1;
					const BYTE DirKey = UE1AndroidNativeDirectionalAxisKey( i, Direction );
					if( DirKey == IK_None )
						continue;
					const char* DirBinding = ( Input && DirKey > 0 && DirKey < IK_MAX && Input->Bindings[DirKey].Length() ) ? *Input->Bindings[DirKey] : NULL;
					if( !UE1AndroidNativeBindingIsAnalogAxisAliasV96( DirBinding ) )
						continue;
					// ANDROID_NATIVE_CONTROLLER_LINEAR_AXIS_RAMP_V97
					// v98 uses the left stick through the real JoyX/JoyY analogue axis path.
					// This block is now right-stick only, keeping RJoyLeft/RJoyRight/RJoyUp/RJoyDown
					// analogue for mouselook-style bindings without turning them into full-speed keys.
					const FLOAT DirectionalAxisValue = AxisOutputValue;
					const FLOAT DirMagnitude = Direction < 0 ? Max( -DirectionalAxisValue, 0.0f ) : Max( DirectionalAxisValue, 0.0f );
					if( DirMagnitude <= 0.0001f )
						continue;
					// Feed movement/look aliases with a linear analogue delta.  This keeps the
					// configured friendly keys analogue, but avoids v96's overly slow multiplier.
					const FLOAT DirectionalScale = Scale * 0.16f;
					CauseInputEvent( DirKey, IST_Axis, DirMagnitude * DirectionalScale );
					bSentDirectionalAxis = 1;
				}
				if( bSentDirectionalAxis )
					continue;
			}
#endif
			if ( ( Client->InvertV && Key == IK_JoyV ) || ( Client->InvertY && Key == IK_JoyY ) )
				Scale = -Scale;
			CauseInputEvent( Key, IST_Axis, AxisOutputValue * Scale );
		}
	}

	InputUpdateTime = CurTime;

	return QuitRequested;

	unguard;
}

#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
// Clean v83: cosmetic controller names only. This does not change SDL event
// handling, axis speed, default bindings, or controller backend logic.
static INT GAndroidCleanLastKeyNameQueryKey = -1;
static INT GAndroidCleanPendingCapturedKeyV93 = -1; // ANDROID_CONTROLLER_KEYMENUING_DEDUP_V93
static UBOOL GAndroidCleanFriendlyAliasScrubDoneV93 = 0; // ANDROID_CONTROLLER_KEYMENUING_DEDUP_V93

static const char* UE1AndroidCleanFriendlyKeyName( INT Key )
{
	switch( Key )
	{
		case IK_Joy1:        return "A";
		case IK_Joy2:        return "B";
		case IK_Joy3:        return "X";
		case IK_Joy4:        return "Y";
		case IK_Joy8:        return "LJoyPush";
		case IK_Joy9:        return "RJoyPush";
		case IK_Joy10:       return "ShoulderL";
		case IK_Joy11:       return "ShoulderR";
		case IK_Joy12:       return "TriggerL"; // 1.2.0 internal trigger key
		case IK_Joy13:       return "TriggerR"; // 1.2.0 internal trigger key
		case IK_UnknownD8:   return "LJoyLeft";
		case IK_UnknownD9:   return "LJoyRight";
		case IK_UnknownDA:   return "LJoyUp";
		case IK_UnknownDF:   return "LJoyDown";
		case IK_Joy14:       return "RJoyLeft";
		case IK_Joy15:       return "RJoyRight";
		case IK_Joy16:       return "RJoyUp";
		case IK_UnknownEA:   return "RJoyDown";
		case IK_JoyPovUp:    return "DPadUp";
		case IK_JoyPovDown:  return "DPadDown";
		case IK_JoyPovLeft:  return "DPadLeft";
		case IK_JoyPovRight: return "DPadRight";
	}
	return NULL;
}

static INT UE1AndroidCleanFriendlyKeyFromName( const char* KeyName )
{
	if( !KeyName || !*KeyName )
		return IK_None;

	struct FFriendlyKeyPair
	{
		const char* Name;
		INT Key;
	};

	static const FFriendlyKeyPair FriendlyKeys[] =
	{
		{ "A", IK_Joy1 }, { "B", IK_Joy2 }, { "X", IK_Joy3 }, { "Y", IK_Joy4 },
		{ "LJoyPush", IK_Joy8 }, { "RJoyPush", IK_Joy9 },
		{ "ShoulderL", IK_Joy10 }, { "ShoulderR", IK_Joy11 },
		{ "TriggerL", IK_Joy12 }, { "TriggerR", IK_Joy13 },
		{ "LJoyLeft", IK_UnknownD8 }, { "LJoyRight", IK_UnknownD9 },
		{ "LJoyUp", IK_UnknownDA }, { "LJoyDown", IK_UnknownDF },
		{ "RJoyLeft", IK_Joy14 }, { "RJoyRight", IK_Joy15 },
		{ "RJoyUp", IK_Joy16 }, { "RJoyDown", IK_UnknownEA },
		{ "DPadUp", IK_JoyPovUp }, { "DPadDown", IK_JoyPovDown },
		{ "DPadLeft", IK_JoyPovLeft }, { "DPadRight", IK_JoyPovRight },
	};

	for( INT i=0; i<(INT)(sizeof(FriendlyKeys)/sizeof(FriendlyKeys[0])); ++i )
		if( !appStricmp( KeyName, FriendlyKeys[i].Name ) )
			return FriendlyKeys[i].Key;

	return IK_None;
}

static UBOOL UE1AndroidCleanConsoleIsKeyMenuingV86( UNSDLViewport* Viewport )
{
	return Viewport && Viewport->Console &&
		((UObject*)Viewport->Console)->GetMainFrame() &&
		((UObject*)Viewport->Console)->GetMainFrame()->StateNode &&
		((UObject*)Viewport->Console)->GetMainFrame()->StateNode->GetFName() == "KeyMenuing";
}

static UBOOL UE1AndroidCleanKeyboardMenuSelectionAliasV86( AMenu* Menu, const char* WantedAlias )
{
	if( !Menu || !WantedAlias || !*WantedAlias || !UE1AndroidMenuIsExactClass( Menu, "UnrealKeyboardMenu" ) )
		return 0;

	UProperty* SelectionProperty  = FindField<UProperty>( Menu->GetClass(), "Selection" );
	UProperty* AliasNamesProperty = FindField<UProperty>( Menu->GetClass(), "AliasNames" );
	if( !SelectionProperty || !AliasNamesProperty )
		return 0;

	const INT Selection = *(INT*)((BYTE*)Menu + SelectionProperty->Offset);
	if( Selection <= 0 || Selection >= AliasNamesProperty->ArrayDim )
		return 0;

	const INT ElementSize = AliasNamesProperty->GetElementSize();
	if( ElementSize <= 1 )
		return 0;

	const char* Alias = (const char*)((BYTE*)Menu + AliasNamesProperty->Offset + Selection * ElementSize);
	return Alias && !appStricmp( Alias, WantedAlias );
}

static UBOOL UE1AndroidCleanKeyboardMenuSelectionAliasNameV92( AMenu* Menu, char* OutAlias, INT OutSize )
{
	if( OutAlias && OutSize > 0 )
		OutAlias[0] = 0;
	if( !Menu || !OutAlias || OutSize <= 0 || !UE1AndroidMenuIsExactClass( Menu, "UnrealKeyboardMenu" ) )
		return 0;

	UProperty* SelectionProperty  = FindField<UProperty>( Menu->GetClass(), "Selection" );
	UProperty* AliasNamesProperty = FindField<UProperty>( Menu->GetClass(), "AliasNames" );
	if( !SelectionProperty || !AliasNamesProperty )
		return 0;

	const INT Selection = *(INT*)((BYTE*)Menu + SelectionProperty->Offset);
	if( Selection <= 0 || Selection >= AliasNamesProperty->ArrayDim )
		return 0;

	const INT ElementSize = AliasNamesProperty->GetElementSize();
	if( ElementSize <= 1 )
		return 0;

	const char* Alias = (const char*)((BYTE*)Menu + AliasNamesProperty->Offset + Selection * ElementSize);
	if( !Alias || !*Alias )
		return 0;

	appStrncpy( OutAlias, Alias, OutSize );
	OutAlias[OutSize-1] = 0;
	return 1; // ANDROID_CONTROLLER_KEYMENUING_DEDUP_V93
}

static UBOOL UE1AndroidCleanResolveCapturedKeyNameV92( UInput* Input, const char* KeyName, INT& OutKey )
{
	OutKey = IK_None;
	if( !KeyName || !*KeyName || !Input )
		return 0;

	// The stock menu may pass display names like "A". On Android this is
	// ambiguous: keyboard A and controller A share the same text. Prefer the
	// key that was just physically captured, then fall back to friendly/native
	// lookup. This prevents controller A from being saved/displayed as keyboard A.
	if( GAndroidCleanPendingCapturedKeyV93 > 0 && GAndroidCleanPendingCapturedKeyV93 < IK_MAX )
	{
		const char* Friendly = UE1AndroidCleanFriendlyKeyName( GAndroidCleanPendingCapturedKeyV93 );
		const char* Native = Input->GetKeyName( (EInputKey)GAndroidCleanPendingCapturedKeyV93 );
		if( ( Friendly && !appStricmp( KeyName, Friendly ) ) || ( Native && !appStricmp( KeyName, Native ) ) )
		{
			OutKey = GAndroidCleanPendingCapturedKeyV93;
			return 1;
		}
	}

	const char* LastQueryName = UE1AndroidCleanFriendlyKeyName( GAndroidCleanLastKeyNameQueryKey );
	if( !LastQueryName && GAndroidCleanLastKeyNameQueryKey >= 0 && GAndroidCleanLastKeyNameQueryKey < IK_MAX )
		LastQueryName = Input->GetKeyName( (EInputKey)GAndroidCleanLastKeyNameQueryKey );

	if( LastQueryName && !appStricmp( KeyName, LastQueryName ) )
		OutKey = GAndroidCleanLastKeyNameQueryKey;
	if( OutKey == IK_None )
		OutKey = UE1AndroidCleanFriendlyKeyFromName( KeyName );
	if( OutKey == IK_None )
	{
		EInputKey NativeKey = IK_None;
		if( Input->FindKeyName( KeyName, NativeKey ) )
			OutKey = NativeKey;
	}

	return OutKey > 0 && OutKey < IK_MAX;
}

static UBOOL UE1AndroidCleanBindingStartsWithAliasV86( const char* Binding, const char* Alias )
{
	if( !Binding || !Alias || !*Alias )
		return 0;
	while( *Binding == ' ' )
		++Binding;
	const INT Len = appStrlen( Alias );
	if( appStrnicmp( Binding, Alias, Len ) )
		return 0;
	return Binding[Len] == 0 || Binding[Len] == ' ' || Binding[Len] == '|';
}

static INT UE1AndroidCleanClearOtherAliasBindingsV86( UInput* Input, INT KeepKey, const char* Alias )
{
	if( !Input || !Alias || !*Alias )
		return 0;

	INT Cleared = 0;
	for( INT Key=0; Key<IK_MAX; ++Key )
	{
		if( Key == KeepKey || Input->Bindings[Key].Length() == 0 )
			continue;
		if( UE1AndroidCleanBindingStartsWithAliasV86( *Input->Bindings[Key], Alias ) )
		{
			Input->Bindings[Key] = "";
			const char* NativeKeyName = Input->GetKeyName( (EInputKey)Key );
			if( NativeKeyName && *NativeKeyName )
				GConfigCache.SetString( "Engine.Input", NativeKeyName, "" );
			++Cleared;
		}
	}
	return Cleared;
}

static void UE1AndroidCleanKeyboardMenuWriteSelectionV86( AMenu* Menu, const char* First, const char* Second )
{
	if( !Menu || !UE1AndroidMenuIsExactClass( Menu, "UnrealKeyboardMenu" ) )
		return;

	UProperty* SelectionProperty   = FindField<UProperty>( Menu->GetClass(), "Selection" );
	UProperty* MenuValues1Property = FindField<UProperty>( Menu->GetClass(), "MenuValues1" );
	UProperty* MenuValues2Property = FindField<UProperty>( Menu->GetClass(), "MenuValues2" );
	if( !SelectionProperty || !MenuValues1Property || !MenuValues2Property )
		return;

	const INT Selection = *(INT*)((BYTE*)Menu + SelectionProperty->Offset);
	if( Selection <= 0 || Selection >= MenuValues1Property->ArrayDim || Selection >= MenuValues2Property->ArrayDim )
		return;

	const INT ElementSize1 = MenuValues1Property->GetElementSize();
	const INT ElementSize2 = MenuValues2Property->GetElementSize();
	if( ElementSize1 <= 1 || ElementSize2 <= 1 )
		return;

	char* Value1 = (char*)((BYTE*)Menu + MenuValues1Property->Offset + Selection * ElementSize1);
	char* Value2 = (char*)((BYTE*)Menu + MenuValues2Property->Offset + Selection * ElementSize2);
	appStrncpy( Value1, First ? First : "", ElementSize1 );
	Value1[ElementSize1-1] = 0;
	appStrncpy( Value2, Second ? Second : "", ElementSize2 );
	Value2[ElementSize2-1] = 0;
}

static void UE1AndroidCleanKeyDisplayNameV86( UInput* Input, INT Key, char* OutName, INT OutSize )
{
	if( OutName && OutSize > 0 )
		OutName[0] = 0;
	if( !Input || !OutName || OutSize <= 0 || Key <= 0 || Key >= IK_MAX )
		return;

	const char* Friendly = UE1AndroidCleanFriendlyKeyName( Key );
	const char* Native = Friendly ? Friendly : Input->GetKeyName( (EInputKey)Key );
	if( Native && *Native )
	{
		appStrncpy( OutName, Native, OutSize );
		OutName[OutSize-1] = 0;
	}
}

static void UE1AndroidCleanScrubFriendlyAliasDuplicatesV92( UInput* Input )
{
	guard(UE1AndroidCleanScrubFriendlyAliasDuplicatesV92);

	if( !Input || GAndroidCleanFriendlyAliasScrubDoneV93 )
		return;

	GAndroidCleanFriendlyAliasScrubDoneV93 = 1;

	UClass* MenuClass = ::FindObject<UClass>( ANY_PACKAGE, "UnrealKeyboardMenu" );
	if( !MenuClass )
		return;

	UProperty* AliasNamesProperty = FindField<UProperty>( MenuClass, "AliasNames" );
	if( !AliasNamesProperty )
		return;

	UObject* DefaultObject = MenuClass->GetDefaultObject();
	if( !DefaultObject )
		return;

	const INT ElementSize = AliasNamesProperty->GetElementSize();
	if( ElementSize <= 1 )
		return;

	INT ClearedTotal = 0;
	for( INT Row=1; Row<AliasNamesProperty->ArrayDim && Row<21; ++Row )
	{
		const char* Alias = (const char*)((BYTE*)DefaultObject + AliasNamesProperty->Offset + Row * ElementSize);
		if( !Alias || !*Alias )
			continue;

		UBOOL bHasFriendlyBinding = 0;
		for( INT Key=1; Key<IK_MAX; ++Key )
		{
			if( Input->Bindings[Key].Length() == 0 )
				continue;
			if( UE1AndroidCleanFriendlyKeyName( Key ) && UE1AndroidCleanBindingStartsWithAliasV86( *Input->Bindings[Key], Alias ) )
			{
				bHasFriendlyBinding = 1;
				break;
			}
		}

		if( !bHasFriendlyBinding )
			continue;

		for( INT Key=1; Key<IK_MAX; ++Key )
		{
			if( Input->Bindings[Key].Length() == 0 )
				continue;
			if( UE1AndroidCleanFriendlyKeyName( Key ) )
				continue;
			if( UE1AndroidCleanBindingStartsWithAliasV86( *Input->Bindings[Key], Alias ) )
			{
				Input->Bindings[Key] = "";
				const char* NativeKeyName = Input->GetKeyName( (EInputKey)Key );
				if( NativeKeyName && *NativeKeyName )
					GConfigCache.SetString( "Engine.Input", NativeKeyName, "" );
				++ClearedTotal;
			}
		}
	}

	if( ClearedTotal > 0 )
	{
		Input->SaveConfig();
		GConfigCache.SaveAllConfigs();
		debugf( NAME_Log, "ANDROID_CONTROLLER_KEYMENUING_DEDUP_V93 scrubbed %i legacy keyboard/default bindings", ClearedTotal );
	}

	unguard;
}

static UBOOL UE1AndroidCleanIsDefaultNextWeaponKeyV86( const char* KeyName )
{
	return KeyName && ( !appStricmp( KeyName, "GreyPlus" ) || !appStricmp( KeyName, "Slash" ) );
}

static void UE1AndroidCleanPatchKeyboardMenuNextWeaponOnObjectV86( UObject* Object, UInput* Input, UProperty* AliasNamesProperty, UProperty* MenuValues1Property, UProperty* MenuValues2Property )
{
	guard(UE1AndroidCleanPatchKeyboardMenuNextWeaponOnObjectV86);

	if( !Object || !Input || !AliasNamesProperty || !MenuValues1Property || !MenuValues2Property )
		return;

	// Row 20 is Next Weapon. The stock compiled Unreal.u may still not rebuild this
	// row from KEYBINDING, even though the binding itself works in-game. Mirror the
	// active [Engine.Input] binding into MenuValues1/2 at runtime, display-only.
	UE1AndroidSetFixedStringArrayElement( Object, AliasNamesProperty, 20, "NextWeapon" ); // CLEAN_NEXTWEAPON_READBACK_V86

	const char* CurrentNext1 = UE1AndroidGetFixedStringArrayElement( Object, MenuValues1Property, 20 );
	const char* CurrentNext2 = UE1AndroidGetFixedStringArrayElement( Object, MenuValues2Property, 20 );
	if( ( CurrentNext1 && !appStricmp( CurrentNext1, "_" ) )
	 || ( CurrentNext2 && !appStricmp( CurrentNext2, "_" ) ) )
		return;

	char PreferredKeys[2][64];
	char DefaultKeys[2][64];
	appMemset( PreferredKeys, 0, sizeof(PreferredKeys) );
	appMemset( DefaultKeys, 0, sizeof(DefaultKeys) );
	INT PreferredCount = 0;
	INT DefaultCount = 0;

	for( INT Key=0; Key<IK_MAX; ++Key )
	{
		if( Input->Bindings[Key].Length() == 0 )
			continue;
		if( !UE1AndroidCleanBindingStartsWithAliasV86( *Input->Bindings[Key], "NextWeapon" ) )
			continue;

		char KeyName[64];
		UE1AndroidCleanKeyDisplayNameV86( Input, Key, KeyName, ARRAY_COUNT(KeyName) );
		if( !KeyName[0] )
			continue;

		if( UE1AndroidCleanIsDefaultNextWeaponKeyV86( KeyName ) )
		{
			if( DefaultCount < 2 )
				appStrncpy( DefaultKeys[DefaultCount++], KeyName, ARRAY_COUNT(DefaultKeys[0]) );
		}
		else
		{
			if( PreferredCount < 2 )
				appStrncpy( PreferredKeys[PreferredCount++], KeyName, ARRAY_COUNT(PreferredKeys[0]) );
		}
	}

	const char* First = "";
	const char* Second = "";
	if( PreferredCount > 0 )
	{
		First = PreferredKeys[0];
		Second = PreferredCount > 1 ? PreferredKeys[1] : "";
	}
	else
	{
		First = DefaultCount > 0 ? DefaultKeys[0] : "";
		Second = DefaultCount > 1 ? DefaultKeys[1] : "";
	}

	UE1AndroidSetFixedStringArrayElement( Object, MenuValues1Property, 20, First );
	UE1AndroidSetFixedStringArrayElement( Object, MenuValues2Property, 20, Second );

	unguard;
}

static void UE1AndroidCleanPatchKeyboardMenuNextWeaponV86( UNSDLViewport* Viewport )
{
	guard(UE1AndroidCleanPatchKeyboardMenuNextWeaponV86);

	if( !Viewport || !Viewport->Input )
		return;

	UClass* MenuClass = ::FindObject<UClass>( ANY_PACKAGE, "UnrealKeyboardMenu" );
	if( !MenuClass )
		return;

	UProperty* AliasNamesProperty  = FindField<UProperty>( MenuClass, "AliasNames" );
	UProperty* MenuValues1Property = FindField<UProperty>( MenuClass, "MenuValues1" );
	UProperty* MenuValues2Property = FindField<UProperty>( MenuClass, "MenuValues2" );
	if( !AliasNamesProperty || !MenuValues1Property || !MenuValues2Property )
		return;

	UE1AndroidCleanPatchKeyboardMenuNextWeaponOnObjectV86( MenuClass->GetDefaultObject(), Viewport->Input, AliasNamesProperty, MenuValues1Property, MenuValues2Property );

	AMenu* ActiveMenu = UE1AndroidGetActiveMenu( Viewport );
	if( UE1AndroidMenuIsExactClass( ActiveMenu, "UnrealKeyboardMenu" ) )
		UE1AndroidCleanPatchKeyboardMenuNextWeaponOnObjectV86( ActiveMenu, Viewport->Input, AliasNamesProperty, MenuValues1Property, MenuValues2Property );

	unguard;
}


static void UE1AndroidCleanSaveCapturedAliasV92( UNSDLViewport* Viewport, AMenu* Menu, INT Key, const char* DisplayName, const char* Alias )
{
	if( !Viewport || !Viewport->Input || !Menu || Key <= 0 || Key >= IK_MAX || !Alias || !*Alias )
		return;
	if( !UE1AndroidMenuIsExactClass( Menu, "UnrealKeyboardMenu" ) )
		return;

	// Customize Controls should show only the keys the user actually assigned.
	// The original PC defaults (NumPad8, A, B, R, etc.) remain in [Engine.Input]
	// unless we clear other bindings for the same alias after a remap. Those old
	// bindings then appear as "ghost" entries and can steal controller buttons.
	const INT Cleared = UE1AndroidCleanClearOtherAliasBindingsV86( Viewport->Input, Key, Alias );
	Viewport->Input->Bindings[Key] = Alias;
	const char* NativeKeyName = Viewport->Input->GetKeyName( (EInputKey)Key );
	if( NativeKeyName && *NativeKeyName )
		GConfigCache.SetString( "Engine.Input", NativeKeyName, Alias );

	Viewport->Input->SaveConfig();
	GConfigCache.SaveAllConfigs();
	UE1AndroidCleanKeyboardMenuWriteSelectionV86( Menu, DisplayName, "" );
	UE1AndroidCleanPatchKeyboardMenuNextWeaponV86( Viewport );
	GAndroidCleanFriendlyAliasScrubDoneV93 = 0;
	debugf( NAME_Log, "ANDROID_CONTROLLER_KEYMENUING_DEDUP_V93 saved alias='%s' key=%i display='%s' cleared=%i", Alias, Key, DisplayName ? DisplayName : "", Cleared );
}

static void UE1AndroidCleanSaveCapturedNextWeaponV86( UNSDLViewport* Viewport, AMenu* Menu, INT Key, const char* DisplayName )
{
	if( !Viewport || !Viewport->Input || Key <= 0 || Key >= IK_MAX )
		return;
	if( !UE1AndroidCleanKeyboardMenuSelectionAliasV86( Menu, "NextWeapon" ) )
		return;

	const INT Cleared = UE1AndroidCleanClearOtherAliasBindingsV86( Viewport->Input, Key, "NextWeapon" );
	Viewport->Input->Bindings[Key] = "NextWeapon";
	const char* NativeKeyName = Viewport->Input->GetKeyName( (EInputKey)Key );
	if( NativeKeyName && *NativeKeyName )
		GConfigCache.SetString( "Engine.Input", NativeKeyName, "NextWeapon" );
	Viewport->Input->SaveConfig();
	GConfigCache.SaveAllConfigs();
	UE1AndroidCleanKeyboardMenuWriteSelectionV86( Menu, DisplayName, "" );
	UE1AndroidCleanPatchKeyboardMenuNextWeaponV86( Viewport );
	debugf( NAME_Log, "CLEAN_NEXTWEAPON_ONLY_V86 saved key=%i display='%s' cleared=%i", Key, DisplayName ? DisplayName : "", Cleared );
}

static UBOOL UE1AndroidCleanDispatchKeyCaptureV86( UNSDLViewport* Viewport, INT Key )
{
	guard(UE1AndroidCleanDispatchKeyCaptureV86);

	if( !Viewport || !Viewport->Input || Key <= 0 || Key >= IK_MAX || !UE1AndroidCleanConsoleIsKeyMenuingV86( Viewport ) )
		return 0;

	AMenu* Menu = UE1AndroidGetActiveMenu( Viewport );
	if( !Menu )
		return 0;

	char SelectionAliasV92[64];
	appMemset( SelectionAliasV92, 0, sizeof(SelectionAliasV92) );
	UE1AndroidCleanKeyboardMenuSelectionAliasNameV92( Menu, SelectionAliasV92, ARRAY_COUNT(SelectionAliasV92) );

	const char* FriendlyName = UE1AndroidCleanFriendlyKeyName( Key );
	const UBOOL bIsNextWeaponCapture = SelectionAliasV92[0] && !appStricmp( SelectionAliasV92, "NextWeapon" );
	if( !FriendlyName && !bIsNextWeaponCapture )
		return 0;

	const char* DisplayName = FriendlyName ? FriendlyName : Viewport->Input->GetKeyName( (EInputKey)Key );
	if( !DisplayName || !*DisplayName )
		return 0;

	FName FunctionName( "ProcessMenuKey", FNAME_Find );
	if( FunctionName == NAME_None )
		return 0;
	UFunction* Function = Menu->FindFunction( FunctionName );
	if( !Function )
		return 0;

	struct FProcessMenuKeyParms
	{
		INT KeyNo;
		CHAR KeyName[32];
	} Parms;
	appMemset( &Parms, 0, sizeof(Parms) );
	Parms.KeyNo = Key;
	appStrncpy( Parms.KeyName, DisplayName, ARRAY_COUNT(Parms.KeyName) );
	Parms.KeyName[ARRAY_COUNT(Parms.KeyName)-1] = 0;

	GAndroidCleanLastKeyNameQueryKey = Key;
	GAndroidCleanPendingCapturedKeyV93 = Key;
	Menu->ProcessEvent( Function, &Parms );
	if( SelectionAliasV92[0] )
		UE1AndroidCleanSaveCapturedAliasV92( Viewport, Menu, Key, DisplayName, SelectionAliasV92 );
	else
		UE1AndroidCleanSaveCapturedNextWeaponV86( Viewport, Menu, Key, DisplayName );
	GAndroidCleanPendingCapturedKeyV93 = -1;

	if( Viewport->Console )
		((UObject*)Viewport->Console)->GotoState( FName("Menuing") );

	return 1; // ANDROID_CONTROLLER_FRIENDLY_KEY_CAPTURE_CLEAN_V86

	unguard;
}
#endif

/*-----------------------------------------------------------------------------
	Command line.
-----------------------------------------------------------------------------*/

UBOOL UNSDLViewport::Exec( const char* Cmd, FOutputDevice* Out )
{
	guard(UNSDLViewport::Exec);
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
	UE1AndroidRefreshRuntimeMenuPatches();
	UE1AndroidCleanScrubFriendlyAliasDuplicatesV92( Input );
	UE1AndroidCleanPatchKeyboardMenuNextWeaponV86( this );

	const char* AndroidKeyCmd = Cmd;
	if( ParseCommand( &AndroidKeyCmd, "KEYNAME" ) )
	{
		const INT KeyNo = appAtoi( AndroidKeyCmd );
		GAndroidCleanLastKeyNameQueryKey = KeyNo;
		const char* FriendlyName = UE1AndroidCleanFriendlyKeyName( KeyNo );
		if( Out )
			Out->Log( FriendlyName ? FriendlyName : ( Input ? Input->GetKeyName( (EInputKey)KeyNo ) : "" ) );
		return 1; // ANDROID_CONTROLLER_FRIENDLY_NAMES_CLEAN_V86
	}

	AndroidKeyCmd = Cmd;
	if( ParseCommand( &AndroidKeyCmd, "KEYBINDING" ) )
	{
		char KeyName[64];
		if( ParseToken( AndroidKeyCmd, KeyName, ARRAY_COUNT(KeyName), 0 ) && Input )
		{
			INT Key = IK_None;
			UE1AndroidCleanResolveCapturedKeyNameV92( Input, KeyName, Key );
			if( Out && Key > 0 && Key < IK_MAX )
				Out->Log( *Input->Bindings[Key] );
		}
		return 1; // ANDROID_CONTROLLER_FRIENDLY_NAMES_CLEAN_V86
	}

	AndroidKeyCmd = Cmd;
	if( ParseCommand( &AndroidKeyCmd, "GET" ) )
	{
		char ClassName[128], PropName[128];
		if( ParseToken( AndroidKeyCmd, ClassName, ARRAY_COUNT(ClassName), 0 )
		 && ParseToken( AndroidKeyCmd, PropName, ARRAY_COUNT(PropName), 0 )
		 && !appStricmp( PropName, "UseJoystick" )
		 && ( !appStricmp( ClassName, "WinDrv.WindowsClient" )
		   || !appStricmp( ClassName, "WindowsClient" )
		   || !appStricmp( ClassName, "NSDLDrv.NSDLClient" )
		   || !appStricmp( ClassName, "NSDLClient" ) ) )
		{
			if( Out )
				Out->Log( "True" );
			return 1; // ANDROID_OPTIONS_JOYPAD_GETSET_TRUE_V107
		}
	}

	AndroidKeyCmd = Cmd;
	if( ParseCommand( &AndroidKeyCmd, "SET" ) )
	{
		char ClassName[128], PropName[128];
		if( ParseToken( AndroidKeyCmd, ClassName, ARRAY_COUNT(ClassName), 0 )
		 && ParseToken( AndroidKeyCmd, PropName, ARRAY_COUNT(PropName), 0 )
		 && !appStricmp( PropName, "UseJoystick" )
		 && ( !appStricmp( ClassName, "WinDrv.WindowsClient" )
		   || !appStricmp( ClassName, "WindowsClient" )
		   || !appStricmp( ClassName, "NSDLDrv.NSDLClient" )
		   || !appStricmp( ClassName, "NSDLClient" ) ) )
		{
			if( Client )
			{
				Client->UseJoystick = true;
				Client->SaveConfig();
			}
			GConfigCache.SetString( "NSDLDrv.NSDLClient", "UseJoystick", "True" );
			GConfigCache.SetString( "WinDrv.WindowsClient", "UseJoystick", "True" );
			GConfigCache.SaveAllConfigs();
			return 1; // ANDROID_OPTIONS_JOYPAD_GETSET_TRUE_V107
		}
	}

	AndroidKeyCmd = Cmd;
	if( ParseCommand( &AndroidKeyCmd, "SET" ) )
	{
		char ClassName[128], PropName[128];
		if( ParseToken( AndroidKeyCmd, ClassName, ARRAY_COUNT(ClassName), 0 )
		 && ParseToken( AndroidKeyCmd, PropName, ARRAY_COUNT(PropName), 0 )
		 && !appStricmp( ClassName, "Input" )
		 && Input )
		{
			INT Key = IK_None;
			UE1AndroidCleanResolveCapturedKeyNameV92( Input, PropName, Key );
			if( Key > 0 && Key < IK_MAX )
			{
				while( *AndroidKeyCmd == ' ' )
					++AndroidKeyCmd;

				// v92: Make SET Input robust for all Customize Controls remaps.
				// Clear old bindings for the same alias so the menu does not resurrect
				// PC defaults like NumPad8/A/B/R next to the newly selected controller key.
				UE1AndroidCleanClearOtherAliasBindingsV86( Input, Key, AndroidKeyCmd );

				Input->Bindings[Key] = AndroidKeyCmd;
				const char* NativeKeyName = Input->GetKeyName( (EInputKey)Key );
				if( NativeKeyName && *NativeKeyName )
					GConfigCache.SetString( "Engine.Input", NativeKeyName, AndroidKeyCmd );
				Input->SaveConfig();
				GConfigCache.SaveAllConfigs();
				UE1AndroidCleanPatchKeyboardMenuNextWeaponV86( this );
				GAndroidCleanFriendlyAliasScrubDoneV93 = 0;
				debugf( NAME_Log, "ANDROID_CONTROLLER_KEYMENUING_DEDUP_V93 SET Input key=%i keyname='%s' binding='%s'", Key, NativeKeyName ? NativeKeyName : "", AndroidKeyCmd );
				return 1; // ANDROID_CONTROLLER_FRIENDLY_NAMES_CLEAN_V86 / CLEAN_NEXTWEAPON_ONLY_V86
			}
		}
	}
#endif
	if( UViewport::Exec( Cmd, Out ) )
	{
		return 1;
	}
	else if( ParseCommand(&Cmd, "ToggleFullscreen") )
	{
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
		// Android is always fullscreen. Reuse the existing, otherwise useless
		// Audio/Video menu entry as a safe native gamma-mode toggle without
		// changing or recompiling Unreal.u.
		UE1AndroidToggleGammaMode( this, Out );
		return 1;
#else
		// Toggle fullscreen.
		if( Client->FullscreenViewport )
			Client->EndFullscreen();
		else if( !(Actor->ShowFlags & SHOW_ChildWindow) )
			Client->TryRenderDevice( this, "ini:Engine.Engine.GameRenderDevice", 1 );
		return 1;
#endif
	}
	else if( ParseCommand(&Cmd, "GetCurrentRes") )
	{
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
		UE1AndroidRefreshGammaMenuLabel();
		Out->Log( UE1AndroidResolutionModeName( Client ? Client->AndroidResolutionMode : 0 ) ); // UE1_ANDROID_RESOLUTION_MENU_NATIVE_FIXED_CLEAN_V85
#else
		Out->Logf( "%ix%i", SizeX, SizeY );
#endif
		return 1;
	}
	else if( ParseCommand(&Cmd, "SetRes") )
	{
#if defined(PLATFORM_ANDROID) || defined(UNREAL_ANDROID) || defined(__ANDROID__)
		char RequestedRes[64];
		appStrncpy( RequestedRes, Cmd, ARRAY_COUNT(RequestedRes) );
		RequestedRes[ARRAY_COUNT(RequestedRes)-1] = 0;
		while( RequestedRes[0] == ' ' )
			appMemmove( RequestedRes, RequestedRes + 1, appStrlen(RequestedRes) );
		INT X = appAtoi(RequestedRes);
		INT Y = appAtoi(appStrchr(RequestedRes,'x') ? appStrchr(RequestedRes,'x')+1 : appStrchr(RequestedRes,'X') ? appStrchr(RequestedRes,'X')+1 : "");
		const INT Mode = UE1AndroidResolutionModeFromName( RequestedRes, X, Y );
		if( Client )
			Client->AndroidResolutionMode = Mode;
		if( Mode == 1 )
		{
			X = 1280;
			Y = 720;
		}
		else if( Mode == 2 )
		{
			X = 1024;
			Y = 768;
		}
		else
		{
			X = SizeX;
			Y = SizeY;
			UE1AndroidApplyConfiguredResolution( Client, hWnd, GLCtx, X, Y );
		}
		if( X && Y )
		{
			if( Client->FullscreenViewport )
				MakeFullscreen( X, Y, 1 );
			else
				SetClientSize( X, Y, 1 );
			if( Client )
			{
				Client->SaveConfig();
				GConfigCache.SaveAllConfigs();
			}
			debugf( NAME_Log, "UE1_ANDROID_RESOLUTION_MENU_NATIVE_FIXED_CLEAN_V85 SetRes %s -> mode=%i size=%ix%i", RequestedRes, Mode, X, Y );
		}
#else
		INT X=appAtoi(Cmd), Y=appAtoi(appStrchr(Cmd,'x') ? appStrchr(Cmd,'x')+1 : appStrchr(Cmd,'X') ? appStrchr(Cmd,'X')+1 : "");
		if( X && Y )
		{
			if( Client->FullscreenViewport )
				MakeFullscreen( X, Y, 1 );
			else
				SetClientSize( X, Y, 1 );
		}
#endif
		return 1;
	}
	else if( ParseCommand(&Cmd, "Preferences") )
	{
		if( Client->FullscreenViewport )
			Client->EndFullscreen();
		return 1;
	}
	else return 0;
	unguard;
}

// ANDROID_SOFT_KEYBOARD_V4_2_SELECTIVE_APPLIED
