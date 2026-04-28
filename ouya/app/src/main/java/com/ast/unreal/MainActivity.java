package com.ast.unreal;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.util.Locale;

public class MainActivity extends Activity {
    private File selectedRoot;

    private File unrealRoot() {
        if (selectedRoot == null) selectedRoot = UnrealDataPaths.findBestUnrealRoot(this);
        return selectedRoot;
    }

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        hideSystemUi();
        continueStartup();
    }

    @Override
    protected void onResume() {
        super.onResume();
        hideSystemUi();
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
        View decor = getWindow().getDecorView();
        decor.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_FULLSCREEN |
                View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        );
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
                "Mindestprüfung: System/Core.u, System/Engine.u, System/UnrealI.u oder System/UnrealShare.u sowie mindestens eine .unr-Datei in Maps/.",
                path, UnrealDataPaths.candidateDescription(this)));
        msg.setTextSize(16);
        msg.setGravity(Gravity.CENTER);
        msg.setPadding(0, 24, 0, 24);
        body.addView(msg);

        Button retry = new Button(this);
        retry.setText("Erneut prüfen");
        retry.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                selectedRoot = UnrealDataPaths.findBestUnrealRoot(MainActivity.this);
                UnrealDataPaths.ensureDirectoryLayout(selectedRoot);
                UnrealDataPaths.installDefaultConfigsIfNeeded(MainActivity.this, selectedRoot);
                UnrealDataPaths.normalizeConfigForDetectedData(selectedRoot);
                if (UnrealDataPaths.hasRequiredData(selectedRoot, true)) {
                    Intent intent = new Intent(MainActivity.this, UnrealSDLActivity.class);
                    intent.putExtra(UnrealDataPaths.EXTRA_UNREAL_ROOT, selectedRoot.getAbsolutePath());
                    startActivity(intent);
                    finish();
                } else {
                    showMissingDataScreen();
                }
            }
        });
        body.addView(retry);

        ScrollView scroll = new ScrollView(this);
        scroll.addView(body);
        setContentView(scroll);
    }
}
