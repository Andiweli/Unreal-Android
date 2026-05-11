package com.ast.unreal;

import android.content.pm.ActivityInfo;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

import java.io.File;

import org.libsdl.app.SDLActivity;

public class UnrealSDLActivity extends SDLActivity {
    private File selectedRoot;

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
}
