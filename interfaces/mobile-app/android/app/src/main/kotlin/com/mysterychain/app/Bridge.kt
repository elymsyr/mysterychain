package com.mysterychain.app

import android.app.Notification
import android.content.ComponentName
import android.media.MediaMetadata
import android.media.session.MediaController
import android.media.session.MediaSessionManager
import android.media.session.PlaybackState
import android.service.notification.NotificationListenerService
import android.service.notification.StatusBarNotification

// Calls and the playing song for the keychain (O3). A notification listener: Android keeps it bound, app open or not,
// once the user allows Notification access. It hands the keychain link (KeychainService.kt):
//   #call <caller> while a call rings, #call 0 once it is answered or gone (read from the dialer's call notification)
//   #music <title - artist> when the playing track changes
//     (read from the player's media session: always there, unlike its notification's text)
// and replays the ringing call and the playing song whenever the link connects.
class Bridge : NotificationListenerService() {
    companion object {
        @Volatile var live: Bridge? = null // bound by Android; MainActivity rebinds it when it isn't
    }

    private var callKey: String? = null // the ringing call's notification
    private var caller = ""
    private var track = ""
    private var player: MediaController? = null // the session whose track goes to the board
    private val sessions by lazy { getSystemService(MediaSessionManager::class.java) }
    private val me by lazy { ComponentName(this, Bridge::class.java) } // a listener may read media sessions
    private val playerCb = object : MediaController.Callback() {
        override fun onMetadataChanged(m: MediaMetadata?) = music(m)
    }
    private val sessionsCb = MediaSessionManager.OnActiveSessionsChangedListener { follow(it) }

    override fun onListenerConnected() {
        live = this
        sessions.addOnActiveSessionsChangedListener(sessionsCb, me)
        follow(sessions.getActiveSessions(me))
        KeychainService.start(this) // after a reboot or a kill, this is often what brings the link back
    }

    override fun onListenerDisconnected() {
        live = null
        sessions.removeOnActiveSessionsChangedListener(sessionsCb)
        follow(null)
    }

    override fun onNotificationPosted(sbn: StatusBarNotification) {
        val n = sbn.notification
        val title = n.extras.getCharSequence(Notification.EXTRA_TITLE)?.toString()?.trim()
        if (title.isNullOrEmpty()) return
        if (n.category == Notification.CATEGORY_CALL) {
            // ponytail: ringing = has a full-screen intent, dialers drop it once the call is answered;
            // a dialer that keeps it: also read "android.callType" (Notification.CallStyle, 1 = incoming)
            val ringing = n.fullScreenIntent != null
            if (ringing && title != caller) {
                callKey = sbn.key
                caller = title
                send("#call $title")
            } else if (!ringing && sbn.key == callKey) endCall()
        }
    }

    override fun onNotificationRemoved(sbn: StatusBarNotification) {
        if (sbn.key == callKey) endCall()
    }

    private fun endCall() {
        callKey = null
        caller = ""
        send("#call 0")
    }

    // ponytail: first active session = the one playing most recently; ignores a second player running at once
    private fun follow(list: List<MediaController>?) {
        val c = list?.firstOrNull()
        if (c?.sessionToken == player?.sessionToken) return
        player?.unregisterCallback(playerCb)
        player = c?.also {
            it.registerCallback(playerCb)
            music(it.metadata)
        }
    }

    private fun music(m: MediaMetadata?) {
        val title = m?.getString(MediaMetadata.METADATA_KEY_TITLE)?.trim()
        if (title.isNullOrEmpty()) return
        val artist = m.getString(MediaMetadata.METADATA_KEY_ARTIST)?.trim()
        val t = if (artist.isNullOrEmpty()) title else "$title - $artist"
        if (t != track) { // metadata repeats on play/pause and seeks
            track = t
            send("#music $t")
        }
    }

    fun replay() { // the link just connected
        if (callKey != null) send("#call $caller")
        if (track.isNotEmpty() && player?.playbackState?.state == PlaybackState.STATE_PLAYING) send("#music $track")
    }

    // Not connected: dropped, replay() resends what's still current. Link not running: started.
    private fun send(s: String) {
        if (KeychainService.live?.send(s.take(48)) == null) KeychainService.start(this) // 48: <= 200 bytes as UTF-8 (board buffer)
    }
}
