package com.ast.unreal;

import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.Looper;
import android.view.KeyEvent;

import com.ast.retrotouch.RetroTouchAdapter;
import com.ast.retrotouch.RetroTouchControl;
import com.ast.retrotouch.RetroTouchControllers;
import com.ast.retrotouch.RetroTouchLayout;
import com.ast.retrotouch.RetroTouchMode;
import com.ast.retrotouch.RetroTouchNavigation;
import com.ast.retrotouch.RetroTouchView;

import java.util.ArrayList;
import java.util.List;

/**
 * Unreal-specific RetroTouch adapter. RetroTouch owns drawing/multitouch/editor state;
 * this class only maps Unreal's existing input actions to the native UE1 input queue.
 * UNREAL_ANDROID_RETROTOUCH_V215
 */
final class UnrealRetroTouchBridge {
    private static final String TAG = "UE1RetroTouch";
    private static final long STATE_POLL_MS = 150L;

    // Private synthetic Android key codes consumed only by NSDLViewport's RetroTouch bridge.
    // They deliberately do not overlap Android KeyEvent constants.
    private static final int RT_FIRE = 920101;
    private static final int RT_ALT_FIRE = 920102;
    private static final int RT_JUMP = 920103;
    private static final int RT_DUCK = 920104;
    private static final int RT_INVENTORY_ACTIVATE = 920105;
    private static final int RT_INVENTORY_NEXT = 920106;
    private static final int RT_INVENTORY_PREVIOUS = 920107;
    private static final int RT_CENTER_VIEW = 920108;
    private static final int RT_WALKING = 920109;
    private static final int RT_NEXT_WEAPON = 920110;
    private static final int RT_MOVE_FORWARD = 920111;
    private static final int RT_MOVE_BACKWARD = 920112;
    private static final int RT_STRAFE_LEFT = 920113;
    private static final int RT_STRAFE_RIGHT = 920114;
    private static final int RT_STRAFE_MODIFIER = 920115;
    private static final int RT_TURN_LEFT = 920116;
    private static final int RT_TURN_RIGHT = 920117;
    private static final int RT_MOUSE_LOOK = 920118;
    private static final int RT_LOOK_UP = 920119;
    private static final int RT_LOOK_DOWN = 920120;

    private static final float MOVE_PRESS = 0.20f;
    private static final float MOVE_RELEASE = 0.10f;

    private final UnrealSDLActivity activity;
    private final RetroTouchView view;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final boolean touchscreenAvailable;

    private boolean hostPaused = true;
    private boolean moveForward;
    private boolean moveBackward;
    private boolean strafeLeft;
    private boolean strafeRight;
    private boolean introTouchGestureCapturedV217;
    private Boolean reportedTouchModeV218; // UNREAL_ANDROID_RETROTOUCH_AUTOMODE_V218

    private final Runnable statePoll = new Runnable() {
        @Override public void run() {
            if (hostPaused) return;
            refreshMode();
            handler.postDelayed(this, STATE_POLL_MS);
        }
    };

    UnrealRetroTouchBridge(UnrealSDLActivity activity) {
        this.activity = activity;
        this.view = new RetroTouchView(activity);
        this.touchscreenAvailable = activity.getPackageManager()
                .hasSystemFeature(PackageManager.FEATURE_TOUCHSCREEN);

        registerUnrealActions();
        view.setGameplayLayout(buildGameplayLayout());
        view.setNavigationLayout(buildNavigationLayout());
        view.setLookWhileHoldingAction("fire", true);
        view.setLookSensitivity(1.0f);

        // Unreal already has an explicit Touch Controls setting. Keep that as the
        // authoritative visibility switch instead of temporarily hiding on controller input.
        // This preserves the old Unreal behaviour on handhelds while OUYA naturally has no touch.
        view.setAutoHideOnController(false);

        // Stay completely out of the SDL touch path until the native engine has
        // reported a safe gameplay/menu state. This also avoids a one-frame overlay
        // flash during Activity startup/resume.
        view.setMode(RetroTouchMode.OFF);

        view.setListener(new RetroTouchAdapter() {
            @Override public void onAction(String actionId, boolean pressed) {
                handleAction(actionId, pressed);
            }

            @Override public void onMove(float x, float y) {
                handleMove(x, y);
            }

            @Override public void onLook(float deltaX, float deltaY) {
                handleLook(deltaX, deltaY);
            }

            @Override public void onEditorStateChanged(boolean editing) {
                if (editing) releaseMovement();
            }
        });
    }

    RetroTouchView getView() {
        return view;
    }

    void bringToFront() {
        try {
            view.bringToFront();
            if (android.os.Build.VERSION.SDK_INT >= 21) {
                view.setElevation(10000.0f);
                view.setTranslationZ(10000.0f);
            }
            view.requestLayout();
            view.invalidate();
        } catch (Throwable ignored) {
        }
    }

    void onHostResume() {
        hostPaused = false;
        handler.removeCallbacks(statePoll);
        refreshMode();
        handler.postDelayed(statePoll, STATE_POLL_MS);
    }

    void onHostPause() {
        hostPaused = true;
        handler.removeCallbacks(statePoll);
        introTouchGestureCapturedV217 = false;
        releaseMovement();
        view.setMode(RetroTouchMode.OFF); // releases every held RetroTouch button first
    }

    void onHostDestroy() {
        onHostPause();
    }

    private void registerUnrealActions() {
        // Exact action names exposed by UnrealKeyboardMenu.uc > Customize Controls.
        view.registerAction("fire", "Fire");
        view.registerAction("alt_fire", "Alt Fire");
        view.registerAction("move_forward", "Move\nForward");
        view.registerAction("move_backward", "Move\nBackward");
        view.registerAction("turn_left", "Turn\nLeft");
        view.registerAction("turn_right", "Turn\nRight");
        view.registerAction("strafe_left", "Strafe\nLeft");
        view.registerAction("strafe_right", "Strafe\nRight");
        view.registerAction("jump", "Jump");
        view.registerAction("crouch", "Crouch");
        view.registerAction("mouse_look", "Mouse\nLook");
        view.registerAction("activate_item", "Activate\nItem");
        view.registerAction("next_item", "Next\nItem");
        view.registerAction("previous_item", "Previous\nItem");
        view.registerAction("look_up", "Look\nUp");
        view.registerAction("look_down", "Look\nDown");
        view.registerAction("center_view", "Center\nView");
        view.registerAction("walk", "Walk");
        view.registerAction("strafe", "Strafe");
        view.registerAction("next_weapon", "Next\nWeapon");

        // RetroTouch-only navigation helpers, mapped to Unreal's existing menu input.
        view.registerAction("menu", "Menu");
        view.registerAction("menu_ok", "OK");
        view.registerAction("menu_back", "Back");
    }

    private RetroTouchLayout buildGameplayLayout() {
        List<RetroTouchControl> controls = new ArrayList<RetroTouchControl>();

        // Look zone first: buttons inside it win hit-testing, while FIRE may keep looking
        // when held because RetroTouch is explicitly told to allow fire+look.
        controls.add(RetroTouchControl.lookZone("look", 0.72f, 0.50f, 0.56f, 0.86f));
        controls.add(RetroTouchControl.moveStick("move", 0.16f, 0.76f, 0.26f));

        // Default buttons mirror the actions that matter most during Unreal gameplay.
        controls.add(RetroTouchControl.button("fire_button", "fire", "Fire", 0.89f, 0.70f, 0.15f));
        controls.add(RetroTouchControl.button("alt_fire_button", "alt_fire", "Alt Fire", 0.88f, 0.50f, 0.12f));
        controls.add(RetroTouchControl.button("jump_button", "jump", "Jump", 0.73f, 0.78f, 0.12f));
        controls.add(RetroTouchControl.button("crouch_button", "crouch", "Crouch", 0.61f, 0.84f, 0.11f));
        controls.add(RetroTouchControl.button("activate_item_button", "activate_item", "Activate\nItem", 0.77f, 0.91f, 0.11f));
        controls.add(RetroTouchControl.button("next_weapon_button", "next_weapon", "Next\nWeapon", 0.92f, 0.32f, 0.11f));
        controls.add(RetroTouchControl.button("menu_button", "menu", "Menu", 0.82f, 0.13f, 0.10f));

        // Every real Customize Controls action is registered above. The remaining actions
        // (including directional Move/Turn/Look and item cycling) can therefore be assigned
        // to added buttons through RetroTouch's ACTION editor without code changes.
        return new RetroTouchLayout("unreal_android_2_1_retrotouch_gameplay_v215", controls);
    }

    private RetroTouchLayout buildNavigationLayout() {
        List<RetroTouchControl> controls = new ArrayList<RetroTouchControl>();
        controls.add(RetroTouchControl.dPad("navigation", 0.18f, 0.72f, 0.30f));
        controls.add(RetroTouchControl.button("menu_ok_button", "menu_ok", "OK", 0.87f, 0.68f, 0.14f));
        controls.add(RetroTouchControl.button("menu_back_button", "menu_back", "Back", 0.73f, 0.82f, 0.12f));
        return new RetroTouchLayout("unreal_android_2_1_retrotouch_navigation_v215", controls);
    }

    // UNREAL_ANDROID_RETROTOUCH_RESET_API_V221 UNREAL_ANDROID_RETROTOUCH_RESET_DIRECT_V222
    // Called on Android's UI thread directly from UnrealSDLActivity when UE1 executes
    // UInput::ResetInput(). Clear bridge movement flags first, then let RetroTouch beta.3
    // release all pointers/buttons/sticks. No polling is involved, so a new respawn touch
    // cannot be cancelled later by a stale engine-reset notification.
    void onEngineInputResetV222() {
        releaseMovement();
        view.resetInputState();
        introTouchGestureCapturedV217 = false;
        android.util.Log.i(TAG, "UNREAL_ANDROID_RETROTOUCH_RESET_DIRECT_V222 engine reset -> RetroTouch resetInputState");
    }

    private void refreshMode() {
        // UNREAL_ANDROID_RETROTOUCH_AUTOMODE_V218:
        // Touch Controls is now an automatic input-mode status, not a manual preference.
        // A real touchscreen with no GAMEPAD/JOYSTICK means touch mode TRUE. Any physical
        // controller (or a device without touch hardware) means FALSE. Report only changes;
        // UE1 applies the config/menu value on its own TickInput thread.
        final boolean controllerConnected = RetroTouchControllers.isControllerConnected();
        final boolean touchMode = touchscreenAvailable && !controllerConnected;
        if (reportedTouchModeV218 == null || reportedTouchModeV218.booleanValue() != touchMode) {
            if (activity.setRetroTouchInputModeV218(touchMode)) {
                reportedTouchModeV218 = Boolean.valueOf(touchMode);
                android.util.Log.i(TAG, "UNREAL_ANDROID_RETROTOUCH_AUTOMODE_V218 touchMode=" + touchMode
                        + " controller=" + controllerConnected + " touchscreen=" + touchscreenAvailable);
            }
        }

        if (view.isEditing()) return;

        if (!touchMode) {
            introTouchGestureCapturedV217 = false;
            releaseMovement();
            if (view.getMode() != RetroTouchMode.OFF) view.setMode(RetroTouchMode.OFF);
            return;
        }

        int nativeMode = activity.getRetroTouchUiModeV215();
        final RetroTouchMode wanted;
        if (nativeMode == UnrealSDLActivity.RETROTOUCH_UI_NAVIGATION_V215) {
            // Every ordinary Unreal menu gets D-pad + OK + Back.
            wanted = RetroTouchMode.NAVIGATION;
        } else if (nativeMode == UnrealSDLActivity.RETROTOUCH_UI_INTRO_TAP_V217) {
            // The Nali flyby must remain visually unobstructed. The Activity catches one
            // full-screen tap while this mode is active and turns it into Escape/Menu.
            wanted = RetroTouchMode.OFF;
        } else if (nativeMode == UnrealSDLActivity.RETROTOUCH_UI_GAMEPLAY_V215) {
            // No controller + real touchscreen always means gameplay touch. Do not gate this
            // behind the legacy bTouchControls value; that value is now written FROM this mode.
            wanted = RetroTouchMode.GAMEPLAY;
        } else {
            // KeyMenuing (Customize Controls) and typing stay completely out of the touch path.
            wanted = RetroTouchMode.OFF;
        }
        if (nativeMode != UnrealSDLActivity.RETROTOUCH_UI_INTRO_TAP_V217)
            introTouchGestureCapturedV217 = false;
        if (view.getMode() != wanted) {
            releaseMovement();
            view.setMode(wanted);
        }
    }

    /**
     * Handles the original Unreal intro semantics without drawing any overlay: the flyby starts
     * normally, and the first screen touch acts exactly like Escape/Menu. Returning true for the
     * rest of that gesture keeps a half-consumed DOWN/UP pair out of SDL.
     * UNREAL_ANDROID_RETROTOUCH_INTRO_TAP_V217
     */
    boolean handleIntroTouchV217(android.view.MotionEvent event) {
        if (event == null || hostPaused || view.isEditing() || !touchscreenAvailable) return false;
        if (RetroTouchControllers.isControllerConnected()) return false;
        if (activity.getRetroTouchUiModeV215() != UnrealSDLActivity.RETROTOUCH_UI_INTRO_TAP_V217) {
            introTouchGestureCapturedV217 = false;
            return false;
        }

        final int action = event.getActionMasked();
        if (action == android.view.MotionEvent.ACTION_DOWN) {
            if (!introTouchGestureCapturedV217) {
                introTouchGestureCapturedV217 = true;
                activity.queueRetroTouchAndroidKeyV215(KeyEvent.KEYCODE_MENU, true);
                activity.queueRetroTouchAndroidKeyV215(KeyEvent.KEYCODE_MENU, false);
                android.util.Log.i(TAG, "UNREAL_ANDROID_RETROTOUCH_INTRO_TAP_V217 touch -> menu");
            }
            return true;
        }
        if (introTouchGestureCapturedV217) {
            if (action == android.view.MotionEvent.ACTION_UP || action == android.view.MotionEvent.ACTION_CANCEL)
                introTouchGestureCapturedV217 = false;
            return true;
        }
        return false;
    }

    private void handleAction(String actionId, boolean pressed) {
        if (actionId == null) return;

        if (RetroTouchNavigation.UP.equals(actionId)) {
            activity.queueRetroTouchAndroidKeyV215(KeyEvent.KEYCODE_DPAD_UP, pressed);
            return;
        }
        if (RetroTouchNavigation.DOWN.equals(actionId)) {
            activity.queueRetroTouchAndroidKeyV215(KeyEvent.KEYCODE_DPAD_DOWN, pressed);
            return;
        }
        if (RetroTouchNavigation.LEFT.equals(actionId)) {
            activity.queueRetroTouchAndroidKeyV215(KeyEvent.KEYCODE_DPAD_LEFT, pressed);
            return;
        }
        if (RetroTouchNavigation.RIGHT.equals(actionId)) {
            activity.queueRetroTouchAndroidKeyV215(KeyEvent.KEYCODE_DPAD_RIGHT, pressed);
            return;
        }
        if ("menu_ok".equals(actionId)) {
            activity.queueRetroTouchAndroidKeyV215(KeyEvent.KEYCODE_BUTTON_A, pressed);
            return;
        }
        if ("menu_back".equals(actionId)) {
            activity.queueRetroTouchAndroidKeyV215(KeyEvent.KEYCODE_BUTTON_B, pressed);
            return;
        }
        if ("menu".equals(actionId)) {
            activity.queueRetroTouchAndroidKeyV215(KeyEvent.KEYCODE_MENU, pressed);
            return;
        }

        int keyCode = semanticCode(actionId);
        if (keyCode != 0) activity.queueRetroTouchAndroidKeyV215(keyCode, pressed);
    }

    private int semanticCode(String actionId) {
        if ("fire".equals(actionId)) return RT_FIRE;
        if ("alt_fire".equals(actionId)) return RT_ALT_FIRE;
        if ("jump".equals(actionId)) return RT_JUMP;
        if ("crouch".equals(actionId)) return RT_DUCK;
        if ("activate_item".equals(actionId)) return RT_INVENTORY_ACTIVATE;
        if ("next_item".equals(actionId)) return RT_INVENTORY_NEXT;
        if ("previous_item".equals(actionId)) return RT_INVENTORY_PREVIOUS;
        if ("center_view".equals(actionId)) return RT_CENTER_VIEW;
        if ("walk".equals(actionId)) return RT_WALKING;
        if ("strafe".equals(actionId)) return RT_STRAFE_MODIFIER;
        if ("next_weapon".equals(actionId)) return RT_NEXT_WEAPON;
        if ("move_forward".equals(actionId)) return RT_MOVE_FORWARD;
        if ("move_backward".equals(actionId)) return RT_MOVE_BACKWARD;
        if ("strafe_left".equals(actionId)) return RT_STRAFE_LEFT;
        if ("strafe_right".equals(actionId)) return RT_STRAFE_RIGHT;
        if ("turn_left".equals(actionId)) return RT_TURN_LEFT;
        if ("turn_right".equals(actionId)) return RT_TURN_RIGHT;
        if ("mouse_look".equals(actionId)) return RT_MOUSE_LOOK;
        if ("look_up".equals(actionId)) return RT_LOOK_UP;
        if ("look_down".equals(actionId)) return RT_LOOK_DOWN;
        return 0;
    }

    private void handleMove(float x, float y) {
        boolean wantForward = moveForward ? y < -MOVE_RELEASE : y < -MOVE_PRESS;
        boolean wantBackward = moveBackward ? y > MOVE_RELEASE : y > MOVE_PRESS;
        boolean wantLeft = strafeLeft ? x < -MOVE_RELEASE : x < -MOVE_PRESS;
        boolean wantRight = strafeRight ? x > MOVE_RELEASE : x > MOVE_PRESS;

        moveForward = setSemanticState(RT_MOVE_FORWARD, moveForward, wantForward);
        moveBackward = setSemanticState(RT_MOVE_BACKWARD, moveBackward, wantBackward);
        strafeLeft = setSemanticState(RT_STRAFE_LEFT, strafeLeft, wantLeft);
        strafeRight = setSemanticState(RT_STRAFE_RIGHT, strafeRight, wantRight);
    }

    private boolean setSemanticState(int keyCode, boolean current, boolean wanted) {
        if (current != wanted) activity.queueRetroTouchAndroidKeyV215(keyCode, wanted);
        return wanted;
    }

    private void releaseMovement() {
        if (moveForward) activity.queueRetroTouchAndroidKeyV215(RT_MOVE_FORWARD, false);
        if (moveBackward) activity.queueRetroTouchAndroidKeyV215(RT_MOVE_BACKWARD, false);
        if (strafeLeft) activity.queueRetroTouchAndroidKeyV215(RT_STRAFE_LEFT, false);
        if (strafeRight) activity.queueRetroTouchAndroidKeyV215(RT_STRAFE_RIGHT, false);
        moveForward = moveBackward = strafeLeft = strafeRight = false;
    }

    private void handleLook(float deltaX, float deltaY) {
        // RetroTouch reports delta/base. Recreate the proven Unreal v134 swipe model
        // exactly: 0.25 px jitter filter, deltaPx * 0.021 gain and per-event clamp.
        // That keeps the already-tested Unreal look feel while remaining resolution-independent.
        float base = Math.max(1.0f, Math.min(view.getWidth(), view.getHeight()));
        float pxX = deltaX * base;
        float pxY = deltaY * base;
        float lookX = Math.abs(pxX) < 0.25f ? 0.0f : clamp(pxX * 0.0210f, -1.0f, 1.0f);
        float lookY = Math.abs(pxY) < 0.25f ? 0.0f : clamp(pxY * 0.0210f, -1.0f, 1.0f);
        if (lookX != 0.0f || lookY != 0.0f) activity.pushRetroTouchLookV215(lookX, lookY);
    }

    private static float clamp(float value, float min, float max) {
        return value < min ? min : (value > max ? max : value);
    }
}
