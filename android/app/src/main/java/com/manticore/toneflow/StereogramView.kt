package com.manticore.toneflow

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.util.AttributeSet
import android.view.View
import kotlin.math.hypot
import kotlin.math.max
import kotlin.math.min
import kotlin.math.sin
import kotlin.random.Random

/**
 * Beat-reactive random-dot stereogram (SIRDS-style).
 * Depth map pulses and drifts with binaural / isochronic vibration phase.
 */
class StereogramView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null
) : View(context, attrs) {

    private var bitmap: Bitmap? = null
    private var pixels: IntArray = IntArray(0)
    private var depth: FloatArray = FloatArray(0)
    private var pattern: IntArray = IntArray(0)
    private var viewW = 0
    private var viewH = 0
    private var dpiScale = 2

    private var beatPhase = 0f
    private var beatHz = 10f
    private var rms = 0f
    private var carrierHz = 210f
    private var animTime = 0f

    private val palette = intArrayOf(
        Color.rgb(14, 21, 24),
        Color.rgb(24, 48, 52),
        Color.rgb(61, 184, 160),
        Color.rgb(126, 217, 198),
        Color.rgb(232, 241, 238),
        Color.rgb(40, 90, 110),
        Color.rgb(20, 70, 80)
    )

    fun updateAudio(phase: Float, beat: Float, level: Float, carrier: Float) {
        beatPhase = phase
        beatHz = beat
        rms = level
        carrierHz = carrier
        animTime += 0.033f
        if (viewW > 0 && viewH > 0) {
            renderFrame()
            invalidate()
        }
    }

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        if (w <= 0 || h <= 0) return
        dpiScale = 2
        viewW = max(80, w / dpiScale)
        viewH = max(60, h / dpiScale)
        bitmap = Bitmap.createBitmap(viewW, viewH, Bitmap.Config.ARGB_8888)
        pixels = IntArray(viewW * viewH)
        depth = FloatArray(viewW * viewH)
        val period = max(24, viewW / 8)
        pattern = IntArray(period) { palette[Random.nextInt(palette.size)] }
        renderFrame()
    }

    private fun renderFrame() {
        val w = viewW
        val h = viewH
        if (w <= 0 || h <= 0 || pixels.isEmpty()) return

        val cx = w * 0.5f
        val cy = h * 0.5f
        val pulse = 0.5f + 0.5f * sin(beatPhase)
        val breath = 0.5f + 0.5f * sin(animTime * 0.7f)
        val energy = min(1f, rms * 18f)
        val rings = 3f + beatHz * 0.08f
        val drift = animTime * (0.4f + beatHz * 0.03f)

        // Depth map: concentric consciousness rings + beat bulge.
        for (y in 0 until h) {
            for (x in 0 until w) {
                val nx = (x - cx) / cx
                val ny = (y - cy) / cy
                val r = hypot(nx.toDouble(), ny.toDouble()).toFloat()
                val angle = kotlin.math.atan2(ny, nx)
                val wave = sin(r * rings * Math.PI.toFloat() - beatPhase * 2f + drift)
                val spiral = 0.08f * sin(angle * 3f + beatPhase + drift * 0.5f)
                val dome = max(0f, 1f - r * (1.15f - 0.15f * pulse))
                var d = 0.35f * dome + 0.25f * (0.5f + 0.5f * wave) + spiral
                d += 0.12f * energy * pulse + 0.05f * breath
                // Carrier subtly scales fine grain of the field.
                d += 0.03f * sin(r * (carrierHz * 0.02f) + animTime)
                depth[y * w + x] = min(1f, max(0f, d))
            }
        }

        // SIRDS: each pixel samples pattern shifted by depth.
        val period = pattern.size
        val maxShift = period * 0.45f
        for (y in 0 until h) {
            val row = IntArray(w)
            for (x in 0 until w) {
                val d = depth[y * w + x]
                val shift = (d * maxShift).toInt()
                if (x < period) {
                    // Seed left strip with animated pattern.
                    val idx = (x + ((beatPhase * 8f).toInt()) + (y / 3)) % period
                    row[x] = pattern[(idx + period) % period]
                } else {
                    val src = x - period + shift
                    row[x] = if (src in 0 until x) row[src] else pattern[x % period]
                }
                // Soft tint by depth for non-stereo viewers.
                val base = row[x]
                val glow = (40 * d * (0.5f + 0.5f * pulse)).toInt()
                val r = min(255, Color.red(base) + glow / 2)
                val g = min(255, Color.green(base) + glow)
                val b = min(255, Color.blue(base) + glow / 3)
                pixels[y * w + x] = Color.rgb(r, g, b)
            }
        }

        // Occasional pattern mutation locked to beat peaks — living texture.
        if (pulse > 0.92f && Random.nextFloat() < 0.35f) {
            pattern[Random.nextInt(period)] = palette[Random.nextInt(palette.size)]
        }

        bitmap?.setPixels(pixels, 0, w, 0, 0, w, h)
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        val bmp = bitmap ?: return
        canvas.drawColor(Color.rgb(14, 21, 24))
        val scaleX = width.toFloat() / bmp.width
        val scaleY = height.toFloat() / bmp.height
        canvas.save()
        canvas.scale(scaleX, scaleY)
        canvas.drawBitmap(bmp, 0f, 0f, null)
        canvas.restore()
    }
}
