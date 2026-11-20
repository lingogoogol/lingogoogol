package io.github.lingogoogol

import android.content.Context
import android.net.Uri
import android.provider.DocumentsContract
import android.provider.DocumentsContract.Document
import java.util.concurrent.ConcurrentHashMap

/**
 * Local files for uploads and downloads through the Storage Access Framework
 * (storage/local/android.cpp). A path is a document URI from the system picker,
 * or "<directory URI>|<relative/path>" for a download target below a picked
 * folder; missing folders and files on that path are created on demand.
 * Called from native transfer threads.
 */
class LocalFiles(private val context: Context) {
    private val resolver = context.contentResolver
    private val resolved = ConcurrentHashMap<String, Uri>()

    private class Target(val base: Uri, val segments: List<String>)

    private fun split(path: String): Target {
        val separator = path.indexOf('|')
        if (separator < 0) return Target(Uri.parse(path), emptyList())
        return Target(Uri.parse(path.substring(0, separator)),
            path.substring(separator + 1).split('/').filter { it.isNotEmpty() })
    }

    // A picked folder arrives as a tree URI; operations need its root document.
    private fun document(uri: Uri): Uri =
        if (DocumentsContract.isTreeUri(uri) && !DocumentsContract.isDocumentUri(context, uri))
            DocumentsContract.buildDocumentUriUsingTree(uri, DocumentsContract.getTreeDocumentId(uri))
        else uri

    private class Entry(val uri: Uri, val name: String, val directory: Boolean)

    private fun list(parent: Uri): List<Entry> {
        val children = DocumentsContract.buildChildDocumentsUriUsingTree(parent, DocumentsContract.getDocumentId(parent))
        val entries = mutableListOf<Entry>()
        resolver.query(children, arrayOf(Document.COLUMN_DOCUMENT_ID, Document.COLUMN_DISPLAY_NAME,
            Document.COLUMN_MIME_TYPE), null, null, null)?.use { cursor ->
            while (cursor.moveToNext()) {
                entries += Entry(DocumentsContract.buildDocumentUriUsingTree(parent, cursor.getString(0)),
                    cursor.getString(1) ?: "", cursor.getString(2) == Document.MIME_TYPE_DIR)
            }
        }
        return entries
    }

    private fun resolve(path: String, create: Boolean, directory: Boolean): Uri? {
        resolved[path]?.let { return it }
        val target = split(path)
        var current = document(target.base)
        for ((index, name) in target.segments.withIndex()) {
            val folder = index < target.segments.lastIndex || directory
            current = list(current).firstOrNull { it.name == name }?.uri
                ?: (if (create) DocumentsContract.createDocument(resolver, current,
                    if (folder) Document.MIME_TYPE_DIR else "application/octet-stream", name) else null)
                ?: return null
        }
        if (target.segments.isNotEmpty()) resolved[path] = current
        return current
    }

    /** "<kind> <size> <modified>": kind 0 missing, 1 file, 2 directory. */
    fun stat(path: String): String {
        val uri = runCatching { resolve(path, false, false) }.getOrNull() ?: return "0 0 0"
        resolver.query(uri, arrayOf(Document.COLUMN_MIME_TYPE, Document.COLUMN_SIZE, Document.COLUMN_LAST_MODIFIED),
            null, null, null)?.use { cursor ->
            if (cursor.moveToFirst()) {
                val kind = if (cursor.getString(0) == Document.MIME_TYPE_DIR) 2 else 1
                return "$kind ${if (cursor.isNull(1)) 0 else cursor.getLong(1)} ${if (cursor.isNull(2)) 0 else cursor.getLong(2)}"
            }
        }
        return "0 0 0"
    }

    /** One "<kind> <uri>" line per child, sorted by display name. */
    fun children(path: String): String {
        val parent = resolve(path, false, true) ?: return ""
        return list(parent).sortedBy { it.name }.joinToString("\n") { "${if (it.directory) 2 else 1} ${it.uri}" }
    }

    fun name(path: String): String {
        val target = split(path)
        if (target.segments.isNotEmpty()) return target.segments.last()
        resolver.query(document(target.base), arrayOf(Document.COLUMN_DISPLAY_NAME), null, null, null)?.use { cursor ->
            if (cursor.moveToFirst()) return cursor.getString(0) ?: ""
        }
        return ""
    }

    fun display(path: String): String {
        val target = split(path)
        return (listOf(name(target.base.toString())) + target.segments).joinToString("/")
    }

    /** A detached file descriptor as text, or "" on failure. Modes: r, rw, rwt. */
    fun open(path: String, mode: String): String = runCatching {
        val uri = resolve(path, mode != "r", false) ?: return ""
        resolver.openFileDescriptor(uri, mode)?.detachFd()?.toString() ?: ""
    }.getOrDefault("")

    fun makeDirectories(path: String): Boolean = runCatching { resolve(path, true, true) != null }.getOrDefault(false)

    fun remove(path: String): Boolean = runCatching {
        val uri = resolve(path, false, false) ?: return true
        resolved.keys.removeIf { it == path || it.startsWith("$path/") }
        DocumentsContract.deleteDocument(resolver, uri)
    }.getOrDefault(false)
}
