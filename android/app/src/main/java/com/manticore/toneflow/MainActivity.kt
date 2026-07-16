package com.manticore.toneflow

import android.Manifest
import android.content.pm.PackageManager
import android.media.AudioManager
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.widget.LinearLayout
import android.widget.SeekBar
import android.widget.TextView
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.manticore.toneflow.databinding.ActivityMainBinding
import kotlin.math.log10
import kotlin.math.max
import kotlin.math.roundToInt

class MainActivity : AppCompatActivity() {
    private lateinit var binding: ActivityMainBinding
    private lateinit var engine: NativeToneEngine
    private var presetItems: List<PresetItem> = emptyList()
    private var selectedPreset = 0
    private var playing = false
    private var syncingUi = false
    private val uiHandler = Handler(Looper.getMainLooper())

    private val micPermissionLauncher =
        registerForActivityResult(ActivityResultContracts.RequestPermission()) { granted ->
            if (granted) {
                enableMicFeedback(true)
            } else {
                syncingUi = true
                binding.micSwitch.isChecked = false
                syncingUi = false
                Toast.makeText(this, R.string.mic_permission_denied, Toast.LENGTH_LONG).show()
            }
        }

    private val meterTick = object : Runnable {
        override fun run() {
            if (::engine.isInitialized) {
                val carrier = engine.carrierHz()
                val beat = engine.beatHz()
                val rms = engine.rms()
                val phase = engine.beatPhase()
                binding.stereogram.updateAudio(phase, beat, rms, carrier)

                if (playing) {
                    val dbfs = if (rms < 1e-9f) -100f else (20f * log10(max(rms, 1e-9f)))
                    binding.statusText.text =
                        getString(R.string.status_playing, carrier, beat, dbfs)
                    binding.statusDetail.text = getString(
                        R.string.status_detail,
                        engine.filterLabel(),
                        engine.perceptionLabel()
                    )
                }

                if (engine.micEnabled()) {
                    val micRms = engine.micRms()
                    val micDb = if (micRms < 1e-9f) -100f else (20f * log10(max(micRms, 1e-9f)))
                    binding.micStatus.text =
                        getString(R.string.mic_status_on, micDb, engine.micAgc())
                } else if (!binding.micSwitch.isChecked) {
                    binding.micStatus.text = getString(R.string.mic_status_off)
                }
            }
            // ~15 Hz UI — must stay off the audio mutex (telemetry is lock-free).
            uiHandler.postDelayed(this, 66L)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        volumeControlStream = AudioManager.STREAM_MUSIC
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        engine = NativeToneEngine()
        setupPresets()
        setupControls()
        refreshCycleButtons()
        uiHandler.post(meterTick)
    }

    private fun setupPresets() {
        val count = engine.presetCount()
        presetItems = (0 until count).map { index ->
            PresetItem(
                index = index,
                title = engine.presetLabel(index),
                description = engine.presetDescription(index)
            )
        }

        val freeIdx = presetItems.indexOfFirst { it.title.contains("Free Play") }
        selectedPreset = if (freeIdx >= 0) freeIdx else 0

        renderPresetList()

        if (presetItems.isNotEmpty()) {
            applyPreset(presetItems[selectedPreset], startIfNeeded = false)
        }
    }

    private fun renderPresetList() {
        val container = binding.presetContainer
        container.removeAllViews()
        val inflater = LayoutInflater.from(this)

        presetItems.forEach { item ->
            val row = inflater.inflate(R.layout.item_preset, container, false) as LinearLayout
            val title = row.findViewById<TextView>(R.id.presetTitle)
            val description = row.findViewById<TextView>(R.id.presetDescription)
            title.text = item.title
            description.text = item.description
            updatePresetRowBackground(row, item.index == selectedPreset)
            row.setOnClickListener {
                selectedPreset = item.index
                renderPresetList()
                applyPreset(item, startIfNeeded = true)
            }
            container.addView(row)
        }
    }

    private fun updatePresetRowBackground(row: View, selected: Boolean) {
        row.setBackgroundResource(
            if (selected) R.drawable.preset_item_selected else R.drawable.preset_item_bg
        )
    }

    private fun applyPreset(item: PresetItem, startIfNeeded: Boolean) {
        engine.setPreset(item.index)
        binding.selectedPreset.text = item.title
        binding.selectedDescription.text = item.description
        syncSlidersFromEngine()
        refreshCycleButtons()
        if (startIfNeeded && !playing) {
            startPlayback()
        }
    }

    private fun setupControls() {
        binding.playButton.setOnClickListener {
            if (playing) pausePlayback() else startPlayback()
        }

        binding.modeButton.setOnClickListener {
            engine.cycleMode()
            refreshCycleButtons()
        }
        binding.perceptionButton.setOnClickListener {
            engine.cyclePerception()
            refreshCycleButtons()
        }
        binding.filterButton.setOnClickListener {
            engine.cycleFilter()
            refreshCycleButtons()
        }
        binding.reverbButton.setOnClickListener {
            engine.cycleReverb()
            refreshCycleButtons()
        }

        binding.micSwitch.setOnCheckedChangeListener { _, checked ->
            if (syncingUi) return@setOnCheckedChangeListener
            if (checked) requestMicAndEnable() else enableMicFeedback(false)
        }

        binding.carrierBar.max = 900
        binding.beatBar.max = 400
        binding.harmonicsBar.max = 2
        binding.noiseBar.max = 100
        binding.volumeBar.max = 100
        binding.micGainBar.max = 150
        binding.toneMixBar.max = 100

        bindSlider(binding.carrierBar) { progress ->
            val hz = 100f + progress.toFloat()
            engine.setCarrier(hz)
            binding.carrierLabel.text = getString(R.string.carrier, hz)
        }
        bindSlider(binding.beatBar) { progress ->
            val hz = progress / 10f
            engine.setBeat(hz)
            binding.beatLabel.text = getString(R.string.beat, hz)
        }
        bindSlider(binding.harmonicsBar) { progress ->
            val layers = progress + 1
            engine.setHarmonics(layers)
            binding.harmonicsLabel.text = getString(R.string.harmonics, layers)
        }
        bindSlider(binding.noiseBar) { progress ->
            engine.setSubLevel(progress / 100f)
            binding.noiseLabel.text = getString(R.string.noise_bed, progress.toFloat())
        }
        bindSlider(binding.volumeBar) { progress ->
            engine.setVolume(progress / 100f)
            binding.volumeLabel.text = getString(R.string.volume, progress)
        }
        bindSlider(binding.micGainBar) { progress ->
            engine.setMicGain(progress / 100f)
            binding.micGainLabel.text = getString(R.string.mic_gain, progress)
        }
        bindSlider(binding.toneMixBar) { progress ->
            engine.setToneMix(progress / 100f)
            binding.toneMixLabel.text =
                getString(R.string.tone_mix, progress, 100 - progress)
        }

        binding.volumeBar.progress = 18
        engine.setVolume(0.18f)
        binding.volumeLabel.text = getString(R.string.volume, 18)
        binding.micGainBar.progress = 0
        engine.setMicGain(0.0f)
        binding.micGainLabel.text = getString(R.string.mic_gain, 0)
        binding.toneMixBar.progress = 100
        engine.setToneMix(1.0f)
        binding.toneMixLabel.text = getString(R.string.tone_mix, 100, 0)
        binding.harmonicsBar.progress = 0
        binding.harmonicsLabel.text = getString(R.string.harmonics, 1)
        binding.noiseBar.progress = 12
        engine.setSubLevel(0.12f)
        binding.noiseLabel.text = getString(R.string.noise_bed, 12f)

        syncSlidersFromEngine()
    }

    private fun hasMicPermission(): Boolean {
        return ContextCompat.checkSelfPermission(
            this,
            Manifest.permission.RECORD_AUDIO
        ) == PackageManager.PERMISSION_GRANTED
    }

    private fun requestMicAndEnable() {
        if (hasMicPermission()) {
            enableMicFeedback(true)
            return
        }
        Toast.makeText(this, R.string.mic_permission_needed, Toast.LENGTH_SHORT).show()
        micPermissionLauncher.launch(Manifest.permission.RECORD_AUDIO)
    }

    private fun enableMicFeedback(enabled: Boolean) {
        if (enabled && !playing) {
            startPlayback()
            if (!playing) {
                syncingUi = true
                binding.micSwitch.isChecked = false
                syncingUi = false
                return
            }
        }
        engine.setMicFeedback(enabled)
        syncingUi = true
        binding.micSwitch.isChecked = enabled
        syncingUi = false
        if (!enabled) {
            binding.micStatus.text = getString(R.string.mic_status_off)
        }
    }

    private fun bindSlider(bar: SeekBar, onChange: (Int) -> Unit) {
        bar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener {
            override fun onProgressChanged(seekBar: SeekBar?, progress: Int, fromUser: Boolean) {
                if (!fromUser || syncingUi) return
                onChange(progress)
            }

            override fun onStartTrackingTouch(seekBar: SeekBar?) = Unit
            override fun onStopTrackingTouch(seekBar: SeekBar?) = Unit
        })
    }

    private fun syncSlidersFromEngine() {
        syncingUi = true
        val carrier = engine.carrierHz().coerceIn(100f, 1000f)
        val beat = engine.beatHz().coerceIn(0f, 40f)
        val sub = (engine.subLevel() * 100f).roundToInt().coerceIn(0, 100)
        binding.carrierBar.progress = (carrier - 100f).roundToInt()
        binding.beatBar.progress = (beat * 10f).roundToInt()
        binding.noiseBar.progress = sub
        binding.carrierLabel.text = getString(R.string.carrier, carrier)
        binding.beatLabel.text = getString(R.string.beat, beat)
        binding.noiseLabel.text = getString(R.string.noise_bed, sub.toFloat())
        syncingUi = false
    }

    private fun refreshCycleButtons() {
        binding.modeButton.text = engine.modeLabel()
        binding.perceptionButton.text = engine.perceptionLabel()
        binding.filterButton.text = engine.filterLabel()
        binding.reverbButton.text = engine.reverbLabel()
        if (playing) {
            binding.statusDetail.text = getString(
                R.string.status_detail,
                engine.filterLabel(),
                engine.perceptionLabel()
            )
        }
    }

    private fun startPlayback() {
        if (!engine.start()) {
            Toast.makeText(this, R.string.audio_start_failed, Toast.LENGTH_LONG).show()
            playing = false
            return
        }
        engine.setPlaying(true)
        if (binding.micSwitch.isChecked && hasMicPermission()) {
            engine.setMicFeedback(true)
        }
        playing = true
        binding.playButton.text = getString(R.string.pause)
        binding.statusText.text = getString(R.string.status_starting)
    }

    private fun pausePlayback() {
        engine.setMicFeedback(false)
        engine.setPlaying(false)
        playing = false
        binding.playButton.text = getString(R.string.play)
        binding.statusText.text = getString(R.string.status_paused)
        binding.statusDetail.text = ""
        if (binding.micSwitch.isChecked) {
            binding.micStatus.text = getString(R.string.mic_status_off)
        }
    }

    override fun onPause() {
        super.onPause()
        if (playing) {
            pausePlayback()
        }
    }

    override fun onDestroy() {
        uiHandler.removeCallbacks(meterTick)
        if (::engine.isInitialized) {
            engine.release()
        }
        super.onDestroy()
    }
}
