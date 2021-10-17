#pragma once
#include "Materials/Material3D.h"


namespace SmolEngine
{
	struct RendererStorage;

	class MaterialPBR : public Material3D
	{					    			    
		static Ref<MaterialPBR>  Create();
		bool                     Initialize();


		friend struct RendererStorage;
	};
}