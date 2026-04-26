package com.ast.unreal;


import android.util.Log;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import android.os.Bundle;
import android.content.pm.ActivityInfo;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.os.Handler;
import android.os.Looper;
import android.graphics.Color;

import org.libsdl.app.SDLActivity;


public class UnrealSDLActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL2", "openal", "Unreal" };
    }

    @Override
    protected String[] getArguments() {
        // Keep UE1's command line empty on Android. The engine treats unknown
        // KEY=value tokens as map URLs, so paths such as ANDROIDROOT=/... must
        // never be passed through argv. The native launcher derives the data
        // root from SDL_AndroidGetExternalStoragePath() instead.
        return new String[] { };
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
        ensureWritableConfigFiles(); // UNREAL_ANDROID_CONFIG_BOOTSTRAP_REV29
        super.onCreate(savedInstanceState);
        hideSystemUi();
        scheduleImmersiveRefresh();
    }

    @Override
    protected void onResume() {
        super.onResume();
        hideSystemUi();
        scheduleImmersiveRefresh();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            hideSystemUi();
            scheduleImmersiveRefresh();
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

    // UNREAL_ANDROID_CONFIG_BOOTSTRAP_REV29
    // Make sure UE1 has writable config targets before the native engine starts.
    // Original game data commonly contains DefUser.ini but not User.ini.
    // UE1 then keeps settings only in memory unless User.ini exists in System/.
    private void ensureWritableConfigFiles() {
        final String tag = "UE1Config";

        try {
            File base = getExternalFilesDir(null);
            if (base == null) {
                base = getFilesDir();
            }

            final File unrealRoot = new File(base, "Unreal");
            final File systemDir = new File(unrealRoot, "System");

            final String[] dirs = new String[] {
                    "System", "Maps", "Textures", "Sounds", "Music",
                    "Meshes", "Help", "Web", "Save"
            };

            for (String dir : dirs) {
                File target = new File(unrealRoot, dir);
                if (!target.exists() && !target.mkdirs()) {
                    Log.w(tag, "Could not create directory: " + target.getAbsolutePath());
                }
            }

            ensureConfigFile(systemDir, "User.ini",
                    new String[] { "DefUser.ini", "DefaultUser.ini" },
                    "[DefaultPlayer]\nName=Player\nClass=UnrealShare.MaleOne\n\n[Engine.Input]\n");

            // Normally Unreal.ini is part of the copied original game data.
            // Do not overwrite it. Only create a writable fallback if it is missing.
            ensureConfigFile(systemDir, "Unreal.ini",
                    new String[] { "Default.ini", "Unreal.ini.default" },
                    "");

            Log.i(tag, "Config root: " + unrealRoot.getAbsolutePath());
            Log.i(tag, "User.ini: " + new File(systemDir, "User.ini").getAbsolutePath());
        } catch (Throwable t) {
            Log.e(tag, "Config bootstrap failed", t);
        }
    }

    private void ensureConfigFile(File systemDir, String targetName, String[] templateNames, String fallbackText)
            throws IOException {
        final String tag = "UE1Config";

        if (!systemDir.exists() && !systemDir.mkdirs()) {
            Log.w(tag, "Could not create System directory: " + systemDir.getAbsolutePath());
        }

        final File target = new File(systemDir, targetName);
        if (target.exists()) {
            Log.i(tag, targetName + " exists: " + target.getAbsolutePath());
            return;
        }

        for (String templateName : templateNames) {
            final File template = new File(systemDir, templateName);
            if (template.exists() && template.isFile()) {
                copyFile(template, target);
                Log.i(tag, targetName + " created from " + templateName);
                return;
            }
        }

        FileOutputStream out = null;
        try {
            out = new FileOutputStream(target);
            if (fallbackText != null && fallbackText.length() > 0) {
                out.write(fallbackText.getBytes("UTF-8"));
            }
            out.flush();
            Log.i(tag, targetName + " created from fallback");
        } finally {
            if (out != null) {
                out.close();
            }
        }
    }

    private void copyFile(File src, File dst) throws IOException {
        FileInputStream in = null;
        FileOutputStream out = null;
        try {
            in = new FileInputStream(src);
            out = new FileOutputStream(dst);

            byte[] buffer = new byte[64 * 1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            out.flush();
        } finally {
            if (in != null) {
                in.close();
            }
            if (out != null) {
                out.close();
            }
        }
    }
}
