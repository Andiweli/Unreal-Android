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
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

import java.io.File;

import org.libsdl.app.SDLActivity;

public class UnrealSDLActivity extends SDLActivity implements InputManager.InputDeviceListener {
    private static final String TAG = "UE1Controller";

    private File selectedRoot;
    private InputManager inputManager;

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

    private void resetAndroidNativeControllerState() {
        try {
            nativeAndroidControllerReset();
        } catch (UnsatisfiedLinkError ignored) {
            // Library may not be ready during early Activity startup. SDL remains fallback.
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
        getWindow().setStatusBarColor(Color.TRANSPARENT);
        getWindow().setNavigationBarColor(Color.TRANSPARENT);
        if (android.os.Build.VERSION.SDK_INT >= 30) {
            getWindow().setDecorFitsSystemWindows(false);
        }
        if (android.os.Build.VERSION.SDK_INT >= 28) {
            WindowManager.LayoutParams attrs = getWindow().getAttributes();
            attrs.layoutInDisplayCutoutMode = WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
            getWindow().setAttributes(attrs);
        }
        hideSystemUi();
        selectedRoot = selectedRootFromIntentOrScan();
        UnrealDataPaths.ensureWritableConfigFiles(this, selectedRoot); // UNREAL_ANDROID_CONFIG_BOOTSTRAP_REV31_PATH_FALLBACK_MORE_ROOTS
        super.onCreate(savedInstanceState);

        inputManager = (InputManager) getSystemService(Context.INPUT_SERVICE);
        if (inputManager != null) {
            inputManager.registerInputDeviceListener(this, new Handler(Looper.getMainLooper()));
            logConnectedControllerDevices();
        }

        hideSystemUi();
        scheduleImmersiveRefresh();
    }

    @Override
    protected void onDestroy() {
        if (inputManager != null) {
            inputManager.unregisterInputDeviceListener(this);
            inputManager = null;
        }
        super.onDestroy();
    }

    @Override
    protected void onPause() {
        resetAndroidNativeControllerState(); // ANDROID_CONTROLLER_NATIVE_RESET_V88
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        resetAndroidNativeControllerState(); // ANDROID_CONTROLLER_NATIVE_RESET_V88
        hideSystemUi();
        scheduleImmersiveRefresh();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            resetAndroidNativeControllerState(); // ANDROID_CONTROLLER_NATIVE_RESET_V88
            hideSystemUi();
            scheduleImmersiveRefresh();
        }
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (isControllerSource(event.getSource()) || isGamepadButton(event.getKeyCode())) {
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
                            device.getVendorId(),
                            device.getProductId(),
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

    @Override
    public boolean dispatchGenericMotionEvent(MotionEvent event) {
        if ((event.getActionMasked() == MotionEvent.ACTION_MOVE
                || event.getActionMasked() == MotionEvent.ACTION_HOVER_MOVE)
                && isControllerSource(event.getSource())) {
            InputDevice device = event.getDevice();
            if (device != null) {
                try {
                    boolean consumed = nativeAndroidControllerMotion(
                            event.getDeviceId(),
                            device.getVendorId(),
                            device.getProductId(),
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
                    device.getVendorId(),
                    device.getProductId(),
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
        if (android.os.Build.VERSION.SDK_INT >= 30) {
            WindowInsetsController controller = decor.getWindowInsetsController();
            if (controller != null) {
                controller.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
                controller.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        } else {
            decor.setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_FULLSCREEN |
                    View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                    View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                    View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            );
        }
    }
}
