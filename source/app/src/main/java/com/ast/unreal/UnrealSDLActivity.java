package com.ast.unreal;

import android.content.Context;
import android.content.pm.ActivityInfo;
import android.graphics.Color;
import android.hardware.input.InputManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;

import java.io.File;

import org.libsdl.app.SDLActivity;

public class UnrealSDLActivity extends SDLActivity implements InputManager.InputDeviceListener {
    // UNREAL_ANDROID_API16_ACTIVITY_V212: isolate post-API16 framework calls behind SDK-specific helpers.

    private static final String TAG = "UE1Controller";

    private File selectedRoot;
    private InputManager inputManager;
    private UnrealTouchOverlayViewV124 touchOverlayViewV124; // UNREAL_ANDROID_TOUCH_OVERLAY_V125

    // UNREAL_ANDROID_CONTROLLER_DIRECT_TOGGLES_V22
    // Detect L3+A/Y before SDL/UE1 translates or consumes controller bindings.
    private boolean controllerToggleL3HeldV22;
    private boolean controllerToggleAmmoHeldV22;
    private boolean controllerToggleHealthHeldV22;
    private boolean controllerToggleAmmoConsumedV22;
    private boolean controllerToggleHealthConsumedV22;

    // ANDROID_NATIVE_CONTROLLER_BACKEND_V88
    private static native boolean nativeAndroidControllerIsEnabled();

    private static native boolean nativeAndroidControllerKey(
            int deviceId,
            int vendorId,
            int productId,
            int keyCode,
            int scanCode,
            int action,
            int repeatCount,
            int source,
            String deviceName);

    private static native boolean nativeAndroidControllerMotion(
            int deviceId,
            int vendorId,
            int productId,
            int source,
            String deviceName,
            float axisX,
            float axisY,
            float axisZ,
            float axisRZ,
            float axisLTrigger,
            float axisRTrigger,
            float axisBrake,
            float axisGas,
            float axisHatX,
            float axisHatY);

    private static native void nativeAndroidControllerDeviceChanged(
            int deviceId,
            int vendorId,
            int productId,
            int source,
            String deviceName,
            int eventType);

    private static native void nativeAndroidControllerReset(); // ANDROID_CONTROLLER_NATIVE_RESET_V88
    private static native boolean nativeAndroidQueueGameplayToggleV22(int toggle); // UNREAL_ANDROID_CONTROLLER_DIRECT_TOGGLES_V22
    private static native void nativeAndroidSetAppVersionName(String versionName); // UNREAL_ANDROID_RUNTIME_VERSION_V141
    private static native boolean nativeAndroidIsMenuV124(); // UNREAL_ANDROID_TOUCH_OVERLAY_V125
    private static native void nativeAndroidTouchLookV131(float x, float y); // UNREAL_ANDROID_TOUCH_RIGHT_LOOK_UT99_V131 explicit native path
    private static native void nativeAndroidTouchLookV101(float x, float y); // UNREAL_ANDROID_TOUCH_RIGHT_LOOK_UT99_V129 fallback
    private static native void nativeAndroidTouchLookV124(float x, float y); // UNREAL_ANDROID_TOUCH_OVERLAY_V125 fallback

    private void resetAndroidNativeControllerState() {
        controllerToggleL3HeldV22 = false; // UNREAL_ANDROID_CONTROLLER_DIRECT_TOGGLES_V22
        controllerToggleAmmoHeldV22 = false;
        controllerToggleHealthHeldV22 = false;
        controllerToggleAmmoConsumedV22 = false;
        controllerToggleHealthConsumedV22 = false;
        try {
            nativeAndroidControllerReset();
        } catch (UnsatisfiedLinkError ignored) {
            // Library may not be ready during early Activity startup. SDL remains fallback.
        }
    }

    private String installedVersionNameV141() {
        try {
            android.content.pm.PackageInfo info = getPackageManager().getPackageInfo(getPackageName(), 0);
            if (info.versionName != null && info.versionName.length() > 0) {
                return info.versionName;
            }
        } catch (Exception e) {
            android.util.Log.w("UE1Version", "Unable to read installed versionName", e);
        }
        return "";
    }

    private void publishInstalledVersionNameV141() {
        String versionName = installedVersionNameV141();
        if (versionName.length() == 0) return;
        try {
            nativeAndroidSetAppVersionName(versionName);
            android.util.Log.i("UE1Version", "Installed versionName: " + versionName);
        } catch (UnsatisfiedLinkError e) {
            android.util.Log.w("UE1Version", "Native version bridge unavailable", e);
        }
    }

    private File selectedRootFromIntentOrScan() {
        if (selectedRoot != null) return selectedRoot;
        String fromIntent = getIntent() != null ? getIntent().getStringExtra(UnrealDataPaths.EXTRA_UNREAL_ROOT) : null;
        if (fromIntent != null && fromIntent.length() > 0) {
            File candidate = new File(fromIntent);
            if (UnrealDataPaths.hasRequiredData(candidate, true)) {
                android.util.Log.i(UnrealDataPaths.TAG_STARTUP, "using data root from intent: " + candidate.getAbsolutePath());
                selectedRoot = candidate;
                return selectedRoot;
            }
            android.util.Log.w(UnrealDataPaths.TAG_STARTUP, "intent data root invalid, rescanning: " + candidate.getAbsolutePath());
        }
        selectedRoot = UnrealDataPaths.findBestUnrealRoot(this);
        return selectedRoot;
    }

    @Override
    protected String[] getLibraries() {
        // libc++ is linked statically (UNREAL_ANDROID_STATIC_LIBCXX_V212), so there is no
        // separate libc++_shared.so to load on API 16 or on 16KB-page devices.
        return new String[] { "SDL2", "openal", "Unreal" };
    }

    @Override
    protected String[] getArguments() {
        // Pass the selected data root only to SDL_main. The native Android
        // launcher consumes --ue1-root before appSetCmdLine(), so UE1 itself
        // never sees this as a map URL or unknown command token.
        selectedRoot = selectedRootFromIntentOrScan();
        return new String[] { "--ue1-root", selectedRoot.getAbsolutePath() };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().getDecorView().setBackgroundColor(Color.BLACK);
        if (android.os.Build.VERSION.SDK_INT >= 21) Api21Window.makeBarsTransparent(getWindow());
        if (android.os.Build.VERSION.SDK_INT >= 28) Api28Window.enableShortEdgeCutout(getWindow());
        if (android.os.Build.VERSION.SDK_INT >= 30) Api30Window.disableDecorFitting(getWindow());
        hideSystemUi();
        selectedRoot = selectedRootFromIntentOrScan();
        UnrealDataPaths.ensureWritableConfigFiles(this, selectedRoot); // UNREAL_ANDROID_CONFIG_BOOTSTRAP_REV31_PATH_FALLBACK_MORE_ROOTS
        super.onCreate(savedInstanceState);
        publishInstalledVersionNameV141(); // UNREAL_ANDROID_RUNTIME_VERSION_V141

        inputManager = (InputManager) getSystemService(Context.INPUT_SERVICE);
        if (inputManager != null) {
            inputManager.registerInputDeviceListener(this, new Handler(Looper.getMainLooper()));
            logConnectedControllerDevices();
        }

        hideSystemUi();
        scheduleImmersiveRefresh();
        installUnrealTouchOverlayV124(); // UNREAL_ANDROID_TOUCH_OVERLAY_V125
    }

    @Override
    protected void onDestroy() {
        final boolean cleanProcessExit = isFinishing() && !isChangingConfigurations();
        if (inputManager != null) {
            inputManager.unregisterInputDeviceListener(this);
            inputManager = null;
        }
        if (touchOverlayViewV124 != null) {
            touchOverlayViewV124.onHostPauseV211(); // UNREAL_ANDROID_LIFECYCLE_PAUSE_V211
        }
        super.onDestroy();

        // UE1 and several of its static native subsystems are not designed to execute
        // SDL_main twice inside the same Android process. After a normal in-game exit,
        // Android otherwise keeps the process alive and the next launch terminates early;
        // the following launch then works only because that failed attempt ended the process.
        // End the already-finished task cleanly so every launch starts with fresh native state.
        if (cleanProcessExit) { // UNREAL_ANDROID_CLEAN_PROCESS_EXIT_V140
            android.util.Log.i("UE1Lifecycle", "Game Activity finished; terminating native process for clean next launch");
            android.os.Process.killProcess(android.os.Process.myPid());
        }
    }

    @Override
    protected void onPause() {
        // Stop the Java overlay timer and release any held virtual controls immediately.
        // The actual UE1/OpenAL pause follows SDL's lifecycle in native code; on Android
        // 7+ SDL intentionally performs the native pause from onStop().
        if (touchOverlayViewV124 != null) {
            touchOverlayViewV124.onHostPauseV211(); // UNREAL_ANDROID_LIFECYCLE_PAUSE_V211
        }
        resetAndroidNativeControllerState(); // ANDROID_CONTROLLER_NATIVE_RESET_V88
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        resetAndroidNativeControllerState(); // ANDROID_CONTROLLER_NATIVE_RESET_V88
        hideSystemUi();
        scheduleImmersiveRefresh();
        installUnrealTouchOverlayV124(); // UNREAL_ANDROID_TOUCH_OVERLAY_V125
        if (touchOverlayViewV124 != null) {
            touchOverlayViewV124.onHostResumeV211(); // UNREAL_ANDROID_LIFECYCLE_PAUSE_V211
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            resetAndroidNativeControllerState(); // ANDROID_CONTROLLER_NATIVE_RESET_V88
            hideSystemUi();
            scheduleImmersiveRefresh();
            bringTouchOverlayToFrontV125(); // UNREAL_ANDROID_TOUCH_OVERLAY_V125
        }
    }

    // UNREAL_ANDROID_CONTROLLER_DIRECT_TOGGLES_V22
    private boolean isControllerToggleL3V22(KeyEvent event) {
        if (event == null) return false;
        final int keyCode = event.getKeyCode();
        final int scanCode = event.getScanCode();
        return keyCode == KeyEvent.KEYCODE_BUTTON_THUMBL
                || scanCode == 317   // Linux BTN_THUMBL
                || scanCode == 289;  // Linux BTN_THUMB on some handhelds
    }

    private int controllerToggleFaceV22(KeyEvent event) {
        if (event == null) return 0;
        final int keyCode = event.getKeyCode();
        final int scanCode = event.getScanCode();
        if (keyCode == KeyEvent.KEYCODE_BUTTON_A || scanCode == 304) return 1; // ammo
        if (keyCode == KeyEvent.KEYCODE_BUTTON_Y || scanCode == 307) return 2; // health
        return 0;
    }

    private boolean queueControllerGameplayToggleV22(int toggle, KeyEvent event) {
        try {
            final boolean queued = nativeAndroidQueueGameplayToggleV22(toggle);
            android.util.Log.i(TAG,
                    "UNREAL_ANDROID_CONTROLLER_DIRECT_TOGGLES_V22 queue=" + toggle
                            + " queued=" + queued
                            + " key=" + (event != null ? event.getKeyCode() : -1)
                            + " scan=" + (event != null ? event.getScanCode() : -1));
            return queued;
        } catch (UnsatisfiedLinkError ignored) {
            return false;
        }
    }

    private boolean handleControllerGameplayToggleChordV22(KeyEvent event) {
        if (event == null) return false;
        if (!isControllerSource(event.getSource()) && !isGamepadButton(event.getKeyCode())) return false;

        final boolean down = event.getAction() == KeyEvent.ACTION_DOWN;
        final boolean up = event.getAction() == KeyEvent.ACTION_UP;
        if (!down && !up) return false;
        final boolean firstDown = down && event.getRepeatCount() == 0;

        if (isControllerToggleL3V22(event)) {
            android.util.Log.i(TAG,
                    "UNREAL_ANDROID_CONTROLLER_DIRECT_TOGGLES_V22 L3 "
                            + (down ? "down" : "up")
                            + " key=" + event.getKeyCode()
                            + " scan=" + event.getScanCode());
            controllerToggleL3HeldV22 = down;
            if (firstDown) {
                boolean consumed = false;
                if (controllerToggleAmmoHeldV22 && !controllerToggleAmmoConsumedV22) {
                    controllerToggleAmmoConsumedV22 = queueControllerGameplayToggleV22(1, event);
                    consumed |= controllerToggleAmmoConsumedV22;
                }
                if (controllerToggleHealthHeldV22 && !controllerToggleHealthConsumedV22) {
                    controllerToggleHealthConsumedV22 = queueControllerGameplayToggleV22(2, event);
                    consumed |= controllerToggleHealthConsumedV22;
                }
                if (consumed) return true;
            }
            return false;
        }

        final int face = controllerToggleFaceV22(event);
        if (face == 0) return false;
        android.util.Log.i(TAG,
                "UNREAL_ANDROID_CONTROLLER_DIRECT_TOGGLES_V22 face=" + face
                        + " " + (down ? "down" : "up")
                        + " l3=" + controllerToggleL3HeldV22
                        + " key=" + event.getKeyCode()
                        + " scan=" + event.getScanCode());

        if (face == 1) {
            if (up) {
                controllerToggleAmmoHeldV22 = false;
                if (controllerToggleAmmoConsumedV22) {
                    controllerToggleAmmoConsumedV22 = false;
                    return true;
                }
                return false;
            }
            controllerToggleAmmoHeldV22 = true;
            if (firstDown && controllerToggleL3HeldV22) {
                controllerToggleAmmoConsumedV22 = queueControllerGameplayToggleV22(1, event);
                return controllerToggleAmmoConsumedV22;
            }
            return false;
        }

        if (up) {
            controllerToggleHealthHeldV22 = false;
            if (controllerToggleHealthConsumedV22) {
                controllerToggleHealthConsumedV22 = false;
                return true;
            }
            return false;
        }
        controllerToggleHealthHeldV22 = true;
        if (firstDown && controllerToggleL3HeldV22) {
            controllerToggleHealthConsumedV22 = queueControllerGameplayToggleV22(2, event);
            return controllerToggleHealthConsumedV22;
        }
        return false;
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (handleControllerGameplayToggleChordV22(event)) return true;
        if (event != null && isMenuStartKeyV124(event.getKeyCode())) {
            // UNREAL_ANDROID_START_MENU_TAP_V124:
            // Some Android/OUYA controllers lose the matching KEY_UP for START/MENU.
            // Queue one native press+release on the first ACTION_DOWN so opening the menu
            // never needs a second physical press.
            if (event.getAction() == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0) {
                if (sendNativeKeyTapV124(event)) return true;
            } else if (event.getAction() == KeyEvent.ACTION_UP) {
                return true;
            }
        }

        if (isControllerSource(event.getSource()) || isGamepadButton(event.getKeyCode()) || isOuyaMenuKey(event.getKeyCode())) {
            InputDevice device = event.getDevice();
            if (device != null) {
                try {
                    // Android pads frequently report L2/R2 both as analog axes and as digital keys.
                    // If the native backend is active and analog trigger ranges exist, prefer the
                    // MotionEvent axis path and suppress the duplicate digital trigger key.
                    // If the backend is disabled, never consume here; SDL remains the fallback.
                    // ANDROID_NATIVE_CONTROLLER_TRIGGER_DEDUPE_V87
                    if (isTriggerKey(event.getKeyCode())
                            && nativeAndroidControllerIsEnabled()
                            && hasAnalogTriggerAxis(device, event.getKeyCode())) {
                        return true;
                    }
                    boolean consumed = nativeAndroidControllerKey(
                            event.getDeviceId(),
                            inputDeviceVendorIdV212(device),
                            inputDeviceProductIdV212(device),
                            event.getKeyCode(),
                            event.getScanCode(),
                            event.getAction(),
                            event.getRepeatCount(),
                            event.getSource(),
                            device.getName());
                    if (consumed) return true;
                } catch (UnsatisfiedLinkError ignored) {
                    // Library not ready: fall back to SDLActivity.
                }
            }
        }
        return super.dispatchKeyEvent(event);
    }

    // UNREAL_ANDROID_CHROMEOS_MOUSE_ACTIVITY_ROUTE_V205F1
    // ChromeOS routes hover/button events through the Activity/DecorView before the
    // SDL child Surface can see them. Route real mouse events directly into SDL.
    private static boolean isPhysicalMouseEventV205F1(MotionEvent event) {
        if (event == null) return false;
        final int source = event.getSource();
        if ((source & InputDevice.SOURCE_MOUSE) == InputDevice.SOURCE_MOUSE) return true;
        if (android.os.Build.VERSION.SDK_INT >= 26
                && (source & InputDevice.SOURCE_MOUSE_RELATIVE) == InputDevice.SOURCE_MOUSE_RELATIVE) return true;
        try {
            return event.getPointerCount() > 0 && event.getToolType(0) == MotionEvent.TOOL_TYPE_MOUSE;
        } catch (Throwable ignored) {
            return false;
        }
    }

    private static boolean hasPointerCaptureV205F1() {
        if (android.os.Build.VERSION.SDK_INT < 26) return false;
        try {
            View content = SDLActivity.getContentView();
            return content != null && Api26View.hasPointerCapture(content);
        } catch (Throwable ignored) {
            return false;
        }
    }

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event) {
        if (isPhysicalMouseEventV205F1(event)) {
            final int action = event.getActionMasked();
            final int source = event.getSource();
            final boolean relative = hasPointerCaptureV205F1()
                    || (android.os.Build.VERSION.SDK_INT >= 26
                    && (source & InputDevice.SOURCE_MOUSE_RELATIVE) == InputDevice.SOURCE_MOUSE_RELATIVE);

            switch (action) {
                case MotionEvent.ACTION_HOVER_MOVE:
                case MotionEvent.ACTION_MOVE: {
                    float x = event.getX(0);
                    float y = event.getY(0);
                    if (relative && android.os.Build.VERSION.SDK_INT >= 26) {
                        final float rx = event.getAxisValue(MotionEvent.AXIS_RELATIVE_X, 0);
                        final float ry = event.getAxisValue(MotionEvent.AXIS_RELATIVE_Y, 0);
                        if (rx != 0.0f || ry != 0.0f) {
                            x = rx;
                            y = ry;
                        }
                    }
                    SDLActivity.onNativeMouse(0, action, x, y, relative);
                    return true;
                }

                case MotionEvent.ACTION_BUTTON_PRESS:
                case MotionEvent.ACTION_BUTTON_RELEASE: {
                    final int nativeAction = action == MotionEvent.ACTION_BUTTON_PRESS
                            ? MotionEvent.ACTION_DOWN : MotionEvent.ACTION_UP;
                    // Android_OnMouse expects the complete current button-state bitmask.
                    // On release it derives the released button from its previous state.
                    SDLActivity.onNativeMouse(event.getButtonState(), nativeAction,
                            event.getX(0), event.getY(0), relative);
                    return true;
                }

                case MotionEvent.ACTION_SCROLL:
                    SDLActivity.onNativeMouse(0, action,
                            event.getAxisValue(MotionEvent.AXIS_HSCROLL, 0),
                            event.getAxisValue(MotionEvent.AXIS_VSCROLL, 0), false);
                    return true;

                default:
                    break;
            }
        }

        if ((event.getActionMasked() == MotionEvent.ACTION_MOVE
                || event.getActionMasked() == MotionEvent.ACTION_HOVER_MOVE)
                && isControllerSource(event.getSource())) {
            InputDevice device = event.getDevice();
            if (device != null) {
                try {
                    boolean consumed = nativeAndroidControllerMotion(
                            event.getDeviceId(),
                            inputDeviceVendorIdV212(device),
                            inputDeviceProductIdV212(device),
                            event.getSource(),
                            device.getName(),
                            event.getAxisValue(MotionEvent.AXIS_X),
                            event.getAxisValue(MotionEvent.AXIS_Y),
                            getSignedControllerAxisWithFallback(event, device, MotionEvent.AXIS_Z, MotionEvent.AXIS_RX), // ANDROID_NATIVE_CONTROLLER_RIGHT_STICK_RXRY_FALLBACK_V116
                            getSignedControllerAxisWithFallback(event, device, MotionEvent.AXIS_RZ, MotionEvent.AXIS_RY), // ANDROID_NATIVE_CONTROLLER_RIGHT_STICK_RXRY_FALLBACK_V116
                            event.getAxisValue(MotionEvent.AXIS_LTRIGGER),
                            event.getAxisValue(MotionEvent.AXIS_RTRIGGER),
                            event.getAxisValue(MotionEvent.AXIS_BRAKE),
                            event.getAxisValue(MotionEvent.AXIS_GAS),
                            event.getAxisValue(MotionEvent.AXIS_HAT_X),
                            event.getAxisValue(MotionEvent.AXIS_HAT_Y));
                    if (consumed) return true;
                } catch (UnsatisfiedLinkError ignored) {
                    // Library not ready: fall back to SDLActivity.
                }
            }
        }
        return super.dispatchGenericMotionEvent(event);
    }

    private static float getSignedControllerAxisWithFallback(MotionEvent event, InputDevice device, int primaryAxis, int fallbackAxis) {
        // ANDROID_NATIVE_CONTROLLER_RIGHT_STICK_RXRY_FALLBACK_V116
        // Some Android devices expose the right stick as Z/RZ, others as RX/RY.
        // Prefer an axis only when Android reports it as signed (-1..+1), so
        // trigger-style 0..1 axes cannot accidentally rotate the camera.
        float primary = getSignedControllerAxis(event, device, primaryAxis);
        float fallback = getSignedControllerAxis(event, device, fallbackAxis);
        return Math.abs(fallback) > Math.abs(primary) ? fallback : primary;
    }

    private static float getSignedControllerAxis(MotionEvent event, InputDevice device, int axis) {
        if (device == null) return 0.0f;
        InputDevice.MotionRange range = device.getMotionRange(axis, event.getSource());
        if (range == null) {
            range = device.getMotionRange(axis);
        }
        if (range == null) return 0.0f;
        if (!(range.getMin() < 0.0f && range.getMax() > 0.0f)) return 0.0f;
        return event.getAxisValue(axis);
    }

    @Override
    public void onInputDeviceAdded(int deviceId) {
        notifyControllerDevice(deviceId, 1);
    }

    @Override
    public void onInputDeviceRemoved(int deviceId) {
        try {
            nativeAndroidControllerDeviceChanged(deviceId, 0, 0, 0, "", 2);
        } catch (UnsatisfiedLinkError ignored) {
        }
    }

    @Override
    public void onInputDeviceChanged(int deviceId) {
        notifyControllerDevice(deviceId, 3);
    }

    private void notifyControllerDevice(int deviceId, int eventType) {
        InputDevice device = InputDevice.getDevice(deviceId);
        if (device == null) return;
        if (!isControllerSource(device.getSources())) return;
        try {
            nativeAndroidControllerDeviceChanged(
                    device.getId(),
                    inputDeviceVendorIdV212(device),
                    inputDeviceProductIdV212(device),
                    device.getSources(),
                    device.getName(),
                    eventType);
        } catch (UnsatisfiedLinkError ignored) {
        }
    }

    private void logConnectedControllerDevices() {
        for (int id : InputDevice.getDeviceIds()) {
            notifyControllerDevice(id, 0);
        }
    }

    private boolean isControllerSource(int source) {
        return (source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                || (source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK
                || (source & InputDevice.SOURCE_DPAD) == InputDevice.SOURCE_DPAD;
    }

    private boolean isGamepadButton(int keyCode) {
        return keyCode >= KeyEvent.KEYCODE_BUTTON_A && keyCode <= KeyEvent.KEYCODE_BUTTON_MODE
                || keyCode >= KeyEvent.KEYCODE_DPAD_UP && keyCode <= KeyEvent.KEYCODE_DPAD_CENTER;
    }

    private boolean isOuyaMenuKey(int keyCode) {
        // UNREAL_ANDROID_CONTROLLER_DIRECT_V122
        // OUYA and several Android-TV pads report their center/system button as
        // KEYCODE_MENU without a gamepad source flag. Route it through the native
        // controller path anyway, matching the proven UT99 controller handling.
        return keyCode == KeyEvent.KEYCODE_MENU || keyCode == KeyEvent.KEYCODE_BUTTON_MODE;
    }

    private boolean isMenuStartKeyV124(int keyCode) {
        return keyCode == KeyEvent.KEYCODE_MENU
                || keyCode == KeyEvent.KEYCODE_BUTTON_START
                || keyCode == KeyEvent.KEYCODE_BUTTON_MODE;
    }

    private boolean sendNativeKeyTapV124(KeyEvent event) {
        InputDevice device = event != null ? event.getDevice() : null;
        int deviceId = event != null ? event.getDeviceId() : -124;
        int vendorId = device != null ? inputDeviceVendorIdV212(device) : 0;
        int productId = device != null ? inputDeviceProductIdV212(device) : 0;
        int source = event != null ? event.getSource() : InputDevice.SOURCE_GAMEPAD;
        String name = device != null ? device.getName() : "UnrealTouchStart";
        int keyCode = event != null ? event.getKeyCode() : KeyEvent.KEYCODE_MENU;
        int scanCode = event != null ? event.getScanCode() : 0;
        try {
            boolean down = nativeAndroidControllerKey(deviceId, vendorId, productId, keyCode, scanCode, KeyEvent.ACTION_DOWN, 0, source, name);
            boolean up = nativeAndroidControllerKey(deviceId, vendorId, productId, keyCode, scanCode, KeyEvent.ACTION_UP, 0, source, name);
            return down || up;
        } catch (UnsatisfiedLinkError ignored) {
            return false;
        }
    }

    private boolean isTriggerKey(int keyCode) {
        return keyCode == KeyEvent.KEYCODE_BUTTON_L2 || keyCode == KeyEvent.KEYCODE_BUTTON_R2;
    }

    private boolean hasAnalogTriggerAxis(InputDevice device, int keyCode) {
        if (device == null) return false;
        int primaryAxis = keyCode == KeyEvent.KEYCODE_BUTTON_L2
                ? MotionEvent.AXIS_LTRIGGER
                : MotionEvent.AXIS_RTRIGGER;
        int aliasAxis = keyCode == KeyEvent.KEYCODE_BUTTON_L2
                ? MotionEvent.AXIS_BRAKE
                : MotionEvent.AXIS_GAS;
        for (InputDevice.MotionRange range : device.getMotionRanges()) {
            int axis = range.getAxis();
            if (axis == primaryAxis || axis == aliasAxis) return true;
        }
        return false;
    }


    // UNREAL_ANDROID_TOUCH_OVERLAY_V125
    private void installUnrealTouchOverlayV124() {
        try {
            if (touchOverlayViewV124 != null) {
                bringTouchOverlayToFrontV125();
                return;
            }
            ensureTouchControlsConfigDefaultV124();
            touchOverlayViewV124 = new UnrealTouchOverlayViewV124(this);
            android.view.ViewGroup.LayoutParams lp = new android.view.ViewGroup.LayoutParams(
                    android.view.ViewGroup.LayoutParams.MATCH_PARENT,
                    android.view.ViewGroup.LayoutParams.MATCH_PARENT);

            // Add directly to SDLActivity's own root layout, above the SDL SurfaceView.
            // addContentView() may end up below the Surface on some Android 8+ devices/ROMs.
            if (mLayout != null) {
                mLayout.addView(touchOverlayViewV124, lp);
            } else {
                addContentView(touchOverlayViewV124, lp);
            }
            bringTouchOverlayToFrontV125();
            android.util.Log.i(TAG, "UNREAL_ANDROID_TOUCH_OVERLAY_V125 installed in SDL root layout");
        } catch (Throwable t) {
            android.util.Log.e(TAG, "UNREAL_ANDROID_TOUCH_OVERLAY_V125 install failed", t);
        }
    }

    // UNREAL_ANDROID_TOUCH_OVERLAY_V125
    private void bringTouchOverlayToFrontV125() {
        if (touchOverlayViewV124 == null) return;
        try {
            touchOverlayViewV124.bringToFront();
            if (android.os.Build.VERSION.SDK_INT >= 21) {
                touchOverlayViewV124.setElevation(10000.0f);
                touchOverlayViewV124.setTranslationZ(10000.0f);
            }
            touchOverlayViewV124.requestLayout();
            touchOverlayViewV124.invalidate();
        } catch (Throwable ignored) {
        }
    }

    private File unrealSystemDirV124() {
        File root = selectedRootFromIntentOrScan();
        if (root == null) return null;
        return new File(root, "System");
    }

    private void ensureTouchControlsConfigDefaultV124() {
        File systemDir = unrealSystemDirV124();
        if (systemDir == null) return;
        File ini = new File(systemDir, "User.ini");
        try {
            if (!systemDir.exists()) systemDir.mkdirs();
            String text = ini.exists() ? readSmallTextFileV124(ini) : "";
            if (text.indexOf("bTouchControls=") < 0) {
                java.io.FileWriter fw = new java.io.FileWriter(ini, true);
                try {
                    fw.write("\n; UNREAL_ANDROID_TOUCH_OVERLAY_V125 default enabled on first start\n");
                    fw.write("[Unreal.UnrealOptionsMenu]\n");
                    fw.write("bTouchControls=True\n");
                } finally {
                    fw.close();
                }
                android.util.Log.i(TAG, "UNREAL_ANDROID_TOUCH_OVERLAY_V125 default config appended to " + ini.getAbsolutePath());
            }
        } catch (Throwable t) {
            android.util.Log.w(TAG, "UNREAL_ANDROID_TOUCH_OVERLAY_V125 could not ensure default", t);
        }
    }

    private String readSmallTextFileV124(File file) throws java.io.IOException {
        java.io.BufferedReader br = new java.io.BufferedReader(new java.io.FileReader(file));
        try {
            StringBuilder sb = new StringBuilder();
            String line;
            int lines = 0;
            while ((line = br.readLine()) != null && lines++ < 4096) {
                sb.append(line).append('\n');
            }
            return sb.toString();
        } finally {
            br.close();
        }
    }

    private boolean readTouchControlsEnabledV124() {
        File systemDir = unrealSystemDirV124();
        Boolean found = null;
        if (systemDir != null) {
            found = readTouchControlsFlagV124(new File(systemDir, "User.ini"), found);
            found = readTouchControlsFlagV124(new File(systemDir, "Unreal.ini"), found);
            found = readTouchControlsFlagV124(new File(systemDir, "AndroidUI.ini"), found);
            found = readTouchControlsFlagV124(new File(systemDir, "Default.ini"), found);
        }
        return found != null ? found.booleanValue() : true;
    }

    private Boolean readTouchControlsFlagV124(File ini, Boolean current) {
        if (ini == null || !ini.exists()) return current;
        try {
            java.io.BufferedReader br = new java.io.BufferedReader(new java.io.FileReader(ini));
            try {
                String line;
                boolean inSection = false;
                Boolean found = current;
                while ((line = br.readLine()) != null) {
                    String t = line.trim();
                    if (t.length() == 0 || t.startsWith(";") || t.startsWith("#")) continue;
                    if (t.startsWith("[") && t.endsWith("]")) {
                        inSection = t.equalsIgnoreCase("[Unreal.UnrealOptionsMenu]")
                                || t.equalsIgnoreCase("[UnrealOptionsMenu]");
                        continue;
                    }
                    int eq = t.indexOf('=');
                    if (eq > 0) {
                        String key = t.substring(0, eq).trim();
                        String value = t.substring(eq + 1).trim();
                        if ((inSection && key.equalsIgnoreCase("bTouchControls")) || key.equalsIgnoreCase("bTouchControls")
                                || (key.equalsIgnoreCase("UseJoystick") && found == null)) {
                            // v125: old Unreal.u still uses the legacy Joystick row.  Treat its
                            // saved UseJoystick value as Touch Controls only when bTouchControls
                            // has not been written yet.
                            found = !(value.equalsIgnoreCase("false")
                                    || value.equals("0")
                                    || value.equalsIgnoreCase("no")
                                    || value.equalsIgnoreCase("off"));
                        }
                    }
                }
                return found;
            } finally {
                br.close();
            }
        } catch (Throwable t) {
            return current;
        }
    }

    private static float clampV124(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    private static final class UnrealTouchOverlayViewV124 extends View {
        private final UnrealSDLActivity activity;
        private final android.graphics.Paint paint = new android.graphics.Paint(android.graphics.Paint.ANTI_ALIAS_FLAG);
        private final android.graphics.RectF rect = new android.graphics.RectF();
        private final android.util.SparseArray<TouchRole> roles = new android.util.SparseArray<TouchRole>();
        private final android.graphics.Bitmap iconFire;
        private final android.graphics.Bitmap iconAltFire;
        private final android.graphics.Bitmap iconJump;
        private final android.graphics.Bitmap iconCrouch;
        private final android.graphics.Bitmap iconNext;
        private final android.graphics.Bitmap iconMenu;
        private final android.graphics.Bitmap iconDpad;
        private long lastConfigReadMs;
        private long lastMenuReadMs;
        private long rightLookLogNextMsV129; // UNREAL_ANDROID_TOUCH_RIGHT_LOOK_UT99_V132
        private long leftStickLogNextMsV132; // UNREAL_ANDROID_TOUCH_STICKS_RESTORE_V132
        private boolean enabled = true;
        private boolean menuVisible = false;
        private float leftBaseX, leftBaseY, rightBaseX, rightBaseY, rightLastX, rightLastY, lx, ly, rx, ry; // UNREAL_ANDROID_TOUCH_RIGHT_LOOK_UT99_V131
        private boolean fire, fireButton, altFire, jump, crouch, next, menu; // UNREAL_ANDROID_ASSISTIVE_TOUCH_SHOOT_V135
        private int rightFireAssistCount = 0; // UNREAL_ANDROID_ASSISTIVE_TOUCH_SHOOT_V135
        private boolean dpadUp, dpadDown, dpadLeft, dpadRight, dpadCenter; // UNREAL_ANDROID_TOUCH_OVERLAY_V125
        private static final int TOUCH_DIRECT_FIRE_V136 = 910105;     // UNREAL_ANDROID_TOUCH_BUTTON_DIRECT_V136
        private static final int TOUCH_DIRECT_ALT_FIRE_V136 = 910104; // UNREAL_ANDROID_TOUCH_BUTTON_DIRECT_V136
        private static final int TOUCH_DIRECT_JUMP_V136 = 910096;     // UNREAL_ANDROID_TOUCH_BUTTON_DIRECT_V136
        private static final int TOUCH_DIRECT_CROUCH_V136 = 910097;   // UNREAL_ANDROID_TOUCH_BUTTON_DIRECT_V136
        private static final int TOUCH_DIRECT_NEXT_V136 = 910103;     // UNREAL_ANDROID_TOUCH_BUTTON_DIRECT_V136
        private enum TouchRole { NONE, LEFT_STICK, RIGHT_LOOK, RIGHT_FIRE_ASSIST, FIRE, ALTFIRE, JUMP, CROUCH, NEXT, MENU, DPAD_UP, DPAD_DOWN, DPAD_LEFT, DPAD_RIGHT, DPAD_CENTER }

        UnrealTouchOverlayViewV124(UnrealSDLActivity activity) {
            super(activity);
            this.activity = activity;
            setWillNotDraw(false);
            setFocusable(false);
            setFocusableInTouchMode(false);
            setClickable(true);
            if (android.os.Build.VERSION.SDK_INT >= 16) {
                setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
            }
            iconFire = loadIcon("touch_overlay/fire.png");
            iconAltFire = loadIcon("touch_overlay/alternate-fire.png");
            iconJump = loadIcon("touch_overlay/jump.png");
            iconCrouch = loadIcon("touch_overlay/crouch.png");
            iconNext = loadIcon("touch_overlay/next-weapon.png");
            iconMenu = loadIcon("touch_overlay/menu.png");
            iconDpad = loadIcon("touch_overlay/dpad.png");
            android.util.Log.i(TAG, "UNREAL_ANDROID_TOUCH_LAYOUT_V134 overlay uses stable left-stick + UT99 right-half FPS look + grey smaller DPAD");
            android.util.Log.i(TAG, "UNREAL_ANDROID_ASSISTIVE_TOUCH_SHOOT_V136 repaired: second right-half touch uses native direct Fire while regular overlay buttons keep their own direct paths");
            android.util.Log.i(TAG, "UNREAL_ANDROID_TOUCH_DISABLE_GATE_V138 consumes all overlay touches while Touch Controls is FALSE");
            startRedrawLoopV211(); // UNREAL_ANDROID_LIFECYCLE_PAUSE_V211
        }

        private boolean redrawLoopActiveV211; // UNREAL_ANDROID_LIFECYCLE_PAUSE_V211

        private final Runnable redrawRunnable = new Runnable() {
            @Override public void run() {
                if (!redrawLoopActiveV211) return;
                invalidate();
                postDelayed(this, 66L);
            }
        };

        private void startRedrawLoopV211() {
            if (redrawLoopActiveV211) return;
            redrawLoopActiveV211 = true;
            removeCallbacks(redrawRunnable);
            postDelayed(redrawRunnable, 66L);
        }

        private void stopRedrawLoopV211() {
            redrawLoopActiveV211 = false;
            removeCallbacks(redrawRunnable);
        }

        void onHostPauseV211() {
            // Release held virtual buttons/sticks before the SDL thread is suspended,
            // then stop the 15 Hz redraw/config polling loop while the app is hidden.
            releaseAll();
            roles.clear();
            stopRedrawLoopV211();
        }

        void onHostResumeV211() {
            startRedrawLoopV211();
            invalidate();
        }

        private android.graphics.Bitmap loadIcon(String assetPath) {
            try {
                java.io.InputStream in = activity.getAssets().open(assetPath);
                try {
                    return android.graphics.BitmapFactory.decodeStream(in);
                } finally {
                    in.close();
                }
            } catch (Throwable t) {
                android.util.Log.w(TAG, "UNREAL_ANDROID_TOUCH_OVERLAY_V125 missing icon " + assetPath, t);
                return null;
            }
        }

        private void refreshState() {
            long now = android.os.SystemClock.uptimeMillis();
            if (now - lastConfigReadMs > 900L) {
                lastConfigReadMs = now;
                enabled = activity.readTouchControlsEnabledV124();
            }
            if (now - lastMenuReadMs > 120L) {
                lastMenuReadMs = now;
                try {
                    menuVisible = nativeAndroidIsMenuV124();
                } catch (Throwable ignored) {
                    menuVisible = false;
                }
            }
        }

        @Override protected void onDraw(android.graphics.Canvas canvas) {
            super.onDraw(canvas);
            refreshState();
            if (!enabled || canvas == null) return;
            float w = getWidth(), h = getHeight();
            if (w <= 0 || h <= 0) return;
            float s = Math.min(w, h);
            float pad = Math.max(10f, s * 0.020f);
            float r = Math.max(50f, s * 0.0705f);
            float gap = Math.max(12f, s * 0.018f);

            final int iconAlpha = 96; // v125: visible, but still translucent like UT99
            float menuR = r * 0.72f;
            float menuCx = pad + menuR;
            float menuCy = pad + menuR;
            drawIconButton(canvas, menuCx, menuCy, menuR, iconMenu, iconAlpha);

            // Unreal needs menu navigation on touch.  Keep the DPAD directly under
            // the menu icon, slightly smaller v134 size, with the same alpha.
            float dpadR = menuR * 2.125f; // UNREAL_ANDROID_TOUCH_LAYOUT_V134: 15% smaller than v133
            float dpadCx = pad + dpadR;
            float dpadCy = menuCy + menuR + gap + dpadR;
            drawIconButton(canvas, dpadCx, dpadCy, dpadR, iconDpad, iconAlpha);

            float buttonUpShift = r + 10f; // UNREAL_ANDROID_TOUCH_LAYOUT_V134: v133 shift plus 10 px up
            float actionY1 = h * 0.42f + r * 2.0f - buttonUpShift;
            float actionY2 = actionY1 + r * 2.0f + gap;
            float nextY = actionY1 - r * 2.0f - gap;
            drawIconButton(canvas, w - pad - r, nextY, r, iconNext, iconAlpha);
            drawIconButton(canvas, w - pad - r, actionY1, r, iconFire, iconAlpha);
            drawIconButton(canvas, w - pad - r, actionY2, r, iconAltFire, iconAlpha);

            float bottomY = h - pad - r - buttonUpShift;
            float bottomShiftLeft = r;
            drawIconButton(canvas, w - pad - r - bottomShiftLeft, bottomY, r, iconCrouch, iconAlpha);
            drawIconButton(canvas, w - pad - r * 3.05f - gap - bottomShiftLeft, bottomY, r, iconJump, iconAlpha);
        }

        private void drawIconButton(android.graphics.Canvas canvas, float cx, float cy, float r, android.graphics.Bitmap icon, int iconAlpha) {
            paint.setStyle(android.graphics.Paint.Style.FILL);
            paint.setColor(0x06202020);
            canvas.drawCircle(cx, cy, r, paint);
            paint.setStyle(android.graphics.Paint.Style.STROKE);
            paint.setStrokeWidth(Math.max(2f, r * 0.055f));
            paint.setColor(0x1AFFFFFF);
            canvas.drawCircle(cx, cy, r, paint);
            if (icon != null) {
                float iconR = r * 0.64f;
                rect.set(cx - iconR, cy - iconR, cx + iconR, cy + iconR);
                paint.setStyle(android.graphics.Paint.Style.FILL);
                paint.setAlpha(iconAlpha);
                canvas.drawBitmap(icon, null, rect, paint);
                paint.setAlpha(255);
            }
        }

        @Override public boolean onTouchEvent(MotionEvent event) {
            if (event == null) return false;
            refreshState();
            if (!enabled) {
                // UNREAL_ANDROID_TOUCH_DISABLE_GATE_V138
                // When Touch Controls is OFF, the transparent overlay must still
                // consume screen touches. Otherwise the SDL SurfaceView below can
                // receive the same tap as a raw mouse/fire event.
                releaseAll();
                roles.clear();
                return true;
            }

            int action = event.getActionMasked();
            int index = event.getActionIndex();
            if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
                TouchRole role = resolveTouchRoleV135(event.getX(index), event.getY(index)); // UNREAL_ANDROID_ASSISTIVE_TOUCH_SHOOT_V135
                if (role == TouchRole.NONE) return false;
                int pointerId = event.getPointerId(index);
                roles.put(pointerId, role);
                if (role == TouchRole.LEFT_STICK) {
                    leftBaseX = event.getX(index);
                    leftBaseY = event.getY(index);
                } else if (role == TouchRole.RIGHT_LOOK) {
                    rightBaseX = event.getX(index); // UNREAL_ANDROID_TOUCH_RIGHT_LOOK_UT99_V129
                    rightBaseY = event.getY(index); // UNREAL_ANDROID_TOUCH_RIGHT_LOOK_UT99_V129
                    rightLastX = rightBaseX;
                    rightLastY = rightBaseY;
                }
                if (role == TouchRole.RIGHT_FIRE_ASSIST) {
                    addRightFireAssistV135();
                } else {
                    updateRole(role, event.getX(index), event.getY(index), true);
                }
                return true;
            }
            if (action == MotionEvent.ACTION_MOVE) {
                boolean consumed = false;
                for (int i = 0; i < event.getPointerCount(); ++i) {
                    TouchRole role = roles.get(event.getPointerId(i));
                    if (role != null && role != TouchRole.NONE) {
                        if (role != TouchRole.RIGHT_FIRE_ASSIST) {
                            updateRole(role, event.getX(i), event.getY(i), true);
                        }
                        consumed = true;
                    }
                }
                return consumed;
            }
            if (action == MotionEvent.ACTION_CANCEL) {
                releaseAll();
                roles.clear();
                return true;
            }
            if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
                int pointerId = event.getPointerId(index);
                TouchRole role = roles.get(pointerId);
                if (role != null && role != TouchRole.NONE) {
                    if (role == TouchRole.RIGHT_FIRE_ASSIST) {
                        removeRightFireAssistV135();
                    } else {
                        updateRole(role, event.getX(index), event.getY(index), false);
                    }
                    roles.remove(pointerId);
                    return true;
                }
                roles.remove(pointerId);
            }
            return false;
        }

        private TouchRole hitRole(float x, float y) {
            float w = getWidth(), h = getHeight(), s = Math.min(w, h);
            float pad = Math.max(10f, s * 0.020f);
            float r = Math.max(50f, s * 0.0705f);
            float gap = Math.max(12f, s * 0.018f);
            float menuR = r * 0.72f;
            float menuCx = pad + menuR;
            float menuCy = pad + menuR;
            if (insideCircle(x, y, menuCx, menuCy, menuR * 1.35f)) return TouchRole.MENU;

            float dpadR = menuR * 2.125f; // UNREAL_ANDROID_TOUCH_LAYOUT_V134: keep hitbox aligned with 15% smaller DPAD
            float dpadCx = pad + dpadR;
            float dpadCy = menuCy + menuR + gap + dpadR;
            if (insideCircle(x, y, dpadCx, dpadCy, dpadR * 1.12f)) {
                float dx = x - dpadCx;
                float dy = y - dpadCy;
                float dist = (float)Math.sqrt(dx * dx + dy * dy);
                if (dist < dpadR * 0.30f) return TouchRole.DPAD_CENTER;
                if (Math.abs(dx) > Math.abs(dy)) return dx < 0 ? TouchRole.DPAD_LEFT : TouchRole.DPAD_RIGHT;
                return dy < 0 ? TouchRole.DPAD_UP : TouchRole.DPAD_DOWN;
            }

            float buttonUpShift = r + 10f; // UNREAL_ANDROID_TOUCH_LAYOUT_V134: keep hitboxes aligned with drawn buttons
            float actionY1 = h * 0.42f + r * 2.0f - buttonUpShift;
            float actionY2 = actionY1 + r * 2.0f + gap;
            float nextY = actionY1 - r * 2.0f - gap;
            if (insideCircle(x, y, w - pad - r, nextY, r * 1.35f)) return TouchRole.NEXT;
            if (insideCircle(x, y, w - pad - r, actionY1, r * 1.35f)) return TouchRole.FIRE;
            if (insideCircle(x, y, w - pad - r, actionY2, r * 1.35f)) return TouchRole.ALTFIRE;
            float bottomY = h - pad - r - buttonUpShift;
            float bottomShiftLeft = r;
            if (insideCircle(x, y, w - pad - r - bottomShiftLeft, bottomY, r * 1.35f)) return TouchRole.CROUCH;
            if (insideCircle(x, y, w - pad - r * 3.05f - gap - bottomShiftLeft, bottomY, r * 1.35f)) return TouchRole.JUMP;

            // UNREAL_ANDROID_TOUCH_LAYOUT_V133:
            // Do not gate the empty left/right gameplay halves by native menu state.
            // On some devices this state can remain true briefly/stale and then the
            // overlay returns NONE, which kills both virtual sticks. Menu/DPAD/buttons
            // still win above by hit area; the remaining screen is always gameplay
            // stick/look exactly like the working UT99 overlay.
            return x < w * 0.5f ? TouchRole.LEFT_STICK : TouchRole.RIGHT_LOOK;
        }

        private boolean hasActiveRightLookPointerV135() {
            for (int i = 0; i < roles.size(); ++i) {
                if (roles.valueAt(i) == TouchRole.RIGHT_LOOK) return true;
            }
            return false;
        }

        private TouchRole resolveTouchRoleV135(float x, float y) {
            TouchRole role = hitRole(x, y);
            // UNREAL_ANDROID_ASSISTIVE_TOUCH_SHOOT_V135:
            // First free right-half touch stays FPS look/aim. A second free
            // right-half touch acts as Fire while it is held. Explicit overlay
            // buttons, DPAD and menu keep their existing roles.
            if (role == TouchRole.RIGHT_LOOK && x >= getWidth() * 0.5f && hasActiveRightLookPointerV135()) {
                return TouchRole.RIGHT_FIRE_ASSIST;
            }
            return role;
        }

        private void syncFireV135() {
            boolean wantFire = fireButton || rightFireAssistCount > 0;
            if (fire != wantFire) {
                fire = wantFire;
                setDirectTouchButtonV136(TOUCH_DIRECT_FIRE_V136, wantFire);
            }
        }

        private void setFireButtonV135(boolean down) {
            if (fireButton != down) {
                fireButton = down;
                syncFireV135();
            }
        }

        private void addRightFireAssistV135() {
            rightFireAssistCount++;
            syncFireV135();
        }

        private void removeRightFireAssistV135() {
            if (rightFireAssistCount > 0) rightFireAssistCount--;
            syncFireV135();
        }

        private boolean insideCircle(float x, float y, float cx, float cy, float r) {
            float dx = x - cx, dy = y - cy;
            return dx * dx + dy * dy <= r * r;
        }

        private float analogValue(float delta, float radius, float dead, float scale) {
            float v = clampV124(delta / radius, -1f, 1f);
            if (Math.abs(v) < dead) return 0f;
            if (v > 0f) v = (v - dead) / (1f - dead);
            else v = (v + dead) / (1f - dead);
            return clampV124(v * scale, -1f, 1f);
        }

        private float touchLookDeltaV129(float deltaPx, float gain) {
            // UNREAL_ANDROID_TOUCH_RIGHT_LOOK_UT99_V129:
            // Exact UT99 principle: relative swipe delta, tiny jitter filter only,
            // no virtual right-stick centre and no continued rotation.
            if (Math.abs(deltaPx) < 0.25f) return 0f;
            return clampV124(deltaPx * gain, -1f, 1f);
        }

        private void sendTouchLookV129(float x, float y) {
            try {
                nativeAndroidTouchLookV131(x, y);
            } catch (Throwable v131) {
                try {
                    nativeAndroidTouchLookV101(x, y);
                } catch (Throwable v101) {
                    try {
                        nativeAndroidTouchLookV124(x, y);
                    } catch (Throwable ignored) {
                    }
                }
            }
        }

        private void updateRole(TouchRole role, float x, float y, boolean down) {
            float s = Math.min(getWidth(), getHeight());
            float moveRadius = Math.max(112f, s * 0.145f);
            switch (role) {
                case LEFT_STICK:
                    lx = down ? analogValue(x - leftBaseX, moveRadius, 0.075f, 0.74f) : 0f;
                    ly = down ? analogValue(y - leftBaseY, moveRadius, 0.075f, 0.74f) : 0f;
                    if (lx != 0f || ly != 0f) {
                        long now = android.os.SystemClock.uptimeMillis();
                        if (now >= leftStickLogNextMsV132) {
                            leftStickLogNextMsV132 = now + 1200L;
                            android.util.Log.i(TAG, "UNREAL_ANDROID_TOUCH_LAYOUT_V133 left-stick lx=" + lx + " ly=" + ly);
                        }
                    }
                    sendNativeMotion();
                    break;
                case RIGHT_LOOK:
                    if (down) {
                        float dx = x - rightLastX;
                        float dy = y - rightLastY;
                        rightLastX = x;
                        rightLastY = y;
                        rx = touchLookDeltaV129(dx, 0.0210f);
                        ry = touchLookDeltaV129(dy, 0.0210f);
                        if (rx != 0f || ry != 0f) {
                            long now = android.os.SystemClock.uptimeMillis();
                            if (now >= rightLookLogNextMsV129) {
                                rightLookLogNextMsV129 = now + 1200L;
                                android.util.Log.i(TAG, "UNREAL_ANDROID_TOUCH_LAYOUT_V134 right-look dx=" + dx + " dy=" + dy + " rx=" + rx + " ry=" + ry);
                            }
                        }
                    } else {
                        rx = ry = 0f;
                    }
                    sendTouchLookV129(rx, ry);
                    break;
                case RIGHT_FIRE_ASSIST:
                    break;
                case FIRE:
                    setFireButtonV135(down);
                    break;
                case ALTFIRE:
                    if (altFire != down) { altFire = down; setDirectTouchButtonV136(TOUCH_DIRECT_ALT_FIRE_V136, down); }
                    break;
                case JUMP:
                    if (jump != down) { jump = down; setDirectTouchButtonV136(TOUCH_DIRECT_JUMP_V136, down); }
                    break;
                case CROUCH:
                    if (crouch != down) { crouch = down; setDirectTouchButtonV136(TOUCH_DIRECT_CROUCH_V136, down); }
                    break;
                case NEXT:
                    if (next != down) { next = down; setDirectTouchButtonV136(TOUCH_DIRECT_NEXT_V136, down); }
                    break;
                case MENU:
                    if (menu != down) { menu = down; setButton(KeyEvent.KEYCODE_MENU, down); }
                    break;
                case DPAD_UP:
                    if (dpadUp != down) { dpadUp = down; setButton(KeyEvent.KEYCODE_DPAD_UP, down); }
                    break;
                case DPAD_DOWN:
                    if (dpadDown != down) { dpadDown = down; setButton(KeyEvent.KEYCODE_DPAD_DOWN, down); }
                    break;
                case DPAD_LEFT:
                    if (dpadLeft != down) { dpadLeft = down; setButton(KeyEvent.KEYCODE_DPAD_LEFT, down); }
                    break;
                case DPAD_RIGHT:
                    if (dpadRight != down) { dpadRight = down; setButton(KeyEvent.KEYCODE_DPAD_RIGHT, down); }
                    break;
                case DPAD_CENTER:
                    if (dpadCenter != down) { dpadCenter = down; setButton(KeyEvent.KEYCODE_BUTTON_A, down); }
                    break;
                case NONE:
                    break;
            }
        }

        private void setDirectTouchButtonV136(int directKeyCode, boolean down) {
            // UNREAL_ANDROID_TOUCH_BUTTON_DIRECT_V136:
            // Gameplay overlay buttons and assistive shoot use a native direct path
            // that selects the current friendly controller binding or a safe PC fallback.
            try {
                nativeAndroidControllerKey(
                        -136,
                        0,
                        0,
                        directKeyCode,
                        0,
                        down ? KeyEvent.ACTION_DOWN : KeyEvent.ACTION_UP,
                        0,
                        InputDevice.SOURCE_GAMEPAD,
                        "UnrealTouchButtonDirectV136");
            } catch (Throwable ignored) {
            }
        }

        private void setButton(int keyCode, boolean down) {
            try {
                nativeAndroidControllerKey(
                        -124,
                        0,
                        0,
                        keyCode,
                        0,
                        down ? KeyEvent.ACTION_DOWN : KeyEvent.ACTION_UP,
                        0,
                        InputDevice.SOURCE_GAMEPAD,
                        "UnrealTouchOverlay");
            } catch (Throwable ignored) {
            }
        }

        private void sendNativeMotion() {
            try {
                nativeAndroidControllerMotion(
                        -124,
                        0,
                        0,
                        InputDevice.SOURCE_JOYSTICK,
                        "UnrealTouchOverlay",
                        lx,
                        ly,
                        0f,
                        0f,
                        0f,
                        0f,
                        0f,
                        0f,
                        0f,
                        0f);
            } catch (Throwable ignored) {
            }
        }

        private void releaseAll() {
            if (lx != 0f || ly != 0f) {
                lx = ly = 0f;
                sendNativeMotion();
            }
            if (rx != 0f || ry != 0f) {
                rx = ry = 0f;
                sendTouchLookV129(0f, 0f);
            }
            if (fire || fireButton || rightFireAssistCount > 0) {
                fire = false;
                fireButton = false;
                rightFireAssistCount = 0;
                setDirectTouchButtonV136(TOUCH_DIRECT_FIRE_V136, false);
            }
            if (altFire) { altFire = false; setDirectTouchButtonV136(TOUCH_DIRECT_ALT_FIRE_V136, false); }
            if (jump) { jump = false; setDirectTouchButtonV136(TOUCH_DIRECT_JUMP_V136, false); }
            if (crouch) { crouch = false; setDirectTouchButtonV136(TOUCH_DIRECT_CROUCH_V136, false); }
            if (next) { next = false; setDirectTouchButtonV136(TOUCH_DIRECT_NEXT_V136, false); }
            if (menu) { menu = false; setButton(KeyEvent.KEYCODE_MENU, false); }
            if (dpadUp) { dpadUp = false; setButton(KeyEvent.KEYCODE_DPAD_UP, false); }
            if (dpadDown) { dpadDown = false; setButton(KeyEvent.KEYCODE_DPAD_DOWN, false); }
            if (dpadLeft) { dpadLeft = false; setButton(KeyEvent.KEYCODE_DPAD_LEFT, false); }
            if (dpadRight) { dpadRight = false; setButton(KeyEvent.KEYCODE_DPAD_RIGHT, false); }
            if (dpadCenter) { dpadCenter = false; setButton(KeyEvent.KEYCODE_BUTTON_A, false); }
        }
    }

    private void scheduleImmersiveRefresh() {
        Handler handler = new Handler(Looper.getMainLooper());
        handler.postDelayed(this::hideSystemUi, 50);
        handler.postDelayed(this::hideSystemUi, 250);
        handler.postDelayed(this::hideSystemUi, 750);
    }

    private void hideSystemUi() {
        View decor = getWindow().getDecorView();
        decor.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_FULLSCREEN |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        );
        if (android.os.Build.VERSION.SDK_INT >= 30) Api30Window.hideSystemBars(decor);
    }

    private static int inputDeviceVendorIdV212(InputDevice device) {
        return device != null && android.os.Build.VERSION.SDK_INT >= 19 ? Api19Input.vendorId(device) : 0;
    }

    private static int inputDeviceProductIdV212(InputDevice device) {
        return device != null && android.os.Build.VERSION.SDK_INT >= 19 ? Api19Input.productId(device) : 0;
    }

    @android.annotation.TargetApi(19)
    private static final class Api19Input {
        private Api19Input() {}
        static int vendorId(InputDevice device) { return device.getVendorId(); }
        static int productId(InputDevice device) { return device.getProductId(); }
    }

    @android.annotation.TargetApi(21)
    private static final class Api21Window {
        private Api21Window() {}
        static void makeBarsTransparent(android.view.Window window) {
            window.setStatusBarColor(Color.TRANSPARENT);
            window.setNavigationBarColor(Color.TRANSPARENT);
        }
    }

    @android.annotation.TargetApi(26)
    private static final class Api26View {
        private Api26View() {}
        static boolean hasPointerCapture(View view) { return view.hasPointerCapture(); }
    }

    @android.annotation.TargetApi(28)
    private static final class Api28Window {
        private Api28Window() {}
        static void enableShortEdgeCutout(android.view.Window window) {
            WindowManager.LayoutParams attrs = window.getAttributes();
            attrs.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            window.setAttributes(attrs);
        }
    }

    @android.annotation.TargetApi(30)
    private static final class Api30Window {
        private Api30Window() {}
        static void disableDecorFitting(android.view.Window window) {
            window.setDecorFitsSystemWindows(false);
        }
        static void hideSystemBars(View decor) {
            android.view.WindowInsetsController controller = decor.getWindowInsetsController();
            if (controller == null) return;
            controller.hide(android.view.WindowInsets.Type.statusBars() | android.view.WindowInsets.Type.navigationBars());
            controller.setSystemBarsBehavior(android.view.WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
        }
    }

}
