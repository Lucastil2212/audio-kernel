package com.manticore.toneflow

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.manticore.toneflow.databinding.ItemPresetBinding

data class PresetItem(
    val index: Int,
    val title: String,
    val description: String
)

class PresetAdapter(
    private val items: List<PresetItem>,
    private val onSelect: (PresetItem) -> Unit
) : RecyclerView.Adapter<PresetAdapter.Holder>() {

    private var selectedIndex = 0

    fun setSelectedIndex(index: Int) {
        val old = selectedIndex
        selectedIndex = index
        if (old in items.indices) notifyItemChanged(old)
        if (selectedIndex in items.indices) notifyItemChanged(selectedIndex)
    }

    inner class Holder(val binding: ItemPresetBinding) : RecyclerView.ViewHolder(binding.root)

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Holder {
        val binding = ItemPresetBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        return Holder(binding)
    }

    override fun onBindViewHolder(holder: Holder, position: Int) {
        val item = items[position]
        val selected = item.index == selectedIndex
        holder.binding.presetTitle.text = item.title
        holder.binding.presetDescription.text = item.description
        holder.binding.presetRoot.setBackgroundResource(
            if (selected) R.drawable.preset_item_selected else R.drawable.preset_item_bg
        )
        holder.binding.root.setOnClickListener {
            setSelectedIndex(item.index)
            onSelect(item)
        }
    }

    override fun getItemCount(): Int = items.size
}
