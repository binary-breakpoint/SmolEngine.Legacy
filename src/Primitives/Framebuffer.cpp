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
		if (out_fb)
		{
#ifdef FROSTIUM_OPENGL_IMPL
			out_fb->m_OpenglFramebuffer.Init(data);
#else
			out_fb->m_VulkanFrameBuffer.Init(data);
#endif
		}
	}

	void Framebuffer::Bind()
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglFramebuffer.Bind();
#else
#endif
	}

	void Framebuffer::BindColorAttachment(uint32_t slot)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglFramebuffer.BindColorAttachment(slot);
#else
#endif
	}

	void Framebuffer::UnBind()
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglFramebuffer.UnBind();
#else
#endif
	}

	void Framebuffer::OnResize(const uint32_t width, const uint32_t height)
	{
#ifdef FROSTIUM_OPENGL_IMPL
		m_OpenglFramebuffer.OnResize(width, height);
#else
		m_VulkanFrameBuffer.OnResize(width, height);
#endif
	}

	const FramebufferSpecification& Framebuffer::GetSpecification() const
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return m_OpenglFramebuffer.GetSpecification();
#else
		return m_VulkanFrameBuffer.GetSpecification();
#endif
	}

	uint32_t Framebuffer::GetColorAttachmentID() const
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return m_OpenglFramebuffer.GetColorAttachmentID();
#else
		return 0;
#endif
	}

	void* Framebuffer::GetImGuiTextureID(uint32_t index)
	{
#ifdef FROSTIUM_OPENGL_IMPL

		return reinterpret_cast<void*>(m_OpenglFramebuffer.GetColorAttachmentID());
#else
		return m_VulkanFrameBuffer.GetAttachment(index)->ImGuiID;
#endif
	}

	uint32_t Framebuffer::GetRendererID() const
	{
#ifdef FROSTIUM_OPENGL_IMPL
		return m_OpenglFramebuffer.GetRendererID();
#else
		return 0;
#endif
	}
}