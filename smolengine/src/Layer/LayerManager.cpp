#include "stdafx.h"
#include "Layer/LayerManager.h"

namespace SmolEngine 
{
	LayerManager::~LayerManager()
	{
		for (Layer* layer : m_Layers) 
		{
			layer->OnDetach();
			delete layer;
		}
	}

	void LayerManager::AddLayer(Layer* layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_Count , layer);
		m_Count++;

		layer->OnAttach();
	}

	void LayerManager::AddOverlay(Layer* overlay)
	{
		m_Layers.emplace_back(overlay);
		overlay->OnAttach();
	}

	void LayerManager::PopLayer(Layer* layer, bool is_overlay)
	{
		auto result = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (result != m_Layers.end()) 
		{
			if(!is_overlay)
				m_Count--;

			m_Layers.erase(result);
			return;
		}
	}
}