// SPDX-FileCopyrightText: 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

package org.yuzu.yuzu_emu.utils

import android.net.Uri
import androidx.documentfile.provider.DocumentFile
import java.io.File
import java.util.*
import org.yuzu.yuzu_emu.model.MinimalDocumentFile

class DocumentsTree {
    private var root: DocumentsNode? = null

    fun setRoot(rootUri: Uri?) {
        val newRoot = DocumentsNode()
        newRoot.uri = rootUri
        newRoot.isDirectory = true
        root = newRoot
    }

    fun openContentUri(filepath: String?, openMode: String?): Int {
        if (filepath.isNullOrEmpty() || filepath.startsWith("content://")) return -1
        try {
            val node = resolvePath(filepath)
            if (node != null) {
                val uriStr = node.uri?.toString()
                if (!uriStr.isNullOrEmpty()) {
                    val fd = FileUtil.openContentUri(uriStr, openMode)
                    if (fd >= 0) return fd
                }
            }
            return FileUtil.openContentUri(filepath, openMode)
        } catch (_: Throwable) {
            return -1
        }
    }

    fun getFileSize(filepath: String?): Long {
        if (filepath.isNullOrEmpty() || filepath.startsWith("content://")) return 0L
        try {
            val node = resolvePath(filepath)
            if (node != null && !node.isDirectory) {
                val uriStr = node.uri?.toString()
                if (!uriStr.isNullOrEmpty()) {
                    val sz = FileUtil.getFileSize(uriStr)
                    if (sz > 0L) return sz
                }
            }
            return FileUtil.getFileSize(filepath)
        } catch (_: Throwable) {
            return 0L
        }
    }

    fun exists(filepath: String?): Boolean {
        if (filepath.isNullOrEmpty() || filepath.startsWith("content://")) return false
        try {
            if (resolvePath(filepath) != null) return true
            return FileUtil.exists(filepath, suppressLog = true)
        } catch (_: Throwable) {
            return false
        }
    }

    fun isDirectory(filepath: String?): Boolean {
        if (filepath.isNullOrEmpty() || filepath.startsWith("content://")) return false
        try {
            val node = resolvePath(filepath)
            return node != null && node.isDirectory
        } catch (_: Throwable) {
            return false
        }
    }

    fun getParentDirectory(filepath: String?): String {
        if (filepath.isNullOrEmpty()) return ""
        if (filepath.startsWith("content://")) return ""
        try {
            val node = resolvePath(filepath) ?: return filepath
            val parentNode = node.parent
            if (parentNode != null && parentNode.isDirectory) {
                return parentNode.uri?.toString() ?: filepath
            }
            return node.uri?.toString() ?: filepath
        } catch (_: Throwable) {
            return filepath
        }
    }

    fun getFilename(filepath: String?): String {
        if (filepath.isNullOrEmpty()) return ""
        if (filepath.startsWith("content://")) return ""
        try {
            val node = resolvePath(filepath)
            if (node != null && !node.name.isNullOrEmpty()) {
                return node.name ?: filepath
            }
        } catch (_: Throwable) {}
        return filepath
    }

    private fun resolvePath(filepath: String?): DocumentsNode? {
        if (filepath.isNullOrEmpty() || filepath.startsWith("content://")) return null
        val currentRoot = root ?: return null
        try {
            val tokens = StringTokenizer(filepath, File.separator, false)
            var iterator: DocumentsNode? = currentRoot
            while (tokens.hasMoreTokens()) {
                val token = tokens.nextToken()
                if (token.isEmpty()) continue
                iterator = find(iterator, token) ?: return null
            }
            return iterator
        } catch (_: Throwable) {
            return null
        }
    }

    private fun find(parent: DocumentsNode?, filename: String?): DocumentsNode? {
        if (parent == null || filename.isNullOrEmpty()) return null
        if (parent.isDirectory && !parent.loaded) {
            structTree(parent)
        }
        return parent.children[filename]
    }

    /**
     * Construct current level directory tree
     * @param parent parent node of this level
     */
    private fun structTree(parent: DocumentsNode) {
        try {
            val parentUri = parent.uri ?: return
            val documents = FileUtil.listFiles(parentUri)
            for (document in documents) {
                val node = DocumentsNode(document)
                node.parent = parent
                val name = node.name
                if (!name.isNullOrEmpty()) {
                    parent.children[name] = node
                }
            }
            parent.loaded = true
        } catch (_: Throwable) {
            parent.loaded = true
        }
    }

    private class DocumentsNode {
        var parent: DocumentsNode? = null
        val children: MutableMap<String?, DocumentsNode> = HashMap()
        var name: String? = null
        var uri: Uri? = null
        var loaded = false
        var isDirectory = false

        constructor()
        constructor(document: MinimalDocumentFile) {
            name = document.filename
            uri = document.uri
            isDirectory = document.isDirectory
            loaded = !isDirectory
        }

        private constructor(document: DocumentFile, isCreateDir: Boolean) {
            name = document.name
            uri = document.uri
            isDirectory = isCreateDir
            loaded = true
        }

        private fun rename(name: String) {
            val p = parent ?: return
            val oldName = this.name
            if (oldName != null) {
                p.children.remove(oldName)
            }
            this.name = name
            p.children[name] = this
        }
    }

    companion object {
        fun isNativePath(path: String?): Boolean {
            return if (!path.isNullOrEmpty()) {
                path[0] == '/'
            } else {
                false
            }
        }
    }
}
