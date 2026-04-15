package com.yepgoryo.CaptureCap

import android.content.res.Configuration
import android.os.Bundle
import android.widget.LinearLayout
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import androidx.appcompat.app.AppCompatDelegate
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.updatePadding
import java.io.InputStream

class LicenseScreen : AppCompatActivity() {
    private var appSettings: GlobalProperties? = null

    public override fun onCreate(bundle: Bundle?) {
        val globalProperties = GlobalProperties(baseContext)
        this.appSettings = globalProperties
        val darkTheme: GlobalProperties.DarkThemeProperty = globalProperties.getDarkTheme(true)
        if (((getResources().configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK) == Configuration.UI_MODE_NIGHT_YES && darkTheme == GlobalProperties.DarkThemeProperty.AUTOMATIC) || darkTheme == GlobalProperties.DarkThemeProperty.DARK) {
            setTheme(R.style.Theme_CaptureCap_Dark)
        } else {
            setTheme(R.style.Theme_CaptureCap_Light)
        }
        super.onCreate(bundle)
        setContentView(R.layout.license)
        val licenseText: TextView = findViewById(R.id.licensetext)

        val licenseInputStream: InputStream = applicationContext.assets.open("license.txt")
        val licenseBytes = ByteArray(licenseInputStream.available())
        licenseInputStream.read(licenseBytes)
        licenseText.setText(String(licenseBytes))
        licenseInputStream.close()

        var statusBarHeight = 0
        val resourceId = getResources().getIdentifier("status_bar_height", "dimen", "android")
        if (resourceId > 0) {
            statusBarHeight = getResources().getDimensionPixelSize(resourceId)
        }

        val statusbarlayout = findViewById<LinearLayout?>(R.id.statusbar)

        val statusbarlayoutparams: LinearLayout.LayoutParams = statusbarlayout!!.getLayoutParams() as LinearLayout.LayoutParams
        statusbarlayoutparams.height = statusBarHeight
        statusbarlayout.setLayoutParams(statusbarlayoutparams)

        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.mainscroll)) { v, insets ->
            val bars = insets.getInsets(
                WindowInsetsCompat.Type.systemBars()
            )
            v.updatePadding(
                left = bars.left,
                top = bars.top-statusBarHeight,
                right = bars.right,
                bottom = bars.bottom,
            )

            WindowInsetsCompat.CONSUMED
        }
    }
}
