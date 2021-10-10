#pragma once
#ifndef OPENGL_IMPL
#include "Backends/Vulkan/Vulkan.h"

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

	class VulkanPBR
	{
	public:
		VulkanPBR();				    
		void                            GeneratePBRCubeMaps(Ref<Texture>& environment_map);
		void                            Free();
									    
		// Getters					    
		static VulkanPBR*               GetSingleton();
		const VkDescriptorImageInfo&    GetBRDFLUTImageInfo();
		const VkDescriptorImageInfo&    GetIrradianceImageInfo();
		const VkDescriptorImageInfo&    GetPrefilteredCubeImageInfo();

	private:
		void                            GenerateBRDFLUT(VkDescriptorImageInfo& outImageInfo);
		void                            GenerateIrradianceCube(VulkanTexture* skyBox, VkDescriptorImageInfo& outImageInfo);
		void                            GeneratePrefilteredCube(VulkanTexture* skyBox, VkDescriptorImageInfo& outImageInfo);
		void                            DestroyAttachment(PBRAttachment& obj);

	private:
		inline static VulkanPBR*        s_Instance = nullptr;
		PBRAttachment                   m_BRDFLUT = {};
		PBRAttachment                   m_Irradiance = {};
		PBRAttachment                   m_PrefilteredCube = {};
		VkDescriptorImageInfo           m_BRDFLUTImageInfo = {};
		VkDescriptorImageInfo           m_IrradianceImageInfo = { };
		VkDescriptorImageInfo           m_PrefilteredCubeImageInfo = {};
	};
}
#endif