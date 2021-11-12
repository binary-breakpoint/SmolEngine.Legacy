#include "stdafx.h"
#include "Layer/Layer.h"

namespace SmolEngine 
{
	Layer::Layer(const std::string& debugName)
		: m_Name(debugName), m_Enabled(true)
	{

	}

	Layer::~Layer()
	{

	}
}