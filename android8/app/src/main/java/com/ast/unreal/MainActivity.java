package com.ast.unreal;

import android.Manifest;
import android.app.Activity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.util.Locale;

public class MainActivity extends Activity {
    private static final int REQ_LEGACY_STORAGE = 2001;
    private File selectedRoot;

    private File unrealRoot() {
        if (selectedRoot == null) selectedRoot = UnrealDataPaths.findBestUnrealRoot(this);
        return selectedRoot;
    }

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        hideSystemUi();

        if (needsLegacyStoragePermission()) {
            requestPermissions(new String[] { Manifest.permission.READ_EXTERNAL_STORAGE }, REQ_LEGACY_STORAGE);
            return;
        }
        continueStartup();
    }

    @Override
    protected void onResume() {
        super.onResume();
        hideSystemUi();
    }

    private boolean needsLegacyStoragePermission() {
        if (Build.VERSION.SDK_INT < 23 || Build.VERSION.SDK_INT > 32) return false;
        if (checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) return false;
        return !UnrealDataPaths.hasRequiredData(UnrealDataPaths.primaryAppRoot(this));
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == REQ_LEGACY_STORAGE) continueStartup();
    }

    private void continueStartup() {
        selectedRoot = UnrealDataPaths.findBestUnrealRoot(this);
        UnrealDataPaths.ensureDirectoryLayout(selectedRoot);
        UnrealDataPaths.installDefaultConfigsIfNeeded(this, selectedRoot);
        UnrealDataPaths.normalizeConfigForDetectedData(selectedRoot);

        if (UnrealDataPaths.hasRequiredData(selectedRoot, true)) {
            android.util.Log.i(UnrealDataPaths.TAG_STARTUP, "data check OK root=" + selectedRoot.getAbsolutePath());
            Intent intent = new Intent(this, UnrealSDLActivity.class);
            intent.putExtra(UnrealDataPaths.EXTRA_UNREAL_ROOT, selectedRoot.getAbsolutePath());
            startActivity(intent);
            finish();
            return;
        }
        android.util.Log.w(UnrealDataPaths.TAG_STARTUP, "data check failed root=" + selectedRoot.getAbsolutePath());
        showMissingDataScreen();
    }

    private void hideSystemUi() {
        Window w = getWindow();
        View decor = w.getDecorView();
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

    private void showMissingDataScreen() {
        File root = unrealRoot();
        String path = root.getAbsolutePath();
        LinearLayout body = new LinearLayout(this);
        body.setOrientation(LinearLayout.VERTICAL);
        body.setGravity(Gravity.CENTER);
        body.setPadding(48, 36, 48, 36);

        TextView title = new TextView(this);
        title.setText("Unreal-Daten fehlen");
        title.setTextSize(24);
        title.setGravity(Gravity.CENTER);
        body.addView(title);

        TextView msg = new TextView(this);
        msg.setText(String.format(Locale.ROOT,
                "Bitte kopiere deine Unreal v200/v205-Daten bevorzugt nach:\n\n%s\n\n" +
                "Zusätzlich wird automatisch nach einem Ordner namens Unreal an diesen Stellen gesucht:\n%s\n\n" +
                "Erwartete Ordner: System, Maps, Textures, Sounds, Music, Meshes, Help, Web, Save. " +
                "Mindestprüfung: System/Core.u, System/Engine.u, System/UnrealI.u oder System/UnrealShare.u sowie mindestens eine .unr-Datei in Maps/.\n\n" +
                "Hinweis: Auf Android 11+ kann der Zugriff auf /storage/emulated/0/Unreal, /storage/sdcard0/Unreal oder /mnt/usbdrive/Unreal wegen Scoped Storage blockiert sein. Ohne MANAGE_EXTERNAL_STORAGE bleibt Android/data/com.ast.unreal/files/Unreal/ der zuverlässigste Pfad.",
                path, UnrealDataPaths.candidateDescription(this)));
        msg.setTextSize(16);
        msg.setGravity(Gravity.CENTER);
        msg.setPadding(0, 24, 0, 24);
        body.addView(msg);

        Button retry = new Button(this);
        retry.setText("Erneut prüfen");
        retry.setOnClickListener(v -> {
            selectedRoot = UnrealDataPaths.findBestUnrealRoot(this);
            UnrealDataPaths.ensureDirectoryLayout(selectedRoot);
            UnrealDataPaths.installDefaultConfigsIfNeeded(this, selectedRoot);
            UnrealDataPaths.normalizeConfigForDetectedData(selectedRoot);
            if (UnrealDataPaths.hasRequiredData(selectedRoot, true)) {
                Intent intent = new Intent(this, UnrealSDLActivity.class);
                intent.putExtra(UnrealDataPaths.EXTRA_UNREAL_ROOT, selectedRoot.getAbsolutePath());
                startActivity(intent);
                finish();
            } else {
                showMissingDataScreen();
            }
        });
        body.addView(retry);

        ScrollView scroll = new ScrollView(this);
        scroll.addView(body);
        setContentView(scroll);
    }
}
