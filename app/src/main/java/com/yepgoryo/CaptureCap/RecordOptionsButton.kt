package com.yepgoryo.CaptureCap

import android.content.Context
import android.graphics.drawable.StateListDrawable
import android.util.AttributeSet
import android.view.Gravity
import android.view.MotionEvent
import android.view.View
import android.widget.RelativeLayout
import androidx.core.content.ContextCompat

class RecordOptionsButton @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : RelativeLayout(context, attrs, defStyleAttr), View.OnClickListener {

    private var activated = false

    init {
        gravity = Gravity.CENTER_VERTICAL
        isClickable = true
        isFocusable = true
        isLongClickable = true

        setupBackground()
        setOnClickListener(this)
    }

    fun setupBackgroundOpened() {
        val states = arrayOf(
            intArrayOf(android.R.attr.state_pressed),
            intArrayOf()
        )

        val drawables = arrayOf(
            ContextCompat.getDrawable(context, R.drawable.bg_rounded_button_opened_pressed)!!,
            ContextCompat.getDrawable(context, R.drawable.bg_rounded_button_opened)!!
        )

        background = StateListDrawable().apply {
            states.forEachIndexed { i, state ->
                addState(state, drawables[i])
            }
        }
    }

    fun setupBackground() {
        val states = arrayOf(
            intArrayOf(android.R.attr.state_pressed),
            intArrayOf()
        )

        val drawables = arrayOf(
            ContextCompat.getDrawable(context, R.drawable.bg_rounded_button_pressed)!!,
            ContextCompat.getDrawable(context, R.drawable.bg_rounded_button)!!
        )

        background = StateListDrawable().apply {
            states.forEachIndexed { i, state ->
                addState(state, drawables[i])
            }
        }
    }

    override fun onClick(v: View) {
        activated = !activated
    }

    override fun dispatchTouchEvent(event: MotionEvent): Boolean {
        return super.dispatchTouchEvent(event)
    }
}