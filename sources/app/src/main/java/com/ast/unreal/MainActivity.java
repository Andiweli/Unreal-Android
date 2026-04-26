package com.ast.unreal;

import android.app.Activity;
import android.content.Intent;
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
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.util.Locale;

public class MainActivity extends Activity {
    private static final String[] UNREAL_DIRS = {
            "System", "Maps", "Textures", "Sounds", "Music", "Meshes", "Help", "Web", "Save", "Cache"
    };

    private File unrealRoot() {
        return new File(getExternalFilesDir(null), "Unreal");
    }

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        hideSystemUi();
        ensureDirectoryLayout();
        installDefaultConfigsIfNeeded();
        normalizeConfigForDetectedData();

        if (hasRequiredData()) {
            startActivity(new Intent(this, UnrealSDLActivity.class));
            finish();
            return;
        }
        showMissingDataScreen();
    }

    @Override
    protected void onResume() {
        super.onResume();
        hideSystemUi();
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

    private void ensureDirectoryLayout() {
        File root = unrealRoot();
        for (String dir : UNREAL_DIRS) {
            //noinspection ResultOfMethodCallIgnored
            new File(root, dir).mkdirs();
        }
    }

    private boolean hasRequiredData() {
        File root = unrealRoot();

        // Unreal retail v200 commonly has UnrealI.u but not UnrealShare.u.
        // Later/Gold/227/UT-derived installs may contain UnrealShare.u.
        // Do not block valid v200 data just because UnrealShare.u is absent.
        boolean hasCore = new File(root, "System/Core.u").isFile();
        boolean hasEngine = new File(root, "System/Engine.u").isFile();
        boolean hasGamePackage = new File(root, "System/UnrealI.u").isFile()
                || new File(root, "System/UnrealShare.u").isFile();
        boolean hasMap = new File(root, "Maps/Entry.unr").isFile() || hasAnyMap(new File(root, "Maps"));

        return hasCore && hasEngine && hasGamePackage && hasMap;
    }

    private boolean hasAnyMap(File mapsDir) {
        File[] files = mapsDir.listFiles((dir, name) -> name.toLowerCase(Locale.ROOT).endsWith(".unr"));
        return files != null && files.length > 0;
    }

    private void installDefaultConfigsIfNeeded() {
        File systemDir = new File(unrealRoot(), "System");
        //noinspection ResultOfMethodCallIgnored
        systemDir.mkdirs();
        copyAssetIfMissing("ue1_config/Unreal.ini", new File(systemDir, "Unreal.ini"));
        copyAssetIfMissing("ue1_config/User.ini", new File(systemDir, "User.ini"));
        copyAssetIfMissing("ue1_config/Default.ini", new File(systemDir, "Default.ini"));
        copyAssetIfMissing("ue1_config/AndroidController.ini", new File(systemDir, "AndroidController.ini"));
        copyAssetIfMissing("ue1_config/AndroidUI.ini", new File(systemDir, "AndroidUI.ini"));
    }

    private void copyAssetIfMissing(String asset, File out) {
        if (out.isFile()) return;
        try (InputStream input = getAssets().open(asset); FileOutputStream fos = new FileOutputStream(out)) {
            byte[] buf = new byte[16 * 1024];
            int read;
            while ((read = input.read(buf)) >= 0) fos.write(buf, 0, read);
        } catch (IOException ignored) {
            // Config assets are a convenience. The data screen still explains what to copy.
        }
    }


    private void normalizeConfigForDetectedData() {
        File root = unrealRoot();
        boolean hasUnrealI = new File(root, "System/UnrealI.u").isFile();
        boolean hasUnrealShare = new File(root, "System/UnrealShare.u").isFile();
        if (!hasUnrealI || hasUnrealShare) return;

        // Retail Unreal v200 commonly ships UnrealI.u but not UnrealShare.u.
        // Older overlay defaults used UnrealShare.*, which makes UGameEngine::Init() abort.
        patchPackageName(new File(root, "System/Unreal.ini"));
        patchPackageName(new File(root, "System/Default.ini"));
    }

    private void patchPackageName(File file) {
        if (!file.isFile()) return;
        try {
            String text = new String(Files.readAllBytes(file.toPath()), StandardCharsets.UTF_8);
            String patched = text
                    .replace("UnrealShare.SinglePlayer", "UnrealI.SinglePlayer")
                    .replace("UnrealShare.DeathMatchGame", "UnrealI.DeathMatchGame")
                    .replace("DefaultGame=UnrealShare.", "DefaultGame=UnrealI.")
                    .replace("DefaultServerGame=UnrealShare.", "DefaultServerGame=UnrealI.");
            if (!patched.equals(text)) {
                Files.write(file.toPath(), patched.getBytes(StandardCharsets.UTF_8));
            }
        } catch (IOException ignored) {
            // If patching fails, Unreal will still show its own fatal message/log.
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
                "Bitte kopiere deine Unreal v200/v205-Daten nach:\n\n%s\n\n" +
                "Erwartete Ordner: System, Maps, Textures, Sounds, Music, Meshes, Help, Web, Save. Mindestprüfung: System/Core.u, System/Engine.u, System/UnrealI.u oder System/UnrealShare.u sowie mindestens eine .unr-Datei in Maps/. \n\n" +
                "Hinweis: Android zeigt diesen Pfad je nach Dateimanager als Android/data/com.ast.unreal/files/Unreal/ an.",
                path));
        msg.setTextSize(16);
        msg.setGravity(Gravity.CENTER);
        msg.setPadding(0, 24, 0, 24);
        body.addView(msg);

        Button retry = new Button(this);
        retry.setText("Erneut prüfen");
        retry.setOnClickListener(v -> {
            ensureDirectoryLayout();
            installDefaultConfigsIfNeeded();
            normalizeConfigForDetectedData();
            if (hasRequiredData()) {
                startActivity(new Intent(this, UnrealSDLActivity.class));
                finish();
            }
        });
        body.addView(retry);

        ScrollView scroll = new ScrollView(this);
        scroll.addView(body);
        setContentView(scroll);
    }
}
