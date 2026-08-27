package com.ast.unreal;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.view.Gravity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;

public class MainActivity extends Activity {
    private static final int REQ_LEGACY_STORAGE = 2001;
    private static final int REQ_SELECT_UNREAL_FOLDER = 3001;
    private static final int REQ_SELECT_UNREAL_ZIP = 3002;
    private static final String DEFAULT_ONLINE_UNREAL_ZIP_URL = "http://ouya.cweiske.de/apks/com.ast.unreal/Unreal_v1.200.zip";

    private File selectedRoot;
    private String lastImportMessage;
    private ProgressBar installProgressBar;
    private TextView installProgressText;
    private TextView installMessageText;

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

    private boolean isOuyaDevice() {
        String model = String.valueOf(Build.MODEL).toLowerCase(Locale.US);
        String manufacturer = String.valueOf(Build.MANUFACTURER).toLowerCase(Locale.US);
        String product = String.valueOf(Build.PRODUCT).toLowerCase(Locale.US);
        return model.contains("ouya") || manufacturer.contains("ouya") || product.contains("ouya");
    }

    private boolean isLegacyZipPickerDevice() {
        return Build.VERSION.SDK_INT <= 17 || isOuyaDevice();
    }

    private Button makeButton(String label, View.OnClickListener listener) {
        Button button = new Button(this);
        button.setText(label);
        button.setAllCaps(false);
        button.setOnClickListener(listener);
        return button;
    }


    private void clearImportScreen() {
        FrameLayout blank = new FrameLayout(this);
        blank.setBackgroundColor(Color.BLACK);
        setContentView(blank);
        hideSystemUi();
    }

    private void showMissingDataScreen() {
        String appPath = UnrealDataPaths.primaryAppRoot(this).getAbsolutePath();
        final boolean ouyaMode = isOuyaDevice();
        final boolean hasLaunchData = selectedRoot != null && UnrealDataPaths.hasRequiredData(selectedRoot, true);

        LinearLayout body = new LinearLayout(this);
        body.setOrientation(LinearLayout.VERTICAL);
        body.setGravity(Gravity.CENTER);
        body.setPadding(48, 36, 48, 36);

        TextView title = new TextView(this);
        title.setText(hasLaunchData
                ? t("Unreal bereit", "Unreal ready")
                : t("Unreal-Daten fehlen", "Unreal data not found"));
        title.setTextSize(24.0f);
        title.setGravity(Gravity.CENTER);
        body.addView(title);

        String extra = "";
        if (lastImportMessage != null && lastImportMessage.length() > 0) {
            extra = t("\n\nLetzte Meldung:\n", "\n\nLast message:\n") + lastImportMessage;
        }

        TextView msg = new TextView(this);
        if (hasLaunchData) {
            msg.setText(t(
                    "Spieldaten gefunden unter:\n" + selectedRoot.getAbsolutePath() + extra,
                    "Game data found at:\n" + selectedRoot.getAbsolutePath() + extra));
        } else if (ouyaMode) {
            msg.setText(t(
                    "Es wurde kein vollständig lesbarer Unreal-Datenordner gefunden.\n\n" +
                    "Auf OUYA kannst du die Spieldaten direkt online herunterladen oder eine lokale ZIP-Datei importieren.\n\n" +
                    "Installationsziel:\n" + appPath + "\n\n" +
                    "Benötigt werden mindestens:\nSystem/Core.u, System/Engine.u, UnrealI.u oder UnrealShare.u und Maps/*.unr" + extra,
                    "No fully readable Unreal data folder was found.\n\n" +
                    "On OUYA you can download the game data directly or import a local ZIP file.\n\n" +
                    "Install target:\n" + appPath + "\n\n" +
                    "Required at minimum:\nSystem/Core.u, System/Engine.u, UnrealI.u or UnrealShare.u and Maps/*.unr" + extra));
        } else {
            msg.setText(t(
                    "Es wurde kein vollständig lesbarer Unreal-Datenordner gefunden.\n\n" +
                    "Du kannst jetzt entweder den Unreal-Ordner auswählen oder eine ZIP-Datei importieren.\n\n" +
                    "Installationsziel:\n" + appPath + "\n\n" +
                    "Benötigt werden mindestens:\nSystem/Core.u, System/Engine.u, UnrealI.u oder UnrealShare.u und Maps/*.unr" + extra,
                    "No fully readable Unreal data folder was found.\n\n" +
                    "Select the Unreal folder or import a ZIP file containing the game data.\n\n" +
                    "Install target:\n" + appPath + "\n\n" +
                    "Required at minimum:\nSystem/Core.u, System/Engine.u, UnrealI.u or UnrealShare.u and Maps/*.unr" + extra));
        }
        msg.setTextSize(16.0f);
        msg.setGravity(Gravity.CENTER);
        msg.setPadding(0, 24, 0, 24);
        body.addView(msg);

        Button firstFocusButton = null;
        if (hasLaunchData) {
            firstFocusButton = makeButton(t("Unreal starten", "Start Unreal"), new View.OnClickListener() {
                @Override public void onClick(View v) { launchGame(selectedRoot); }
            });
            body.addView(firstFocusButton);
        }

        if (ouyaMode) {
            Button onlineZip = makeButton(t("Online-ZIP herunterladen", "Download online ZIP"), new View.OnClickListener() {
                @Override public void onClick(View v) { showOnlineZipDialog(); }
            });
            body.addView(onlineZip);
            if (firstFocusButton == null) firstFocusButton = onlineZip;

            Button chooseZip = makeButton(t("Lokales ZIP auswählen", "Select local ZIP"), new View.OnClickListener() {
                @Override public void onClick(View v) { openUnrealZipPicker(); }
            });
            body.addView(chooseZip);
            if (firstFocusButton == null) firstFocusButton = chooseZip;
        } else {
            Button chooseFolder = makeButton(t("Unreal-Ordner auswählen", "Select Unreal folder"), new View.OnClickListener() {
                @Override public void onClick(View v) { openUnrealFolderPicker(); }
            });
            body.addView(chooseFolder);
            if (firstFocusButton == null) firstFocusButton = chooseFolder;

            Button chooseZip = makeButton(t("Unreal-ZIP auswählen", "Select Unreal ZIP"), new View.OnClickListener() {
                @Override public void onClick(View v) { openUnrealZipPicker(); }
            });
            body.addView(chooseZip);
            if (firstFocusButton == null) firstFocusButton = chooseZip;
        }

        Button retry = makeButton(t("Erneut prüfen", "Check again"), new View.OnClickListener() {
            @Override public void onClick(View v) {
                selectedRoot = null;
                continueStartup();
            }
        });
        body.addView(retry);
        if (firstFocusButton == null) firstFocusButton = retry;

        ScrollView scroll = new ScrollView(this);
        scroll.addView(body);
        setContentView(scroll);
        hideSystemUi();
        restoreControllerFocus(firstFocusButton);
    }

    private void restoreControllerFocus(final View focusTarget) {
        if (focusTarget == null) return;
        focusTarget.setFocusable(true);
        focusTarget.setFocusableInTouchMode(true);
        focusTarget.postDelayed(new Runnable() {
            @Override public void run() {
                try { focusTarget.requestFocus(); } catch (Throwable ignored) {}
            }
        }, 80L);
    }

    private void showBusyScreen(String titleText, String messageText) {
        LinearLayout body = new LinearLayout(this);
        body.setOrientation(LinearLayout.VERTICAL);
        body.setGravity(Gravity.CENTER);
        body.setPadding(48, 36, 48, 36);

        TextView title = new TextView(this);
        title.setText(titleText);
        title.setTextSize(24.0f);
        title.setGravity(Gravity.CENTER);
        body.addView(title);

        installMessageText = new TextView(this);
        installMessageText.setText(messageText);
        installMessageText.setTextSize(16.0f);
        installMessageText.setGravity(Gravity.CENTER);
        installMessageText.setPadding(0, 24, 0, 16);
        body.addView(installMessageText);

        installProgressBar = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
        installProgressBar.setIndeterminate(false);
        installProgressBar.setMax(100);
        installProgressBar.setProgress(0);
        int progressWidth = Math.max(240, getResources().getDisplayMetrics().widthPixels / 2);
        LinearLayout.LayoutParams progressParams = new LinearLayout.LayoutParams(progressWidth, LinearLayout.LayoutParams.WRAP_CONTENT);
        progressParams.gravity = Gravity.CENTER_HORIZONTAL;
        body.addView(installProgressBar, progressParams);

        installProgressText = new TextView(this);
        installProgressText.setText("0%");
        installProgressText.setTextSize(16.0f);
        installProgressText.setGravity(Gravity.CENTER);
        installProgressText.setPadding(0, 8, 0, 0);
        body.addView(installProgressText);

        setContentView(body);
        hideSystemUi();
    }

    private void updateInstallMessage(final String message) {
        runOnUiThread(new Runnable() {
            @Override public void run() {
                if (installMessageText != null) installMessageText.setText(message);
            }
        });
    }

    private void updateInstallProgress(final String phase, final int percent) {
        final int safePercent = Math.max(0, Math.min(100, percent));
        runOnUiThread(new Runnable() {
            @Override public void run() {
                if (installProgressBar != null) installProgressBar.setProgress(safePercent);
                if (installProgressText != null) {
                    if (phase != null && phase.length() > 0) {
                        installProgressText.setText(phase + " " + safePercent + "%");
                    } else {
                        installProgressText.setText(safePercent + "%");
                    }
                }
            }
        });
    }

    private UnrealDataPaths.ProgressCallback installProgressCallback() {
        return new UnrealDataPaths.ProgressCallback() {
            @Override public void onProgress(String phase, int percent) {
                updateInstallProgress(phase, percent);
            }
        };
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
        if (isLegacyZipPickerDevice()) {
            openLegacyZipPicker(legacyZipStartDir());
            return;
        }
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
            if (isLegacyZipPickerDevice()) {
                openLegacyZipPicker(legacyZipStartDir());
                return;
            }
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
                final UnrealDataPaths.ImportResult result = UnrealDataPaths.importUnrealFolderFromSaf(MainActivity.this, uri, installProgressCallback());
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
                final UnrealDataPaths.ImportResult result = UnrealDataPaths.importUnrealZip(MainActivity.this, uri, installProgressCallback());
                runOnUiThread(new Runnable() { @Override public void run() { handleImportResult(result); }});
            }
        }, "UE1ZipImport").start();
    }

    private void showOnlineZipDialog() {
        final EditText input = new EditText(this);
        input.setSingleLine(true);
        input.setText(DEFAULT_ONLINE_UNREAL_ZIP_URL);
        input.setSelectAllOnFocus(false);
        input.setSelection(input.getText().length());

        final AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle(t("Online-ZIP herunterladen", "Download online ZIP"))
                .setMessage(t("Download-URL:", "Download URL:"))
                .setView(input)
                .setPositiveButton(t("Download starten", "Start download"), new android.content.DialogInterface.OnClickListener() {
                    @Override public void onClick(android.content.DialogInterface d, int which) {
                        String url = String.valueOf(input.getText()).trim();
                        if (url.length() == 0) {
                            lastImportMessage = t("Keine Download-URL eingegeben.", "No download URL entered.");
                            showMissingDataScreen();
                            return;
                        }
                        importOnlineZipInBackground(url);
                    }
                })
                .setNegativeButton(t("Abbrechen", "Cancel"), new android.content.DialogInterface.OnClickListener() {
                    @Override public void onClick(android.content.DialogInterface d, int which) { showMissingDataScreen(); }
                })
                .create();

        dialog.setOnCancelListener(new android.content.DialogInterface.OnCancelListener() {
            @Override public void onCancel(android.content.DialogInterface d) { showMissingDataScreen(); }
        });
        dialog.setOnShowListener(new android.content.DialogInterface.OnShowListener() {
            @Override public void onShow(android.content.DialogInterface d) {
                try {
                    Button positive = dialog.getButton(AlertDialog.BUTTON_POSITIVE);
                    if (positive != null) {
                        positive.setFocusable(true);
                        positive.requestFocus();
                    }
                } catch (Throwable ignored) {}
            }
        });
        dialog.show();
    }

    private void importOnlineZipInBackground(final String urlText) {
        showBusyScreen(
                t("Installiere Unreal-Daten", "Installing Unreal data"),
                t("Online-ZIP wird gestreamt und entpackt …", "Streaming and extracting online ZIP …"));
        new Thread(new Runnable() {
            @Override public void run() {
                final UnrealDataPaths.ImportResult result = importOnlineZip(urlText, installProgressCallback());
                runOnUiThread(new Runnable() { @Override public void run() { handleImportResult(result); }});
            }
        }, "UE1OnlineZipStreamingImport").start();
    }

    private UnrealDataPaths.ImportResult importOnlineZip(String urlText, UnrealDataPaths.ProgressCallback progress) {
        try {
            URL url = new URL(urlText);
            for (int redirect = 0; redirect < 5; ++redirect) {
                String protocol = url.getProtocol();
                if (protocol == null) throw new IOException("Download URL has no protocol.");
                protocol = protocol.toLowerCase(Locale.US);
                if (!"http".equals(protocol) && !"https".equals(protocol)) {
                    throw new IOException("Only HTTP/HTTPS URLs are supported.");
                }
                if (isOuyaDevice() && "https".equals(protocol)) {
                    throw new IOException("OUYA cannot download HTTPS URLs. Use a direct HTTP URL without HTTPS redirect.");
                }

                HttpURLConnection connection = (HttpURLConnection) url.openConnection();
                connection.setConnectTimeout(20000);
                connection.setReadTimeout(60000);
                connection.setInstanceFollowRedirects(false);
                connection.setRequestProperty("User-Agent", "Unreal-Android-Installer/1.3.2-streaming");
                connection.connect();

                int code = connection.getResponseCode();
                if (code == HttpURLConnection.HTTP_MOVED_PERM || code == HttpURLConnection.HTTP_MOVED_TEMP ||
                        code == HttpURLConnection.HTTP_SEE_OTHER || code == 307 || code == 308) {
                    String location = connection.getHeaderField("Location");
                    connection.disconnect();
                    if (location == null || location.trim().length() == 0) throw new IOException("Server redirected without Location header.");
                    URL next = new URL(url, location);
                    if (isOuyaDevice() && "https".equalsIgnoreCase(next.getProtocol())) {
                        throw new IOException("Server redirects to HTTPS, which OUYA cannot download. Use a direct HTTP mirror.");
                    }
                    url = next;
                    continue;
                }

                if (code < 200 || code >= 300) {
                    connection.disconnect();
                    throw new IOException("HTTP error " + code + " while downloading ZIP.");
                }

                InputStream input = null;
                try {
                    long totalBytes = connection.getContentLength();
                    if (progress != null) progress.onProgress(t("Download/Installation", "Download/installation"), 1);
                    updateInstallMessage(t("Online-ZIP wird direkt ins Ziel-Staging entpackt …", "Online ZIP is being extracted directly into target staging …"));
                    input = connection.getInputStream();
                    return UnrealDataPaths.importUnrealZipStream(MainActivity.this, input, totalBytes, progress,
                            t("Download/Installation", "Download/installation"));
                } finally {
                    if (input != null) try { input.close(); } catch (Throwable ignored) {}
                    connection.disconnect();
                }
            }
            throw new IOException("Too many redirects while downloading ZIP.");
        } catch (Throwable t) {
            return UnrealDataPaths.ImportResult.fail(
                    MainActivity.this.t("Online-ZIP-Import fehlgeschlagen.", "Online ZIP import failed."), t);
        }
    }

    private File legacyZipStartDir() {
        try {
            File external = Environment.getExternalStorageDirectory();
            if (external != null && external.isDirectory()) return external;
        } catch (Throwable ignored) {}
        File[] candidates = new File[] {
                new File("/sdcard"),
                new File("/mnt/sdcard"),
                new File("/storage/sdcard0"),
                new File("/storage/emulated/0"),
                new File("/mnt/usbdrive"),
                new File("/mnt/usbdrive0"),
                new File("/mnt/usb_storage")
        };
        for (File candidate : candidates) if (candidate.isDirectory()) return candidate;
        return new File("/");
    }

    private void openLegacyZipPicker(final File dir) {
        final File current = dir != null && dir.isDirectory() ? dir : legacyZipStartDir();
        final List<LegacyChoice> choices = legacyZipChoices(current);
        if (choices.isEmpty()) {
            lastImportMessage = t("Keine ZIP-Dateien oder Unterordner gefunden in:\n", "No ZIP files or subfolders found in:\n") + current.getAbsolutePath();
            showMissingDataScreen();
            return;
        }

        String[] labels = new String[choices.size()];
        for (int i = 0; i < choices.size(); ++i) labels[i] = choices.get(i).label;

        new AlertDialog.Builder(this)
                .setTitle(t("Lokales Unreal-ZIP auswählen", "Select local Unreal ZIP") + "\n" + current.getAbsolutePath())
                .setItems(labels, new android.content.DialogInterface.OnClickListener() {
                    @Override public void onClick(android.content.DialogInterface dialog, int which) {
                        LegacyChoice choice = choices.get(which);
                        if (choice.parent) {
                            File parent = current.getParentFile();
                            if (parent == null) parent = current;
                            openLegacyZipPicker(parent);
                        } else if (choice.directory) {
                            openLegacyZipPicker(choice.file);
                        } else {
                            importLegacyZipInBackground(choice.file);
                        }
                    }
                })
                .setNegativeButton(t("Abbrechen", "Cancel"), null)
                .show();
    }

    private List<LegacyChoice> legacyZipChoices(File dir) {
        ArrayList<LegacyChoice> choices = new ArrayList<LegacyChoice>();
        if (dir == null || !dir.isDirectory()) return choices;
        File parent = dir.getParentFile();
        if (parent != null) choices.add(LegacyChoice.parent());
        File[] files = dir.listFiles();
        if (files == null) return choices;
        ArrayList<File> sorted = new ArrayList<File>();
        for (File file : files) {
            if (file == null || file.isHidden()) continue;
            if (file.isDirectory() || (file.isFile() && file.getName().toLowerCase(Locale.US).endsWith(".zip"))) sorted.add(file);
        }
        Collections.sort(sorted, new Comparator<File>() {
            @Override public int compare(File a, File b) {
                if (a.isDirectory() != b.isDirectory()) return a.isDirectory() ? -1 : 1;
                return a.getName().compareToIgnoreCase(b.getName());
            }
        });
        for (File file : sorted) choices.add(new LegacyChoice(file));
        return choices;
    }

    private void importLegacyZipInBackground(final File zipFile) {
        if (zipFile == null || !zipFile.isFile()) {
            Toast.makeText(this, t("ZIP-Datei nicht lesbar.", "ZIP file is not readable."), Toast.LENGTH_LONG).show();
            return;
        }
        showBusyScreen(
                t("Unreal-ZIP wird importiert", "Importing Unreal ZIP"),
                t("Die ZIP-Datei wird geprüft und in den sicheren App-Ordner entpackt.\nDas kann je nach Gerät einige Minuten dauern.",
                  "The ZIP file is being checked and extracted into the safe app folder.\nThis may take a few minutes depending on the device."));
        new Thread(new Runnable() {
            @Override public void run() {
                final UnrealDataPaths.ImportResult result = UnrealDataPaths.importUnrealZipFile(MainActivity.this, zipFile, installProgressCallback());
                runOnUiThread(new Runnable() { @Override public void run() { handleImportResult(result); }});
            }
        }, "UE1LegacyZipImport").start();
    }

    private static final class LegacyChoice {
        final File file;
        final boolean directory;
        final boolean parent;
        final String label;

        LegacyChoice(File file) {
            this.file = file;
            this.directory = file != null && file.isDirectory();
            this.parent = false;
            this.label = directory ? "[" + file.getName() + "]" : file.getName();
        }

        private LegacyChoice() {
            this.file = null;
            this.directory = false;
            this.parent = true;
            this.label = "[..]";
        }

        static LegacyChoice parent() { return new LegacyChoice(); }
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
