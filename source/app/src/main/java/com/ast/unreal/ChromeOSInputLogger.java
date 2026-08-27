package com.ast.unreal;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.os.SystemClock;
import android.provider.MediaStore;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;

/**
 * Temporary diagnostic logger for ChromeOS keyboard/mouse input.
 *
 * The logger intentionally uses MediaStore on Android 10+ so the file is visible
 * in the ChromeOS Files app without MANAGE_EXTERNAL_STORAGE. It first tries
 * Documents/Unreal and falls back to Downloads/Unreal when the Android runtime
 * does not allow a non-media file in Documents through MediaStore.Files.
 *
 * UNREAL_ANDROID_CHROMEOS_INPUT_FILELOG_DIAG1
 */
public final class ChromeOSInputLogger {
    private static final Object LOCK = new Object();
    private static final String FILE_NAME = "Unreal_ChromeOS_Input.log";
    private static final String DOCUMENTS_RELATIVE = "Documents/Unreal/"; // API16-safe literal
    private static final String DOWNLOADS_RELATIVE = Environment.DIRECTORY_DOWNLOADS + "/Unreal/";
    private static final int MAX_LINES = 20000;

    private static Context appContext;
    private static BufferedWriter writer;
    private static Uri logUri;
    private static String displayLocation = "not initialized";
    private static int lineCount;
    private static long lastMoveLogMs;

    private ChromeOSInputLogger() {
    }

    public static void init(Context context) {
        synchronized (LOCK) {
            closeLocked();
            appContext = context != null ? context.getApplicationContext() : null;
            lineCount = 0;
            lastMoveLogMs = 0L;

            if (appContext == null) {
                displayLocation = "logger unavailable (no context)";
                return;
            }

            Throwable documentsFailure = null;
            if (Build.VERSION.SDK_INT >= 29) {
                try {
                    if (Api29MediaStore.openDocuments()) {
                        displayLocation = "Documents/Unreal/" + FILE_NAME;
                    }
                } catch (Throwable t) {
                    documentsFailure = t;
                }

                if (writer == null) {
                    try {
                        if (Api29MediaStore.openDownloads()) {
                            displayLocation = "Downloads/Unreal/" + FILE_NAME;
                        }
                    } catch (Throwable ignored) {
                    }
                }
            }

            if (writer == null) {
                try {
                    File base = appContext.getExternalFilesDir("Documents");
                    if (base != null) {
                        File dir = new File(base, "Unreal");
                        if (!dir.exists()) dir.mkdirs();
                        File file = new File(dir, FILE_NAME);
                        writer = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(file, false), "UTF-8"));
                        displayLocation = file.getAbsolutePath() + " (fallback; app-specific)";
                    }
                } catch (Throwable ignored) {
                }
            }

            if (writer == null) {
                displayLocation = "logger could not create output file";
                return;
            }

            rawLocked("============================================================");
            rawLocked("Unreal Android 2.0.5 ChromeOS Input Diagnostic DIAG1");
            rawLocked("Started: " + new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.US).format(new Date()));
            rawLocked("Output: " + displayLocation);
            rawLocked("Android SDK=" + Build.VERSION.SDK_INT
                    + " release=" + Build.VERSION.RELEASE
                    + " manufacturer=" + Build.MANUFACTURER
                    + " model=" + Build.MODEL
                    + " device=" + Build.DEVICE
                    + " product=" + Build.PRODUCT);
            if (documentsFailure != null) {
                rawLocked("Documents MediaStore attempt failed; fallback may be used: "
                        + documentsFailure.getClass().getSimpleName() + ": " + safeMessage(documentsFailure));
            }
            dumpInputDevicesLocked();
            rawLocked("============================================================");
        }
    }

    @android.annotation.TargetApi(29)
    private static final class Api29MediaStore {
        private Api29MediaStore() {}

        static boolean openDocuments() throws Exception {
            return openFile(MediaStore.Files.getContentUri(MediaStore.VOLUME_EXTERNAL_PRIMARY), DOCUMENTS_RELATIVE);
        }

        static boolean openDownloads() throws Exception {
            return openFile(MediaStore.Downloads.EXTERNAL_CONTENT_URI, DOWNLOADS_RELATIVE);
        }

        private static boolean openFile(Uri collection, String relativePath) throws Exception {
            ContentResolver resolver = appContext.getContentResolver();
            Uri oldUri = findExisting(resolver, collection, relativePath);
            if (oldUri != null) {
                try {
                    resolver.delete(oldUri, null, null);
                } catch (Throwable ignored) {
                }
            }

            ContentValues values = new ContentValues();
            values.put(MediaStore.MediaColumns.DISPLAY_NAME, FILE_NAME);
            values.put(MediaStore.MediaColumns.MIME_TYPE, "text/plain");
            values.put(MediaStore.MediaColumns.RELATIVE_PATH, relativePath);
            Uri created = resolver.insert(collection, values);
            if (created == null) return false;

            OutputStream output = resolver.openOutputStream(created, "wt");
            if (output == null) {
                try {
                    resolver.delete(created, null, null);
                } catch (Throwable ignored) {
                }
                return false;
            }

            logUri = created;
            writer = new BufferedWriter(new OutputStreamWriter(output, "UTF-8"));
            return true;
        }

        private static Uri findExisting(ContentResolver resolver, Uri collection, String relativePath) {
            Cursor cursor = null;
            try {
                String[] projection = new String[] { MediaStore.MediaColumns._ID };
                String selection = MediaStore.MediaColumns.DISPLAY_NAME + "=? AND "
                        + MediaStore.MediaColumns.RELATIVE_PATH + "=?";
                String[] args = new String[] { FILE_NAME, relativePath };
                cursor = resolver.query(collection, projection, selection, args, null);
                if (cursor != null && cursor.moveToFirst()) {
                    long id = cursor.getLong(0);
                    return Uri.withAppendedPath(collection, Long.toString(id));
                }
            } catch (Throwable ignored) {
            } finally {
                if (cursor != null) cursor.close();
            }
            return null;
        }
    }

    public static String getDisplayLocation() {
        synchronized (LOCK) {
            return displayLocation;
        }
    }

    public static void close() {
        synchronized (LOCK) {
            rawLocked("Logger closing");
            closeLocked();
        }
    }

    private static void closeLocked() {
        if (writer != null) {
            try {
                writer.flush();
            } catch (Throwable ignored) {
            }
            try {
                writer.close();
            } catch (Throwable ignored) {
            }
        }
        writer = null;
        logUri = null;
    }

    public static void log(String stage, String message) {
        synchronized (LOCK) {
            if (writer == null || lineCount >= MAX_LINES) return;
            rawLocked(stage + " | " + (message != null ? message : ""));
        }
    }

    /** JNI callback target used by NSDLViewport.cpp. */
    public static void logNative(String message) {
        log("NATIVE", message);
    }

    public static void logKey(String stage, KeyEvent event) {
        if (event == null) {
            log(stage, "KeyEvent=null");
            return;
        }
        InputDevice device = event.getDevice();
        String text = "action=" + keyActionName(event.getAction())
                + " keyCode=" + event.getKeyCode() + "(" + KeyEvent.keyCodeToString(event.getKeyCode()) + ")"
                + " scanCode=" + event.getScanCode()
                + " repeat=" + event.getRepeatCount()
                + " source=0x" + Integer.toHexString(event.getSource()) + "[" + sourceNames(event.getSource()) + "]"
                + " deviceId=" + event.getDeviceId()
                + " device=" + deviceName(device)
                + " vendor=" + (device != null ? inputDeviceVendorIdV212(device) : 0)
                + " product=" + (device != null ? inputDeviceProductIdV212(device) : 0)
                + " meta=0x" + Integer.toHexString(event.getMetaState())
                + " flags=0x" + Integer.toHexString(event.getFlags())
                + " unicode=" + event.getUnicodeChar();
        log(stage, text);
    }

    public static void logMotion(String stage, MotionEvent event, View targetView) {
        if (event == null) {
            log(stage, "MotionEvent=null");
            return;
        }

        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_MOVE || action == MotionEvent.ACTION_HOVER_MOVE) {
            long now = SystemClock.uptimeMillis();
            synchronized (LOCK) {
                if (now - lastMoveLogMs < 15L) return;
                lastMoveLogMs = now;
            }
        }

        InputDevice device = event.getDevice();
        int toolType = event.getPointerCount() > 0 ? event.getToolType(0) : MotionEvent.TOOL_TYPE_UNKNOWN;
        boolean captured = false;
        if (Build.VERSION.SDK_INT >= 26 && targetView != null) {
            try {
                captured = Api26View.hasPointerCapture(targetView);
            } catch (Throwable ignored) {
            }
        }

        String text = "action=" + motionActionName(action)
                + " actionButton=" + event.getActionButton()
                + " buttonState=0x" + Integer.toHexString(event.getButtonState())
                + " source=0x" + Integer.toHexString(event.getSource()) + "[" + sourceNames(event.getSource()) + "]"
                + " tool=" + toolTypeName(toolType)
                + " deviceId=" + event.getDeviceId()
                + " device=" + deviceName(device)
                + " vendor=" + (device != null ? inputDeviceVendorIdV212(device) : 0)
                + " product=" + (device != null ? inputDeviceProductIdV212(device) : 0)
                + " x=" + f(event.getX(0))
                + " y=" + f(event.getY(0))
                + " relX=" + f(event.getAxisValue(MotionEvent.AXIS_RELATIVE_X))
                + " relY=" + f(event.getAxisValue(MotionEvent.AXIS_RELATIVE_Y))
                + " hScroll=" + f(event.getAxisValue(MotionEvent.AXIS_HSCROLL))
                + " vScroll=" + f(event.getAxisValue(MotionEvent.AXIS_VSCROLL))
                + " pointers=" + event.getPointerCount()
                + " pointerCapture=" + captured;
        log(stage, text);
    }

    public static void logPointerCapture(String stage, View view, boolean requestedState) {
        boolean actual = false;
        if (Build.VERSION.SDK_INT >= 26 && view != null) {
            try {
                actual = Api26View.hasPointerCapture(view);
            } catch (Throwable ignored) {
            }
        }
        log(stage, "requested=" + requestedState + " actual=" + actual
                + " view=" + (view != null ? view.getClass().getName() : "null"));
    }

    private static void dumpInputDevicesLocked() {
        try {
            int[] ids = InputDevice.getDeviceIds();
            rawLocked("Input devices: " + ids.length);
            for (int id : ids) {
                InputDevice d = InputDevice.getDevice(id);
                if (d == null) continue;
                rawLocked("DEVICE id=" + id
                        + " name=" + deviceName(d)
                        + " vendor=" + inputDeviceVendorIdV212(d)
                        + " product=" + inputDeviceProductIdV212(d)
                        + " sources=0x" + Integer.toHexString(d.getSources())
                        + "[" + sourceNames(d.getSources()) + "]"
                        + " keyboardType=" + d.getKeyboardType()
                        + " virtual=" + d.isVirtual());
            }
        } catch (Throwable t) {
            rawLocked("Input device enumeration failed: " + t.getClass().getSimpleName() + ": " + safeMessage(t));
        }
    }

    // UNREAL_ANDROID_API16_CHROMEOS_DIAG_V212
    private static int inputDeviceVendorIdV212(InputDevice device) {
        return device != null && Build.VERSION.SDK_INT >= 19 ? Api19Input.vendorId(device) : 0;
    }

    private static int inputDeviceProductIdV212(InputDevice device) {
        return device != null && Build.VERSION.SDK_INT >= 19 ? Api19Input.productId(device) : 0;
    }

    @android.annotation.TargetApi(19)
    private static final class Api19Input {
        private Api19Input() {}
        static int vendorId(InputDevice device) { return device.getVendorId(); }
        static int productId(InputDevice device) { return device.getProductId(); }
    }

    @android.annotation.TargetApi(26)
    private static final class Api26View {
        private Api26View() {}
        static boolean hasPointerCapture(View view) { return view.hasPointerCapture(); }
    }

    private static void rawLocked(String text) {
        if (writer == null || lineCount >= MAX_LINES) return;
        try {
            long elapsed = SystemClock.elapsedRealtime();
            writer.write(String.format(Locale.US, "%9d ms | %s", elapsed, text != null ? text : ""));
            writer.newLine();
            writer.flush();
            lineCount++;
        } catch (Throwable ignored) {
        }
    }

    private static String safeMessage(Throwable t) {
        String message = t.getMessage();
        return message != null ? message : "";
    }

    private static String deviceName(InputDevice device) {
        if (device == null) return "<none>";
        String name = device.getName();
        return name != null ? name.replace('|', '/') : "<unnamed>";
    }

    private static String keyActionName(int action) {
        switch (action) {
            case KeyEvent.ACTION_DOWN: return "DOWN";
            case KeyEvent.ACTION_UP: return "UP";
            case KeyEvent.ACTION_MULTIPLE: return "MULTIPLE";
            default: return Integer.toString(action);
        }
    }

    private static String motionActionName(int action) {
        switch (action) {
            case MotionEvent.ACTION_DOWN: return "DOWN";
            case MotionEvent.ACTION_UP: return "UP";
            case MotionEvent.ACTION_MOVE: return "MOVE";
            case MotionEvent.ACTION_CANCEL: return "CANCEL";
            case MotionEvent.ACTION_OUTSIDE: return "OUTSIDE";
            case MotionEvent.ACTION_POINTER_DOWN: return "POINTER_DOWN";
            case MotionEvent.ACTION_POINTER_UP: return "POINTER_UP";
            case MotionEvent.ACTION_HOVER_MOVE: return "HOVER_MOVE";
            case MotionEvent.ACTION_SCROLL: return "SCROLL";
            case MotionEvent.ACTION_HOVER_ENTER: return "HOVER_ENTER";
            case MotionEvent.ACTION_HOVER_EXIT: return "HOVER_EXIT";
            case MotionEvent.ACTION_BUTTON_PRESS: return "BUTTON_PRESS";
            case MotionEvent.ACTION_BUTTON_RELEASE: return "BUTTON_RELEASE";
            default: return Integer.toString(action);
        }
    }

    private static String toolTypeName(int type) {
        switch (type) {
            case MotionEvent.TOOL_TYPE_FINGER: return "FINGER";
            case MotionEvent.TOOL_TYPE_STYLUS: return "STYLUS";
            case MotionEvent.TOOL_TYPE_MOUSE: return "MOUSE";
            case MotionEvent.TOOL_TYPE_ERASER: return "ERASER";
            case MotionEvent.TOOL_TYPE_UNKNOWN:
            default: return "UNKNOWN(" + type + ")";
        }
    }

    private static String sourceNames(int source) {
        StringBuilder sb = new StringBuilder();
        addSource(sb, source, InputDevice.SOURCE_KEYBOARD, "KEYBOARD");
        addSource(sb, source, InputDevice.SOURCE_DPAD, "DPAD");
        addSource(sb, source, InputDevice.SOURCE_GAMEPAD, "GAMEPAD");
        addSource(sb, source, InputDevice.SOURCE_JOYSTICK, "JOYSTICK");
        addSource(sb, source, InputDevice.SOURCE_MOUSE, "MOUSE");
        if (Build.VERSION.SDK_INT >= 26) {
            addSource(sb, source, InputDevice.SOURCE_MOUSE_RELATIVE, "MOUSE_RELATIVE");
        }
        addSource(sb, source, InputDevice.SOURCE_TOUCHSCREEN, "TOUCHSCREEN");
        addSource(sb, source, InputDevice.SOURCE_TOUCHPAD, "TOUCHPAD");
        if (sb.length() == 0) sb.append("unknown");
        return sb.toString();
    }

    private static void addSource(StringBuilder sb, int source, int flag, String name) {
        if ((source & flag) == flag) {
            if (sb.length() > 0) sb.append(',');
            sb.append(name);
        }
    }

    private static String f(float value) {
        return String.format(Locale.US, "%.3f", value);
    }
}
