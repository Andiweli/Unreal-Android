package com.ast.unreal;

import android.os.Build;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;

import java.util.Locale;

/**
 * Decides whether a real game controller is attached, for the automatic
 * touch/controller mode. RetroTouchControllers.isControllerConnected() counts every
 * input device that claims a GAMEPAD or JOYSTICK source. Several phones expose their
 * fingerprint sensor (uinput-fpc, uinput-goodix, gf_input, ...) that way, which made
 * the game think a controller was connected and hid the touch controls.
 * UNREAL_ANDROID_CONTROLLER_DETECT_V231
 */
final class UnrealControllerDetector {
    private static final String TAG = "UE1RetroTouch";

    // Built-in devices that report gamepad sources but are not game controllers.
    private static final String[] NON_CONTROLLER_NAME_PARTS = {
            "fingerprint", "fpc", "goodix", "gf_input", "uinput_nav", "uinput-nav",
            "silead", "egistec", "elan_fp", "qbt1000", "sunwave", "chipone_fp"
    };

    private static String lastLogSignature = "";

    private UnrealControllerDetector() {
    }

    static boolean isControllerConnected() {
        boolean found = false;
        StringBuilder log = new StringBuilder();
        for (int id : InputDevice.getDeviceIds()) {
            InputDevice device = InputDevice.getDevice(id);
            if (device == null || !hasGamepadSource(device.getSources())) continue;
            String reason = rejectReason(device);
            if (reason == null) found = true;
            log.append(" [").append(device.getName()).append(reason == null ? ": controller" : ": ignored, " + reason).append(']');
        }
        String signature = log.toString();
        if (!signature.equals(lastLogSignature)) {
            lastLogSignature = signature;
            android.util.Log.i(TAG, "UNREAL_ANDROID_CONTROLLER_DETECT_V231 controller=" + found
                    + (signature.length() > 0 ? signature : " [no gamepad devices]"));
        }
        return found;
    }

    private static boolean hasGamepadSource(int sources) {
        return (sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                || (sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
    }

    /** Returns null for a real controller, otherwise why the device does not count. */
    private static String rejectReason(InputDevice device) {
        if (device.isVirtual()) return "virtual";
        String name = device.getName() != null ? device.getName().toLowerCase(Locale.US) : "";
        for (String part : NON_CONTROLLER_NAME_PARTS) {
            if (name.contains(part)) return "fingerprint/system device";
        }
        // Real pads (OUYA, Retroid, Xbox, PlayStation, 8BitDo, ...) report analog sticks
        // or a hat D-pad. Button-only devices must at least have A and B buttons.
        if (hasJoystickAxis(device)) return null;
        if (Build.VERSION.SDK_INT >= 19) {
            boolean[] keys = device.hasKeys(KeyEvent.KEYCODE_BUTTON_A, KeyEvent.KEYCODE_BUTTON_B);
            if (keys.length == 2 && keys[0] && keys[1]) return null;
            return "no sticks and no A/B buttons";
        }
        return null;
    }

    private static boolean hasJoystickAxis(InputDevice device) {
        int[] axes = { MotionEvent.AXIS_X, MotionEvent.AXIS_Y, MotionEvent.AXIS_HAT_X, MotionEvent.AXIS_HAT_Y };
        for (int axis : axes) {
            if (device.getMotionRange(axis, InputDevice.SOURCE_JOYSTICK) != null) return true;
        }
        return false;
    }
}
