package com.aquaholics.tankmonitor

import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.Service
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import androidx.core.app.ServiceCompat

// Keeps checking the cloud every 2 minutes. Android requires the small ongoing notification.
class MonitorService : Service() {
    @Volatile private var running = false

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (Build.VERSION.SDK_INT >= 26)
            (getSystemService(NOTIFICATION_SERVICE) as NotificationManager)
                .createNotificationChannel(NotificationChannel("monitor", "Monitoring status", NotificationManager.IMPORTANCE_LOW))
        val n = NotificationCompat.Builder(this, "monitor").setSmallIcon(android.R.drawable.ic_dialog_info)
            .setContentTitle("Tank monitor running").setContentText("Checking your water readings every 2 minutes")
            .setOngoing(true).build()
        ServiceCompat.startForeground(this, 100, n, ServiceInfo.FOREGROUND_SERVICE_TYPE_MANIFEST)
        if (!running) {
            running = true
            Thread {
                while (running) {
                    try { Checker.run(applicationContext); Thread.sleep(120000) } catch (e: Exception) { }
                }
            }.start()
        }
        return START_STICKY
    }

    override fun onDestroy() { running = false; super.onDestroy() }
}
