package com.ast.unreal;

import android.content.pm.ActivityInfo;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
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
        return new String[] { "c++_shared", "SDL2", "openal", "Unreal" };
    }

    @Override
    protected String[] getArguments() {
        selectedRoot = selectedRootFromIntentOrScan();
        return new String[] { "--ue1-root", selectedRoot.getAbsolutePath() };
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        getWindow().getDecorView().setBackgroundColor(Color.BLACK);
        hideSystemUi();
        selectedRoot = selectedRootFromIntentOrScan();
        UnrealDataPaths.ensureWritableConfigFiles(this, selectedRoot);
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
        handler.postDelayed(new Runnable() { @Override public void run() { hideSystemUi(); } }, 50);
        handler.postDelayed(new Runnable() { @Override public void run() { hideSystemUi(); } }, 250);
        handler.postDelayed(new Runnable() { @Override public void run() { hideSystemUi(); } }, 750);
    }

    private void hideSystemUi() {
        View decor = getWindow().getDecorView();
        decor.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_FULLSCREEN |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        );
    }
}
