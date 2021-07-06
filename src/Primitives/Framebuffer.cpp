#include "stdafx.h"
#include "Common/SLog.h"
#include "Primitives/Framebuffer.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	void Framebuffer::Create(const FramebufferSpecification& data, Framebuffer* out_fb)
	{
		out_fb->m_Info = data;
		out_fb->Init(&out_fb->m_Info);
	}

	void Framebuffer::Bind()
	{

	}

	void Framebuffer::UnBind()
	{

	}

	void Framebuffer::OnResize(const uint32_t width, const uint32_t height)
	{
#ifdef FROSTIUM_OPENGL_IMPL

#else
		SetSize(width, height);
#endif
	}

	const FramebufferSpecification& Framebuffer::GetSpecification() const
	{
		return m_Info;
	}

	void* Framebuffer::GetImGuiTextureID(uint32_t index)
	{
#ifdef FROSTIUM_OPENGL_IMPL

		return reinterpret_cast<void*>(GetColorAttachmentID());
#else
		return GetAttachment(index)->ImGuiID;
#endif
	}
}