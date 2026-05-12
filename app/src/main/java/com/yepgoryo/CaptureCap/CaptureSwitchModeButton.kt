package com.yepgoryo.CaptureCap

import android.content.Context
import android.graphics.drawable.Drawable
import android.util.AttributeSet
import android.util.TypedValue
import androidx.appcompat.content.res.AppCompatResources
import androidx.appcompat.widget.AppCompatButton

class CaptureSwitchModeButton(
    context: Context,
    attrs: AttributeSet? = null
) : AppCompatButton(context, attrs) {

    private var defaultBg: Drawable? = null
    private var defaultTextColor: Int = 0

    init {
        defaultBg = background
        defaultTextColor = textColors.defaultColor

        applyState(isSelected)
    }

    override fun setSelected(selected: Boolean) {
        if (isSelected != selected) {
            super.setSelected(selected)
            applyState(selected)
        }
    }

    private fun applyState(isSelected: Boolean) {
        if (isSelected) {
            val typedValue = TypedValue()
            val theme = context.getTheme()
            theme.resolveAttribute(androidx.appcompat.R.attr.colorPrimary, typedValue, true)
            val color = typedValue.data
            setTextColor(color)

            if (id == R.id.button_mode_record) {
                setCompoundDrawablesWithIntrinsicBounds(AppCompatResources.getDrawable(context, R.drawable.icon_record_mode_small_inverse), null, null, null)
            } else {
                setCompoundDrawablesWithIntrinsicBounds(AppCompatResources.getDrawable(context, R.drawable.icon_record_stream_small_dark), null, null, null)
            }

            background = AppCompatResources.getDrawable(context, R.drawable.bg_setting_rounded_button_checked)
        } else {
            val typedValue = TypedValue()
            val theme = context.getTheme()
            theme.resolveAttribute(com.google.android.material.R.attr.colorOnPrimary, typedValue, true)
            val color = typedValue.data
            setTextColor(color)

            if (id == R.id.button_mode_record) {
                setCompoundDrawablesWithIntrinsicBounds(AppCompatResources.getDrawable(context, R.drawable.icon_record_mode_small), null, null, null)
            } else {
                setCompoundDrawablesWithIntrinsicBounds(AppCompatResources.getDrawable(context, R.drawable.icon_record_stream_small), null, null, null)
            }

            background = AppCompatResources.getDrawable(context, R.drawable.bg_setting_rounded_button_disabled)
        }
    }
}