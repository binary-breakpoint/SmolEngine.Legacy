#include "stdafx.h"
#include "Renderer/RendererShared.h"

namespace SmolEngine
{
	void RendererStorageBase::Build()
	{
		AddStorage();
		Initilize();
	}

	void RendererStorageBase::AddStorage()
	{
		auto context = GraphicsContext::GetSingleton();
		bool found = std::find(context->m_StorageList.begin(), context->m_StorageList.end(), this) != context->m_StorageList.end();
		if (!found)
			context->m_StorageList.push_back(this);
	}
}