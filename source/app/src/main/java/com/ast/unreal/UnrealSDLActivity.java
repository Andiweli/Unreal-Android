package com.ast.unreal;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.hardware.input.InputManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.text.InputType;
import android.widget.EditText;
import android.widget.Toast;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.InputStream;
import java.util.Arrays;

import org.libsdl.app.SDLActivity;

public class UnrealSDLActivity extends SDLActivity implements InputManager.InputDeviceListener {
    private static final String TAG = "UE1Controller";

    private File selectedRoot;
    private InputManager inputManager;
    private UnrealTouchOverlayViewV124 touchOverlayViewV124; // UNREAL_ANDROID_TOUCH_OVERLAY_V125
    private EditText consoleEditTextV147; // UNREAL_ANDROID_CONSOLE_OVERLAY_V149

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
    private static native boolean nativeAndroidConsoleExecV148(String command); // UNREAL_ANDROID_NATIVE_CONSOLE_EXEC_V149

    private void forceLandscapeV141(String reason) {
        try {
            // V142: allow both landscape rotations but never portrait.
            // SDLActivity may request orientation later from native code, so all callbacks
            // are funneled through this method and sensor-landscape is reapplied.
            setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
            android.util.Log.i(TAG, "UNREAL_ANDROID_SENSOR_LANDSCAPE_LOCK_V142 " + reason);
        } catch (Throwable ignored) {
        }
    }

    @Override
    public void setOrientationBis(int w, int h, boolean resizable, String hint) {
        // SDLActivity normally decides orientation from SDL's hint/window size. Ignore
        // SDL portrait requests, but keep reverse-landscape rotation available.
        forceLandscapeV141("setOrientationBis w=" + w + " h=" + h + " hint=" + hint);
    }


    private void setSoftKeyboardOverlayModeV147(String reason) {
        try {
            getWindow().setSoftInputMode(
                    WindowManager.LayoutParams.SOFT_INPUT_ADJUST_NOTHING
                            | WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
            android.util.Log.i(TAG, "UNREAL_ANDROID_CONSOLE_IME_OVERLAY_V147 " + reason);
        } catch (Throwable ignored) {
        }
    }

    private void ensureConsoleEditTextV147() {
        if (consoleEditTextV147 != null) return;

        consoleEditTextV147 = new EditText(this);
        consoleEditTextV147.setSingleLine(true);
        consoleEditTextV147.setTextColor(Color.WHITE);
        consoleEditTextV147.setHintTextColor(0x99FFFFFF);
        consoleEditTextV147.setTextSize(18.0f);
        consoleEditTextV147.setHint("Unreal console command");
        consoleEditTextV147.setPadding(18, 8, 18, 8);
        consoleEditTextV147.setSelectAllOnFocus(false);
        consoleEditTextV147.setBackgroundColor(0xCC000000);
        consoleEditTextV147.setInputType(InputType.TYPE_CLASS_TEXT
                | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS
                | InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD);
        consoleEditTextV147.setImeOptions(EditorInfo.IME_ACTION_DONE
                | EditorInfo.IME_FLAG_NO_EXTRACT_UI
                | EditorInfo.IME_FLAG_NO_FULLSCREEN);
        consoleEditTextV147.setVisibility(View.GONE);
        if (android.os.Build.VERSION.SDK_INT >= 21) {
            consoleEditTextV147.setElevation(20000.0f);
            consoleEditTextV147.setTranslationZ(20000.0f);
        }

        consoleEditTextV147.setOnEditorActionListener((v, actionId, event) -> {
            boolean enter = event != null
                    && event.getKeyCode() == KeyEvent.KEYCODE_ENTER
                    && event.getAction() == KeyEvent.ACTION_UP;
            if (actionId == EditorInfo.IME_ACTION_DONE
                    || actionId == EditorInfo.IME_ACTION_GO
                    || actionId == EditorInfo.IME_ACTION_SEND
                    || enter) {
                submitConsoleCommandV147();
                return true;
            }
            return false;
        });

        ViewGroup root = mLayout != null ? mLayout : null;
        if (root == null) {
            try {
                View decor = getWindow() != null ? getWindow().getDecorView() : null;
                if (decor instanceof ViewGroup) root = (ViewGroup) decor;
            } catch (Throwable ignored) {
            }
        }

        if (root != null) {
            ViewGroup.LayoutParams lp;
            int margin = dpV147(10);
            int height = dpV147(46);
            if (root instanceof android.widget.RelativeLayout) {
                android.widget.RelativeLayout.LayoutParams rlp = new android.widget.RelativeLayout.LayoutParams(
                        android.widget.RelativeLayout.LayoutParams.MATCH_PARENT,
                        height);
                rlp.leftMargin = margin;
                rlp.rightMargin = margin;
                rlp.topMargin = margin;
                rlp.addRule(android.widget.RelativeLayout.ALIGN_PARENT_TOP);
                lp = rlp;
            } else if (root instanceof android.widget.FrameLayout) {
                android.widget.FrameLayout.LayoutParams flp = new android.widget.FrameLayout.LayoutParams(
                        android.widget.FrameLayout.LayoutParams.MATCH_PARENT,
                        height,
                        Gravity.TOP);
                flp.leftMargin = margin;
                flp.rightMargin = margin;
                flp.topMargin = margin;
                lp = flp;
            } else {
                lp = new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, height);
            }
            root.addView(consoleEditTextV147, lp);
            consoleEditTextV147.bringToFront();
        } else {
            addContentView(consoleEditTextV147, new ViewGroup.LayoutParams(
                    ViewGroup.LayoutParams.MATCH_PARENT, dpV147(46)));
        }
    }

    private int dpV147(int value) {
        try {
            return Math.max(1, (int) (value * getResources().getDisplayMetrics().density + 0.5f));
        } catch (Throwable ignored) {
            return value;
        }
    }

    private void openConsoleCommandOverlayV147() {
        runOnUiThread(new Runnable() {
            @Override public void run() {
                try {
                    setSoftKeyboardOverlayModeV147("console-open");
                    ensureConsoleEditTextV147();
                    consoleEditTextV147.setText("");
                    consoleEditTextV147.setVisibility(View.VISIBLE);
                    consoleEditTextV147.bringToFront();
                    consoleEditTextV147.requestFocus();

                    InputMethodManager imm = (InputMethodManager)
                            getSystemService(Context.INPUT_METHOD_SERVICE);
                    if (imm != null) {
                        imm.showSoftInput(consoleEditTextV147, InputMethodManager.SHOW_IMPLICIT);
                    }
                } catch (Throwable t) {
                    android.util.Log.w(TAG, "console overlay open failed", t);
                }
            }
        });
    }

    private void hideConsoleCommandOverlayV147(final boolean hideKeyboard) {
        runOnUiThread(new Runnable() {
            @Override public void run() {
                try {
                    if (consoleEditTextV147 != null) {
                        if (hideKeyboard) {
                            InputMethodManager imm = (InputMethodManager)
                                    getSystemService(Context.INPUT_METHOD_SERVICE);
                            if (imm != null) imm.hideSoftInputFromWindow(consoleEditTextV147.getWindowToken(), 0);
                        }
                        consoleEditTextV147.clearFocus();
                        consoleEditTextV147.setVisibility(View.GONE);
                    }
                    setSoftKeyboardOverlayModeV147("console-hide");
                    hideSystemUi();
                    bringTouchOverlayToFrontV125();
                } catch (Throwable ignored) {
                }
            }
        });
    }

    private void submitConsoleCommandV147() {
        final String command = consoleEditTextV147 != null
                ? String.valueOf(consoleEditTextV147.getText()).trim()
                : "";
        if (command.length() == 0) {
            hideConsoleCommandOverlayV147(true);
            return;
        }

        // V148: execute through native UE1 Exec instead of trying to fake
        // Tab + keyboard text + Enter.  The old fake-typing path opened the
        // input UI but the game did not receive a real console command.
        hideConsoleCommandOverlayV147(true);
        new Handler(Looper.getMainLooper()).postDelayed(new Runnable() {
            @Override public void run() {
                boolean ok = false;
                try {
                    ok = nativeAndroidConsoleExecV148(command);
                } catch (Throwable t) {
                    android.util.Log.w(TAG, "native console exec unavailable", t);
                }
                if (!ok) {
                    Toast.makeText(UnrealSDLActivity.this,
                            "Console command was not accepted: " + command,
                            Toast.LENGTH_SHORT).show();
                }
            }
        }, 40L);
    }

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
        forceLandscapeV141("onCreate-before-super");
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().getDecorView().setBackgroundColor(Color.BLACK);
        getWindow().setStatusBarColor(Color.TRANSPARENT);
        getWindow().setNavigationBarColor(Color.TRANSPARENT);
        setSoftKeyboardOverlayModeV147("onCreate");
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
        forceLandscapeV141("onCreate-after-super");
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
        resetAndroidNativeControllerState(); // ANDROID_CONTROLLER_NATIVE_RESET_V88
        super.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        forceLandscapeV141("onResume");
        resetAndroidNativeControllerState(); // ANDROID_CONTROLLER_NATIVE_RESET_V88
        hideSystemUi();
        scheduleImmersiveRefresh();
        installUnrealTouchOverlayV124(); // UNREAL_ANDROID_TOUCH_OVERLAY_V125
    }

    @Override
    protected void onStart() {
        forceLandscapeV141("onStart-before-super");
        super.onStart();
        forceLandscapeV141("onStart-after-super");
    }

    @Override
    protected void onPostResume() {
        super.onPostResume();
        forceLandscapeV141("onPostResume");
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        forceLandscapeV141("onConfigurationChanged");
        hideSystemUi();
        scheduleImmersiveRefresh();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            forceLandscapeV141("onWindowFocusChanged");
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
            return content != null && content.hasPointerCapture();
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
        int vendorId = device != null ? device.getVendorId() : 0;
        int productId = device != null ? device.getProductId() : 0;
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
        private final Paint paint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint labelPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final RectF rect = new RectF();
        private final android.util.SparseArray<TouchRole> roles = new android.util.SparseArray<>();
        private final android.util.SparseArray<TouchRole> lookButtonPointers = new android.util.SparseArray<>();
        private final SharedPreferences layoutPrefs;
        private final float[] buttonX = new float[TouchRole.values().length];
        private final float[] buttonY = new float[TouchRole.values().length];
        private final float[] buttonScale = new float[TouchRole.values().length];

        private static final String LAYOUT_PREFS_NAME = "unreal_touch_overlay_layout_v139";
        private static final float BUTTON_SCALE_MIN = 0.55f;
        private static final float BUTTON_SCALE_MAX = 1.85f;

        private final Bitmap iconFire;
        private final Bitmap iconAltFire;
        private final Bitmap iconJump;
        private final Bitmap iconCrouch;
        private final Bitmap iconNext;
        private final Bitmap iconMenu;
        private final Bitmap iconDpad;

        private long lastConfigReadMs;
        private long rightLookLogNextMsV129;
        private long leftStickLogNextMsV132;
        private long lastEditToggleMs;

        private boolean enabled = true;
        private float leftBaseX, leftBaseY, rightLastX, rightLastY, lx, ly, rx, ry;
        private boolean fire, fireButton, altFire, jump, crouch, next, menu, quickSave, quickLoad, console;
        private int rightFireAssistCount = 0;
        private boolean dpadUp, dpadDown, dpadLeft, dpadRight, dpadCenter;

        private boolean editMode = false;
        private int editDragPointerId = -1;
        private int editPinchPointerId = -1;
        private TouchRole editRole = null;
        private float editGrabDx = 0f, editGrabDy = 0f;
        private float editPinchStartDistance = 0f, editPinchStartScale = 1f;
        private boolean editStartedOnEditButton = false, editDragMoved = false, editUsedPinch = false;
        private float editDownX = 0f, editDownY = 0f;

        private static final int TOUCH_DIRECT_FIRE_V136 = 910105;
        private static final int TOUCH_DIRECT_ALT_FIRE_V136 = 910104;
        private static final int TOUCH_DIRECT_JUMP_V136 = 910096;
        private static final int TOUCH_DIRECT_CROUCH_V136 = 910097;
        private static final int TOUCH_DIRECT_NEXT_V136 = 910103;

        private enum TouchRole {
            NONE,
            LEFT_STICK,
            RIGHT_LOOK,
            RIGHT_FIRE_ASSIST,
            FIRE,
            ALTFIRE,
            JUMP,
            CROUCH,
            NEXT,
            MENU,
            DPAD_UP,
            DPAD_DOWN,
            DPAD_LEFT,
            DPAD_RIGHT,
            DPAD_CENTER,
            SAVE,
            LOAD,
            CONSOLE,
            EDIT
        }

        UnrealTouchOverlayViewV124(UnrealSDLActivity activity) {
            super(activity);
            this.activity = activity;
            this.layoutPrefs = activity.getSharedPreferences(LAYOUT_PREFS_NAME, Context.MODE_PRIVATE);

            Arrays.fill(buttonX, -1f);
            Arrays.fill(buttonY, -1f);
            Arrays.fill(buttonScale, 1f);
            loadLayoutPrefs();

            setWillNotDraw(false);
            setFocusable(false);
            setFocusableInTouchMode(false);
            setClickable(true);
            labelPaint.setTextAlign(Paint.Align.CENTER);
            labelPaint.setFakeBoldText(true);

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

            android.util.Log.i(TAG,
                    "UNREAL_ANDROID_TOUCH_EDIT_V139: UT99-style aim+fire, editable layout");
            postDelayed(redrawRunnable, 66L);
        }

        private final Runnable redrawRunnable = new Runnable() {
            @Override public void run() {
                invalidate();
                postDelayed(this, 66L);
            }
        };

        private Bitmap loadIcon(String assetPath) {
            try {
                InputStream in = activity.getAssets().open(assetPath);
                try {
                    return BitmapFactory.decodeStream(in);
                } finally {
                    in.close();
                }
            } catch (Throwable t) {
                android.util.Log.w(TAG, "missing touch icon " + assetPath, t);
                return null;
            }
        }

        private void loadLayoutPrefs() {
            if (layoutPrefs == null) return;
            for (TouchRole role : TouchRole.values()) {
                int i = role.ordinal();
                buttonX[i] = layoutPrefs.getFloat(role.name() + "_x", -1f);
                buttonY[i] = layoutPrefs.getFloat(role.name() + "_y", -1f);
                buttonScale[i] = clampV124(layoutPrefs.getFloat(role.name() + "_scale", 1f),
                        BUTTON_SCALE_MIN, BUTTON_SCALE_MAX);
            }
        }

        private boolean isLayoutEditableButton(TouchRole role) {
            return role == TouchRole.FIRE
                    || role == TouchRole.ALTFIRE
                    || role == TouchRole.JUMP
                    || role == TouchRole.CROUCH
                    || role == TouchRole.NEXT
                    || role == TouchRole.SAVE
                    || role == TouchRole.LOAD
                    || role == TouchRole.CONSOLE
                    || role == TouchRole.MENU
                    || role == TouchRole.DPAD_CENTER
                    || role == TouchRole.EDIT;
        }

        private float baseButtonRadius(TouchRole role, float w, float h) {
            float s = Math.min(w, h);
            float r = Math.max(50f, s * 0.0705f);
            if (role == TouchRole.MENU || role == TouchRole.SAVE || role == TouchRole.LOAD || role == TouchRole.CONSOLE) return r * 0.82f;
            if (role == TouchRole.DPAD_CENTER) return r * 1.52f;
            if (role == TouchRole.EDIT) return Math.max(54f, r * 1.10f);
            return r;
        }

        private float buttonScale(TouchRole role) {
            if (role == null) return 1f;
            float v = buttonScale[role.ordinal()];
            return clampV124(v <= 0f ? 1f : v, BUTTON_SCALE_MIN, BUTTON_SCALE_MAX);
        }

        private float buttonRadius(TouchRole role, float w, float h) {
            return baseButtonRadius(role, w, h) * buttonScale(role);
        }

        private float defaultButtonCx(TouchRole role, float w, float h) {
            float s = Math.min(w, h);
            float pad = Math.max(10f, s * 0.020f);
            float r = Math.max(50f, s * 0.0705f);
            float gap = Math.max(12f, s * 0.018f);
            float bottomShiftLeft = r;
            switch (role) {
                case MENU:
                    return pad + r * 0.72f;
                case EDIT:
                    return pad + r * 2.25f;
                case DPAD_CENTER:
                    return pad + r * 1.62f;
                case SAVE:
                case LOAD:
                case CONSOLE:
                case NEXT:
                case FIRE:
                case ALTFIRE:
                    return w - pad - r;
                case CROUCH:
                    return w - pad - r - bottomShiftLeft;
                case JUMP:
                    return w - pad - r * 3.05f - gap - bottomShiftLeft;
                default:
                    return w * 0.5f;
            }
        }

        private float defaultButtonCy(TouchRole role, float w, float h) {
            float s = Math.min(w, h);
            float pad = Math.max(10f, s * 0.020f);
            float r = Math.max(50f, s * 0.0705f);
            float gap = Math.max(12f, s * 0.018f);
            float menuR = r * 0.76f;
            float menuCy = pad + menuR;
            float dpadR = r * 1.52f;
            float dpadCy = menuCy + menuR + gap + dpadR;
            float saveY = menuCy;
            float loadY = saveY + r * 1.65f + gap;
            float consoleY = loadY + r * 1.65f + gap;
            float buttonUpShift = r + 10f;
            float actionY1 = h * 0.42f + r * 2.0f - buttonUpShift;
            float actionY2 = actionY1 + r * 2.0f + gap;
            float nextY = actionY1 - r * 2.0f - gap;
            float bottomY = h - pad - r - buttonUpShift;
            switch (role) {
                case MENU:
                    return menuCy;
                case EDIT:
                    return menuCy;
                case DPAD_CENTER:
                    return dpadCy;
                case SAVE:
                    return saveY;
                case LOAD:
                    return loadY;
                case CONSOLE:
                    return consoleY;
                case NEXT:
                    return nextY;
                case FIRE:
                    return actionY1;
                case ALTFIRE:
                    return actionY2;
                case CROUCH:
                case JUMP:
                    return bottomY;
                default:
                    return h * 0.5f;
            }
        }

        private float buttonCx(TouchRole role, float w, float h) {
            float r = buttonRadius(role, w, h);
            float cx = (buttonX[role.ordinal()] >= 0f)
                    ? buttonX[role.ordinal()] * w
                    : defaultButtonCx(role, w, h);
            return clampV124(cx, r, Math.max(r, w - r));
        }

        private float buttonCy(TouchRole role, float w, float h) {
            float r = buttonRadius(role, w, h);
            float cy = (buttonY[role.ordinal()] >= 0f)
                    ? buttonY[role.ordinal()] * h
                    : defaultButtonCy(role, w, h);
            return clampV124(cy, r, Math.max(r, h - r));
        }

        private float hitMultiplier(TouchRole role) {
            if (role == TouchRole.DPAD_CENTER) return 1.12f;
            if (role == TouchRole.EDIT) return 1.45f;
            return 1.35f;
        }

        private boolean insideButton(TouchRole role, float x, float y, float w, float h) {
            float r = buttonRadius(role, w, h) * hitMultiplier(role);
            return insideCircle(x, y, buttonCx(role, w, h), buttonCy(role, w, h), r);
        }

        private void saveButtonPositionPx(TouchRole role, float cx, float cy) {
            if (!isLayoutEditableButton(role)) return;
            float w = getWidth(), h = getHeight();
            if (w <= 0f || h <= 0f) return;
            float r = buttonRadius(role, w, h);
            cx = clampV124(cx, r, Math.max(r, w - r));
            cy = clampV124(cy, r, Math.max(r, h - r));
            buttonX[role.ordinal()] = cx / w;
            buttonY[role.ordinal()] = cy / h;
            layoutPrefs.edit()
                    .putFloat(role.name() + "_x", buttonX[role.ordinal()])
                    .putFloat(role.name() + "_y", buttonY[role.ordinal()])
                    .apply();
        }

        private void saveButtonScale(TouchRole role, float scale) {
            if (!isLayoutEditableButton(role)) return;
            buttonScale[role.ordinal()] = clampV124(scale, BUTTON_SCALE_MIN, BUTTON_SCALE_MAX);
            layoutPrefs.edit().putFloat(role.name() + "_scale", buttonScale[role.ordinal()]).apply();
            float w = getWidth(), h = getHeight();
            if (w > 0f && h > 0f) {
                saveButtonPositionPx(role, buttonCx(role, w, h), buttonCy(role, w, h));
            }
        }

        @SuppressWarnings("unused")
        private void resetTouchLayout() {
            for (TouchRole role : TouchRole.values()) {
                if (!isLayoutEditableButton(role)) continue;
                buttonX[role.ordinal()] = -1f;
                buttonY[role.ordinal()] = -1f;
                buttonScale[role.ordinal()] = 1f;
            }
            layoutPrefs.edit().clear().apply();
            Toast.makeText(activity, "Touch layout reset", Toast.LENGTH_SHORT).show();
            invalidate();
        }

        private int findPointerIndex(MotionEvent event, int pointerId) {
            if (event == null || pointerId < 0) return -1;
            for (int i = 0; i < event.getPointerCount(); ++i) {
                if (event.getPointerId(i) == pointerId) return i;
            }
            return -1;
        }

        private float pointerDistance(MotionEvent event, int pointerA, int pointerB) {
            int ia = findPointerIndex(event, pointerA);
            int ib = findPointerIndex(event, pointerB);
            if (ia < 0 || ib < 0) return 0f;
            float dx = event.getX(ia) - event.getX(ib);
            float dy = event.getY(ia) - event.getY(ib);
            return (float)Math.sqrt(dx * dx + dy * dy);
        }

        private void toggleEditMode() {
            long now = android.os.SystemClock.uptimeMillis();
            if (now - lastEditToggleMs < 250L) return;
            lastEditToggleMs = now;
            releaseAll();
            roles.clear();
            lookButtonPointers.clear();
            editDragPointerId = -1;
            editPinchPointerId = -1;
            editRole = null;
            editStartedOnEditButton = false;
            editDragMoved = false;
            editUsedPinch = false;
            editMode = !editMode;
            Toast.makeText(activity,
                    editMode ? "Touch layout edit: drag buttons, pinch to resize" : "Touch layout saved",
                    Toast.LENGTH_SHORT).show();
            invalidate();
        }

        private void refreshState() {
            long now = android.os.SystemClock.uptimeMillis();
            if (now - lastConfigReadMs > 900L) {
                lastConfigReadMs = now;
                enabled = activity.readTouchControlsEnabledV124();
            }
        }

        @Override
        protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            refreshState();
            if (!enabled || canvas == null) return;
            float w = getWidth(), h = getHeight();
            if (w <= 0 || h <= 0) return;

            drawRoleButton(canvas, TouchRole.MENU, iconMenu, null);
            drawRoleButton(canvas, TouchRole.DPAD_CENTER, iconDpad, null);
            drawIconButton(canvas,
                    buttonCx(TouchRole.EDIT, w, h),
                    buttonCy(TouchRole.EDIT, w, h),
                    buttonRadius(TouchRole.EDIT, w, h),
                    null,
                    editMode ? "DONE" : "EDIT");

            // V140: keep gameplay buttons visible after leaving EDIT.
            // On this Unreal build native menu detection may stay true too long,
            // which hid FIRE/ALT/NEXT/JUMP/CROUCH after DONE in v139.
            drawRoleButton(canvas, TouchRole.SAVE, null, "SAVE");
            drawRoleButton(canvas, TouchRole.LOAD, null, "LOAD");
            drawRoleButton(canvas, TouchRole.CONSOLE, null, "CON");
            drawRoleButton(canvas, TouchRole.NEXT, iconNext, null);
            drawRoleButton(canvas, TouchRole.FIRE, iconFire, null);
            drawRoleButton(canvas, TouchRole.ALTFIRE, iconAltFire, null);
            drawRoleButton(canvas, TouchRole.CROUCH, iconCrouch, null);
            drawRoleButton(canvas, TouchRole.JUMP, iconJump, null);

            if (editMode) drawEditModeOverlay(canvas, w, h);
        }

        private void drawRoleButton(Canvas canvas, TouchRole role, Bitmap icon, String label) {
            float w = getWidth(), h = getHeight();
            drawIconButton(canvas, buttonCx(role, w, h), buttonCy(role, w, h),
                    buttonRadius(role, w, h), icon, label);
        }

        private void drawIconButton(Canvas canvas, float cx, float cy, float r, Bitmap icon, String label) {
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(0x06202020);
            canvas.drawCircle(cx, cy, r, paint);

            paint.setStyle(Paint.Style.STROKE);
            paint.setStrokeWidth(Math.max(2f, r * 0.055f));
            paint.setColor(0x1AFFFFFF);
            canvas.drawCircle(cx, cy, r, paint);

            if (icon != null) {
                float iconR = r * 0.64f;
                rect.set(cx - iconR, cy - iconR, cx + iconR, cy + iconR);
                paint.setStyle(Paint.Style.FILL);
                paint.setAlpha(96);
                canvas.drawBitmap(icon, null, rect, paint);
                paint.setAlpha(255);
            } else if (label != null) {
                labelPaint.setColor(0x66FFFFFF);
                boolean editLabel = "EDIT".equals(label) || "DONE".equals(label);
                labelPaint.setTextSize(editLabel
                        ? Math.max(15f, Math.min(28f, r * 0.50f))
                        : Math.max(12f, Math.min(18f, r * 0.36f)));
                Paint.FontMetrics fm = labelPaint.getFontMetrics();
                canvas.drawText(label, cx, cy - (fm.ascent + fm.descent) * 0.5f, labelPaint);
            }
        }

        private void drawEditModeOverlay(Canvas canvas, float w, float h) {
            paint.setStyle(Paint.Style.FILL);
            paint.setColor(0x22000000);
            canvas.drawRect(0f, 0f, w, Math.max(34f, h * 0.075f), paint);

            labelPaint.setColor(0xAAFFFFFF);
            labelPaint.setTextSize(Math.max(14f, Math.min(22f, h * 0.038f)));
            Paint.FontMetrics fm = labelPaint.getFontMetrics();
            canvas.drawText("EDIT MODE: drag buttons, pinch to resize, DONE to save",
                    w * 0.5f,
                    Math.max(17f, h * 0.037f) - (fm.ascent + fm.descent) * 0.5f,
                    labelPaint);

            for (TouchRole role : TouchRole.values()) {
                if (!isLayoutEditableButton(role)) continue;
                float cx = buttonCx(role, w, h);
                float cy = buttonCy(role, w, h);
                float r = buttonRadius(role, w, h);

                paint.setStyle(Paint.Style.STROKE);
                paint.setStrokeWidth(Math.max(2f, r * 0.05f));
                paint.setColor(role == editRole ? 0x99FFFFFF : 0x44FFFFFF);
                canvas.drawCircle(cx, cy, r * 1.12f, paint);

                labelPaint.setColor(0x88FFFFFF);
                labelPaint.setTextSize(Math.max(10f, r * 0.22f));
                canvas.drawText(roleLabel(role), cx, cy + r + Math.max(10f, r * 0.28f), labelPaint);
            }
        }

        private String roleLabel(TouchRole role) {
            if (role == TouchRole.DPAD_CENTER) return "DPAD";
            if (role == TouchRole.ALTFIRE) return "ALT";
            return role.name();
        }

        @Override
        public boolean onTouchEvent(MotionEvent event) {
            if (event == null) return false;
            refreshState();
            if (!enabled) {
                releaseAll();
                roles.clear();
                lookButtonPointers.clear();
                return true;
            }

            int action = event.getActionMasked();
            int index = event.getActionIndex();
            int pointerId = (index >= 0 && index < event.getPointerCount())
                    ? event.getPointerId(index)
                    : -1;

            if (editMode) {
                return handleLayoutEditTouchEvent(event);
            }

            if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
                if (pointerId < 0) return true;

                TouchRole directRole = hitRole(event.getX(index), event.getY(index));
                if (directRole == TouchRole.EDIT) {
                    toggleEditMode();
                    return true;
                }

                TouchRole role = resolveTouchRoleV139(event.getX(index), event.getY(index));


                // Fire / AltFire / Crouch are transparent for aiming.
                // One finger can hold the action and keep moving the camera.
                if (isLookTransparentButton(role)) {
                    setLookButtonPressed(role, true);
                    lookButtonPointers.put(pointerId, role);
                    role = TouchRole.RIGHT_LOOK;
                }

                roles.put(pointerId, role);

                if (role == TouchRole.LEFT_STICK) {
                    leftBaseX = event.getX(index);
                    leftBaseY = event.getY(index);
                } else if (role == TouchRole.RIGHT_LOOK) {
                    rightLastX = event.getX(index);
                    rightLastY = event.getY(index);
                } else if (role == TouchRole.RIGHT_FIRE_ASSIST) {
                    addRightFireAssistV139();
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
                lookButtonPointers.clear();
                return true;
            }

            if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
                TouchRole role = roles.get(pointerId);
                if (role != null && role != TouchRole.NONE) {
                    TouchRole transparentButtonRole = lookButtonPointers.get(pointerId);
                    if (transparentButtonRole != null) {
                        setLookButtonPressed(transparentButtonRole, false);
                        lookButtonPointers.remove(pointerId);
                    }

                    if (role == TouchRole.RIGHT_FIRE_ASSIST) {
                        removeRightFireAssistV139();
                    } else {
                        updateRole(role, event.getX(index), event.getY(index), false);
                    }
                    roles.remove(pointerId);
                    return true;
                }
                roles.remove(pointerId);
                lookButtonPointers.remove(pointerId);
            }

            return roles.size() > 0;
        }

        private boolean handleLayoutEditTouchEvent(MotionEvent event) {
            if (event == null) return true;
            int action = event.getActionMasked();
            int index = event.getActionIndex();
            int pointerId = (index >= 0 && index < event.getPointerCount())
                    ? event.getPointerId(index)
                    : -1;

            if (action == MotionEvent.ACTION_DOWN || action == MotionEvent.ACTION_POINTER_DOWN) {
                if (pointerId < 0) return true;

                TouchRole role = hitEditableRole(event.getX(index), event.getY(index));

                if (editDragPointerId < 0 && isLayoutEditableButton(role)) {
                    editRole = role;
                    editDragPointerId = pointerId;
                    editPinchPointerId = -1;
                    editStartedOnEditButton = role == TouchRole.EDIT;
                    editDragMoved = false;
                    editUsedPinch = false;
                    editDownX = event.getX(index);
                    editDownY = event.getY(index);
                    float w = getWidth(), h = getHeight();
                    editGrabDx = event.getX(index) - buttonCx(role, w, h);
                    editGrabDy = event.getY(index) - buttonCy(role, w, h);
                    invalidate();
                    return true;
                }

                if (editRole != null && editPinchPointerId < 0 && pointerId != editDragPointerId) {
                    editPinchPointerId = pointerId;
                    editUsedPinch = true;
                    editPinchStartDistance = pointerDistance(event, editDragPointerId, editPinchPointerId);
                    editPinchStartScale = buttonScale(editRole);
                    invalidate();
                    return true;
                }
                return true;
            }

            if (action == MotionEvent.ACTION_MOVE) {
                if (editRole != null && editDragPointerId >= 0) {
                    int dragIndex = findPointerIndex(event, editDragPointerId);
                    if (dragIndex >= 0) {
                        float dx = event.getX(dragIndex) - editDownX;
                        float dy = event.getY(dragIndex) - editDownY;
                        if ((dx * dx + dy * dy) > 64f) editDragMoved = true;
                        saveButtonPositionPx(editRole,
                                event.getX(dragIndex) - editGrabDx,
                                event.getY(dragIndex) - editGrabDy);
                    }

                    if (editPinchPointerId >= 0) {
                        editUsedPinch = true;
                        float dist = pointerDistance(event, editDragPointerId, editPinchPointerId);
                        if (editPinchStartDistance > 12f && dist > 1f) {
                            saveButtonScale(editRole,
                                    editPinchStartScale * (dist / editPinchStartDistance));
                        }
                    }
                    invalidate();
                }
                return true;
            }

            if (action == MotionEvent.ACTION_UP || action == MotionEvent.ACTION_POINTER_UP) {
                if (pointerId == editPinchPointerId) {
                    editPinchPointerId = -1;
                    editPinchStartDistance = 0f;
                    editPinchStartScale = editRole != null ? buttonScale(editRole) : 1f;
                    return true;
                }

                if (pointerId == editDragPointerId) {
                    boolean finishEdit = editRole == TouchRole.EDIT
                            && editStartedOnEditButton
                            && !editDragMoved
                            && !editUsedPinch;
                    editDragPointerId = -1;
                    editPinchPointerId = -1;
                    editRole = null;
                    editGrabDx = editGrabDy = 0f;
                    editPinchStartDistance = 0f;
                    editPinchStartScale = 1f;
                    editStartedOnEditButton = false;
                    editDragMoved = false;
                    editUsedPinch = false;
                    invalidate();
                    if (finishEdit) toggleEditMode();
                    return true;
                }
                return true;
            }

            if (action == MotionEvent.ACTION_CANCEL) {
                editDragPointerId = -1;
                editPinchPointerId = -1;
                editRole = null;
                editGrabDx = editGrabDy = 0f;
                editPinchStartDistance = 0f;
                editPinchStartScale = 1f;
                editStartedOnEditButton = false;
                editDragMoved = false;
                editUsedPinch = false;
                invalidate();
                return true;
            }

            return true;
        }

        private TouchRole hitEditableRole(float x, float y) {
            float w = getWidth(), h = getHeight();
            if (w <= 0f || h <= 0f) return TouchRole.NONE;
            if (insideButton(TouchRole.EDIT, x, y, w, h)) return TouchRole.EDIT;
            if (insideButton(TouchRole.MENU, x, y, w, h)) return TouchRole.MENU;
            if (insideButton(TouchRole.DPAD_CENTER, x, y, w, h)) return TouchRole.DPAD_CENTER;
            if (insideButton(TouchRole.SAVE, x, y, w, h)) return TouchRole.SAVE;
            if (insideButton(TouchRole.LOAD, x, y, w, h)) return TouchRole.LOAD;
            if (insideButton(TouchRole.CONSOLE, x, y, w, h)) return TouchRole.CONSOLE;
            if (insideButton(TouchRole.NEXT, x, y, w, h)) return TouchRole.NEXT;
            if (insideButton(TouchRole.FIRE, x, y, w, h)) return TouchRole.FIRE;
            if (insideButton(TouchRole.ALTFIRE, x, y, w, h)) return TouchRole.ALTFIRE;
            if (insideButton(TouchRole.CROUCH, x, y, w, h)) return TouchRole.CROUCH;
            if (insideButton(TouchRole.JUMP, x, y, w, h)) return TouchRole.JUMP;
            return TouchRole.NONE;
        }

        private TouchRole hitRole(float x, float y) {
            float w = getWidth(), h = getHeight();
            if (w <= 0f || h <= 0f) return TouchRole.RIGHT_LOOK;

            // EDIT first so it stays easy to hit after moving buttons around.
            if (insideButton(TouchRole.EDIT, x, y, w, h)) return TouchRole.EDIT;
            if (insideButton(TouchRole.MENU, x, y, w, h)) return TouchRole.MENU;

            if (insideButton(TouchRole.DPAD_CENTER, x, y, w, h)) {
                float cx = buttonCx(TouchRole.DPAD_CENTER, w, h);
                float cy = buttonCy(TouchRole.DPAD_CENTER, w, h);
                float r = buttonRadius(TouchRole.DPAD_CENTER, w, h);
                float dx = x - cx;
                float dy = y - cy;
                float dist = (float)Math.sqrt(dx * dx + dy * dy);
                if (dist < r * 0.30f) return TouchRole.DPAD_CENTER;
                if (Math.abs(dx) > Math.abs(dy)) return dx < 0 ? TouchRole.DPAD_LEFT : TouchRole.DPAD_RIGHT;
                return dy < 0 ? TouchRole.DPAD_UP : TouchRole.DPAD_DOWN;
            }

            if (insideButton(TouchRole.SAVE, x, y, w, h)) return TouchRole.SAVE;
            if (insideButton(TouchRole.LOAD, x, y, w, h)) return TouchRole.LOAD;
            if (insideButton(TouchRole.CONSOLE, x, y, w, h)) return TouchRole.CONSOLE;
            if (insideButton(TouchRole.NEXT, x, y, w, h)) return TouchRole.NEXT;
            if (insideButton(TouchRole.FIRE, x, y, w, h)) return TouchRole.FIRE;
            if (insideButton(TouchRole.ALTFIRE, x, y, w, h)) return TouchRole.ALTFIRE;
            if (insideButton(TouchRole.CROUCH, x, y, w, h)) return TouchRole.CROUCH;
            if (insideButton(TouchRole.JUMP, x, y, w, h)) return TouchRole.JUMP;

            return x < w * 0.5f ? TouchRole.LEFT_STICK : TouchRole.RIGHT_LOOK;
        }

        private boolean hasActiveRightLookPointerV139() {
            for (int i = 0; i < roles.size(); ++i) {
                if (roles.valueAt(i) == TouchRole.RIGHT_LOOK) return true;
            }
            return false;
        }

        private TouchRole resolveTouchRoleV139(float x, float y) {
            TouchRole role = hitRole(x, y);
            if (role == TouchRole.RIGHT_LOOK
                    && x >= getWidth() * 0.5f
                    && hasActiveRightLookPointerV139()) {
                return TouchRole.RIGHT_FIRE_ASSIST;
            }
            return role;
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
            // Relative swipe delta, like UT99 touch look. No virtual right-stick center.
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
                    } catch (Throwable ignored) { }
                }
            }
        }

        private boolean isLookTransparentButton(TouchRole role) {
            return role == TouchRole.FIRE || role == TouchRole.ALTFIRE || role == TouchRole.CROUCH;
        }

        private void setLookButtonPressed(TouchRole role, boolean down) {
            if (role == TouchRole.FIRE) {
                setFireButtonV139(down);
            } else if (role == TouchRole.ALTFIRE) {
                if (altFire != down) {
                    altFire = down;
                    setDirectTouchButtonV136(TOUCH_DIRECT_ALT_FIRE_V136, down);
                }
            } else if (role == TouchRole.CROUCH) {
                if (crouch != down) {
                    crouch = down;
                    setDirectTouchButtonV136(TOUCH_DIRECT_CROUCH_V136, down);
                }
            }
        }

        private void syncFireV139() {
            boolean wantFire = fireButton || rightFireAssistCount > 0;
            if (fire != wantFire) {
                fire = wantFire;
                setDirectTouchButtonV136(TOUCH_DIRECT_FIRE_V136, wantFire);
            }
        }

        private void setFireButtonV139(boolean down) {
            if (fireButton != down) {
                fireButton = down;
                syncFireV139();
            }
        }

        private void addRightFireAssistV139() {
            rightFireAssistCount++;
            syncFireV139();
        }

        private void removeRightFireAssistV139() {
            if (rightFireAssistCount > 0) rightFireAssistCount--;
            syncFireV139();
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
                            android.util.Log.i(TAG, "touch left-stick lx=" + lx + " ly=" + ly);
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
                                android.util.Log.i(TAG, "touch right-look rx=" + rx + " ry=" + ry);
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
                    setFireButtonV139(down);
                    break;
                case ALTFIRE:
                    if (altFire != down) {
                        altFire = down;
                        setDirectTouchButtonV136(TOUCH_DIRECT_ALT_FIRE_V136, down);
                    }
                    break;
                case JUMP:
                    if (jump != down) {
                        jump = down;
                        setDirectTouchButtonV136(TOUCH_DIRECT_JUMP_V136, down);
                    }
                    break;
                case CROUCH:
                    if (crouch != down) {
                        crouch = down;
                        setDirectTouchButtonV136(TOUCH_DIRECT_CROUCH_V136, down);
                    }
                    break;
                case NEXT:
                    if (next != down) {
                        next = down;
                        setDirectTouchButtonV136(TOUCH_DIRECT_NEXT_V136, down);
                    }
                    break;
                case SAVE:
                    if (quickSave != down) {
                        quickSave = down;
                        setKeyboardAndButton(KeyEvent.KEYCODE_F6, down);
                    }
                    break;
                case LOAD:
                    if (quickLoad != down) {
                        quickLoad = down;
                        setKeyboardAndButton(KeyEvent.KEYCODE_F7, down);
                    }
                    break;
                case CONSOLE:
                    if (down && !console) {
                        console = true;
                        openConsoleInputV146();
                    } else if (!down) {
                        console = false;
                    }
                    break;
                case MENU:
                    if (menu != down) {
                        menu = down;
                        setButton(KeyEvent.KEYCODE_MENU, down);
                    }
                    break;
                case DPAD_UP:
                    if (dpadUp != down) {
                        dpadUp = down;
                        setButton(KeyEvent.KEYCODE_DPAD_UP, down);
                    }
                    break;
                case DPAD_DOWN:
                    if (dpadDown != down) {
                        dpadDown = down;
                        setButton(KeyEvent.KEYCODE_DPAD_DOWN, down);
                    }
                    break;
                case DPAD_LEFT:
                    if (dpadLeft != down) {
                        dpadLeft = down;
                        setButton(KeyEvent.KEYCODE_DPAD_LEFT, down);
                    }
                    break;
                case DPAD_RIGHT:
                    if (dpadRight != down) {
                        dpadRight = down;
                        setButton(KeyEvent.KEYCODE_DPAD_RIGHT, down);
                    }
                    break;
                case DPAD_CENTER:
                    if (dpadCenter != down) {
                        dpadCenter = down;
                        setButton(KeyEvent.KEYCODE_BUTTON_A, down);
                    }
                    break;
                case EDIT:
                case NONE:
                    break;
            }
        }


        private void openConsoleInputV146() {
            // V147: the real UE1 Type line is hard to see on Android and may be
            // hidden by the IME. Show a small Android command field at the top;
            // when the user presses Done/Enter, replay Tab + text + Enter into SDL.
            activity.openConsoleCommandOverlayV147();
        }

        private void setDirectTouchButtonV136(int directKeyCode, boolean down) {
            try {
                nativeAndroidControllerKey(
                        -136, 0, 0, directKeyCode, 0,
                        down ? KeyEvent.ACTION_DOWN : KeyEvent.ACTION_UP,
                        0, InputDevice.SOURCE_GAMEPAD, "UnrealTouchButtonDirectV139");
            } catch (Throwable ignored) { }
        }

        private void setButton(int keyCode, boolean down) {
            try {
                nativeAndroidControllerKey(
                        -124, 0, 0, keyCode, 0,
                        down ? KeyEvent.ACTION_DOWN : KeyEvent.ACTION_UP,
                        0, InputDevice.SOURCE_GAMEPAD, "UnrealTouchOverlayV139");
            } catch (Throwable ignored) { }
        }

        private void setKeyboardAndButton(int keyCode, boolean down) {
            // V143: F6/F7 are not controller buttons, so the native controller bridge
            // may ignore them. Send them directly into SDL keyboard input too.
            try {
                if (down) {
                    SDLActivity.onNativeKeyDown(keyCode);
                } else {
                    SDLActivity.onNativeKeyUp(keyCode);
                }
            } catch (Throwable ignored) { }
            setButton(keyCode, down);
        }

        private void sendNativeMotion() {
            try {
                nativeAndroidControllerMotion(
                        -124, 0, 0, InputDevice.SOURCE_JOYSTICK, "UnrealTouchOverlayV139",
                        lx, ly,
                        0f, 0f,
                        0f, 0f,
                        0f, 0f,
                        0f, 0f);
            } catch (Throwable ignored) { }
        }

        private void releaseAll() {
            lookButtonPointers.clear();

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
            if (altFire) {
                altFire = false;
                setDirectTouchButtonV136(TOUCH_DIRECT_ALT_FIRE_V136, false);
            }
            if (jump) {
                jump = false;
                setDirectTouchButtonV136(TOUCH_DIRECT_JUMP_V136, false);
            }
            if (crouch) {
                crouch = false;
                setDirectTouchButtonV136(TOUCH_DIRECT_CROUCH_V136, false);
            }
            if (next) {
                next = false;
                setDirectTouchButtonV136(TOUCH_DIRECT_NEXT_V136, false);
            }
            if (quickSave) {
                quickSave = false;
                setKeyboardAndButton(KeyEvent.KEYCODE_F6, false);
            }
            if (quickLoad) {
                quickLoad = false;
                setKeyboardAndButton(KeyEvent.KEYCODE_F7, false);
            }
            if (console) {
                console = false;
            }
            if (menu) {
                menu = false;
                setButton(KeyEvent.KEYCODE_MENU, false);
            }
            if (dpadUp) {
                dpadUp = false;
                setButton(KeyEvent.KEYCODE_DPAD_UP, false);
            }
            if (dpadDown) {
                dpadDown = false;
                setButton(KeyEvent.KEYCODE_DPAD_DOWN, false);
            }
            if (dpadLeft) {
                dpadLeft = false;
                setButton(KeyEvent.KEYCODE_DPAD_LEFT, false);
            }
            if (dpadRight) {
                dpadRight = false;
                setButton(KeyEvent.KEYCODE_DPAD_RIGHT, false);
            }
            if (dpadCenter) {
                dpadCenter = false;
                setButton(KeyEvent.KEYCODE_BUTTON_A, false);
            }
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
