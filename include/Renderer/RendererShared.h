#pragma once
#include "Common/Memory.h"

namespace SmolEngine
{
	struct RendererStorageBase
	{
		virtual void  OnResize(uint32_t width, uint32_t height) {};
		void          Build();

	private:          
		virtual void  Initilize() = 0;
		void          AddStorage();
	};

	struct GLSL_BOOLPAD
	{
		bool data[3];
	};
}