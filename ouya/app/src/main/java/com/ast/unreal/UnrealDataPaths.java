package com.ast.unreal;

import android.content.Context;
import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.Environment;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.zip.ZipEntry;
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
        } catch (Throwable ignored) {}
    }


    private static void copyStream(InputStream in, FileOutputStream out) throws IOException {
        byte[] buffer = new byte[128 * 1024];
        int read;
        while ((read = in.read(buffer)) != -1) out.write(buffer, 0, read);
        out.flush();
    }

    static ImportResult importUnrealFolderFromSaf(Context context, Uri treeUri) {
        if (treeUri == null) return ImportResult.fail(tr(context, "Kein Ordner ausgewählt.", "No folder selected."));
        if (Build.VERSION.SDK_INT < 21) {
            return ImportResult.fail(tr(context,
                    "Dieser Android-Stand bietet keinen systemeigenen Ordnerimport. Bitte den Unreal-Ordner auf USB/SD oder in den App-Ordner kopieren.",
                    "This Android version has no system folder import. Please copy the Unreal folder to USB/SD or into the app folder."));
        }
        try {
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
            copySafTree(context, treeUri, unrealDocId, target);
            installDefaultConfigsIfNeeded(context, target);
            normalizeConfigForDetectedData(target);

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
        if (zipUri == null) return ImportResult.fail(tr(context, "Keine ZIP-Datei ausgewählt.", "No ZIP file selected."));
        try {
            String rootPrefix = detectUnrealZipRootPrefix(context, zipUri);
            if (rootPrefix == null) {
                return ImportResult.fail(tr(context,
                        "Die ZIP-Datei enthält keine gültige Unreal-Datenstruktur. Erwartet werden mindestens System/Core.u, System/Engine.u, UnrealI.u oder UnrealShare.u und Maps/*.unr.",
                        "The ZIP file does not contain a valid Unreal data structure. Expected at least: System/Core.u, System/Engine.u, UnrealI.u or UnrealShare.u, and Maps/*.unr."));
            }

            File target = primaryAppRoot(context);
            ensureDirectoryLayout(target);
            Log.i(TAG_IMPORT, "Importing ZIP Unreal root prefix='" + rootPrefix + "' to " + target.getAbsolutePath());
            extractZipRoot(context, zipUri, rootPrefix, target);
            installDefaultConfigsIfNeeded(context, target);
            normalizeConfigForDetectedData(target);

            if (!hasRequiredData(target, true)) {
                return ImportResult.fail(tr(context,
                        "Die ZIP-Datei wurde entpackt, aber danach fehlen weiterhin Pflichtdateien in ",
                        "The ZIP file was extracted, but required files are still missing in ") + target.getAbsolutePath());
            }
            return ImportResult.ok(target, tr(context,
                    "Unreal-Daten wurden erfolgreich aus der ZIP-Datei importiert nach:\n",
                    "Unreal data was imported successfully from the ZIP file to:\n") + target.getAbsolutePath());
        } catch (Throwable t) {
            return ImportResult.fail(tr(context,
                    "Import aus der ZIP-Datei fehlgeschlagen.",
                    "Import from the ZIP file failed."), t);
        }
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

    private static void copySafTree(Context context, Uri treeUri, String parentDocId, File outDir) throws IOException {
        if (!outDir.exists() && !outDir.mkdirs()) throw new IOException("Could not create " + outDir.getAbsolutePath());
        for (SafNode child : listSafChildren(context, treeUri, parentDocId)) {
            String safeName = sanitizeFileName(child.name);
            if (safeName.length() == 0) continue;
            File out = new File(outDir, safeName);
            if (child.isDirectory()) {
                copySafTree(context, treeUri, child.docId, out);
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
        Log.i(TAG_IMPORT, "Detected ZIP Unreal root prefix: " + bestPrefix);
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

    private static void extractZipRoot(Context context, Uri zipUri, String rootPrefix, File targetRoot) throws IOException {
        String targetCanonical = targetRoot.getCanonicalPath() + File.separator;
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
                File out = new File(targetRoot, relative.replace('/', File.separatorChar));
                String outCanonical = out.getCanonicalPath();
                if (!outCanonical.startsWith(targetCanonical)) throw new IOException("Unsafe ZIP entry: " + entry.getName());
                File parent = out.getParentFile();
                if (parent != null && !parent.exists() && !parent.mkdirs()) throw new IOException("Could not create " + parent.getAbsolutePath());
                FileOutputStream fos = null;
                try {
                    fos = new FileOutputStream(out);
                    copyStream(zip, fos);
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
