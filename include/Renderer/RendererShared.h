#pragma once
#include "Common/Memory.h"

namespace SmolEngine
{
	struct RendererStorageBase
	{
		virtual void  OnResize(uint32_t width, uint32_t height) {};
		void          Build();

		template<typename T>
		T* Cast() { return dynamic_cast<T*>(this); }

	private:          
		virtual void  Initilize() = 0;
		void          AddStorage();
	};

	struct GLSL_BOOLPAD
	{
		bool data[3];
	};
}