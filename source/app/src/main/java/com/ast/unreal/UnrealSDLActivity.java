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
    private UnrealRetroTouchBridge retroTouchBridgeV215; // UNREAL_ANDROID_RETROTOUCH_V215
    private long retroTouchConfigReadMsV215;
    private boolean retroTouchEnabledCacheV215 = true;

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
    static final int RETROTOUCH_UI_GAMEPLAY_V215 = 0;
    static final int RETROTOUCH_UI_NAVIGATION_V215 = 1;
    static final int RETROTOUCH_UI_BLOCKED_V215 = 2;
    static final int RETROTOUCH_UI_INTRO_TAP_V217 = 3; // invisible full-screen tap -> Escape/Menu
    private static native int nativeRetroTouchUiModeV215(); // UNREAL_ANDROID_RETROTOUCH_V215
    private static native void nativeRetroTouchLookV215(float x, float y); // UNREAL_ANDROID_RETROTOUCH_V215
    private static native void nativeRetroTouchSetTouchModeV218(boolean touchMode); // UNREAL_ANDROID_RETROTOUCH_AUTOMODE_V218

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
        installRetroTouchV215(); // UNREAL_ANDROID_RETROTOUCH_V215
    }

    @Override
    protected void onDestroy() {
        final boolean cleanProcessExit = isFinishing() && !isChangingConfigurations();
        if (inputManager != null) {
            inputManager.unregisterInputDeviceListener(this);
            inputManager = null;
        }
        if (retroTouchBridgeV215 != null) {
            retroTouchBridgeV215.onHostDestroy(); // UNREAL_ANDROID_RETROTOUCH_V215 UNREAL_ANDROID_LIFECYCLE_PAUSE_V211
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
        // Release RetroTouch state before SDL/native pause. RetroTouch itself owns
        // pointer tracking, while UE1 releases semantic actions on the next input tick.
        if (retroTouchBridgeV215 != null) {
            retroTouchBridgeV215.onHostPause(); // UNREAL_ANDROID_RETROTOUCH_V215 UNREAL_ANDROID_LIFECYCLE_PAUSE_V211
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
        installRetroTouchV215(); // UNREAL_ANDROID_RETROTOUCH_V215
        if (retroTouchBridgeV215 != null) {
            retroTouchBridgeV215.onHostResume(); // UNREAL_ANDROID_RETROTOUCH_V215 UNREAL_ANDROID_LIFECYCLE_PAUSE_V211
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            resetAndroidNativeControllerState(); // ANDROID_CONTROLLER_NATIVE_RESET_V88
            hideSystemUi();
            scheduleImmersiveRefresh();
            bringRetroTouchToFrontV215(); // UNREAL_ANDROID_RETROTOUCH_V215
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
    public boolean dispatchTouchEvent(MotionEvent event) {
        // UNREAL_ANDROID_RETROTOUCH_INTRO_TAP_V217:
        // During Unreal.Intro the RetroTouch view intentionally stays OFF so the Nali flyby is
        // unobstructed. Intercept only that intro gesture here; all other touch events continue
        // through RetroTouch/SDL normally.
        if (retroTouchBridgeV215 != null && retroTouchBridgeV215.handleIntroTouchV217(event))
            return true;
        return super.dispatchTouchEvent(event);
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


    // UNREAL_ANDROID_RETROTOUCH_V215
    private void installRetroTouchV215() {
        try {
            if (retroTouchBridgeV215 == null) {
                ensureRetroTouchConfigDefaultV215();
                retroTouchBridgeV215 = new UnrealRetroTouchBridge(this);
                android.view.ViewGroup.LayoutParams lp = new android.view.ViewGroup.LayoutParams(
                        android.view.ViewGroup.LayoutParams.MATCH_PARENT,
                        android.view.ViewGroup.LayoutParams.MATCH_PARENT);
                if (mLayout != null) {
                    mLayout.addView(retroTouchBridgeV215.getView(), lp);
                } else {
                    addContentView(retroTouchBridgeV215.getView(), lp);
                }
                android.util.Log.i(TAG, "UNREAL_ANDROID_RETROTOUCH_V215 installed in SDL root layout");
            }
            bringRetroTouchToFrontV215();
        } catch (Throwable t) {
            android.util.Log.e(TAG, "UNREAL_ANDROID_RETROTOUCH_V215 install failed", t);
        }
    }

    // UNREAL_ANDROID_RETROTOUCH_V215
    private void bringRetroTouchToFrontV215() {
        if (retroTouchBridgeV215 != null) retroTouchBridgeV215.bringToFront();
    }

    private File unrealSystemDirV215() {
        File root = selectedRootFromIntentOrScan();
        return root == null ? null : new File(root, "System");
    }

    private void ensureRetroTouchConfigDefaultV215() {
        File systemDir = unrealSystemDirV215();
        if (systemDir == null) return;
        File ini = new File(systemDir, "User.ini");
        try {
            if (!systemDir.exists()) systemDir.mkdirs();
            String text = ini.exists() ? readSmallTextFileV215(ini) : "";
            if (text.indexOf("bTouchControls=") < 0) {
                java.io.FileWriter fw = new java.io.FileWriter(ini, true);
                try {
                    fw.write("\n; UNREAL_ANDROID_RETROTOUCH_V215 default enabled on first start\n");
                    fw.write("[Unreal.UnrealOptionsMenu]\n");
                    fw.write("bTouchControls=True\n");
                } finally {
                    fw.close();
                }
                android.util.Log.i(TAG, "UNREAL_ANDROID_RETROTOUCH_V215 default config appended to " + ini.getAbsolutePath());
            }
        } catch (Throwable t) {
            android.util.Log.w(TAG, "UNREAL_ANDROID_RETROTOUCH_V215 could not ensure default", t);
        }
    }

    private String readSmallTextFileV215(File file) throws java.io.IOException {
        java.io.BufferedReader br = new java.io.BufferedReader(new java.io.FileReader(file));
        try {
            StringBuilder sb = new StringBuilder();
            String line;
            int lines = 0;
            while ((line = br.readLine()) != null && lines++ < 4096) sb.append(line).append('\n');
            return sb.toString();
        } finally {
            br.close();
        }
    }

    boolean isRetroTouchEnabledV215() {
        // Match the proven old overlay cadence: native menu state may be polled often,
        // but INI files are disk I/O and must not be reopened every 150 ms on Tegra 3.
        long now = android.os.SystemClock.uptimeMillis();
        if (retroTouchConfigReadMsV215 != 0L && now - retroTouchConfigReadMsV215 < 900L)
            return retroTouchEnabledCacheV215;
        retroTouchConfigReadMsV215 = now;

        File systemDir = unrealSystemDirV215();
        Boolean found = null;
        if (systemDir != null) {
            found = readRetroTouchFlagV215(new File(systemDir, "User.ini"), found);
            found = readRetroTouchFlagV215(new File(systemDir, "Unreal.ini"), found);
            found = readRetroTouchFlagV215(new File(systemDir, "AndroidUI.ini"), found);
            found = readRetroTouchFlagV215(new File(systemDir, "Default.ini"), found);
        }
        retroTouchEnabledCacheV215 = found != null ? found.booleanValue() : true;
        return retroTouchEnabledCacheV215;
    }

    private Boolean readRetroTouchFlagV215(File ini, Boolean current) {
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
                    if (eq <= 0) continue;
                    String key = t.substring(0, eq).trim();
                    String value = t.substring(eq + 1).trim();
                    if ((inSection && key.equalsIgnoreCase("bTouchControls")) || key.equalsIgnoreCase("bTouchControls")
                            || (key.equalsIgnoreCase("UseJoystick") && found == null)) {
                        found = !(value.equalsIgnoreCase("false") || value.equals("0")
                                || value.equalsIgnoreCase("no") || value.equalsIgnoreCase("off"));
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


    boolean setRetroTouchInputModeV218(boolean touchMode) {
        try {
            nativeRetroTouchSetTouchModeV218(touchMode);
            retroTouchEnabledCacheV215 = touchMode;
            retroTouchConfigReadMsV215 = android.os.SystemClock.uptimeMillis();
            return true;
        } catch (Throwable ignored) {
            return false;
        }
    }

    int getRetroTouchUiModeV215() {
        try {
            return nativeRetroTouchUiModeV215();
        } catch (Throwable ignored) {
            return RETROTOUCH_UI_BLOCKED_V215;
        }
    }

    // UNREAL_ANDROID_RETROTOUCH_RESET_API_V221 UNREAL_ANDROID_RETROTOUCH_RESET_DIRECT_V222
    // Called by NSDLViewport synchronously with UInput::ResetInput(). SDL's game thread
    // cannot mutate Android Views, so enqueue the reset immediately on the main looper.
    // A Runnable posted now is ordered ahead of touch events that arrive afterwards.
    public void onNativeRetroTouchInputResetV222() {
        if (Looper.myLooper() == Looper.getMainLooper()) {
            if (retroTouchBridgeV215 != null) retroTouchBridgeV215.onEngineInputResetV222();
            return;
        }
        runOnUiThread(new Runnable() {
            @Override public void run() {
                if (retroTouchBridgeV215 != null) retroTouchBridgeV215.onEngineInputResetV222();
            }
        });
    }

    void pushRetroTouchLookV215(float x, float y) {
        try {
            nativeRetroTouchLookV215(x, y);
        } catch (Throwable ignored) {
        }
    }

    void queueRetroTouchAndroidKeyV215(int keyCode, boolean down) {
        try {
            nativeAndroidControllerKey(
                    -215,
                    0,
                    0,
                    keyCode,
                    0,
                    down ? KeyEvent.ACTION_DOWN : KeyEvent.ACTION_UP,
                    0,
                    InputDevice.SOURCE_GAMEPAD,
                    "RetroTouch");
        } catch (Throwable ignored) {
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
