package com.aquaholics.tankmonitor

import android.Manifest
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.ContextCompat
import androidx.work.Worker
import androidx.work.WorkerParameters
import org.json.JSONObject
import java.net.HttpURLConnection
import java.net.URL
import java.text.SimpleDateFormat
import java.util.Locale
import java.util.TimeZone

// Shared check used by the 2-minute service and the 15-minute backup worker.
// State lives in SharedPreferences, so the two never send the same alert twice.
object Checker {
    fun run(ctx: Context): Boolean {
        val sp = ctx.getSharedPreferences("aqua", Context.MODE_PRIVATE)
        val chan = sp.getString("chan", "") ?: ""
        val key = sp.getString("key", "") ?: ""
        if (chan.isEmpty() || key.isEmpty()) return true
        return try {
            val c = URL("https://api.thingspeak.com/channels/$chan/feeds/last.json?api_key=$key")
                .openConnection() as HttpURLConnection
            c.connectTimeout = 10000; c.readTimeout = 10000
            val j = JSONObject(c.inputStream.bufferedReader().readText())
            val fmt = SimpleDateFormat("yyyy-MM-dd'T'HH:mm:ss'Z'", Locale.US).apply { timeZone = TimeZone.getTimeZone("UTC") }
            val age = System.currentTimeMillis() - (fmt.parse(j.getString("created_at"))?.time ?: 0L)
            if (age > 1800000) {   // no reading for 30 minutes
                if (!sp.getBoolean("offline", false)) {
                    sp.edit().putBoolean("offline", true).apply()
                    show(ctx, 2, "Tank monitor offline", "No reading for over 30 minutes. Check the ESP32 power and Wi-Fi.")
                }
                return true
            }
            sp.edit().putBoolean("offline", false).apply()
            val level = j.optString("field5", "0").toDoubleOrNull()?.toInt() ?: 0
            if (level != sp.getInt("lastLevel", 0)) {
                sp.edit().putInt("lastLevel", level).apply()
                val title = arrayOf("Tank back to Normal", "Caution: water quality changing", "Inspection needed")[level.coerceIn(0, 2)]
                show(ctx, 1, title, "Turbidity ${j.optString("field1", "?")} NTU. Check the tank if it is not Normal. Sensors cannot confirm water is safe to drink.")
            }
            true
        } catch (e: Exception) { false }
    }

    private fun show(ctx: Context, id: Int, title: String, text: String) {
        if (Build.VERSION.SDK_INT >= 33 &&
            ContextCompat.checkSelfPermission(ctx, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) return
        if (Build.VERSION.SDK_INT >= 26)
            (ctx.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager)
                .createNotificationChannel(NotificationChannel("alerts", "Tank alerts", NotificationManager.IMPORTANCE_HIGH))
        val n = NotificationCompat.Builder(ctx, "alerts").setSmallIcon(android.R.drawable.ic_dialog_alert)
            .setContentTitle(title).setContentText(text).setStyle(NotificationCompat.BigTextStyle().bigText(text))
            .setAutoCancel(true).build()
        NotificationManagerCompat.from(ctx).notify(id, n)
    }
}

class AlertWorker(private val ctx: Context, p: WorkerParameters) : Worker(ctx, p) {
    override fun doWork(): Result = if (Checker.run(ctx)) Result.success() else Result.retry()
}
