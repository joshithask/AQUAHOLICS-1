package com.aquaholics.tankmonitor

import android.annotation.SuppressLint
import android.Manifest
import android.content.pm.PackageManager
import android.content.Intent
import android.os.Build
import android.os.Bundle
import android.webkit.JavascriptInterface
import androidx.core.content.ContextCompat
import androidx.work.*
import java.util.concurrent.TimeUnit
import android.webkit.WebSettings
import android.webkit.WebView
import android.webkit.WebViewClient
import androidx.appcompat.app.AppCompatActivity

// Shows the bundled Aquaholics dashboard (app/src/main/assets/index.html).
// In the app: Setup tab -> type http://<esp32-ip>/reading -> Save. The address is remembered.
class MainActivity : AppCompatActivity() {
    private lateinit var web: WebView

    // The page calls this when you save the cloud channel, so background alerts know what to check.
    inner class Bridge {
        @JavascriptInterface
        fun saveCloud(chan: String, key: String) {
            getSharedPreferences("aqua", MODE_PRIVATE).edit()
                .putString("chan", chan.trim()).putString("key", key.trim()).apply()
        }
    }

    @SuppressLint("SetJavaScriptEnabled")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        web = WebView(this)
        web.settings.apply {
            javaScriptEnabled = true
            domStorageEnabled = true              // remembers address and calibration
            allowFileAccess = true
            @Suppress("DEPRECATION")
            allowUniversalAccessFromFileURLs = true   // bundled page may call the ESP32 over http
            mixedContentMode = WebSettings.MIXED_CONTENT_ALWAYS_ALLOW
        }
        web.addJavascriptInterface(Bridge(), "Android")
        web.webViewClient = WebViewClient()
        setContentView(web)
        if (Build.VERSION.SDK_INT >= 33 &&
            ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED)
            requestPermissions(arrayOf(Manifest.permission.POST_NOTIFICATIONS), 1)
        // Check the cloud about every 15 minutes (Android's minimum), even when the app is closed.
        val req = PeriodicWorkRequestBuilder<AlertWorker>(15, TimeUnit.MINUTES)
            .setConstraints(Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build()).build()
        WorkManager.getInstance(this).enqueueUniquePeriodicWork("tank-alerts", ExistingPeriodicWorkPolicy.KEEP, req)
        // Every-2-minutes check while the phone is on (the 15-minute worker above is the backup).
        ContextCompat.startForegroundService(this, Intent(this, MonitorService::class.java))
        if (savedInstanceState == null) web.loadUrl("file:///android_asset/index.html")
        else web.restoreState(savedInstanceState)
    }

    override fun onResume() { super.onResume(); web.onResume() }
    override fun onPause() { web.onPause(); super.onPause() }
    override fun onSaveInstanceState(outState: Bundle) { super.onSaveInstanceState(outState); web.saveState(outState) }
}
