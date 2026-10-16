package com.mysterychain.app

import android.Manifest
import android.bluetooth.BluetoothAdapter
import android.bluetooth.le.ScanFilter
import android.companion.AssociationRequest
import android.companion.BluetoothLeDeviceFilter
import android.companion.CompanionDeviceManager
import android.content.ComponentName
import android.content.Intent
import android.content.IntentSender
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import io.flutter.embedding.android.FlutterActivity
import io.flutter.embedding.engine.FlutterEngine
import io.flutter.plugin.common.EventChannel
import io.flutter.plugin.common.MethodChannel

// The app's side of the keychain link (KeychainService.kt), over two channels (lib/keychain.dart):
//   "mysterychain/link" methods: status, send <line>, pair, openAccess, openAppInfo [autostart], enableBluetooth
//   "mysterychain/events": link changes {bluetooth, connected, synced} and the board's lines {line}
class MainActivity : FlutterActivity() {
    private val pairCode = 1
    private var pairing: MethodChannel.Result? = null
    private val cdm by lazy { getSystemService(CompanionDeviceManager::class.java) }
    private val prefs by lazy { getSharedPreferences("link", MODE_PRIVATE) }

    override fun onCreate(b: Bundle?) {
        super.onCreate(b)
        val need = listOfNotNull(
            Manifest.permission.BLUETOOTH_CONNECT.takeIf { Build.VERSION.SDK_INT >= 31 },
            Manifest.permission.POST_NOTIFICATIONS.takeIf { Build.VERSION.SDK_INT >= 33 }, // the link's notification
        ).filter { checkSelfPermission(it) != PackageManager.PERMISSION_GRANTED }
        if (need.isEmpty()) KeychainService.start(this) else requestPermissions(need.toTypedArray(), 0)
    }

    override fun onRequestPermissionsResult(code: Int, permissions: Array<out String>, results: IntArray) {
        super.onRequestPermissionsResult(code, permissions, results)
        KeychainService.start(this)
    }

    override fun configureFlutterEngine(engine: FlutterEngine) {
        super.configureFlutterEngine(engine)
        val m = engine.dartExecutor.binaryMessenger
        MethodChannel(m, "mysterychain/link").setMethodCallHandler { call, result ->
            when (call.method) {
                "status" -> result.success(status())
                "send" -> result.success(KeychainService.live?.send(call.arguments as String) == true)
                "pair" -> pair(result)
                "openAccess" -> {
                    openAccess()
                    result.success(null)
                }
                "openAppInfo" -> { // permissions; on Xiaomi also Autostart and Battery saver
                    if (call.arguments == true) prefs.edit().putBoolean("autostart", true).apply()
                    startActivity(Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, Uri.fromParts("package", packageName, null)))
                    result.success(null)
                }
                "enableBluetooth" -> {
                    startActivity(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE))
                    result.success(null)
                }
                else -> result.notImplemented()
            }
        }
        EventChannel(m, "mysterychain/events").setStreamHandler(object : EventChannel.StreamHandler {
            override fun onListen(args: Any?, events: EventChannel.EventSink) {
                KeychainService.sink = events
                KeychainService.live?.kept?.values?.forEach { events.success(mapOf("line" to it)) }
            }

            override fun onCancel(args: Any?) {
                KeychainService.sink = null
            }
        })
    }

    override fun cleanUpFlutterEngine(engine: FlutterEngine) {
        KeychainService.sink = null
        super.cleanUpFlutterEngine(engine)
    }

    private fun status(): Map<String, Any> {
        val access = Settings.Secure.getString(contentResolver, "enabled_notification_listeners")?.contains(packageName) == true
        // MIUI can leave an allowed listener unbound after an update (requestRebind doesn't help);
        // switching the component off and on makes Android bind it again
        if (access && Bridge.live == null) {
            val c = ComponentName(this, Bridge::class.java)
            packageManager.setComponentEnabledSetting(c, PackageManager.COMPONENT_ENABLED_STATE_DISABLED, PackageManager.DONT_KILL_APP)
            packageManager.setComponentEnabledSetting(c, PackageManager.COMPONENT_ENABLED_STATE_ENABLED, PackageManager.DONT_KILL_APP)
        }
        return KeychainService.link(this) + mapOf(
            "permitted" to KeychainService.permitted(this),
            "paired" to (KeychainService.board(this) != null),
            "access" to access,
            // no API tells whether MIUI's Autostart is on: the hint stays until its settings were opened once
            "autostart" to (Build.MANUFACTURER.equals("Xiaomi", true) && !prefs.getBoolean("autostart", false)),
        )
    }

    // Pairing: the system's companion device picker finds the board by name. Once paired, Android lets the app run the
    // link from the background and wakes it when the board is in range.
    @Suppress("DEPRECATION") // the Executor overload is API 33+
    private fun pair(result: MethodChannel.Result) {
        pairing = result
        val filter = BluetoothLeDeviceFilter.Builder()
            .setScanFilter(ScanFilter.Builder().setDeviceName(KeychainService.NAME).build())
            .build()
        val request = AssociationRequest.Builder().addDeviceFilter(filter).setSingleDevice(true).build()
        cdm.associate(request, object : CompanionDeviceManager.Callback() {
            @Suppress("OVERRIDE_DEPRECATION") // onAssociationPending (API 33+) calls it by default
            override fun onDeviceFound(chooser: IntentSender) = startIntentSenderForResult(chooser, pairCode, null, 0, 0, 0)

            override fun onFailure(error: CharSequence?) = paired(false)
        }, null)
    }

    override fun onActivityResult(code: Int, result: Int, data: Intent?) {
        super.onActivityResult(code, result, data)
        if (code == pairCode) paired(result == RESULT_OK)
    }

    private fun paired(ok: Boolean) {
        pairing?.success(ok) // once: a cancel can come as both a failure and a result
        pairing = null
        if (ok) KeychainService.start(this)
    }

    // A paired companion app gets the system's prompt for its own listener; otherwise the list of all of them
    private fun openAccess() {
        if (KeychainService.board(this) != null) {
            runCatching { cdm.requestNotificationAccess(ComponentName(this, Bridge::class.java)) }.onSuccess { return }
        }
        startActivity(Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS))
    }
}
