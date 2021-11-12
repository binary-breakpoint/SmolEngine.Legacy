#pragma once
#include "Layer/Layer.h"

#include <vector>

namespace SmolEngine 
{
	class LayerManager
	{
	public:
		LayerManager() = default;
		~LayerManager();

		void                           AddLayer(Layer* layer);
		void                           AddOverlay(Layer* layer);
		void                           PopLayer(Layer* layer, bool is_overlay = false);
		const std::vector<Layer*>&     GetLayers() const {return m_Layers; }

		std::vector<Layer*>::iterator  begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator  end() { return m_Layers.end(); }

	private:
		uint32_t                       m_Count = 0;
		std::vector<Layer*>            m_Layers;				 
	};
}

