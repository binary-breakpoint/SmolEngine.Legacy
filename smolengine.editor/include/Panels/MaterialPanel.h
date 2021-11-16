#pragma once
#include "Core/Core.h"
#include "Materials/PBRFactory.h"
#include "TexturesLoader.h"

#include <future>

namespace SmolEngine
{
	class EditorLayer;
	class GraphicsPipeline;
	class Framebuffer;
	class EditorCamera;

	struct PreviewRenderingData
	{
		Ref<GraphicsPipeline>   Pipeline = nullptr;
		Ref<Framebuffer>        Framebuffer = nullptr;
		Ref<EditorCamera>       Camera = nullptr;
	};

	class MaterialPanel
	{
	public:

		MaterialPanel();
		~MaterialPanel();

		void OpenExisting(const std::string& path);
		void OpenNew(const std::string& path);
		void Close();
		void Update();
		void Save();

		std::string GetCurrentPath() const;
		static MaterialPanel* GetSingleton();

	private:
		bool DrawTextureInfo(TextureCreateInfo& texInfo, const std::string& title, PBRTexture type);
		void InitPreviewRenderer();
		void RenderImage();
		void ApplyChanges();

	private:
		inline static MaterialPanel* s_Instance = nullptr;
	    TexturesLoader*               m_TexturesLoader = nullptr;
		PBRCreateInfo*                m_MaterialCI = nullptr;
		PreviewRenderingData*         m_Data = nullptr;
		Ref<PBRHandle>                m_PBRHandle = nullptr;
		bool                          m_bShowPreview = false;
		int                           m_GeometryType = 1;
		const uint32_t                m_BindingPoint = 277;
		std::future<void>             m_DrawResult = {};
		std::string                   m_CurrentFilePath = "";
		std::string                   m_CurrentName = "";
	};
}