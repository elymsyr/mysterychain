package com.mysterychain.app

import android.Manifest
import android.annotation.SuppressLint
import android.content.Context
import android.content.pm.PackageManager
import android.location.Geocoder
import android.location.Location
import android.location.LocationManager
import android.os.Build
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder
import java.util.Locale

// Weather (A5): the next 4 hours from open-meteo (free, no key), boiled down to the one kind the board shows
// (#weather <kind>). The worst hour wins: one rainy hour of the four means rain.
// Where: the phone's coarse location. Android gives a background service no location without "Allow all the time", so
// the app saves the last fix whenever it is open (prefs "link": lat, lon) and the service asks for that place. With the
// location off, a place chosen in the app by name stands in (pick); none chosen = the last fix.
object Weather {
    val KINDS = listOf("clear", "cloudy", "wind", "rain", "snow", "storm") // the board's order (pet.h Wx), mildest first
    const val WIND_KMH = 30.0 // ponytail: a dry hour this windy (or gusty) counts as wind; tune once it's been lived with
    const val GUST_KMH = 50.0

    // One hour: its WMO weather code (open-meteo docs), then wind if it's dry and windy
    fun hour(code: Int, wind: Double, gust: Double): Int {
        val k = when (code) {
            95, 96, 99 -> 5 // thunderstorm (with hail)
            71, 73, 75, 77, 85, 86 -> 4 // snow, snow grains, snow showers
            in 51..67, in 80..82 -> 3 // drizzle, rain, freezing rain, showers
            3, 45, 48 -> 1 // overcast, fog
            else -> 0 // clear to partly cloudy
        }
        return if (k < 2 && (wind >= WIND_KMH || gust >= GUST_KMH)) 2 else k
    }

    fun worst(codes: List<Int>, wind: List<Double>, gust: List<Double>): String =
        KINDS[codes.indices.maxOf { hour(codes[it], wind.getOrElse(it) { 0.0 }, gust.getOrElse(it) { 0.0 }) }]

    // Network: off the main thread. Throws when offline or the answer is off.
    fun fetch(lat: Double, lon: Double): String {
        val url = URL(String.format(Locale.US, "https://api.open-meteo.com/v1/forecast?latitude=%.2f&longitude=%.2f" +
            "&hourly=weather_code,wind_speed_10m,wind_gusts_10m&forecast_hours=4&timezone=auto", lat, lon)) // ~1 km is plenty
        val c = url.openConnection() as HttpURLConnection
        c.connectTimeout = 10_000
        c.readTimeout = 10_000
        try {
            val h = JSONObject(c.inputStream.bufferedReader().readText()).getJSONObject("hourly")
            fun list(k: String) = h.getJSONArray(k).let { a -> List(a.length()) { a.optDouble(it, 0.0) } }
            return worst(list("weather_code").map { it.toInt() }, list("wind_speed_10m"), list("wind_gusts_10m"))
        } finally {
            c.disconnect()
        }
    }

    fun permitted(c: Context) = c.checkSelfPermission(Manifest.permission.ACCESS_COARSE_LOCATION) == PackageManager.PERMISSION_GRANTED

    // A place by name (open-meteo's geocoding, free): (name, lat, lon), null when nothing matches. Off the main thread.
    fun search(name: String): Triple<String, Double, Double>? {
        val url = URL("https://geocoding-api.open-meteo.com/v1/search?count=1&language=en&name=" + URLEncoder.encode(name, "UTF-8"))
        val c = url.openConnection() as HttpURLConnection
        c.connectTimeout = 10_000
        c.readTimeout = 10_000
        try {
            val r = JSONObject(c.inputStream.bufferedReader().readText()).optJSONArray("results")?.optJSONObject(0) ?: return null
            val label = listOf(r.optString("name"), r.optString("country_code")).filter { it.isNotEmpty() }.joinToString(", ")
            return Triple(label, r.getDouble("latitude"), r.getDouble("longitude"))
        } finally {
            c.disconnect()
        }
    }

    // The place the user chose in the app (prefs "link": place, plat, plon), "" = none: used while the phone's location is off
    fun pick(c: Context, p: Triple<String, Double, Double>?) = c.getSharedPreferences("link", Context.MODE_PRIVATE).edit().apply {
        if (p == null) remove("place") else putString("place", p.first).putFloat("plat", p.second.toFloat()).putFloat("plon", p.third.toFloat())
    }.apply()

    fun picked(c: Context): String = c.getSharedPreferences("link", Context.MODE_PRIVATE).getString("place", "")!!

    // The place to ask for, with its name: where the phone is now (location on), else the chosen place, else the last fix
    // saved (the app, in the foreground, also saves it). Off the main thread: naming a fix asks Android's Geocoder.
    @SuppressLint("MissingPermission")
    fun where(c: Context): Triple<String, Double, Double>? {
        val p = c.getSharedPreferences("link", Context.MODE_PRIVATE)
        val now = if (permitted(c)) runCatching { // a background service may get nothing, or a SecurityException
            val lm = c.getSystemService(LocationManager::class.java)
            lm.getProviders(true).mapNotNull { lm.getLastKnownLocation(it) }.maxByOrNull { it.time }
        }.getOrNull() else null
        if (now != null) save(c, now)
        else if (p.contains("place")) return Triple(p.getString("place", "")!!, p.getFloat("plat", 0f).toDouble(), p.getFloat("plon", 0f).toDouble())
        if (!p.contains("lat")) return null
        val lat = p.getFloat("lat", 0f).toDouble()
        val lon = p.getFloat("lon", 0f).toDouble()
        return Triple(name(c, lat, lon), lat, lon)
    }

    // A fix's town, remembered per ~1 km so the Geocoder isn't asked every hour
    @Suppress("DEPRECATION") // the listener overload is API 33+
    private fun name(c: Context, lat: Double, lon: Double): String {
        val p = c.getSharedPreferences("link", Context.MODE_PRIVATE)
        val key = String.format(Locale.US, "%.2f,%.2f", lat, lon)
        if (p.getString("fixKey", null) == key) return p.getString("fixName", "")!!
        val a = runCatching { Geocoder(c, Locale.ENGLISH).getFromLocation(lat, lon, 1)?.firstOrNull() }.getOrNull() ?: return "Here"
        val n = listOfNotNull(a.locality ?: a.subAdminArea ?: a.adminArea, a.countryCode).joinToString(", ")
        p.edit().putString("fixKey", key).putString("fixName", n).apply()
        return n
    }

    private fun save(c: Context, l: Location) =
        c.getSharedPreferences("link", Context.MODE_PRIVATE).edit().putFloat("lat", l.latitude.toFloat()).putFloat("lon", l.longitude.toFloat()).apply()

    // The app came to the front: save where the phone is, asking Android for a fix if it has none yet, then refresh
    @SuppressLint("MissingPermission")
    fun locate(c: Context) {
        val lm = c.getSystemService(LocationManager::class.java)
        val fixed = permitted(c) && runCatching { lm.getProviders(true).any { lm.getLastKnownLocation(it) != null } }.getOrDefault(false)
        if (permitted(c) && lm.isLocationEnabled && !fixed && Build.VERSION.SDK_INT >= 30) runCatching {
            lm.getCurrentLocation(LocationManager.NETWORK_PROVIDER, null, c.mainExecutor) {
                if (it != null) save(c, it)
                KeychainService.live?.weather()
            }
        } else KeychainService.live?.weather()
    }
}
