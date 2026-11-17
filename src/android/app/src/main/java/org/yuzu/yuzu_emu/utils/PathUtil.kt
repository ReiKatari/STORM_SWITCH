// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

package org.yuzu.yuzu_emu.utils

import android.net.Uri
import android.provider.DocumentsContract
import java.io.File
import org.yuzu.yuzu_emu.YuzuApplication

object PathUtil {

    /**
     * Converts a content:// URI from the Storage Access Framework to a real filesystem path.
     */
    fun getPathFromUri(uri: Uri): String? {
        if (uri.scheme == "file") {
            return uri.path
        }
        val docId = try {
            if (DocumentsContract.isDocumentUri(YuzuApplication.appContext, uri)) {
                DocumentsContract.getDocumentId(uri)
            } else if (DocumentsContract.isTreeUri(uri)) {
                DocumentsContract.getTreeDocumentId(uri)
            } else {
                DocumentsContract.getDocumentId(uri)
            }
        } catch (_: Exception) {
            try {
                val path = uri.path ?: ""
                when {
                    path.contains("/document/") -> Uri.decode(path.substringAfter("/document/"))
                    path.contains("/tree/") -> Uri.decode(path.substringAfter("/tree/"))
                    else -> null
                }
            } catch (_: Exception) {
                null
            }
        } ?: return null

        if (docId.startsWith("primary:")) {
            val relativePath = docId.substringAfter(":")
            val primaryStoragePath = android.os.Environment.getExternalStorageDirectory().absolutePath
            return "$primaryStoragePath/$relativePath"
        }

        // external SD cards and other volumes)
        val storageIdString = docId.substringBefore(":")
        val removablePath = getRemovableStoragePath(storageIdString)
        if (removablePath != null) {
            return "$removablePath/${docId.substringAfter(":")}"
        }

        return null
    }

    /**
     * Converts a filesystem path into an initial SAF Document Uri for EXTRA_INITIAL_URI.
     */
    fun getInitialUriForPath(path: String): Uri? {
        try {
            val file = File(path)
            if (!file.exists()) {
                file.mkdirs()
            }
            val primaryPath = android.os.Environment.getExternalStorageDirectory().absolutePath
            val canonical = file.canonicalPath
            if (canonical.startsWith(primaryPath)) {
                val relative = canonical.removePrefix(primaryPath).trimStart('/', '\\')
                val docId = if (relative.isEmpty()) "primary:" else "primary:$relative"
                return DocumentsContract.buildDocumentUri("com.android.externalstorage.documents", docId)
            }
            val storagePrefix = "/storage/"
            if (canonical.startsWith(storagePrefix)) {
                val parts = canonical.removePrefix(storagePrefix).split(File.separatorChar)
                if (parts.isNotEmpty()) {
                    val volumeId = parts[0]
                    if (volumeId != "emulated" && volumeId != "self") {
                        val relative = parts.drop(1).joinToString("/")
                        val docId = if (relative.isEmpty()) "$volumeId:" else "$volumeId:$relative"
                        return DocumentsContract.buildDocumentUri("com.android.externalstorage.documents", docId)
                    }
                }
            }
        } catch (_: Exception) {}
        return null
    }

    /**
     * Validates that a path is a valid, writable directory.
     * Creates the directory if it doesn't exist.
     */
    fun validateDirectory(path: String): Boolean {
        val dir = File(path)

        if (!dir.exists()) {
            if (!dir.mkdirs()) {
                return false
            }
        }

        return dir.isDirectory && dir.canWrite()
    }

    /**
     * Copies a directory recursively from source to destination.
     */
    fun copyDirectory(source: File, destination: File, overwrite: Boolean = true): Boolean {
        return try {
            source.copyRecursively(destination, overwrite)
            true
        } catch (_: Exception) {
            false
        }
    }

    /**
     * Checks if a directory has any content.
     */
    fun hasContent(path: String): Boolean {
        val dir = File(path)
        return dir.exists() && dir.listFiles()?.isNotEmpty() == true
    }

    /** The folders of a NAND directory that hold what the user installed and saved. */
    val NAND_CONTENT_FOLDERS = listOf("system", "user")

    enum class CopyResult { COPIED, CANCELLED, FAILED }

    fun directorySize(directory: File): Long =
        directory.walkTopDown().filter { it.isFile }.sumOf { it.length() }

    /** Whether [path] is [parent] or a folder inside it. */
    fun isSameOrInside(path: String, parent: String): Boolean {
        val root = File(parent).canonicalFile
        return generateSequence(File(path).canonicalFile) { it.parentFile }.any { it == root }
    }

    /**
     * Copies the folders called [names] from [sourceRoot] into [destinationRoot].
     */
    fun copyFolders(
        sourceRoot: File,
        destinationRoot: File,
        names: List<String>,
        shouldStop: (copiedBytes: Long) -> Boolean
    ): CopyResult {
        val copy = FolderCopy(shouldStop)
        val copied = try {
            names.all { name ->
                val source = File(sourceRoot, name)
                !source.exists() || copy.copy(source, File(destinationRoot, name))
            }
        } catch (_: Exception) {
            false
        }
        if (copied) {
            return CopyResult.COPIED
        }
        copy.created.asReversed().forEach { it.delete() }
        return if (copy.stopped) CopyResult.CANCELLED else CopyResult.FAILED
    }

    private class FolderCopy(private val shouldStop: (Long) -> Boolean) {
        val created = ArrayList<File>()
        var stopped = false
        private var copiedBytes = 0L
        private val buffer = ByteArray(1024 * 1024)

        fun copy(source: File, destination: File): Boolean {
            if (source.isDirectory) {
                if (!destination.exists()) {
                    if (!destination.mkdirs()) {
                        return false
                    }
                    created.add(destination)
                }
                val children = source.listFiles() ?: return false
                return children.all { copy(it, File(destination, it.name)) }
            }

            if (!destination.exists()) {
                created.add(destination)
            }
            source.inputStream().use { input ->
                destination.outputStream().use { output ->
                    while (true) {
                        val read = input.read(buffer)
                        if (read < 0) {
                            break
                        }
                        output.write(buffer, 0, read)
                        copiedBytes += read
                        if (shouldStop(copiedBytes)) {
                            stopped = true
                            return false
                        }
                    }
                }
            }
            return destination.length() == source.length()
        }
    }


    fun truncatePathForDisplay(path: String, maxLength: Int = 45): String {
        return if (path.length > maxLength) {
            "...${path.takeLast(maxLength - 3)}"
        } else {
            path
        }
    }

    fun getRemovableStoragePath(idString: String): String? {
        val possibleMountPaths = listOf("/mnt/media_rw/$idString", "/storage/$idString")

        for (mountPath in possibleMountPaths) {
            val pathFile = File(mountPath);
            if (pathFile.exists()) {
                return pathFile.absolutePath
            }
        }

        return null
    }
}
