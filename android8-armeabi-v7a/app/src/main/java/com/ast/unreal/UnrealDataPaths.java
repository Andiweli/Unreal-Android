package com.ast.unreal;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Environment;
import android.os.Build;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
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
        Locale locale;
        if (Build.VERSION.SDK_INT >= 24) {
            locale = context.getResources().getConfiguration().getLocales().get(0);
        } else {
            locale = context.getResources().getConfiguration().locale;
        }
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
            return Document.MIME_TYPE_DIR.equals(mimeType);
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
        ArrayList<File> out = new ArrayList<>();
        HashSet<String> seen = new HashSet<>();

        addCandidate(out, seen, primaryAppRoot(context));
        File[] appExternalDirs = context.getExternalFilesDirs(null);
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
        addCandidate(out, seen, new File("/storage/emulated/0/Unreal"));
        addCandidate(out, seen, new File("/sdcard/Unreal"));
        addCandidate(out, seen, new File("/storage/sdcard0/Unreal"));
        addCandidate(out, seen, new File("/mnt/sdcard/Unreal"));
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

    static boolean hasRequiredData(File root) {
        return hasRequiredData(root, false);
    }

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
        for (File f : files) {
            if (f.isFile() && expectedName.equalsIgnoreCase(f.getName())) return f;
        }
        return null;
    }

    private static boolean hasAnyMap(File mapsDir) {
        File[] files = mapsDir.listFiles((dir, name) -> name.toLowerCase(Locale.ROOT).endsWith(".unr"));
        return files != null && files.length > 0;
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
        try (InputStream input = context.getAssets().open(asset); FileOutputStream fos = new FileOutputStream(out)) {
            byte[] buf = new byte[16 * 1024];
            int read;
            while ((read = input.read(buf)) >= 0) fos.write(buf, 0, read);
            fos.flush();
            Log.i(TAG_CONFIG, "Installed default config: " + out.getAbsolutePath());
        } catch (IOException ex) {
            Log.w(TAG_CONFIG, "Could not install default config " + out.getAbsolutePath() + ": " + ex);
        }
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
            String text = new String(Files.readAllBytes(file.toPath()), StandardCharsets.UTF_8);
            String patched = text
                    .replace("UnrealShare.SinglePlayer", "UnrealI.SinglePlayer")
                    .replace("UnrealShare.DeathMatchGame", "UnrealI.DeathMatchGame")
                    .replace("DefaultGame=UnrealShare.", "DefaultGame=UnrealI.")
                    .replace("DefaultServerGame=UnrealShare.", "DefaultServerGame=UnrealI.");
            if (!patched.equals(text)) {
                Files.write(file.toPath(), patched.getBytes(StandardCharsets.UTF_8));
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
            ensureAndroidControllerDirectPatch(systemDir);
            Log.i(TAG_CONFIG, "Config root: " + root.getAbsolutePath());
            Log.i(TAG_CONFIG, "User.ini: " + new File(systemDir, "User.ini").getAbsolutePath());
        } catch (Throwable t) {
            Log.e(TAG_CONFIG, "Config bootstrap failed for root=" + root.getAbsolutePath(), t);
        }
    }


    private static void ensureAndroidControllerDirectPatch(File systemDir) {
        // UNREAL_ANDROID_CONFIG_PRESERVE_V139
        // Keep the Android controller/touch runtime flags available, but do not
        // re-append or rewrite [Engine.Input] on every app update.  Reinstalling
        // the APK retains /Android/data, therefore existing user key bindings must
        // be treated as authoritative.
        patchNsdlControllerDefaults(new File(systemDir, "Unreal.ini"));
        patchNsdlControllerDefaults(new File(systemDir, "Default.ini"));
        appendControllerInputFallbacks(new File(systemDir, "User.ini"));
    }

    private static void patchNsdlControllerDefaults(File file) {
        if (file == null || !file.isFile()) return;
        try {
            String text = new String(Files.readAllBytes(file.toPath()), StandardCharsets.UTF_8);
            String patched = text;
            patched = setIniValue(patched, "NSDLDrv.NSDLClient", "UseJoystick", "True");
            patched = setIniValue(patched, "NSDLDrv.NSDLClient", "AndroidNativeController", "True");
            patched = setIniValue(patched, "NSDLDrv.NSDLClient", "AndroidNativeDirectInput", "True"); // UNREAL_ANDROID_CONFIG_PRESERVE_V139
            patched = setIniValue(patched, "NSDLDrv.NSDLClient", "AndroidNativeRightStickScale", "1.00"); // UNREAL_ANDROID_CONFIG_PRESERVE_V139
            patched = setIniValue(patched, "NSDLDrv.NSDLClient", "AndroidNativeLeftStickDeadzone", "0.08"); // UNREAL_ANDROID_CONFIG_PRESERVE_V139
            patched = setIniValue(patched, "NSDLDrv.NSDLClient", "AndroidNativeRightStickDeadzone", "0.10"); // UNREAL_ANDROID_CONFIG_PRESERVE_V139
            patched = setIniValue(patched, "NSDLDrv.NSDLClient", "AndroidNativeTriggerDeadzone", "0.12"); // UNREAL_ANDROID_CONFIG_PRESERVE_V139
            patched = setIniValue(patched, "NSDLDrv.NSDLClient", "AndroidNativeAxisCurve", "1.00"); // UNREAL_ANDROID_CONFIG_PRESERVE_V139
            patched = setIniValue(patched, "NSDLDrv.NSDLClient", "DeadZoneXYZ", "0.10");
            patched = setIniValue(patched, "NSDLDrv.NSDLClient", "DeadZoneRUV", "0.10");
            patched = setIniValue(patched, "NSDLDrv.NSDLClient", "ScaleXYZ", "100.0");
            patched = setIniValue(patched, "NSDLDrv.NSDLClient", "ScaleRUV", "100.0");
            if (!patched.equals(text)) {
                Files.write(file.toPath(), patched.getBytes(StandardCharsets.UTF_8));
                Log.i(TAG_CONFIG, "Patched Android controller defaults: " + file.getAbsolutePath());
            }
        } catch (IOException ex) {
            Log.w(TAG_CONFIG, "Could not patch controller defaults in " + file.getAbsolutePath() + ": " + ex);
        }
    }

    private static void appendControllerInputFallbacks(File file) {
        if (file == null) return;
        try {
            String text = file.isFile()
                    ? new String(Files.readAllBytes(file.toPath()), StandardCharsets.UTF_8)
                    : "[DefaultPlayer]\nName=Player\nClass=UnrealShare.MaleOne\n\n";
            if (text.contains("UNREAL_ANDROID_CONTROLLER_DIRECT_V122")
                    || hasAnyEngineInputBindings(text)
                    || hasAndroidControllerFallbackBindings(text)) {
                Log.i(TAG_CONFIG, "Preserved existing input bindings: " + file.getAbsolutePath()); // UNREAL_ANDROID_CONFIG_PRESERVE_V139
                return;
            }
            String block = "\n\n; UNREAL_ANDROID_CONTROLLER_DIRECT_V122 UNREAL_ANDROID_CONFIG_PRESERVE_V139\n" +
                    "; Robust Android controller fallbacks. Direct mode uses W/A/S/D + mouse buttons for gameplay,\n" +
                    "; while these Joy*/friendly aliases keep Customize Controls and SDL fallback usable.\n" +
                    "[Engine.Input]\n" +
                    "LeftMouse=Fire\n" +
                    "RightMouse=AltFire\n" +
                    "MouseX=Axis aMouseX Speed=6.0\n" +
                    "MouseY=Axis aMouseY Speed=6.0\n" +
                    "W=MoveForward\n" +
                    "S=MoveBackward\n" +
                    "A=StrafeLeft\n" +
                    "D=StrafeRight\n" +
                    "Space=Jump\n" +
                    "C=Duck\n" +
                    "G=Grab\n" +
                    "Joy1=Jump\n" +
                    "Joy2=Duck\n" +
                    "Joy3=Grab\n" +
                    "Joy4=Walking\n" +
                    "Joy5=ActivateTranslator\n" +
                    "Joy8=Duck\n" +
                    "Joy9=CenterView\n" +
                    "Joy10=PrevWeapon\n" +
                    "Joy11=NextWeapon\n" +
                    "Joy12=AltFire\n" +
                    "Joy13=Fire\n" +
                    "Joy14=TurnLeft\n" +
                    "Joy15=TurnRight\n" +
                    "Joy16=LookUp\n" +
                    "JoyX=Axis aStrafe Speed=1\n" +
                    "JoyY=Axis aBaseY Speed=1\n" +
                    "JoyU=Axis aTurn Speed=1\n" +
                    "JoyV=Axis aLookUp Speed=-1\n" +
                    "JoyPovRight=NextWeapon\n" +
                    "JoyPovLeft=PrevWeapon\n" +
                    "JoyPovUp=InventoryPrevious\n" +
                    "JoyPovDown=InventoryNext\n" +
                    "UnknownD8=StrafeLeft\n" +
                    "UnknownD9=StrafeRight\n" +
                    "UnknownDA=MoveForward\n" +
                    "UnknownDF=MoveBackward\n" +
                    "UnknownEA=LookDown\n";
            Files.write(file.toPath(), (text + block).getBytes(StandardCharsets.UTF_8));
            Log.i(TAG_CONFIG, "Appended Android controller input fallbacks: " + file.getAbsolutePath());
        } catch (IOException ex) {
            Log.w(TAG_CONFIG, "Could not append controller input fallbacks in " + file.getAbsolutePath() + ": " + ex);
        }
    }

    private static boolean hasAnyEngineInputBindings(String text) {
        // UNREAL_ANDROID_CONFIG_PRESERVE_V139
        // A retained install may already contain user-customized controls.  Do not
        // append a second default [Engine.Input] block because UE1 keeps the last
        // duplicate key and that would effectively reset the user's bindings.
        if (text == null) return false;
        String[] lines = text.replace("\r\n", "\n").replace('\r', '\n').split("\n");
        boolean inInput = false;
        for (String raw : lines) {
            String t = raw.trim();
            if (t.length() == 0 || t.startsWith(";") || t.startsWith("#")) continue;
            if (t.startsWith("[") && t.endsWith("]")) {
                inInput = t.equalsIgnoreCase("[Engine.Input]");
                continue;
            }
            if (inInput && t.indexOf('=') > 0) return true;
        }
        return false;
    }

    private static boolean hasAndroidControllerFallbackBindings(String text) {
        // UNREAL_ANDROID_CONFIG_PRESERVE_V139
        if (text == null) return false;
        return text.contains("UnknownD8=StrafeLeft")
                || text.contains("UnknownDA=MoveForward")
                || text.contains("Joy11=NextWeapon")
                || text.contains("Joy13=Fire");
    }

    private static String setIniValue(String text, String section, String key, String value) {
        if (text == null) text = "";
        String normalized = text.replace("\r\n", "\n").replace('\r', '\n');
        String sectionHeader = "[" + section + "]";
        java.util.regex.Pattern sectionPattern = java.util.regex.Pattern.compile("(?im)^\\[" + java.util.regex.Pattern.quote(section) + "\\]\\s*$");
        java.util.regex.Matcher sectionMatcher = sectionPattern.matcher(normalized);
        if (!sectionMatcher.find()) {
            if (normalized.length() > 0 && !normalized.endsWith("\n")) normalized += "\n";
            return normalized + "\n" + sectionHeader + "\n" + key + "=" + value + "\n";
        }

        int sectionStart = sectionMatcher.end();
        java.util.regex.Pattern nextSectionPattern = java.util.regex.Pattern.compile("(?m)^\\[[^\\]]+\\]\\s*$");
        java.util.regex.Matcher nextSectionMatcher = nextSectionPattern.matcher(normalized);
        int sectionEnd = normalized.length();
        while (nextSectionMatcher.find(sectionStart)) {
            sectionEnd = nextSectionMatcher.start();
            break;
        }

        String before = normalized.substring(0, sectionStart);
        String body = normalized.substring(sectionStart, sectionEnd);
        String after = normalized.substring(sectionEnd);
        java.util.regex.Pattern keyPattern = java.util.regex.Pattern.compile("(?im)^" + java.util.regex.Pattern.quote(key) + "\\s*=.*$");
        java.util.regex.Matcher keyMatcher = keyPattern.matcher(body);
        if (keyMatcher.find()) {
            body = keyMatcher.replaceAll(java.util.regex.Matcher.quoteReplacement(key + "=" + value));
        } else {
            if (!body.endsWith("\n")) body += "\n";
            body += key + "=" + value + "\n";
        }
        return before + body + after;
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
        try (FileOutputStream out = new FileOutputStream(target)) {
            if (fallbackText != null && fallbackText.length() > 0) out.write(fallbackText.getBytes("UTF-8"));
            out.flush();
            Log.i(TAG_CONFIG, targetName + " created from fallback: " + target.getAbsolutePath());
        }
    }

    private static void copyFile(File src, File dst) throws IOException {
        File parent = dst.getParentFile();
        if (parent != null && !parent.exists() && !parent.mkdirs()) throw new IOException("Could not create " + parent.getAbsolutePath());
        try (FileInputStream in = new FileInputStream(src); FileOutputStream out = new FileOutputStream(dst)) {
            copyStream(in, out);
        }
    }

    private static void copyStream(InputStream in, FileOutputStream out) throws IOException {
        byte[] buffer = new byte[128 * 1024];
        int read;
        while ((read = in.read(buffer)) != -1) out.write(buffer, 0, read);
        out.flush();
    }

    static ImportResult importUnrealFolderFromSaf(Context context, Uri treeUri) {
        if (treeUri == null) return ImportResult.fail(tr(context, "Kein Ordner ausgewählt.", "No folder selected."));
        try {
            String selectedDocId = DocumentsContract.getTreeDocumentId(treeUri);
            if (selectedDocId == null || selectedDocId.length() == 0) return ImportResult.fail(tr(context, "Der ausgewählte Ordner konnte nicht gelesen werden.", "The selected folder could not be read."));

            String unrealDocId = findSafUnrealRootDocId(context, treeUri, selectedDocId);
            if (unrealDocId == null) {
                return ImportResult.fail(tr(context, "Der ausgewählte Ordner enthält keine gültigen Unreal-Daten. Bitte den Ordner 'Unreal' wählen. Erwartet werden mindestens System/Core.u, System/Engine.u, UnrealI.u oder UnrealShare.u und Maps/*.unr.", "The selected folder does not contain valid Unreal data. Please select the 'Unreal' folder. Expected at least: System/Core.u, System/Engine.u, UnrealI.u or UnrealShare.u, and Maps/*.unr."));
            }

            File target = primaryAppRoot(context);
            ensureDirectoryLayout(target);
            Log.i(TAG_IMPORT, "Importing SAF Unreal folder to " + target.getAbsolutePath());
            copySafTree(context, treeUri, unrealDocId, target);
            installDefaultConfigsIfNeeded(context, target);
            normalizeConfigForDetectedData(target);
            ensureAndroidControllerDirectPatch(new File(target, "System"));

            if (!hasRequiredData(target, true)) {
                return ImportResult.fail(tr(context, "Der Ordner wurde kopiert, aber danach fehlen weiterhin Pflichtdateien in ", "The folder was copied, but required files are still missing in ") + target.getAbsolutePath());
            }
            return ImportResult.ok(target, tr(context, "Unreal-Daten wurden erfolgreich importiert nach:\n", "Unreal data was imported successfully to:\n") + target.getAbsolutePath());
        } catch (Throwable t) {
            return ImportResult.fail(tr(context, "Import aus dem ausgewählten Ordner fehlgeschlagen.", "Import from the selected folder failed."), t);
        }
    }

    static ImportResult importUnrealZip(Context context, Uri zipUri) {
        if (zipUri == null) return ImportResult.fail(tr(context, "Keine ZIP-Datei ausgewählt.", "No ZIP file selected."));
        try {
            String rootPrefix = detectUnrealZipRootPrefix(context, zipUri);
            if (rootPrefix == null) {
                return ImportResult.fail(tr(context, "Die ZIP-Datei enthält keine gültige Unreal-Datenstruktur. Erwartet werden mindestens System/Core.u, System/Engine.u, UnrealI.u oder UnrealShare.u und Maps/*.unr.", "The ZIP file does not contain a valid Unreal data structure. Expected at least: System/Core.u, System/Engine.u, UnrealI.u or UnrealShare.u, and Maps/*.unr."));
            }

            File target = primaryAppRoot(context);
            ensureDirectoryLayout(target);
            Log.i(TAG_IMPORT, "Importing ZIP Unreal root prefix='" + rootPrefix + "' to " + target.getAbsolutePath());
            extractZipRoot(context, zipUri, rootPrefix, target);
            installDefaultConfigsIfNeeded(context, target);
            normalizeConfigForDetectedData(target);
            ensureAndroidControllerDirectPatch(new File(target, "System"));

            if (!hasRequiredData(target, true)) {
                return ImportResult.fail(tr(context, "Die ZIP-Datei wurde entpackt, aber danach fehlen weiterhin Pflichtdateien in ", "The ZIP file was extracted, but required files are still missing in ") + target.getAbsolutePath());
            }
            return ImportResult.ok(target, tr(context, "Unreal-Daten wurden erfolgreich aus der ZIP-Datei importiert nach:\n", "Unreal data was imported successfully from the ZIP file to:\n") + target.getAbsolutePath());
        } catch (Throwable t) {
            return ImportResult.fail(tr(context, "Import aus der ZIP-Datei fehlgeschlagen.", "Import from the ZIP file failed."), t);
        }
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
        ArrayList<SafNode> out = new ArrayList<>();
        ContentResolver resolver = context.getContentResolver();
        Uri childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(treeUri, parentDocId);
        String[] projection = new String[] {
                Document.COLUMN_DOCUMENT_ID,
                Document.COLUMN_DISPLAY_NAME,
                Document.COLUMN_MIME_TYPE
        };
        try (Cursor cursor = resolver.query(childrenUri, projection, null, null, null)) {
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
                Uri fileUri = DocumentsContract.buildDocumentUriUsingTree(treeUri, child.docId);
                File parent = out.getParentFile();
                if (parent != null && !parent.exists() && !parent.mkdirs()) throw new IOException("Could not create " + parent.getAbsolutePath());
                try (InputStream in = context.getContentResolver().openInputStream(fileUri); FileOutputStream fos = new FileOutputStream(out)) {
                    if (in == null) throw new IOException("Could not open SAF file " + child.name);
                    copyStream(in, fos);
                }
            }
        }
    }

    private static String sanitizeFileName(String name) {
        if (name == null) return "";
        return name.replace('/', '_').replace('\\', '_').trim();
    }

    private static String detectUnrealZipRootPrefix(Context context, Uri zipUri) throws IOException {
        HashMap<String, ZipRootFlags> roots = new HashMap<>();
        InputStream raw = context.getContentResolver().openInputStream(zipUri);
        if (raw == null) throw new IOException("Could not open ZIP stream");
        try (ZipInputStream zip = new ZipInputStream(raw)) {
            ZipEntry entry;
            while ((entry = zip.getNextEntry()) != null) {
                if (entry.isDirectory()) continue;
                String normalized = normalizeZipName(entry.getName());
                if (normalized.length() == 0) continue;
                updateZipRootFlags(roots, normalized);
            }
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
        try (ZipInputStream zip = new ZipInputStream(raw)) {
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
                try (FileOutputStream fos = new FileOutputStream(out)) {
                    copyStream(zip, fos);
                }
            }
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
