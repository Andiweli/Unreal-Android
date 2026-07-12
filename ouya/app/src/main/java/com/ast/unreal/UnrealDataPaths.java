package com.ast.unreal;

import android.content.Context;
import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.PushbackInputStream;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipInputStream;

final class UnrealDataPaths {
    static final String TAG_STARTUP = "UE1Startup";
    static final String TAG_CONFIG = "UE1Config";
    static final String TAG_IMPORT = "UE1Import";
    static final String EXTRA_UNREAL_ROOT = "com.ast.unreal.EXTRA_UNREAL_ROOT";

    static final String[] UNREAL_DIRS = {
            "System", "Maps", "Textures", "Sounds", "Music", "Meshes", "Help", "Web", "Save", "Cache"
    };

    private UnrealDataPaths() {}

    static boolean isGermanLocale(Context context) {
        Locale locale = Locale.getDefault();
        try {
            if (Build.VERSION.SDK_INT >= 24) locale = context.getResources().getConfiguration().getLocales().get(0);
            else locale = context.getResources().getConfiguration().locale;
        } catch (Throwable ignored) {}
        return locale != null && "de".equalsIgnoreCase(locale.getLanguage());
    }

    static String tr(Context context, String de, String en) {
        return isGermanLocale(context) ? de : en;
    }

    static final class ImportResult {
        final boolean ok;
        final File root;
        final String message;

        private ImportResult(boolean ok, File root, String message) {
            this.ok = ok;
            this.root = root;
            this.message = message;
        }

        static ImportResult ok(File root, String message) {
            return new ImportResult(true, root, message);
        }

        static ImportResult fail(String message) {
            return new ImportResult(false, null, message);
        }

        static ImportResult fail(String message, Throwable t) {
            Log.e(TAG_IMPORT, message, t);
            return new ImportResult(false, null, message + "\n\n" + t.getClass().getSimpleName() + ": " + t.getMessage());
        }
    }

    static interface ProgressCallback {
        void onProgress(String phase, int percent);
    }

    private static void progress(ProgressCallback callback, Context context, String de, String en, int percent) {
        if (callback != null) callback.onProgress(tr(context, de, en), percent);
    }

    private static final String SAF_MIME_TYPE_DIR = "vnd.android.document/directory";
    private static final String SAF_COL_DOCUMENT_ID = "document_id";
    private static final String SAF_COL_DISPLAY_NAME = "_display_name";
    private static final String SAF_COL_MIME_TYPE = "mime_type";

    private static final class SafNode {
        final String docId;
        final String name;
        final String mimeType;

        SafNode(String docId, String name, String mimeType) {
            this.docId = docId;
            this.name = name;
            this.mimeType = mimeType;
        }

        boolean isDirectory() {
            return SAF_MIME_TYPE_DIR.equals(mimeType);
        }
    }

    private static final class ZipRootFlags {
        boolean core;
        boolean engine;
        boolean unrealI;
        boolean unrealShare;
        boolean map;

        boolean valid() {
            return core && engine && (unrealI || unrealShare) && map;
        }

        int score() {
            int s = 0;
            if (core) s++;
            if (engine) s++;
            if (unrealI) s += 2;
            if (unrealShare) s++;
            if (map) s += 2;
            return s;
        }
    }


    static File primaryAppRoot(Context context) {
        File base = context.getExternalFilesDir(null);
        if (base == null) base = context.getFilesDir();
        return new File(base, "Unreal");
    }

    static File findBestUnrealRoot(Context context) {
        List<File> candidates = candidateRoots(context);
        for (File candidate : candidates) {
            boolean valid = hasRequiredData(candidate);
            Log.i(TAG_STARTUP, "data candidate: valid=" + valid + " root=" + candidate.getAbsolutePath());
            if (valid) return candidate;
        }
        File fallback = primaryAppRoot(context);
        Log.i(TAG_STARTUP, "data fallback root=" + fallback.getAbsolutePath());
        return fallback;
    }

    static List<File> candidateRoots(Context context) {
        ArrayList<File> out = new ArrayList<File>();
        HashSet<String> seen = new HashSet<String>();

        addCandidate(out, seen, primaryAppRoot(context));

        File[] appExternalDirs = externalFilesDirsCompat(context);
        if (appExternalDirs != null) {
            for (File appDir : appExternalDirs) {
                if (appDir == null) continue;
                addCandidate(out, seen, new File(appDir, "Unreal"));
            }
        }

        try {
            File publicRoot = Environment.getExternalStorageDirectory();
            if (publicRoot != null) addCandidate(out, seen, new File(publicRoot, "Unreal"));
        } catch (Throwable ignored) {}

        addCandidate(out, seen, new File("/sdcard/Android/data/com.ast.unreal/files/Unreal"));
        addCandidate(out, seen, new File("/mnt/sdcard/Android/data/com.ast.unreal/files/Unreal"));
        addCandidate(out, seen, new File("/storage/sdcard0/Android/data/com.ast.unreal/files/Unreal"));
        addCandidate(out, seen, new File("/storage/emulated/0/Android/data/com.ast.unreal/files/Unreal"));
        addCandidate(out, seen, new File("/sdcard/Unreal"));
        addCandidate(out, seen, new File("/mnt/sdcard/Unreal"));
        addCandidate(out, seen, new File("/storage/sdcard0/Unreal"));
        addCandidate(out, seen, new File("/storage/emulated/0/Unreal"));
        addCandidate(out, seen, new File("/mnt/usbdrive/Unreal"));
        addCandidate(out, seen, new File("/mnt/usbdrive0/Unreal"));
        addCandidate(out, seen, new File("/mnt/usb_storage/Unreal"));

        if (appExternalDirs != null) {
            for (File appDir : appExternalDirs) {
                File storageRoot = storageRootFromExternalFilesDir(appDir);
                if (storageRoot != null) addCandidate(out, seen, new File(storageRoot, "Unreal"));
            }
        }

        File[] volumes = new File("/storage").listFiles();
        if (volumes != null) {
            for (File volume : volumes) {
                String name = volume.getName();
                if ("self".equals(name) || "emulated".equals(name)) continue;
                addCandidate(out, seen, new File(volume, "Unreal"));
            }
        }

        File[] mntVolumes = new File("/mnt").listFiles();
        if (mntVolumes != null) {
            for (File volume : mntVolumes) {
                String name = volume.getName();
                if ("runtime".equals(name) || "asec".equals(name) || "obb".equals(name)) continue;
                addCandidate(out, seen, new File(volume, "Unreal"));
            }
        }
        return out;
    }

    private static File[] externalFilesDirsCompat(Context context) {
        if (context == null) return null;
        try {
            Method method = Context.class.getMethod("getExternalFilesDirs", String.class);
            Object result = method.invoke(context, new Object[] { null });
            if (result instanceof File[]) return (File[]) result;
        } catch (Throwable ignored) {}
        File single = context.getExternalFilesDir(null);
        if (single != null) return new File[] { single };
        return null;
    }

    private static File storageRootFromExternalFilesDir(File appDir) {
        if (appDir == null) return null;
        File p = appDir;
        for (int i = 0; i < 4 && p != null; ++i) p = p.getParentFile();
        return p;
    }

    private static void addCandidate(ArrayList<File> out, HashSet<String> seen, File candidate) {
        if (candidate == null) return;
        try { candidate = candidate.getCanonicalFile(); } catch (IOException ignored) { candidate = candidate.getAbsoluteFile(); }
        String key = candidate.getAbsolutePath();
        if (seen.add(key)) out.add(candidate);
    }

    static boolean hasRequiredData(File root) { return hasRequiredData(root, false); }

    static boolean hasRequiredData(File root, boolean verbose) {
        if (root == null) return false;
        File systemDir = new File(root, "System");
        File mapsDir = new File(root, "Maps");
        boolean rootDir = root.isDirectory();
        boolean system = systemDir.isDirectory();
        boolean maps = mapsDir.isDirectory();
        boolean hasCore = findCaseInsensitive(systemDir, "Core.u") != null;
        boolean hasEngine = findCaseInsensitive(systemDir, "Engine.u") != null;
        boolean hasUnrealI = findCaseInsensitive(systemDir, "UnrealI.u") != null;
        boolean hasUnrealShare = findCaseInsensitive(systemDir, "UnrealShare.u") != null;
        boolean hasGamePackage = hasUnrealI || hasUnrealShare;
        boolean hasMap = findCaseInsensitive(mapsDir, "Entry.unr") != null || hasAnyMap(mapsDir);
        boolean ok = rootDir && system && maps && hasCore && hasEngine && hasGamePackage && hasMap;
        if (verbose || root.exists()) {
            Log.i(TAG_STARTUP, "data candidate detail: ok=" + ok
                    + " rootDir=" + rootDir
                    + " system=" + system
                    + " maps=" + maps
                    + " core=" + hasCore
                    + " engine=" + hasEngine
                    + " game=" + hasGamePackage
                    + " map=" + hasMap
                    + " root=" + root.getAbsolutePath());
        }
        return ok;
    }

    private static File findCaseInsensitive(File dir, String expectedName) {
        if (dir == null || expectedName == null) return null;
        File exact = new File(dir, expectedName);
        if (exact.isFile()) return exact;
        File[] files = dir.listFiles();
        if (files == null) return null;
        for (File f : files) if (f.isFile() && expectedName.equalsIgnoreCase(f.getName())) return f;
        return null;
    }

    private static boolean hasAnyMap(File mapsDir) {
        File[] files = mapsDir.listFiles();
        if (files == null) return false;
        for (File f : files) if (f.isFile() && f.getName().toLowerCase(Locale.ROOT).endsWith(".unr")) return true;
        return false;
    }

    static boolean sameFile(File a, File b) {
        if (a == null || b == null) return false;
        try { return a.getCanonicalFile().equals(b.getCanonicalFile()); }
        catch (IOException ignored) { return a.getAbsoluteFile().equals(b.getAbsoluteFile()); }
    }

    static boolean copyUnrealDataTree(File sourceRoot, File targetRoot) {
        if (sourceRoot == null || targetRoot == null) return false;
        if (!hasRequiredData(sourceRoot)) return false;
        if (sameFile(sourceRoot, targetRoot)) return true;
        ensureDirectoryLayout(targetRoot);
        boolean ok = true;
        for (String dir : UNREAL_DIRS) {
            File src = new File(sourceRoot, dir);
            if (src.exists()) ok &= copyRecursive(src, new File(targetRoot, dir));
        }
        return ok && hasRequiredData(targetRoot, true);
    }

    private static boolean copyRecursive(File src, File dst) {
        if (src == null || dst == null) return false;
        if (src.isDirectory()) {
            if (!dst.exists() && !dst.mkdirs()) {
                Log.w(TAG_STARTUP, "Could not create target directory: " + dst.getAbsolutePath());
                return false;
            }
            File[] children = src.listFiles();
            if (children == null) return true;
            boolean ok = true;
            for (File child : children) ok &= copyRecursive(child, new File(dst, child.getName()));
            return ok;
        }
        if (!src.isFile()) return true;
        File parent = dst.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) return false;
        InputStream input = null;
        FileOutputStream output = null;
        try {
            input = new FileInputStream(src);
            output = new FileOutputStream(dst);
            byte[] buf = new byte[64 * 1024];
            int read;
            while ((read = input.read(buf)) >= 0) output.write(buf, 0, read);
            output.flush();
            return true;
        } catch (IOException ex) {
            Log.w(TAG_STARTUP, "Could not copy " + src.getAbsolutePath() + " -> " + dst.getAbsolutePath() + ": " + ex);
            return false;
        } finally { closeQuietly(input); closeQuietly(output); }
    }

    static void ensureDirectoryLayout(File root) {
        if (root == null) return;
        for (String dir : UNREAL_DIRS) {
            File target = new File(root, dir);
            if (!target.exists() && !target.mkdirs()) Log.w(TAG_CONFIG, "Could not create directory: " + target.getAbsolutePath());
        }
    }

    static void installDefaultConfigsIfNeeded(Context context, File root) {
        if (root == null) return;
        File systemDir = new File(root, "System");
        if (!systemDir.exists() && !systemDir.mkdirs()) Log.w(TAG_CONFIG, "Could not create System directory: " + systemDir.getAbsolutePath());
        copyAssetIfMissing(context, "ue1_config/Unreal.ini", new File(systemDir, "Unreal.ini"));
        copyAssetIfMissing(context, "ue1_config/User.ini", new File(systemDir, "User.ini"));
        copyAssetIfMissing(context, "ue1_config/Default.ini", new File(systemDir, "Default.ini"));
        copyAssetIfMissing(context, "ue1_config/AndroidController.ini", new File(systemDir, "AndroidController.ini"));
        copyAssetIfMissing(context, "ue1_config/AndroidUI.ini", new File(systemDir, "AndroidUI.ini"));
    }

    private static void copyAssetIfMissing(Context context, String asset, File out) {
        if (out.isFile()) return;
        InputStream input = null;
        FileOutputStream fos = null;
        try {
            input = context.getAssets().open(asset);
            fos = new FileOutputStream(out);
            byte[] buf = new byte[16 * 1024];
            int read;
            while ((read = input.read(buf)) >= 0) fos.write(buf, 0, read);
            fos.flush();
            Log.i(TAG_CONFIG, "Installed default config: " + out.getAbsolutePath());
        } catch (IOException ex) {
            Log.w(TAG_CONFIG, "Could not install default config " + out.getAbsolutePath() + ": " + ex);
        } finally { closeQuietly(input); closeQuietly(fos); }
    }

    static void normalizeConfigForDetectedData(File root) {
        if (root == null) return;
        File systemDir = new File(root, "System");
        boolean hasUnrealI = findCaseInsensitive(systemDir, "UnrealI.u") != null;
        boolean hasUnrealShare = findCaseInsensitive(systemDir, "UnrealShare.u") != null;
        if (!hasUnrealI || hasUnrealShare) return;
        patchPackageName(new File(root, "System/Unreal.ini"));
        patchPackageName(new File(root, "System/Default.ini"));
    }

    private static void patchPackageName(File file) {
        if (!file.isFile()) return;
        try {
            String text = readFileUtf8(file);
            String patched = text
                    .replace("UnrealShare.SinglePlayer", "UnrealI.SinglePlayer")
                    .replace("UnrealShare.DeathMatchGame", "UnrealI.DeathMatchGame")
                    .replace("DefaultGame=UnrealShare.", "DefaultGame=UnrealI.")
                    .replace("DefaultServerGame=UnrealShare.", "DefaultServerGame=UnrealI.");
            if (!patched.equals(text)) {
                writeFileUtf8(file, patched);
                Log.i(TAG_CONFIG, "Patched retail v200 config package names: " + file.getAbsolutePath());
            }
        } catch (IOException ex) {
            Log.w(TAG_CONFIG, "Could not patch config " + file.getAbsolutePath() + ": " + ex);
        }
    }

    static void ensureWritableConfigFiles(Context context, File root) {
        if (root == null) return;
        try {
            ensureDirectoryLayout(root);
            installDefaultConfigsIfNeeded(context, root);
            File systemDir = new File(root, "System");
            ensureConfigFile(systemDir, "User.ini", new String[] { "DefUser.ini", "DefaultUser.ini" },
                    "[DefaultPlayer]\nName=Player\nClass=UnrealShare.MaleOne\n\n[Engine.Input]\n");
            ensureConfigFile(systemDir, "Unreal.ini", new String[] { "Default.ini", "Unreal.ini.default" }, "");
            Log.i(TAG_CONFIG, "Config root: " + root.getAbsolutePath());
            Log.i(TAG_CONFIG, "User.ini: " + new File(systemDir, "User.ini").getAbsolutePath());
            Log.i(TAG_CONFIG, "Unreal.ini: " + new File(systemDir, "Unreal.ini").getAbsolutePath());
        } catch (Throwable t) {
            Log.e(TAG_CONFIG, "Config bootstrap failed for root=" + root.getAbsolutePath(), t);
        }
    }

    private static void ensureConfigFile(File systemDir, String targetName, String[] templateNames, String fallbackText) throws IOException {
        if (!systemDir.exists() && !systemDir.mkdirs()) Log.w(TAG_CONFIG, "Could not create System directory: " + systemDir.getAbsolutePath());
        final File target = new File(systemDir, targetName);
        if (target.exists()) {
            Log.i(TAG_CONFIG, targetName + " exists: " + target.getAbsolutePath());
            return;
        }
        for (String templateName : templateNames) {
            final File template = new File(systemDir, templateName);
            if (template.exists() && template.isFile()) {
                copyFile(template, target);
                Log.i(TAG_CONFIG, targetName + " created from " + templateName + ": " + target.getAbsolutePath());
                return;
            }
        }
        FileOutputStream out = null;
        try {
            out = new FileOutputStream(target);
            if (fallbackText != null && fallbackText.length() > 0) out.write(fallbackText.getBytes("UTF-8"));
            out.flush();
            Log.i(TAG_CONFIG, targetName + " created from fallback: " + target.getAbsolutePath());
        } finally { closeQuietly(out); }
    }

    private static void copyFile(File src, File dst) throws IOException {
        FileInputStream in = null;
        FileOutputStream out = null;
        try {
            in = new FileInputStream(src);
            out = new FileOutputStream(dst);
            byte[] buffer = new byte[64 * 1024];
            int read;
            while ((read = in.read(buffer)) != -1) out.write(buffer, 0, read);
            out.flush();
        } finally { closeQuietly(in); closeQuietly(out); }
    }

    private static String readFileUtf8(File file) throws IOException {
        FileInputStream in = null;
        try {
            in = new FileInputStream(file);
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buf = new byte[16 * 1024];
            int read;
            while ((read = in.read(buf)) != -1) out.write(buf, 0, read);
            return new String(out.toByteArray(), "UTF-8");
        } finally { closeQuietly(in); }
    }

    private static void writeFileUtf8(File file, String text) throws IOException {
        FileOutputStream out = null;
        try {
            out = new FileOutputStream(file);
            out.write(text.getBytes("UTF-8"));
            out.flush();
        } finally { closeQuietly(out); }
    }

    private static void closeQuietly(Object stream) {
        if (stream == null) return;
        try {
            if (stream instanceof InputStream) ((InputStream) stream).close();
            else if (stream instanceof FileOutputStream) ((FileOutputStream) stream).close();
            else if (stream instanceof ZipFile) ((ZipFile) stream).close();
        } catch (Throwable ignored) {}
    }


    private static void copyStream(InputStream in, FileOutputStream out) throws IOException {
        copyStream(in, out, null, null, 0, 0, -1);
    }

    private static long copyStream(InputStream in, FileOutputStream out, ProgressCallback progress, String phase, int startPercent, int spanPercent, long totalBytes) throws IOException {
        byte[] buffer = new byte[128 * 1024];
        long copied = 0;
        int read;
        int lastPercent = -1;
        while ((read = in.read(buffer)) != -1) {
            out.write(buffer, 0, read);
            copied += read;
            if (progress != null && phase != null && totalBytes > 0 && spanPercent > 0) {
                int percent = startPercent + (int) Math.min(spanPercent, (copied * spanPercent) / totalBytes);
                if (percent != lastPercent) {
                    lastPercent = percent;
                    progress.onProgress(phase, percent);
                }
            }
        }
        out.flush();
        if (progress != null && phase != null && totalBytes > 0 && spanPercent > 0) {
            progress.onProgress(phase, startPercent + spanPercent);
        }
        return copied;
    }

    static ImportResult importUnrealFolderFromSaf(Context context, Uri treeUri) {
        return importUnrealFolderFromSaf(context, treeUri, null);
    }

    static ImportResult importUnrealFolderFromSaf(Context context, Uri treeUri, ProgressCallback progress) {
        if (treeUri == null) return ImportResult.fail(tr(context, "Kein Ordner ausgewählt.", "No folder selected."));
        if (Build.VERSION.SDK_INT < 21) {
            return ImportResult.fail(tr(context,
                    "Dieser Android-Stand bietet keinen systemeigenen Ordnerimport. Bitte den Unreal-Ordner auf USB/SD oder in den App-Ordner kopieren.",
                    "This Android version has no system folder import. Please copy the Unreal folder to USB/SD or into the app folder."));
        }
        try {
            progress(progress, context, "Prüfung", "Checking", 2);
            String selectedDocId = safGetTreeDocumentId(treeUri);
            if (selectedDocId == null || selectedDocId.length() == 0) {
                return ImportResult.fail(tr(context, "Der ausgewählte Ordner konnte nicht gelesen werden.", "The selected folder could not be read."));
            }

            String unrealDocId = findSafUnrealRootDocId(context, treeUri, selectedDocId);
            if (unrealDocId == null) {
                return ImportResult.fail(tr(context,
                        "Der ausgewählte Ordner enthält keine gültigen Unreal-Daten. Bitte den Ordner 'Unreal' wählen. Erwartet werden mindestens System/Core.u, System/Engine.u, UnrealI.u oder UnrealShare.u und Maps/*.unr.",
                        "The selected folder does not contain valid Unreal data. Please select the 'Unreal' folder. Expected at least: System/Core.u, System/Engine.u, UnrealI.u or UnrealShare.u, and Maps/*.unr."));
            }

            File target = primaryAppRoot(context);
            ensureDirectoryLayout(target);
            Log.i(TAG_IMPORT, "Importing SAF Unreal folder to " + target.getAbsolutePath());
            int[] copied = new int[] { 0 };
            int totalFiles = countSafFiles(context, treeUri, unrealDocId);
            progress(progress, context, "Kopieren", "Copying", 5);
            copySafTree(context, treeUri, unrealDocId, target, progress, tr(context, "Kopieren", "Copying"), copied, totalFiles);
            progress(progress, context, "Konfiguration", "Config", 92);
            installDefaultConfigsIfNeeded(context, target);
            normalizeConfigForDetectedData(target);
            progress(progress, context, "Fertig", "Done", 100);

            if (!hasRequiredData(target, true)) {
                return ImportResult.fail(tr(context,
                        "Der Ordner wurde kopiert, aber danach fehlen weiterhin Pflichtdateien in ",
                        "The folder was copied, but required files are still missing in ") + target.getAbsolutePath());
            }
            return ImportResult.ok(target, tr(context,
                    "Unreal-Daten wurden erfolgreich importiert nach:\n",
                    "Unreal data was imported successfully to:\n") + target.getAbsolutePath());
        } catch (Throwable t) {
            return ImportResult.fail(tr(context,
                    "Import aus dem ausgewählten Ordner fehlgeschlagen.",
                    "Import from the selected folder failed."), t);
        }
    }

    static ImportResult importUnrealZip(Context context, Uri zipUri) {
        return importUnrealZip(context, zipUri, null);
    }

    static ImportResult importUnrealZip(Context context, Uri zipUri, ProgressCallback progress) {
        if (zipUri == null) return ImportResult.fail(tr(context, "Keine ZIP-Datei ausgewählt.", "No ZIP file selected."));
        InputStream input = null;
        try {
            input = context.getContentResolver().openInputStream(zipUri);
            if (input == null) throw new IOException("Could not open selected ZIP stream.");
            return importUnrealZipStream(context, input, -1L, progress, tr(context, "ZIP-Installation", "ZIP installation"));
        } catch (Throwable t) {
            return ImportResult.fail(tr(context,
                    "Import aus der ZIP-Datei fehlgeschlagen.",
                    "Import from the ZIP file failed."), t);
        } finally {
            closeQuietly(input);
        }
    }

    static ImportResult importUnrealZipFile(Context context, File zipFile) {
        return importUnrealZipFile(context, zipFile, null);
    }

    static ImportResult importUnrealZipFile(Context context, File zipFile, ProgressCallback progress) {
        if (zipFile == null || !zipFile.isFile()) return ImportResult.fail(tr(context, "Keine ZIP-Datei ausgewählt.", "No ZIP file selected."));
        FileInputStream input = null;
        try {
            input = new FileInputStream(zipFile);
            return importUnrealZipStream(context, input, zipFile.length(), progress, tr(context, "ZIP-Installation", "ZIP installation"));
        } catch (Throwable t) {
            return ImportResult.fail(tr(context,
                    "Import aus der ZIP-Datei fehlgeschlagen.",
                    "Import from the ZIP file failed."), t);
        } finally {
            closeQuietly(input);
        }
    }

    static ImportResult importUnrealZipStream(Context context, InputStream rawInput, long totalCompressedBytes,
                                              ProgressCallback progress, String progressPhase) {
        if (rawInput == null) return ImportResult.fail(tr(context, "ZIP-Stream nicht lesbar.", "ZIP stream is not readable."));
        File target = primaryAppRoot(context);
        File stagingData = null;
        File stagingRoot = null;
        boolean replaced = false;
        InstallStats stats = new InstallStats();
        try {
            progress(progress, context, "ZIP prüfen", "Checking ZIP", 3);
            stagingData = createTargetSiblingStagingData(target);
            stagingRoot = stagingData.getParentFile();

            CountingInputStream countingInput = new CountingInputStream(rawInput);
            InputStream checkedInput = checkedZipInputStream(countingInput);
            ZipInputStream zipInput = null;
            try {
                zipInput = new ZipInputStream(checkedInput);
                extractZipStreamDirect(context, zipInput, stagingData, stats, progress,
                        countingInput, totalCompressedBytes, progressPhase);
            } finally {
                closeQuietly(zipInput);
            }

            if (stats.files <= 0) {
                throw new IOException("ZIP did not contain extractable Unreal files.");
            }

            progress(progress, context, "Konfiguration", "Config", 90);
            installDefaultConfigsIfNeeded(context, stagingData);
            normalizeConfigForDetectedData(stagingData);
            if (!hasRequiredData(stagingData, true)) {
                throw new IOException("ZIP extracted to staging folder, but required Unreal files were not found. Extracted files="
                        + stats.files + ", bytes=" + stats.bytes);
            }

            progress(progress, context, "Aktivieren", "Activating", 92);
            replaceTargetWithStagedData(stagingData, target);
            replaced = true;

            progress(progress, context, "Konfiguration", "Config", 96);
            installDefaultConfigsIfNeeded(context, target);
            normalizeConfigForDetectedData(target);
            progress(progress, context, "Fertig", "Done", 100);

            if (!hasRequiredData(target, true)) {
                throw new IOException("ZIP installed, but required Unreal files were not found in " + target.getAbsolutePath());
            }
            return ImportResult.ok(target, tr(context,
                    "Unreal-Daten wurden erfolgreich aus der ZIP-Datei importiert nach:\n",
                    "Unreal data was successfully imported from the ZIP file to:\n") + target.getAbsolutePath());
        } catch (Throwable t) {
            return ImportResult.fail(tr(context,
                    "Import aus der ZIP-Datei fehlgeschlagen.",
                    "Import from the ZIP file failed."), t);
        } finally {
            if (!replaced && stagingRoot != null) {
                try { deleteRecursive(stagingRoot); } catch (Throwable ignored) {}
            } else if (stagingRoot != null && stagingRoot.exists()) {
                try { deleteRecursive(stagingRoot); } catch (Throwable ignored) {}
            }
        }
    }

    private static void extractZipStreamDirect(Context context, ZipInputStream zipInput, File targetRoot, InstallStats stats,
                                               ProgressCallback progress, CountingInputStream countingInput,
                                               long totalCompressedBytes, String progressPhase) throws IOException {
        String prefix = null;
        String prefixLower = null;
        int lastPercent = -1;
        progress(progress, context, "Installation", "Installing", 5);

        ZipEntry entry;
        while ((entry = zipInput.getNextEntry()) != null) {
            String name = normalizeZipName(entry.getName());
            String lowerName = name.toLowerCase(Locale.US);
            if (name.length() == 0 || shouldSkipZipEntry(name)) {
                zipInput.closeEntry();
                continue;
            }

            if (prefix == null) {
                prefix = findZipUnrealDataPrefixFromEntry(name);
                if (prefix != null) prefixLower = prefix.toLowerCase(Locale.US);
            }

            if (prefix == null || !lowerName.startsWith(prefixLower)) {
                zipInput.closeEntry();
                continue;
            }

            String relative = name.substring(prefix.length());
            if (relative.length() == 0 || shouldSkipZipEntry(relative)) {
                zipInput.closeEntry();
                continue;
            }

            File out = safeZipOutputFile(targetRoot, relative);
            if (entry.isDirectory() || relative.endsWith("/")) {
                if (!out.exists() && !out.mkdirs()) throw new IOException("Could not create " + out.getAbsolutePath());
            } else {
                File parent = out.getParentFile();
                if (parent != null && !parent.exists() && !parent.mkdirs()) throw new IOException("Could not create " + parent.getAbsolutePath());
                FileOutputStream fos = null;
                try {
                    fos = new FileOutputStream(out, false);
                    stats.bytes += copyZipEntry(zipInput, fos, progress, countingInput,
                            totalCompressedBytes, progressPhase, lastPercent);
                    stats.files++;
                    if (progress != null && totalCompressedBytes <= 0) {
                        progress.onProgress(tr(context, "Installation", "Installing"), 45);
                    }
                    if (progress != null && totalCompressedBytes > 0) {
                        int percent = zipStreamPercent(countingInput, totalCompressedBytes);
                        if (percent != lastPercent) {
                            lastPercent = percent;
                            progress.onProgress(progressPhase, percent);
                        }
                    }
                } finally {
                    closeQuietly(fos);
                }
            }
            zipInput.closeEntry();
        }

        if (prefix == null) {
            throw new IOException("ZIP does not contain System, Maps and required Unreal game data.");
        }
    }

    private static long copyZipEntry(ZipInputStream input, FileOutputStream output, ProgressCallback progress,
                                     CountingInputStream countingInput, long totalCompressedBytes,
                                     String progressPhase, int lastPercent) throws IOException {
        byte[] buffer = new byte[128 * 1024];
        long total = 0;
        int read;
        int localLastPercent = lastPercent;
        while ((read = input.read(buffer)) != -1) {
            output.write(buffer, 0, read);
            total += read;
            if (progress != null && totalCompressedBytes > 0) {
                int percent = zipStreamPercent(countingInput, totalCompressedBytes);
                if (percent != localLastPercent) {
                    localLastPercent = percent;
                    progress.onProgress(progressPhase, percent);
                }
            }
        }
        output.flush();
        return total;
    }

    private static String findZipUnrealDataPrefixFromEntry(String normalizedName) {
        if (normalizedName == null) return null;
        String lower = normalizeZipName(normalizedName).toLowerCase(Locale.US);
        String[] markers = { "system/", "maps/", "textures/", "sounds/", "music/", "meshes/" };
        for (String marker : markers) {
            int idx = lower.indexOf(marker);
            while (idx >= 0) {
                if (idx == 0 || lower.charAt(idx - 1) == '/') {
                    return normalizedName.substring(0, idx);
                }
                idx = lower.indexOf(marker, idx + 1);
            }
        }
        return null;
    }

    private static InputStream checkedZipInputStream(InputStream input) throws IOException {
        PushbackInputStream pushback = new PushbackInputStream(new BufferedInputStream(input), 4);
        byte[] signature = new byte[4];
        int read = 0;
        while (read < signature.length) {
            int got = pushback.read(signature, read, signature.length - read);
            if (got < 0) break;
            read += got;
        }
        if (read > 0) pushback.unread(signature, 0, read);
        if (read < 4 || signature[0] != 'P' || signature[1] != 'K' ||
                !((signature[2] == 3 || signature[2] == 5 || signature[2] == 7) &&
                        (signature[3] == 4 || signature[3] == 6 || signature[3] == 8))) {
            throw new IOException("Selected file is not a ZIP archive.");
        }
        return pushback;
    }

    private static int zipStreamPercent(CountingInputStream input, long totalCompressedBytes) {
        if (input == null || totalCompressedBytes <= 0) return 45;
        return 5 + (int) Math.min(85L, (input.bytesRead * 85L) / totalCompressedBytes);
    }

    private static boolean shouldSkipZipEntry(String relative) {
        String lower = relative.toLowerCase(Locale.US);
        return lower.startsWith("__macosx/") || lower.endsWith("/.ds_store") || lower.equals(".ds_store");
    }

    private static File safeZipOutputFile(File targetRoot, String relative) throws IOException {
        String normalized = normalizeZipName(relative);
        if (normalized.length() == 0 || normalized.contains("../") || normalized.equals("..") || normalized.startsWith("../")) {
            throw new IOException("Unsafe ZIP entry: " + relative);
        }
        File out = new File(targetRoot, normalized.replace('/', File.separatorChar));
        String rootPath = targetRoot.getCanonicalPath() + File.separator;
        String outPath = out.getCanonicalPath();
        if (!outPath.startsWith(rootPath)) {
            throw new IOException("Unsafe ZIP entry path: " + relative);
        }
        return out;
    }

    private static File createTargetSiblingStagingData(File targetRoot) throws IOException {
        File parent = targetRoot.getParentFile();
        if (parent == null) throw new IOException("Install target has no parent folder.");
        if (!parent.exists() && !parent.mkdirs()) throw new IOException("Could not create " + parent.getAbsolutePath());
        cleanupOldInstallWorkDirs(parent);
        File stagingRoot = uniqueChild(parent, ".unreal-install-staging-");
        File stagingData = new File(stagingRoot, targetRoot.getName());
        if (!stagingData.mkdirs() && !stagingData.isDirectory()) {
            throw new IOException("Could not create temporary install folder in " + parent.getAbsolutePath());
        }
        Log.i(TAG_IMPORT, "streaming ZIP staging=" + stagingData.getAbsolutePath() + " target=" + targetRoot.getAbsolutePath());
        return stagingData;
    }

    private static File uniqueChild(File parent, String prefix) {
        long now = android.os.SystemClock.uptimeMillis();
        for (int i = 0; i < 100; i++) {
            File child = new File(parent, prefix + now + (i == 0 ? "" : "-" + i));
            if (!child.exists()) return child;
        }
        return new File(parent, prefix + now + "-" + java.lang.System.nanoTime());
    }

    private static void cleanupOldInstallWorkDirs(File parent) {
        File[] children = parent.listFiles();
        if (children == null) return;
        for (File child : children) {
            if (child == null) continue;
            String name = child.getName();
            if (name.startsWith(".unreal-install-staging-") || name.startsWith(".unreal-install-backup-")) {
                try { deleteRecursive(child); }
                catch (IOException ex) { Log.w(TAG_IMPORT, "could not delete old install work folder " + child.getAbsolutePath(), ex); }
            }
        }
    }

    private static void replaceTargetWithStagedData(File stagingData, File targetRoot) throws IOException {
        if (stagingData == null || !stagingData.isDirectory()) throw new IOException("Staged install folder is not readable.");
        File parent = targetRoot.getParentFile();
        if (parent == null) throw new IOException("Install target has no parent folder.");
        if (!parent.exists() && !parent.mkdirs()) throw new IOException("Could not create " + parent.getAbsolutePath());

        File backup = null;
        boolean backupActive = false;
        try {
            if (targetRoot.exists()) {
                backup = uniqueChild(parent, ".unreal-install-backup-");
                if (backup.exists()) deleteRecursive(backup);
                if (targetRoot.renameTo(backup)) {
                    backupActive = true;
                } else {
                    clearDirectory(targetRoot);
                }
            }

            if (!stagingData.renameTo(targetRoot)) {
                if (!targetRoot.exists() && !targetRoot.mkdirs()) {
                    throw new IOException("Could not create " + targetRoot.getAbsolutePath());
                }
                copyDirectoryThrow(stagingData, targetRoot);
            }
        } catch (IOException ex) {
            if (backupActive && backup != null && backup.exists()) {
                try {
                    deleteRecursive(targetRoot);
                    backup.renameTo(targetRoot);
                } catch (Throwable restoreError) {
                    Log.e(TAG_IMPORT, "could not restore previous Unreal data after failed activation", restoreError);
                }
            }
            throw ex;
        }

        if (backupActive && backup != null) {
            try { deleteRecursive(backup); }
            catch (IOException ex) { Log.w(TAG_IMPORT, "could not delete old Unreal backup folder " + backup.getAbsolutePath(), ex); }
        }
    }

    private static void copyDirectoryThrow(File source, File target) throws IOException {
        if (source == null || target == null) return;
        if (source.isDirectory()) {
            if (!target.exists() && !target.mkdirs()) throw new IOException("Could not create " + target.getAbsolutePath());
            File[] children = source.listFiles();
            if (children == null) return;
            for (File child : children) copyDirectoryThrow(child, new File(target, child.getName()));
            return;
        }
        if (!source.isFile()) return;
        File parent = target.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) throw new IOException("Could not create " + parent.getAbsolutePath());
        copyFile(source, target);
    }

    private static void clearDirectory(File dir) throws IOException {
        File[] children = dir.listFiles();
        if (children == null) return;
        for (File child : children) deleteRecursive(child);
    }

    private static void deleteRecursive(File file) throws IOException {
        if (file == null || !file.exists()) return;
        if (file.isDirectory()) {
            File[] children = file.listFiles();
            if (children != null) {
                for (File child : children) deleteRecursive(child);
            }
        }
        if (!file.delete() && file.exists()) {
            throw new IOException("Could not delete " + file.getAbsolutePath());
        }
    }

    private static final class CountingInputStream extends InputStream {
        private final InputStream input;
        long bytesRead;

        CountingInputStream(InputStream input) {
            this.input = input;
        }

        @Override
        public int read() throws IOException {
            int value = input.read();
            if (value >= 0) bytesRead++;
            return value;
        }

        @Override
        public int read(byte[] buffer, int offset, int length) throws IOException {
            int read = input.read(buffer, offset, length);
            if (read > 0) bytesRead += read;
            return read;
        }

        @Override
        public void close() throws IOException {
            input.close();
        }
    }

    private static final class InstallStats {
        int files;
        long bytes;
    }

    private static String safGetTreeDocumentId(Uri treeUri) throws Exception {
        Class<?> cls = Class.forName("android.provider.DocumentsContract");
        Method method = cls.getMethod("getTreeDocumentId", Uri.class);
        Object result = method.invoke(null, treeUri);
        return result instanceof String ? (String) result : null;
    }

    private static Uri safBuildChildDocumentsUriUsingTree(Uri treeUri, String parentDocId) throws Exception {
        Class<?> cls = Class.forName("android.provider.DocumentsContract");
        Method method = cls.getMethod("buildChildDocumentsUriUsingTree", Uri.class, String.class);
        Object result = method.invoke(null, treeUri, parentDocId);
        return result instanceof Uri ? (Uri) result : null;
    }

    private static Uri safBuildDocumentUriUsingTree(Uri treeUri, String docId) throws Exception {
        Class<?> cls = Class.forName("android.provider.DocumentsContract");
        Method method = cls.getMethod("buildDocumentUriUsingTree", Uri.class, String.class);
        Object result = method.invoke(null, treeUri, docId);
        return result instanceof Uri ? (Uri) result : null;
    }

    private static String findSafUnrealRootDocId(Context context, Uri treeUri, String selectedDocId) {
        if (safTreeHasRequiredData(context, treeUri, selectedDocId)) return selectedDocId;
        SafNode unrealChild = findSafChild(context, treeUri, selectedDocId, "Unreal", true);
        if (unrealChild != null && safTreeHasRequiredData(context, treeUri, unrealChild.docId)) return unrealChild.docId;
        return null;
    }

    private static boolean safTreeHasRequiredData(Context context, Uri treeUri, String rootDocId) {
        SafNode system = findSafChild(context, treeUri, rootDocId, "System", true);
        SafNode maps = findSafChild(context, treeUri, rootDocId, "Maps", true);
        if (system == null || maps == null) return false;
        boolean core = findSafChild(context, treeUri, system.docId, "Core.u", false) != null;
        boolean engine = findSafChild(context, treeUri, system.docId, "Engine.u", false) != null;
        boolean unrealI = findSafChild(context, treeUri, system.docId, "UnrealI.u", false) != null;
        boolean unrealShare = findSafChild(context, treeUri, system.docId, "UnrealShare.u", false) != null;
        boolean map = hasAnySafMap(context, treeUri, maps.docId);
        Log.i(TAG_IMPORT, "SAF data check: core=" + core + " engine=" + engine + " game=" + (unrealI || unrealShare) + " map=" + map + " doc=" + rootDocId);
        return core && engine && (unrealI || unrealShare) && map;
    }

    private static SafNode findSafChild(Context context, Uri treeUri, String parentDocId, String expectedName, boolean expectedDir) {
        for (SafNode child : listSafChildren(context, treeUri, parentDocId)) {
            if (!expectedName.equalsIgnoreCase(child.name)) continue;
            if (expectedDir && !child.isDirectory()) continue;
            if (!expectedDir && child.isDirectory()) continue;
            return child;
        }
        return null;
    }

    private static boolean hasAnySafMap(Context context, Uri treeUri, String mapsDocId) {
        for (SafNode child : listSafChildren(context, treeUri, mapsDocId)) {
            if (!child.isDirectory() && child.name != null && child.name.toLowerCase(Locale.ROOT).endsWith(".unr")) return true;
        }
        return false;
    }

    private static List<SafNode> listSafChildren(Context context, Uri treeUri, String parentDocId) {
        ArrayList<SafNode> out = new ArrayList<SafNode>();
        if (Build.VERSION.SDK_INT < 21) return out;
        ContentResolver resolver = context.getContentResolver();
        Cursor cursor = null;
        try {
            Uri childrenUri = safBuildChildDocumentsUriUsingTree(treeUri, parentDocId);
            if (childrenUri == null) return out;
            String[] projection = new String[] { SAF_COL_DOCUMENT_ID, SAF_COL_DISPLAY_NAME, SAF_COL_MIME_TYPE };
            cursor = resolver.query(childrenUri, projection, null, null, null);
            if (cursor == null) return out;
            while (cursor.moveToNext()) {
                String docId = cursor.getString(0);
                String name = cursor.getString(1);
                String mime = cursor.getString(2);
                if (docId == null || name == null) continue;
                out.add(new SafNode(docId, name, mime));
            }
        } catch (Throwable t) {
            Log.w(TAG_IMPORT, "Could not list SAF children for doc=" + parentDocId + ": " + t);
        } finally {
            if (cursor != null) try { cursor.close(); } catch (Throwable ignored) {}
        }
        return out;
    }

    private static int countSafFiles(Context context, Uri treeUri, String parentDocId) {
        int count = 0;
        for (SafNode child : listSafChildren(context, treeUri, parentDocId)) {
            if (child.isDirectory()) count += countSafFiles(context, treeUri, child.docId);
            else count++;
        }
        return count;
    }

    private static void copySafTree(Context context, Uri treeUri, String parentDocId, File outDir) throws IOException {
        copySafTree(context, treeUri, parentDocId, outDir, null, null, null, 0);
    }

    private static void copySafTree(Context context, Uri treeUri, String parentDocId, File outDir, ProgressCallback progress, String phase, int[] copiedFiles, int totalFiles) throws IOException {
        if (!outDir.exists() && !outDir.mkdirs()) throw new IOException("Could not create " + outDir.getAbsolutePath());
        for (SafNode child : listSafChildren(context, treeUri, parentDocId)) {
            String safeName = sanitizeFileName(child.name);
            if (safeName.length() == 0) continue;
            File out = new File(outDir, safeName);
            if (child.isDirectory()) {
                copySafTree(context, treeUri, child.docId, out, progress, phase, copiedFiles, totalFiles);
            } else {
                InputStream in = null;
                FileOutputStream fos = null;
                try {
                    Uri fileUri = safBuildDocumentUriUsingTree(treeUri, child.docId);
                    if (fileUri == null) throw new IOException("Could not build SAF file URI for " + child.name);
                    File parent = out.getParentFile();
                    if (parent != null && !parent.exists() && !parent.mkdirs()) throw new IOException("Could not create " + parent.getAbsolutePath());
                    in = context.getContentResolver().openInputStream(fileUri);
                    if (in == null) throw new IOException("Could not open SAF file " + child.name);
                    fos = new FileOutputStream(out);
                    copyStream(in, fos);
                    if (progress != null && copiedFiles != null && totalFiles > 0) {
                        copiedFiles[0]++;
                        int percent = 5 + (int) Math.min(85, (copiedFiles[0] * 85L) / totalFiles);
                        progress.onProgress(phase, percent);
                    }
                } catch (Exception ex) {
                    if (ex instanceof IOException) throw (IOException) ex;
                    throw new IOException("Could not copy SAF file " + child.name + ": " + ex);
                } finally {
                    closeQuietly(in);
                    closeQuietly(fos);
                }
            }
        }
    }

    private static String sanitizeFileName(String name) {
        if (name == null) return "";
        return name.replace('/', '_').replace('\\', '_').trim();
    }

    private static String detectUnrealZipRootPrefix(File zipFile) throws IOException {
        HashMap<String, ZipRootFlags> roots = new HashMap<String, ZipRootFlags>();
        FileInputStream raw = null;
        ZipInputStream zip = null;
        try {
            raw = new FileInputStream(zipFile);
            zip = new ZipInputStream(raw);
            ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null) {
                if (entry.isDirectory()) continue;
                String normalized = normalizeZipName(entry.getName());
                if (normalized.length() == 0) continue;
                updateZipRootFlags(roots, normalized);
            }
        } finally {
            closeQuietly(zip);
            closeQuietly(raw);
        }

        String bestPrefix = bestZipRootPrefix(roots);
        Log.i(TAG_IMPORT, "Detected ZIP file Unreal root prefix: " + bestPrefix);
        return bestPrefix;
    }

    private static String detectUnrealZipRootPrefix(Context context, Uri zipUri) throws IOException {
        HashMap<String, ZipRootFlags> roots = new HashMap<String, ZipRootFlags>();
        InputStream raw = context.getContentResolver().openInputStream(zipUri);
        if (raw == null) throw new IOException("Could not open ZIP stream");
        ZipInputStream zip = null;
        try {
            zip = new ZipInputStream(raw);
            ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null) {
                if (entry.isDirectory()) continue;
                String normalized = normalizeZipName(entry.getName());
                if (normalized.length() == 0) continue;
                updateZipRootFlags(roots, normalized);
            }
        } finally {
            closeQuietly(zip);
        }

        String bestPrefix = bestZipRootPrefix(roots);
        Log.i(TAG_IMPORT, "Detected ZIP Unreal root prefix: " + bestPrefix);
        return bestPrefix;
    }

    private static String bestZipRootPrefix(HashMap<String, ZipRootFlags> roots) {
        String bestPrefix = null;
        int bestScore = -1;
        for (Map.Entry<String, ZipRootFlags> e : roots.entrySet()) {
            ZipRootFlags flags = e.getValue();
            if (!flags.valid()) continue;
            int score = flags.score();
            if (score > bestScore || (score == bestScore && (bestPrefix == null || e.getKey().length() < bestPrefix.length()))) {
                bestScore = score;
                bestPrefix = e.getKey();
            }
        }
        return bestPrefix;
    }

    private static void updateZipRootFlags(HashMap<String, ZipRootFlags> roots, String normalizedName) {
        String[] parts = normalizedName.split("/");
        for (int i = 0; i < parts.length - 1; ++i) {
            String dir = parts[i];
            String file = parts[i + 1];
            String prefix = joinPrefix(parts, i);
            ZipRootFlags flags = roots.get(prefix);
            if (flags == null) {
                flags = new ZipRootFlags();
                roots.put(prefix, flags);
            }
            if ("System".equalsIgnoreCase(dir)) {
                if ("Core.u".equalsIgnoreCase(file)) flags.core = true;
                else if ("Engine.u".equalsIgnoreCase(file)) flags.engine = true;
                else if ("UnrealI.u".equalsIgnoreCase(file)) flags.unrealI = true;
                else if ("UnrealShare.u".equalsIgnoreCase(file)) flags.unrealShare = true;
            } else if ("Maps".equalsIgnoreCase(dir) && file.toLowerCase(Locale.ROOT).endsWith(".unr")) {
                flags.map = true;
            }
        }
    }

    private static String joinPrefix(String[] parts, int count) {
        if (count <= 0) return "";
        StringBuilder b = new StringBuilder();
        for (int i = 0; i < count; ++i) {
            if (i > 0) b.append('/');
            b.append(parts[i]);
        }
        b.append('/');
        return b.toString();
    }

    private static int countExtractableZipFiles(File zipFile, String rootPrefix) throws IOException {
        int count = 0;
        ZipFile zip = new ZipFile(zipFile);
        try {
            java.util.Enumeration<? extends ZipEntry> entries = zip.entries();
            while (entries.hasMoreElements()) {
                ZipEntry entry = entries.nextElement();
                if (entry.isDirectory()) continue;
                String normalized = normalizeZipName(entry.getName());
                if (normalized.length() == 0 || !normalized.startsWith(rootPrefix)) continue;
                String relative = normalized.substring(rootPrefix.length());
                if (relative.length() == 0 || relative.contains("../") || relative.startsWith("/")) continue;
                count++;
            }
        } finally {
            closeQuietly(zip);
        }
        return count;
    }

    private static int countExtractableZipFiles(Context context, Uri zipUri, String rootPrefix) throws IOException {
        int count = 0;
        InputStream raw = context.getContentResolver().openInputStream(zipUri);
        if (raw == null) throw new IOException("Could not open ZIP stream");
        ZipInputStream zip = null;
        try {
            zip = new ZipInputStream(raw);
            ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null) {
                if (entry.isDirectory()) continue;
                String normalized = normalizeZipName(entry.getName());
                if (normalized.length() == 0 || !normalized.startsWith(rootPrefix)) continue;
                String relative = normalized.substring(rootPrefix.length());
                if (relative.length() == 0 || relative.contains("../") || relative.startsWith("/")) continue;
                count++;
            }
        } finally {
            closeQuietly(zip);
        }
        return count;
    }

    private static void extractZipRoot(File zipFile, String rootPrefix, File targetRoot) throws IOException {
        extractZipRoot(zipFile, rootPrefix, targetRoot, null, null);
    }

    private static void extractZipRoot(File zipFile, String rootPrefix, File targetRoot, ProgressCallback progress, Context context) throws IOException {
        String targetCanonical = targetRoot.getCanonicalPath() + File.separator;
        int totalExtractFiles = countExtractableZipFiles(zipFile, rootPrefix);
        if (progress != null) progress.onProgress(context != null ? tr(context, "Installation", "Installing") : "Installing", 45);
        FileInputStream raw = null;
        ZipInputStream zip = null;
        int files = 0;
        try {
            raw = new FileInputStream(zipFile);
            zip = new ZipInputStream(raw);
            ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null) {
                if (entry.isDirectory()) continue;
                String normalized = normalizeZipName(entry.getName());
                if (normalized.length() == 0 || !normalized.startsWith(rootPrefix)) continue;
                String relative = normalized.substring(rootPrefix.length());
                if (relative.length() == 0 || relative.contains("../") || relative.startsWith("/")) continue;
                File out = new File(targetRoot, relative.replace('/', File.separatorChar));
                String outCanonical = out.getCanonicalPath();
                if (!outCanonical.startsWith(targetCanonical)) throw new IOException("Unsafe ZIP entry: " + entry.getName());
                File parent = out.getParentFile();
                if (parent != null && !parent.exists() && !parent.mkdirs()) throw new IOException("Could not create " + parent.getAbsolutePath());
                FileOutputStream fos = null;
                try {
                    fos = new FileOutputStream(out);
                    copyStream(zip, fos);
                    files++;
                    if (progress != null && totalExtractFiles > 0) {
                        int percent = 45 + (int) Math.min(45, (files * 45L) / totalExtractFiles);
                        progress.onProgress(context != null ? tr(context, "Installation", "Installing") : "Installing", percent);
                    }
                } finally {
                    closeQuietly(fos);
                }
            }
        } finally {
            closeQuietly(zip);
            closeQuietly(raw);
        }
    }


    private static void extractZipRoot(Context context, Uri zipUri, String rootPrefix, File targetRoot) throws IOException {
        extractZipRoot(context, zipUri, rootPrefix, targetRoot, null);
    }

    private static void extractZipRoot(Context context, Uri zipUri, String rootPrefix, File targetRoot, ProgressCallback progress) throws IOException {
        String targetCanonical = targetRoot.getCanonicalPath() + File.separator;
        int totalExtractFiles = countExtractableZipFiles(context, zipUri, rootPrefix);
        progress(progress, context, "Installation", "Installing", 45);
        InputStream raw = context.getContentResolver().openInputStream(zipUri);
        if (raw == null) throw new IOException("Could not open ZIP stream");
        ZipInputStream zip = null;
        int files = 0;
        try {
            zip = new ZipInputStream(raw);
            ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null) {
                if (entry.isDirectory()) continue;
                String normalized = normalizeZipName(entry.getName());
                if (normalized.length() == 0 || !normalized.startsWith(rootPrefix)) continue;
                String relative = normalized.substring(rootPrefix.length());
                if (relative.length() == 0 || relative.contains("../") || relative.startsWith("/")) continue;
                File out = new File(targetRoot, relative.replace('/', File.separatorChar));
                String outCanonical = out.getCanonicalPath();
                if (!outCanonical.startsWith(targetCanonical)) throw new IOException("Unsafe ZIP entry: " + entry.getName());
                File parent = out.getParentFile();
                if (parent != null && !parent.exists() && !parent.mkdirs()) throw new IOException("Could not create " + parent.getAbsolutePath());
                FileOutputStream fos = null;
                try {
                    fos = new FileOutputStream(out);
                    copyStream(zip, fos);
                    files++;
                    if (progress != null && totalExtractFiles > 0) {
                        int percent = 45 + (int) Math.min(45, (files * 45L) / totalExtractFiles);
                        progress.onProgress(tr(context, "Installation", "Installing"), percent);
                    }
                } finally {
                    closeQuietly(fos);
                }
            }
        } finally {
            closeQuietly(zip);
        }
    }


    private static String normalizeZipName(String name) {
        if (name == null) return "";
        String s = name.replace('\\', '/');
        while (s.startsWith("/")) s = s.substring(1);
        while (s.contains("//")) s = s.replace("//", "/");
        if (s.contains("../") || s.equals("..")) return "";
        return s;
    }


    static String candidateDescription(Context context) {
        StringBuilder b = new StringBuilder();
        for (File candidate : candidateRoots(context)) b.append("\n- ").append(candidate.getAbsolutePath());
        return b.toString();
    }
}
