#include "stdafx.h"
#include "Common/DebugLog.h"
#include "Primitives/Framebuffer.h"

#ifdef OPENGL_IMPL
#else
#include "Backends/Vulkan/VulkanFramebuffer.h"
#endif

namespace SmolEngine
{
	Ref<Framebuffer> Framebuffer::Create()
	{
		Ref<Framebuffer> fb = nullptr;
#ifdef OPENGL_IMPL
#else
		fb = std::make_shared<VulkanFramebuffer>();
#endif
		return fb;
	}

	bool Framebuffer::BuildBase(FramebufferSpecification* info)
	{
		m_Info = *info;
		return true;
	}

	const FramebufferSpecification& Framebuffer::GetSpecification() const
	{
		return m_Info;
	}
}