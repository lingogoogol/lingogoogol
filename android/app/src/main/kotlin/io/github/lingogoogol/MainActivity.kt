package io.github.lingogoogol

import android.accounts.Account
import android.content.ActivityNotFoundException
import android.content.ClipData
import android.content.ClipboardManager
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import androidx.activity.addCallback
import androidx.activity.result.IntentSenderRequest
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AlertDialog
import com.google.android.gms.auth.api.identity.AuthorizationRequest
import com.google.android.gms.auth.api.identity.Identity
import com.google.android.gms.common.api.Scope
import com.google.androidgamesdk.GameActivity

/**
 * Hosts the shared C++ application (libstorage.so, see android/native). The GUI is
 * drawn by the same Vulkan code as on Windows; this class only provides system
 * services the native code calls through JNI.
 */
class MainActivity : GameActivity() {
    private val clipboard by lazy { getSystemService(ClipboardManager::class.java) }

    // Uploads read several files or a whole folder; downloads write into a folder.
    // Persisted grants let unfinished transfers resume after a restart.
    private val openFiles = registerForActivityResult(ActivityResultContracts.OpenMultipleDocuments()) { uris ->
        for (uri in uris) {
            runCatching { contentResolver.takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION) }
        }
        nativeDocumentsPicked(uris.map { it.toString() }.toTypedArray())
    }
    private val openFolder = registerForActivityResult(ActivityResultContracts.OpenDocumentTree()) { uri ->
        if (uri != null) {
            runCatching {
                contentResolver.takePersistableUriPermission(
                    uri, Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION)
            }
        }
        nativeDocumentsPicked(listOfNotNull(uri?.toString()).toTypedArray())
    }

    // Google sign-in through Google Play services; the Android OAuth client is
    // matched by this app's package name and signing key (storage/google/oauth/android.cpp).
    private var pendingAuthorization = 0L
    private val authorizationScreen = registerForActivityResult(ActivityResultContracts.StartIntentSenderForResult()) { result ->
        val request = pendingAuthorization
        pendingAuthorization = 0L
        try {
            val authorization = Identity.getAuthorizationClient(this).getAuthorizationResultFromIntent(result.data)
            nativeAuthorized(request, authorization.accessToken ?: "", "")
        } catch (error: Exception) {
            nativeAuthorized(request, "", error.message ?: "cancelled")
        }
    }

    // Called from any native thread; the result arrives through nativeAuthorized.
    @Suppress("unused")
    fun authorize(request: Long, email: String, scopes: String, interactive: Boolean) = runOnUiThread {
        val builder = AuthorizationRequest.builder()
            .setRequestedScopes(scopes.split(' ').filter { it.isNotEmpty() }.map { Scope(it) })
        if (email.isNotEmpty()) builder.setAccount(Account(email, "com.google"))
        // Adding a user must always let the user choose the account. Without a
        // prompt, Google Play services silently reuses an account that already
        // granted these scopes (e.g. a user removed from this app earlier).
        // Token renewals pass the account and stay non-interactive.
        else if (interactive) builder.setPrompt(AuthorizationRequest.Prompt.SELECT_ACCOUNT)
        Identity.getAuthorizationClient(this).authorize(builder.build())
            .addOnSuccessListener { result ->
                val screen = result.pendingIntent
                when {
                    !result.hasResolution() -> nativeAuthorized(request, result.accessToken ?: "", "")
                    !interactive || screen == null -> nativeAuthorized(request, "", "")
                    else -> {
                        pendingAuthorization = request
                        authorizationScreen.launch(IntentSenderRequest.Builder(screen.intentSender).build())
                    }
                }
            }
            .addOnFailureListener { error -> nativeAuthorized(request, "", error.message ?: "failed") }
    }

    // Called from the app thread whenever the transfer list becomes empty or not.
    @Suppress("unused")
    fun setTransfersActive(active: Boolean) = runOnUiThread {
        val service = Intent(this, TransferService::class.java)
        if (active) runCatching { startForegroundService(service) } else stopService(service)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Android 13+: the transfer notification needs permission; transfers run either way.
        if (android.os.Build.VERSION.SDK_INT >= 33)
            requestPermissions(arrayOf(android.Manifest.permission.POST_NOTIFICATIONS), 0)
        // Back closes menus and dialogs like Esc on the desktop.
        onBackPressedDispatcher.addCallback(this) { nativeBack() }
    }

    // Called from the app thread (android/native/bridge.cpp).
    @Suppress("unused")
    fun pickDocuments(folder: Boolean) = runOnUiThread {
        if (folder) openFolder.launch(null) else openFiles.launch(arrayOf("*/*"))
    }

    @Suppress("unused")
    fun clipboardText(): String? =
        clipboard.primaryClip?.takeIf { it.itemCount > 0 }?.getItemAt(0)?.coerceToText(this)?.toString()

    @Suppress("unused")
    fun setClipboardText(text: String) = runOnUiThread {
        clipboard.setPrimaryClip(ClipData.newPlainText("lingogoogol", text))
    }

    @Suppress("unused")
    fun openUrl(url: String): Boolean = try {
        startActivity(Intent(Intent.ACTION_VIEW, Uri.parse(url)))
        true
    } catch (_: ActivityNotFoundException) {
        false
    }

    @Suppress("unused")
    fun reportError(message: String) = runOnUiThread {
        AlertDialog.Builder(this)
            .setTitle("Storage")
            .setMessage(message)
            .setPositiveButton(android.R.string.ok, null)
            .setOnDismissListener { nativeErrorDismissed() }
            .show()
    }

    // Local files for transfers (storage/local/android.cpp), on transfer threads.
    private val localFiles by lazy { LocalFiles(this) }
    @Suppress("unused") fun localStat(path: String) = localFiles.stat(path)
    @Suppress("unused") fun localChildren(path: String) = localFiles.children(path)
    @Suppress("unused") fun localName(path: String) = localFiles.name(path)
    @Suppress("unused") fun localDisplay(path: String) = localFiles.display(path)
    @Suppress("unused") fun localOpen(path: String, mode: String) = localFiles.open(path, mode)
    @Suppress("unused") fun localMakeDirectories(path: String) = localFiles.makeDirectories(path)
    @Suppress("unused") fun localRemove(path: String) = localFiles.remove(path)

    // Registered by android_main (android/native/android_main.cpp).
    private external fun nativeBack()
    private external fun nativeDocumentsPicked(uris: Array<String>)
    private external fun nativeErrorDismissed()
    private external fun nativeAuthorized(request: Long, token: String, error: String)
}
