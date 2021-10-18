#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/Vulkan.h"
#include "Renderer/PBRLoader.h"

namespace SmolEngine
{
	struct PBRAttachment
	{
		VkImage       Image = nullptr;
		VkImageView   ImageView = nullptr;
		VkSampler     Sampler = nullptr;
		VmaAllocation Alloc = nullptr;
	};

	class VulkanTexture;
	class Texture;

	class VulkanPBRLoader: public PBRLoader
	{
	public:		    
		virtual void          Free() override;
		virtual void          GeneratePBRCubeMaps(Ref<Texture>& environment_map) override;
		virtual void*         GetBRDFLUTDesriptor() override;
		virtual void*         GetIrradianceDesriptor() override;
		virtual void*         GetPrefilteredCubeDesriptor() override;

	private:
		void                  GenerateBRDFLUT(VkDescriptorImageInfo& outImageInfo);
		void                  GenerateIrradianceCube(VulkanTexture* skyBox, VkDescriptorImageInfo& outImageInfo);
		void                  GeneratePrefilteredCube(VulkanTexture* skyBox, VkDescriptorImageInfo& outImageInfo);
		void                  DestroyAttachment(PBRAttachment& obj);

	private:
		PBRAttachment         m_BRDFLUT = {};
		PBRAttachment         m_Irradiance = {};
		PBRAttachment         m_PrefilteredCube = {};
		VkDescriptorImageInfo m_BRDFLUTImageInfo = {};
		VkDescriptorImageInfo m_IrradianceImageInfo = { };
		VkDescriptorImageInfo m_PrefilteredCubeImageInfo = {};
	};
}
#endif