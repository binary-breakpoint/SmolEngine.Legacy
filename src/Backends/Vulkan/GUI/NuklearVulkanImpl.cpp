#include "stdafx.h"
#ifndef FROSTIUM_OPENGL_IMPL
#include "Backends/Vulkan/GUI/NuklearVulkanImpl.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_VULKAN_IMPLEMENTATION
#include "nuklear/nuklear.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifndef NK_GLFW_DOUBLE_CLICK_LO
#define NK_GLFW_DOUBLE_CLICK_LO 0.02
#endif
#ifndef NK_GLFW_DOUBLE_CLICK_HI
#define NK_GLFW_DOUBLE_CLICK_HI 0.2
#endif

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_INDEX_BUFFER 128 * 1024

	struct NuklearStorage
	{
		nk_buffer cmds{};
		nk_context ctx{};
		Texture atlas_tex{};
		nk_draw_null_texture null{};
		nk_font_atlas atlas{};
		struct nk_vec2 fb_scale;
		struct nk_vec2 scroll;
		struct nk_vec2 double_click_pos;
		int text_len;
		double last_button_click;
		int is_double_click_down;
		unsigned int text[NK_GLFW_TEXT_MAX];
		GLFWwindow* win = nullptr;
		nk_font* font = nullptr;
	};

	struct nk_glfw_vertex
	{
		float position[2];
		float uv[2];
		nk_byte col[4];
	};

	enum nk_glfw_init_state {
		NK_GLFW3_DEFAULT = 0,
		NK_GLFW3_INSTALL_CALLBACKS
	};


	/// Nuklear API
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	NK_INTERN void nk_glfw3_clipbard_paste(nk_handle usr, struct nk_text_edit* edit)
	{
		auto& info = NuklearVulkanImpl::s_Instance->m_Info;
		const char* text = glfwGetClipboardString(info->win);
		if (text) nk_textedit_paste(edit, text, nk_strlen(text));
		(void)usr;
	}

	NK_INTERN void nk_glfw3_clipbard_copy(nk_handle usr, const char* text, int len)
	{
		char* str = 0;
		(void)usr;
		if (!len) return;
		str = (char*)malloc((size_t)len + 1);
		str = (char*)malloc((size_t)len + 1);
		if (!str) return;
		memcpy(str, text, (size_t)len);
		str[len] = '\0';
		auto& info = NuklearVulkanImpl::s_Instance->m_Info;
		glfwSetClipboardString(info->win, str);
		free(str);
	}

	NK_INTERN void nk_glfw3_device_upload_atlas(const void* image, int width, int height, Texture* _tex)
	{
		auto& texture = *_tex;
		texture = {};

		auto vulkanTex = texture.GetVulkanTexture();
		vulkanTex->SetFormat(VK_FORMAT_R8G8B8A8_UNORM);
		vulkanTex->CreateTexture(static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1, image, nullptr);

		NuklearVulkanImpl::s_Instance->m_Pipeline.UpdateSampler(_tex, 1);
	}

	NK_API void nk_glfw3_mouse_button_callback(GLFWwindow* win, int button, int action, int mods)
	{
		double x, y;
		auto& info = NuklearVulkanImpl::s_Instance->m_Info;

		if (button != GLFW_MOUSE_BUTTON_LEFT) return;
		glfwGetCursorPos(win, &x, &y);
		if (action == GLFW_PRESS) {
			double dt = glfwGetTime() - info->last_button_click;
			if (dt > NK_GLFW_DOUBLE_CLICK_LO && dt < NK_GLFW_DOUBLE_CLICK_HI) {
				info->is_double_click_down = nk_true;
				info->double_click_pos = nk_vec2((float)x, (float)y);
			}
			info->last_button_click = glfwGetTime();
		}
		else info->is_double_click_down = nk_false;
	}

	NK_API void nk_gflw3_scroll_callback(double xoff, double yoff)
	{
		auto& info = NuklearVulkanImpl::s_Instance->m_Info;

		info->scroll.x += (float)xoff;
		info->scroll.y += (float)yoff;
	}

	NK_API void nk_glfw3_char_callback(unsigned int codepoint)
	{
		auto info = NuklearVulkanImpl::s_Instance->m_Info;
		if (info->text_len < NK_GLFW_TEXT_MAX)
			info->text[info->text_len++] = codepoint;
	}

	NK_API void nk_glfw3_new_frame()
	{
		auto& info = NuklearVulkanImpl::s_Instance->m_Info;
		auto& fb_spec = GraphicsContext::GetSingleton()->GetFramebuffer()->GetSpecification();

		int i;
		double x, y;
		struct nk_context* ctx = &info->ctx;
		GLFWwindow* win = info->win;

		info->fb_scale.x = static_cast<float>(fb_spec.Width);
		info->fb_scale.y = static_cast<float>(fb_spec.Height);

		nk_input_begin(ctx);
		for (i = 0; i < info->text_len; ++i)
			nk_input_unicode(ctx, info->text[i]);

#if NK_GLFW_GL3_MOUSE_GRABBING
		/* optional grabbing behavior */
		if (ctx->input.mouse.grab)
			glfwSetInputMode(glfw.win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		else if (ctx->input.mouse.ungrab)
			glfwSetInputMode(glfw.win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
#endif

		nk_input_key(ctx, NK_KEY_DEL, glfwGetKey(win, GLFW_KEY_DELETE) == GLFW_PRESS);
		nk_input_key(ctx, NK_KEY_ENTER, glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS);
		nk_input_key(ctx, NK_KEY_TAB, glfwGetKey(win, GLFW_KEY_TAB) == GLFW_PRESS);
		nk_input_key(ctx, NK_KEY_BACKSPACE, glfwGetKey(win, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
		nk_input_key(ctx, NK_KEY_UP, glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS);
		nk_input_key(ctx, NK_KEY_DOWN, glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS);
		nk_input_key(ctx, NK_KEY_TEXT_START, glfwGetKey(win, GLFW_KEY_HOME) == GLFW_PRESS);
		nk_input_key(ctx, NK_KEY_TEXT_END, glfwGetKey(win, GLFW_KEY_END) == GLFW_PRESS);
		nk_input_key(ctx, NK_KEY_SCROLL_START, glfwGetKey(win, GLFW_KEY_HOME) == GLFW_PRESS);
		nk_input_key(ctx, NK_KEY_SCROLL_END, glfwGetKey(win, GLFW_KEY_END) == GLFW_PRESS);
		nk_input_key(ctx, NK_KEY_SCROLL_DOWN, glfwGetKey(win, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS);
		nk_input_key(ctx, NK_KEY_SCROLL_UP, glfwGetKey(win, GLFW_KEY_PAGE_UP) == GLFW_PRESS);
		nk_input_key(ctx, NK_KEY_SHIFT, glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
			glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

		if (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
			glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
			nk_input_key(ctx, NK_KEY_COPY, glfwGetKey(win, GLFW_KEY_C) == GLFW_PRESS);
			nk_input_key(ctx, NK_KEY_PASTE, glfwGetKey(win, GLFW_KEY_V) == GLFW_PRESS);
			nk_input_key(ctx, NK_KEY_CUT, glfwGetKey(win, GLFW_KEY_X) == GLFW_PRESS);
			nk_input_key(ctx, NK_KEY_TEXT_UNDO, glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS);
			nk_input_key(ctx, NK_KEY_TEXT_REDO, glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS);
			nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
			nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
			nk_input_key(ctx, NK_KEY_TEXT_LINE_START, glfwGetKey(win, GLFW_KEY_B) == GLFW_PRESS);
			nk_input_key(ctx, NK_KEY_TEXT_LINE_END, glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS);
		}
		else {
			nk_input_key(ctx, NK_KEY_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
			nk_input_key(ctx, NK_KEY_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
			nk_input_key(ctx, NK_KEY_COPY, 0);
			nk_input_key(ctx, NK_KEY_PASTE, 0);
			nk_input_key(ctx, NK_KEY_CUT, 0);
			nk_input_key(ctx, NK_KEY_SHIFT, 0);
		}

		glfwGetCursorPos(win, &x, &y);
		nk_input_motion(ctx, (int)x, (int)y);
#if NK_GLFW_GL3_MOUSE_GRABBING
		if (ctx->input.mouse.grabbed) {
			glfwSetCursorPos(glfw.win, ctx->input.mouse.prev.x, ctx->input.mouse.prev.y);
			ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
			ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
		}
#endif
		nk_input_button(ctx, NK_BUTTON_LEFT, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
		nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
		nk_input_button(ctx, NK_BUTTON_RIGHT, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
		nk_input_button(ctx, NK_BUTTON_DOUBLE, (int)info->double_click_pos.x, (int)info->double_click_pos.y, info->is_double_click_down);
		nk_input_scroll(ctx, info->scroll);
		nk_input_end(&info->ctx);
		info->text_len = 0;
		info->scroll = nk_vec2(0, 0);
	}

	NK_API void nk_glfw3_font_stash_end()
	{
		const void* image; int w, h;
		auto& info = NuklearVulkanImpl::s_Instance->m_Info;
		image = nk_font_atlas_bake(&info->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
		nk_glfw3_device_upload_atlas(image, w, h, &info->atlas_tex);
		nk_font_atlas_end(&info->atlas, nk_handle_ptr(&info->atlas_tex), &info->null);
		if (info->atlas.default_font) {
			nk_style_set_font(&info->ctx, &info->atlas.default_font->handle);
		}
	}

	NK_API void nk_glfw3_font_stash_begin(nk_font_atlas** atlas)
	{
		auto& info = NuklearVulkanImpl::s_Instance->m_Info;
		nk_font_atlas_init_default(&info->atlas);
		nk_font_atlas_begin(&info->atlas);
		*atlas = &info->atlas;
	}

	NK_API void nk_glfw3_init(GLFWwindow* win, nk_glfw_init_state state)
	{
		auto& info = NuklearVulkanImpl::s_Instance->m_Info;

		info->win = win;
#if 0
		if (state == NK_GLFW3_INSTALL_CALLBACKS) {
			glfwSetCharCallback(win, nk_glfw3_char_callback);
		}
#endif

		nk_init_default(&info->ctx, 0);
		info->ctx.clip.copy = nk_glfw3_clipbard_copy;
		info->ctx.clip.paste = nk_glfw3_clipbard_paste;
		info->ctx.clip.userdata = nk_handle_ptr(0);
		info->last_button_click = 0;

		info->is_double_click_down = nk_false;
		info->double_click_pos = nk_vec2(0, 0);

		nk_buffer_init_default(&info->cmds);
	}

	NK_API void nk_glfw3_render(nk_anti_aliasing AA, VkRenderPass pass, VkFramebuffer fb, uint32_t w, uint32_t h)
	{
		auto& info = NuklearVulkanImpl::s_Instance->m_Info;
		GLFWwindow* win = info->win;
		nk_buffer vbuf, ebuf;

		float ortho[4][4] = {
			 {2.0f, 0.0f, 0.0f, 0.0f},
			 {0.0f,-2.0f, 0.0f, 0.0f},
			 {0.0f, 0.0f,-1.0f, 0.0f},
			 {-1.0f,1.0f, 0.0f, 1.0f},
		};

		ortho[0][0] /= static_cast<float>(w);
		ortho[1][1] /= static_cast<float>(h);

		NuklearVulkanImpl* inst = NuklearVulkanImpl::s_Instance;
		GraphicsPipeline* pipeline = &inst->m_Pipeline;
		VulkanIndexBuffer* indexBuffer = &inst->m_IndexBuffer->GetVulkanIndexBuffer();
		VulkanVertexBuffer* vertexBuffer = &inst->m_VertexBuffer->GetVulkanVertexBuffer();

		pipeline->SubmitBuffer(0, sizeof(ortho), &ortho);
		pipeline->BeginCommandBuffer(true);
		{
			/* convert from command queue into draw list and draw to screen */
			const struct nk_draw_command* cmd;

			void* vertices = vertexBuffer->MapMemory();
			void* elements = indexBuffer->MapMemory();

			assert(vertices != nullptr);
			assert(elements != nullptr);

			{
				/* fill convert configuration */
				struct nk_convert_config config;
				static const struct nk_draw_vertex_layout_element vertex_layout[] = {
					{NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, position)},
					{NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, uv)},
					{NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_glfw_vertex, col)},
					{NK_VERTEX_LAYOUT_END}
				};
				memset(&config, 0, sizeof(config));
				config.vertex_layout = vertex_layout;
				config.vertex_size = sizeof(struct nk_glfw_vertex);
				config.vertex_alignment = NK_ALIGNOF(struct nk_glfw_vertex);
				config.null = info->null;
				config.circle_segment_count = 22;
				config.curve_segment_count = 22;
				config.arc_segment_count = 22;
				config.global_alpha = 1.0f;
				config.shape_AA = AA;
				config.line_AA = AA;

				/* setup buffers to load vertices and elements */
				nk_buffer_init_fixed(&vbuf, vertices, MAX_VERTEX_BUFFER);
				nk_buffer_init_fixed(&ebuf, elements, MAX_INDEX_BUFFER);

				nk_convert(&info->ctx, &info->cmds, &vbuf, &ebuf, &config);
			}

			vertexBuffer->UnMapMemory();
			indexBuffer->UnMapMemory();

			VkCommandBuffer cmdBuffer = pipeline->GetVkCommandBuffer();

			// Set clear values for all framebuffer attachments with loadOp set to clear
			// We use two attachments (color and depth) that are cleared at the start of the subpass and as such we need to set clear values for both
			VkClearValue clearValues[2];
			clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
			clearValues[1].depthStencil = { 1.0f, 0 };

			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderPass = pass;
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = w;
			renderPassBeginInfo.renderArea.extent.height = h;
			renderPassBeginInfo.clearValueCount = 2;
			renderPassBeginInfo.pClearValues = clearValues;
			// Set target frame buffer
			renderPassBeginInfo.framebuffer = fb;

			// Start the first sub pass specified in our default render pass setup by the base class
			// This will clear the color and depth attachment
			vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.x = 0;
			viewport.y = (float)h;
			viewport.height = -(float)h;
			viewport.width = (float)w;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);

			{

				pipeline->BindPipeline();
				pipeline->BindDescriptors();
				/* iterate over and execute each draw command */
				VkDeviceSize doffset = 0;
				vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer->GetBuffer(), &doffset);
				vkCmdBindIndexBuffer(cmdBuffer, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT16);

				uint32_t index_offset = 0;
				nk_draw_foreach(cmd, &info->ctx, &info->cmds)
				{
					if (!cmd->elem_count) 
						continue;

					// Apply scissor/clipping rectangle
					VkRect2D scissor;
					const struct nk_command_scissor* s = (const struct nk_command_scissor*)cmd;

					scissor.offset.x = 0;
					scissor.offset.y = 0;
					scissor.extent.width = static_cast<uint32_t>(cmd->clip_rect.w * info->fb_scale.x);
					scissor.extent.height = static_cast<uint32_t>(cmd->clip_rect.h * info->fb_scale.y);

					vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
					vkCmdDrawIndexed(cmdBuffer, cmd->elem_count, 1, index_offset, 0, 0);

					index_offset += cmd->elem_count;
				}
				nk_clear(&info->ctx);
			}

			vkCmdEndRenderPass(cmdBuffer);
		}
	}


	void NuklearVulkanImpl::Init()
	{
		s_Instance = this;
		m_Info = new NuklearStorage();

		SetupBuffers();
		SetupPipeline();

		nk_font_atlas* atlas;
		auto win = GraphicsContext::GetSingleton()->GetWindow()->GetNativeWindow();

		nk_glfw3_init(win, NK_GLFW3_INSTALL_CALLBACKS);

		nk_glfw3_font_stash_begin(&atlas);
		{
			m_Info->font = nk_font_atlas_add_from_file(atlas, std::string(GraphicsContext::GetSingleton()->GetResourcesPath() + "Fonts/Font1.ttf").c_str(), 24, nullptr);
		}
		nk_glfw3_font_stash_end();
		nk_style_set_font(&m_Info->ctx, &m_Info->font->handle);
	}

	void NuklearVulkanImpl::ShutDown()
	{

	}

	void NuklearVulkanImpl::NewFrame()
	{
		nk_glfw3_new_frame();
	}

	void NuklearVulkanImpl::Draw(Framebuffer* target)
	{
		auto& spec = target->GetSpecification();
		auto& vkFB = target->GetVulkanFramebuffer();
		nk_glfw3_render(NK_ANTI_ALIASING_ON, vkFB.GetRenderPass(), vkFB.GetCurrentVkFramebuffer(), spec.Width, spec.Height);
	}

	void NuklearVulkanImpl::OnEvent(Event& e)
	{
		if (e.IsType(EventType::MOUSE_SCROLL))
		{
			MouseScrollEvent* scroll = e.Cast<MouseScrollEvent>();
			nk_gflw3_scroll_callback(scroll->GetXoffset(), scroll->GetYoffset());
		}

		if (e.IsType(EventType::MOUSE_BUTTON))
		{
			MouseButtonEvent* mouseButton = e.Cast<MouseButtonEvent>();
			nk_glfw3_mouse_button_callback(m_Info->win, mouseButton->m_MouseButton, mouseButton->m_Action, 0);
		}

		if (e.IsType(EventType::CHAR_INPUT))
		{
			CharInputEvent* input = e.Cast<CharInputEvent>();
			nk_glfw3_char_callback(input->m_Codepoint);
		}
	}

	nk_context* NuklearVulkanImpl::GetContext()
	{
		return &NuklearVulkanImpl::s_Instance->m_Info->ctx;
	}

	nk_font* NuklearVulkanImpl::GetFont()
	{
		return m_Info->font;
	}

	nk_font_atlas* NuklearVulkanImpl::GetFontAtlas()
	{
		return &m_Info->atlas;
	}

	void NuklearVulkanImpl::UpdateAtlas()
	{
		auto& info = NuklearVulkanImpl::s_Instance->m_Info;

		const void* image; int w, h;
		image = nk_font_atlas_bake(&info->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
		nk_glfw3_device_upload_atlas(image, w, h, &info->atlas_tex);
		nk_font_atlas_end(&info->atlas, nk_handle_ptr(&info->atlas_tex), &info->null);
	}

	void NuklearVulkanImpl::ClearAtlas()
	{
		auto& info = NuklearVulkanImpl::s_Instance->m_Info;
		nk_font_atlas_clear(&info->atlas);
	}

	void NuklearVulkanImpl::SetupPipeline()
	{
		BufferLayout bLayout =
		{
			{ DataTypes::Float2, "pos" },
			{ DataTypes::Float2, "uv" },
			{ DataTypes::Byte, "color" },
		};

		GraphicsPipelineCreateInfo pipelineCI{};
		GraphicsPipelineShaderCreateInfo shaderCI{};
		{
			const std::string& path = GraphicsContext::GetSingleton()->GetResourcesPath();

			shaderCI.FilePaths[ShaderType::Vertex] = path + "Shaders/Nuklear.vert";
			shaderCI.FilePaths[ShaderType::Fragment] = path + "Shaders/Nuklear.frag";

			shaderCI.BufferInfos[0].bGlobal = false;
		};


		pipelineCI.PipelineName = "Nuklear";
		pipelineCI.eCullMode = CullMode::None;
		pipelineCI.VertexInputInfos = { VertexInputInfo(sizeof(nk_glfw_vertex), bLayout) };
		pipelineCI.eAlphaBlendOp = BlendOp::ADD;
		pipelineCI.eDstAlphaBlendFactor = BlendFactor::ZERO;
		pipelineCI.eSrcAlphaBlendFactor = BlendFactor::ONE;
		pipelineCI.eColorBlendOp = BlendOp::ADD;
		pipelineCI.eDstColorBlendFactor = BlendFactor::ONE_MINUS_SRC_ALPHA;
		pipelineCI.eSrcColorBlendFactor = BlendFactor::SRC_ALPHA;
		pipelineCI.ShaderCreateInfo = shaderCI;
		pipelineCI.TargetFramebuffers = { GraphicsContext::GetSingleton()->GetFramebuffer() };

		assert(m_Pipeline.Create(&pipelineCI) == PipelineCreateResult::SUCCESS);

		m_Pipeline.SetVertexBuffers({m_VertexBuffer});
		m_Pipeline.SetIndexBuffers({ m_IndexBuffer });
	}

	void NuklearVulkanImpl::SetupBuffers()
	{
		m_VertexBuffer = std::make_shared<VertexBuffer>();
		m_IndexBuffer = std::make_shared<IndexBuffer>();

		IndexBuffer::Create(m_IndexBuffer.get(), MAX_INDEX_BUFFER);
		VertexBuffer::Create(m_VertexBuffer.get(), MAX_VERTEX_BUFFER);
	}

}

#endif