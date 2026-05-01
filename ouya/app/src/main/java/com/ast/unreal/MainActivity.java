package com.ast.unreal;

import android.Manifest;
import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.util.Locale;

public class MainActivity extends Activity {
    private static final int REQ_LEGACY_STORAGE = 2001;
    private static final int REQ_SELECT_UNREAL_FOLDER = 3001;
    private static final int REQ_SELECT_UNREAL_ZIP = 3002;

    private File selectedRoot;
    private String lastImportMessage;

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
        selectedRoot = maybeCopyFirstFoundDataToAppRoot(selectedRoot);

        UnrealDataPaths.ensureDirectoryLayout(selectedRoot);
        UnrealDataPaths.installDefaultConfigsIfNeeded(this, selectedRoot);
        UnrealDataPaths.normalizeConfigForDetectedData(selectedRoot);

        if (UnrealDataPaths.hasRequiredData(selectedRoot, true)) {
            android.util.Log.i(UnrealDataPaths.TAG_STARTUP, "data check OK root=" + selectedRoot.getAbsolutePath());
            launchGame(selectedRoot);
            return;
        }
        android.util.Log.w(UnrealDataPaths.TAG_STARTUP, "data check failed root=" + selectedRoot.getAbsolutePath());
        showMissingDataScreen();
    }

    private File maybeCopyFirstFoundDataToAppRoot(File root) {
        File appRoot = UnrealDataPaths.primaryAppRoot(this);
        if (UnrealDataPaths.hasRequiredData(root, true)
                && !UnrealDataPaths.sameFile(root, appRoot)
                && !UnrealDataPaths.hasRequiredData(appRoot)) {
            android.util.Log.i(UnrealDataPaths.TAG_STARTUP, "copying first valid Unreal data root to app data folder: from=" + root.getAbsolutePath() + " to=" + appRoot.getAbsolutePath());
            UnrealDataPaths.ensureDirectoryLayout(appRoot);
            if (UnrealDataPaths.copyUnrealDataTree(root, appRoot)) {
                android.util.Log.i(UnrealDataPaths.TAG_STARTUP, "copy to app data folder finished: " + appRoot.getAbsolutePath());
                return appRoot;
            }
            android.util.Log.w(UnrealDataPaths.TAG_STARTUP, "copy to app data folder failed; using existing source root: " + root.getAbsolutePath());
        }
        return root;
    }

    private void launchGame(File root) {
        Intent intent = new Intent(this, UnrealSDLActivity.class);
        intent.putExtra(UnrealDataPaths.EXTRA_UNREAL_ROOT, root.getAbsolutePath());
        startActivity(intent);
        finish();
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

    private Locale currentLocale() {
        if (Build.VERSION.SDK_INT >= 24) return getResources().getConfiguration().getLocales().get(0);
        return getResources().getConfiguration().locale;
    }

    private boolean isGermanUi() {
        Locale locale = currentLocale();
        return locale != null && "de".equalsIgnoreCase(locale.getLanguage());
    }

    private String t(String de, String en) {
        return isGermanUi() ? de : en;
    }


    private void clearImportScreen() {
        FrameLayout blank = new FrameLayout(this);
        blank.setBackgroundColor(Color.BLACK);
        setContentView(blank);
        hideSystemUi();
    }

    private void showMissingDataScreen() {
        File root = unrealRoot();
        String appPath = UnrealDataPaths.primaryAppRoot(this).getAbsolutePath();

        LinearLayout body = new LinearLayout(this);
        body.setOrientation(LinearLayout.VERTICAL);
        body.setGravity(Gravity.CENTER);
        body.setPadding(48, 36, 48, 36);
        body.setBackgroundColor(Color.BLACK);

        TextView title = new TextView(this);
        title.setText(t("Unreal-Daten fehlen", "Unreal data not found"));
        title.setTextSize(24);
        title.setTextColor(Color.WHITE);
        title.setGravity(Gravity.CENTER);
        body.addView(title);

        TextView msg = new TextView(this);
        String extra = "";
        if (lastImportMessage != null && lastImportMessage.length() > 0) {
            extra = t("\n\nLetzte Meldung:\n", "\n\nLast message:\n") + lastImportMessage;
        }
        msg.setText(t(
                "Es wurde kein vollständig lesbarer Unreal-Datenordner gefunden.\n\n" +
                "Wähle jetzt entweder den Ordner 'Unreal' aus oder importiere direkt eine ZIP-Datei mit den Unreal-Daten.\n\n" +
                "Importziel:\n" + appPath + "\n\n" +
                "Der gewählte Ordner bzw. die ZIP-Datei muss mindestens enthalten:\n" +
                "System/Core.u\nSystem/Engine.u\nSystem/UnrealI.u oder System/UnrealShare.u\nMaps/*.unr" + extra,
                "No fully readable Unreal data folder was found.\n\n" +
                "Select the 'Unreal' folder or import a ZIP file containing the Unreal data.\n\n" +
                "Import target:\n" + appPath + "\n\n" +
                "The selected folder or ZIP file must contain at least:\n" +
                "System/Core.u\nSystem/Engine.u\nSystem/UnrealI.u or System/UnrealShare.u\nMaps/*.unr" + extra));
        msg.setTextSize(16);
        msg.setTextColor(Color.WHITE);
        msg.setGravity(Gravity.CENTER);
        msg.setPadding(0, 24, 0, 24);
        body.addView(msg);

        Button chooseFolder = new Button(this);
        chooseFolder.setText(t("Unreal-Ordner auswählen", "Select Unreal folder"));
        chooseFolder.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { openUnrealFolderPicker(); }
        });
        body.addView(chooseFolder);

        Button chooseZip = new Button(this);
        chooseZip.setText(t("Unreal-ZIP auswählen", "Select Unreal ZIP"));
        chooseZip.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) { openUnrealZipPicker(); }
        });
        body.addView(chooseZip);

        Button retry = new Button(this);
        retry.setText(t("Erneut prüfen", "Check again"));
        retry.setOnClickListener(new View.OnClickListener() {
            @Override public void onClick(View v) {
                selectedRoot = null;
                continueStartup();
            }
        });
        body.addView(retry);

        ScrollView scroll = new ScrollView(this);
        scroll.setBackgroundColor(Color.BLACK);
        scroll.addView(body);
        setContentView(scroll);
        hideSystemUi();
    }

    private void showBusyScreen(String titleText, String messageText) {
        clearImportScreen();

        LinearLayout body = new LinearLayout(this);
        body.setOrientation(LinearLayout.VERTICAL);
        body.setGravity(Gravity.CENTER);
        body.setPadding(48, 36, 48, 36);
        body.setBackgroundColor(Color.BLACK);

        TextView title = new TextView(this);
        title.setText(titleText);
        title.setTextSize(24);
        title.setTextColor(Color.WHITE);
        title.setGravity(Gravity.CENTER);
        body.addView(title);

        ProgressBar progress = new ProgressBar(this);
        progress.setIndeterminate(true);
        body.addView(progress);

        TextView msg = new TextView(this);
        msg.setText(messageText);
        msg.setTextSize(16);
        msg.setTextColor(Color.WHITE);
        msg.setGravity(Gravity.CENTER);
        msg.setPadding(0, 24, 0, 24);
        body.addView(msg);

        setContentView(body);
        hideSystemUi();
    }

    private void openUnrealFolderPicker() {
        if (Build.VERSION.SDK_INT < 21) {
            lastImportMessage = t(
                    "Dieser Android-Stand bietet keinen systemeigenen Ordnerauswahldialog. Kopiere den Ordner 'Unreal' auf USB/SD oder in den angezeigten App-Ordner und wähle 'Erneut prüfen'.",
                    "This Android version has no system folder picker. Copy the 'Unreal' folder to USB/SD or into the shown app folder and choose 'Check again'.");
            showMissingDataScreen();
            return;
        }
        try {
            Intent intent = new Intent("android.intent.action.OPEN_DOCUMENT_TREE");
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            if (Build.VERSION.SDK_INT >= 19) intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
            if (Build.VERSION.SDK_INT >= 21) intent.addFlags(Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
            clearImportScreen();
            startActivityForResult(intent, REQ_SELECT_UNREAL_FOLDER);
        } catch (ActivityNotFoundException ex) {
            lastImportMessage = t(
                    "Auf diesem Gerät wurde kein kompatibler Ordnerauswahldialog gefunden: ",
                    "No compatible folder picker was found on this device: ") + ex.getMessage();
            showMissingDataScreen();
        }
    }

    private void openUnrealZipPicker() {
        try {
            String action = Build.VERSION.SDK_INT >= 19 ? Intent.ACTION_OPEN_DOCUMENT : Intent.ACTION_GET_CONTENT;
            Intent intent = new Intent(action);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            intent.putExtra(Intent.EXTRA_LOCAL_ONLY, true);
            intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            if (Build.VERSION.SDK_INT >= 19) intent.addFlags(Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
            Intent chooser = Intent.createChooser(intent, t("Unreal-ZIP auswählen", "Select Unreal ZIP file"));
            clearImportScreen();
            startActivityForResult(chooser, REQ_SELECT_UNREAL_ZIP);
        } catch (ActivityNotFoundException ex) {
            lastImportMessage = t(
                    "Auf diesem Gerät wurde kein kompatibler Dateiauswahldialog gefunden: ",
                    "No compatible file picker was found on this device: ") + ex.getMessage();
            showMissingDataScreen();
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode != REQ_SELECT_UNREAL_FOLDER && requestCode != REQ_SELECT_UNREAL_ZIP) return;

        if (resultCode != RESULT_OK || data == null || data.getData() == null) {
            lastImportMessage = t("Auswahl abgebrochen.", "Selection cancelled.");
            showMissingDataScreen();
            return;
        }

        Uri uri = data.getData();
        takePersistableReadPermission(uri);

        if (requestCode == REQ_SELECT_UNREAL_FOLDER) importFolderInBackground(uri);
        else importZipInBackground(uri);
    }

    private void takePersistableReadPermission(Uri uri) {
        if (Build.VERSION.SDK_INT < 19) return;
        try {
            getContentResolver().takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
        } catch (Throwable t) {
            android.util.Log.w(UnrealDataPaths.TAG_IMPORT, "Could not persist read permission for " + uri + ": " + t);
        }
    }

    private void importFolderInBackground(final Uri uri) {
        showBusyScreen(
                t("Unreal-Daten werden importiert", "Importing Unreal data"),
                t("Der ausgewählte Ordner wird geprüft und in den sicheren App-Ordner kopiert.\nDas kann je nach SD-Karte einige Minuten dauern.",
                  "The selected folder is being checked and copied into the safe app folder.\nThis may take a few minutes depending on the SD card."));
        new Thread(new Runnable() {
            @Override public void run() {
                final UnrealDataPaths.ImportResult result = UnrealDataPaths.importUnrealFolderFromSaf(MainActivity.this, uri);
                runOnUiThread(new Runnable() { @Override public void run() { handleImportResult(result); }});
            }
        }, "UE1FolderImport").start();
    }

    private void importZipInBackground(final Uri uri) {
        showBusyScreen(
                t("Unreal-ZIP wird importiert", "Importing Unreal ZIP"),
                t("Die ZIP-Datei wird geprüft und in den sicheren App-Ordner entpackt.\nDas kann je nach Gerät einige Minuten dauern.",
                  "The ZIP file is being checked and extracted into the safe app folder.\nThis may take a few minutes depending on the device."));
        new Thread(new Runnable() {
            @Override public void run() {
                final UnrealDataPaths.ImportResult result = UnrealDataPaths.importUnrealZip(MainActivity.this, uri);
                runOnUiThread(new Runnable() { @Override public void run() { handleImportResult(result); }});
            }
        }, "UE1ZipImport").start();
    }

    private void handleImportResult(UnrealDataPaths.ImportResult result) {
        if (result == null) {
            lastImportMessage = t("Import fehlgeschlagen: unbekannter Fehler.", "Import failed: unknown error.");
            showMissingDataScreen();
            return;
        }
        lastImportMessage = result.message;
        if (result.ok && result.root != null) {
            selectedRoot = result.root;
            UnrealDataPaths.ensureDirectoryLayout(selectedRoot);
            UnrealDataPaths.installDefaultConfigsIfNeeded(this, selectedRoot);
            UnrealDataPaths.normalizeConfigForDetectedData(selectedRoot);
            if (UnrealDataPaths.hasRequiredData(selectedRoot, true)) {
                android.util.Log.i(UnrealDataPaths.TAG_IMPORT, "import OK root=" + selectedRoot.getAbsolutePath());
                launchGame(selectedRoot);
                return;
            }
            lastImportMessage = t("Import abgeschlossen, aber die Pflichtdateien wurden danach nicht vollständig gefunden.",
                    "Import finished, but the required files were still not found afterwards.");
        }
        showMissingDataScreen();
    }
}
