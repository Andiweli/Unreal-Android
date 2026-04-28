package com.ast.unreal;

import android.content.Context;
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
import java.util.HashSet;
import java.util.List;
import java.util.Locale;

final class UnrealDataPaths {
    static final String TAG_STARTUP = "UE1Startup";
    static final String TAG_CONFIG = "UE1Config";
    static final String EXTRA_UNREAL_ROOT = "com.ast.unreal.EXTRA_UNREAL_ROOT";

    static final String[] UNREAL_DIRS = {
            "System", "Maps", "Textures", "Sounds", "Music", "Meshes", "Help", "Web", "Save", "Cache"
    };

    private UnrealDataPaths() {}

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

    static String candidateDescription(Context context) {
        StringBuilder b = new StringBuilder();
        for (File candidate : candidateRoots(context)) b.append("\n- ").append(candidate.getAbsolutePath());
        return b.toString();
    }
}
