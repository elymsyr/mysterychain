package com.mysterychain.app

import android.Manifest
import android.annotation.SuppressLint
import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.companion.CompanionDeviceManager
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageManager
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.util.Log
import io.flutter.plugin.common.EventChannel
import java.util.TimeZone
import java.util.UUID
import kotlin.concurrent.thread

// The keychain link: the app's one Bluetooth connection to the board, held by a foreground service so it lives on with
// the app closed. The board is the one paired through CompanionDeviceManager (MainActivity "pair").
// Started by the app, by the notification listener (Bridge.kt) and by Android when the board comes in range
// (Presence.kt); sticky, so Android restarts it if it's killed.
// On every connect it sends #time and the weather (Weather.kt; again every hour while connected), and Bridge replays
// the ringing call and the playing song. The board's lines go to the app while it's open (MainActivity's event
// channel); the last stats and settings are kept for when it opens.
// Protocol: interfaces/watch/README.md#protocol.
@SuppressLint("MissingPermission") // start() checks BLUETOOTH_CONNECT; the service doesn't run without it
class KeychainService : Service() {
    companion object {
        const val NAME = "MysteryChain" // the board's advertised name, for pairing
        private const val TAG = "Keychain"
        private val uart = UUID.fromString("6E400001-B5A3-F393-E0A9-E50E24DCCA9E")
        private val rxUuid = UUID.fromString("6E400002-B5A3-F393-E0A9-E50E24DCCA9E") // board RX: phone writes
        private val txUuid = UUID.fromString("6E400003-B5A3-F393-E0A9-E50E24DCCA9E") // board TX: phone listens
        private val cccd = UUID.fromString("00002902-0000-1000-8000-00805F9B34FB") // turns TX notifications on
        private val main = Handler(Looper.getMainLooper())
        private const val WX_EVERY_MS = 3_600_000L // weather refresh while connected
        private const val WX_FRESH_MS = 1_800_000L // a forecast this young is sent again as it is

        @Volatile var live: KeychainService? = null
        var sink: EventChannel.EventSink? = null // the app, while it's open; main thread only

        fun permitted(c: Context) = Build.VERSION.SDK_INT < 31 ||
            c.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED

        @Suppress("DEPRECATION") // myAssociations is API 33+; the address is all that's needed
        fun board(c: Context): String? =
            c.getSystemService(CompanionDeviceManager::class.java).associations.firstOrNull()?.uppercase()

        // The app's on/off switch (settings): off, nothing starts the link until it's switched on again
        fun on(c: Context) = c.getSharedPreferences("link", Context.MODE_PRIVATE).getBoolean("service", true)

        fun switch(c: Context, on: Boolean) {
            c.getSharedPreferences("link", Context.MODE_PRIVATE).edit().putBoolean("service", on).apply()
            if (on) start(c) else c.stopService(Intent(c, KeychainService::class.java))
        }

        fun start(c: Context) {
            if (live != null || !on(c) || board(c) == null || !permitted(c)) return
            try {
                c.startForegroundService(Intent(c, KeychainService::class.java))
            } catch (e: IllegalStateException) { // a background start Android refused: the next trigger tries again
                Log.w(TAG, "not started: $e")
            }
        }

        // What the app shows about the link; sent to it again on every change
        fun link(c: Context): Map<String, Any> = mapOf(
            "service" to on(c),
            "bluetooth" to (c.getSystemService(BluetoothManager::class.java).adapter?.isEnabled == true),
            "connected" to (live?.ready == true),
            "synced" to (live?.synced ?: 0L),
            "location" to Weather.permitted(c),
            "weather" to (live?.wx ?: ""),
            "place" to (live?.wxPlace ?: ""), // where that forecast is for
            "picked" to Weather.picked(c), // the place chosen in the app, "" = none
        )
    }

    private var gatt: BluetoothGatt? = null
    private var rx: BluetoothGattCharacteristic? = null // non-null = connected and ready
    val ready get() = rx != null
    var synced = 0L // when the board last got the time, ms
    @Volatile var wx: String? = null // the last forecast sent (Weather.KINDS), and when it was fetched (ms)
    @Volatile var wxPlace: String? = null
    private var wxAt = 0L
    private val wxRefresh = Runnable { weather() }
    private val queue = ArrayDeque<String>() // one GATT write at a time
    private var busy = false
    val kept = mutableMapOf<String, String>() // last stats/settings line, for the app when it opens; main thread only
    private val bt get() = getSystemService(BluetoothManager::class.java).adapter
    private val notes by lazy { getSystemService(NotificationManager::class.java) }

    private val btState = object : BroadcastReceiver() { // Bluetooth off kills the GATT client: start over when it's back
        override fun onReceive(c: Context, i: Intent) {
            when (i.getIntExtra(BluetoothAdapter.EXTRA_STATE, 0)) {
                BluetoothAdapter.STATE_ON -> connect()
                BluetoothAdapter.STATE_OFF -> { close(); changed() }
            }
        }
    }

    override fun onCreate() {
        super.onCreate()
        // MIN: no status bar icon, folded at the bottom of the shade; the app shows the link's state.
        // A new id because a channel's importance can't change once made.
        notes.deleteNotificationChannel("link")
        notes.createNotificationChannel(NotificationChannel("link_quiet", "Keychain link", NotificationManager.IMPORTANCE_MIN))
        try {
            if (Build.VERSION.SDK_INT >= 29) startForeground(1, note(), ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE)
            else startForeground(1, note())
        } catch (e: RuntimeException) { // not allowed from the background right now: the next trigger tries again
            Log.w(TAG, "not in the foreground: $e")
            stopSelf()
            return
        }
        live = this
        registerReceiver(btState, IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED))
        observe()
        connect()
    }

    override fun onStartCommand(i: Intent?, flags: Int, id: Int) = START_STICKY // Android restarts it if it's killed

    override fun onDestroy() {
        main.removeCallbacks(wxRefresh)
        if (live === this) {
            live = null
            unregisterReceiver(btState)
            close()
            emit(link(this))
        }
        super.onDestroy()
    }

    override fun onBind(i: Intent?): IBinder? = null

    @Suppress("DEPRECATION") // the ObservingDevicePresenceRequest overload is API 36+
    private fun observe() { // Android binds Presence.kt while the board is in range
        val mac = board(this) ?: return
        if (Build.VERSION.SDK_INT >= 31) runCatching {
            getSystemService(CompanionDeviceManager::class.java).startObservingDevicePresence(mac)
        }
    }

    // Fixed text, never updated: a foreground service must show one, the link's state is in the app
    private fun note(): Notification = Notification.Builder(this, "link_quiet")
        .setSmallIcon(android.R.drawable.stat_sys_data_bluetooth)
        .setContentTitle("MysteryChain")
        .setContentIntent(PendingIntent.getActivity(this, 0, Intent(this, MainActivity::class.java), PendingIntent.FLAG_IMMUTABLE))
        .setOngoing(true)
        .build()

    private fun changed() = emit(link(this)) // the link changed: the app follows

    private fun emit(e: Map<String, Any>) = main.post { sink?.success(e) }

    @Synchronized
    private fun connect() {
        close()
        val mac = board(this) ?: return
        val bt = bt
        if (bt?.isEnabled == true) {
            // ponytail: the board (nRF52) uses a random static address, a board with a public one needs ADDRESS_TYPE_PUBLIC.
            // Android < 13 guesses the type (public unless it saw the board advertise lately).
            val d = if (Build.VERSION.SDK_INT >= 33) bt.getRemoteLeDevice(mac, BluetoothDevice.ADDRESS_TYPE_RANDOM)
                    else bt.getRemoteDevice(mac)
            gatt = d.connectGatt(this, false, cb, BluetoothDevice.TRANSPORT_LE) // direct first: quick when it's in range
        }
        changed()
    }

    @Synchronized
    private fun close() {
        gatt?.close()
        gatt = null
        rx = null
        queue.clear()
        busy = false
    }

    // One message to the board; false: not connected, dropped (a stale call or song is no use later)
    @Synchronized
    fun send(s: String): Boolean {
        Log.i(TAG, (if (rx == null) "dropped " else "send ") + s)
        if (rx == null) return false
        queue.addLast("$s\n") // \n: back-to-back writes never merge on the board
        if (!busy) next()
        return true
    }

    @Synchronized
    @Suppress("DEPRECATION") // the new writeCharacteristic overload is API 33+
    private fun next() {
        val s = queue.removeFirstOrNull() ?: return
        rx!!.value = s.toByteArray()
        busy = gatt!!.writeCharacteristic(rx)
        if (!busy) {
            Log.w(TAG, "write refused, queue dropped")
            queue.clear()
        }
    }

    // The weather for the next hours to the board (#weather), on connect, every hour while connected, and when the app
    // opens (it may have a new location), or a place is chosen (fresh). Fetched also while disconnected, for the app.
    // Offline or no location yet: the board keeps what it has (it expires there).
    fun weather(fresh: Boolean = false) {
        main.removeCallbacks(wxRefresh)
        if (ready) main.postDelayed(wxRefresh, WX_EVERY_MS)
        val last = wx
        if (!fresh && last != null && System.currentTimeMillis() - wxAt < WX_FRESH_MS) {
            if (ready) send("#weather $last")
            return
        }
        thread {
            val (place, lat, lon) = Weather.where(this) ?: run { Log.i(TAG, "weather: no location"); return@thread }
            val k = runCatching { Weather.fetch(lat, lon) }.onFailure { Log.w(TAG, "weather: $it") }.getOrNull() ?: return@thread
            wx = k
            wxPlace = place
            wxAt = System.currentTimeMillis()
            main.post {
                if (ready) send("#weather $k")
                changed()
            }
        }
    }

    private val cb = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, state: Int) {
            synchronized(this@KeychainService) {
                if (g != gatt) return
                Log.i(TAG, "state $state status $status")
                if (state == BluetoothProfile.STATE_CONNECTED) {
                    if (!g.requestMtu(247)) g.discoverServices() // long messages go in one write (board uses 247 too)
                } else {
                    rx = null
                    queue.clear()
                    busy = false
                    changed()
                    g.connect() // in the background: Android connects whenever the board is back in range
                }
            }
        }

        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
            g.discoverServices()
        }

        @Suppress("DEPRECATION") // the new writeDescriptor overload is API 33+
        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            synchronized(this@KeychainService) {
                if (g != gatt) return
                val tx = g.getService(uart)?.getCharacteristic(txUuid) ?: return
                g.setCharacteristicNotification(tx, true)
                val d = tx.getDescriptor(cccd) ?: return
                d.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                g.writeDescriptor(d) // ready once it's written: onDescriptorWrite
            }
        }

        override fun onDescriptorWrite(g: BluetoothGatt, d: BluetoothGattDescriptor, status: Int) {
            synchronized(this@KeychainService) {
                if (g != gatt) return
                rx = g.getService(uart)?.getCharacteristic(rxUuid) ?: return
                val now = System.currentTimeMillis()
                send("#time ${now / 1000} ${TimeZone.getDefault().getOffset(now) / 60000}")
                synced = now
                changed()
                main.post { weather() }
                main.post { Bridge.live?.replay() } // the ringing call and the playing song
            }
        }

        override fun onCharacteristicWrite(g: BluetoothGatt, c: BluetoothGattCharacteristic, status: Int) {
            synchronized(this@KeychainService) {
                busy = false
                next()
            }
        }

        // The deprecated overload: Android 13+ calls it from the new one, older versions only have this one
        @Suppress("DEPRECATION", "OVERRIDE_DEPRECATION")
        override fun onCharacteristicChanged(g: BluetoothGatt, c: BluetoothGattCharacteristic) {
            val lines = String(c.value).split('\n').map { it.trim() }.filter { it.isNotEmpty() } // one notify may hold several
            main.post {
                for (t in lines) {
                    if (t.startsWith("stats ") || t.startsWith("settings ")) kept[t.substringBefore(' ')] = t
                    sink?.success(mapOf("line" to t))
                }
            }
        }
    }
}
