package com.mysterychain.app

import android.companion.CompanionDeviceService
import androidx.annotation.RequiresApi

// Android binds this while the paired board is in range (presence observing, started by KeychainService): brings the
// keychain link back after the app was killed.
@RequiresApi(31)
@Suppress("OVERRIDE_DEPRECATION") // the AssociationInfo overloads are API 33+ and call these by default
class Presence : CompanionDeviceService() {
    override fun onDeviceAppeared(address: String) = KeychainService.start(this)

    override fun onDeviceDisappeared(address: String) {} // abstract on Android 12; the link waits for the board itself
}
