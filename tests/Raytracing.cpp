#include "Raytracing.h"

#include <GraphicsCore.h>

#include "Backends/Vulkan/VulkanTexture.h"
#include "Backends/Vulkan/VulkanContext.h"
#include "Backends/Vulkan/VulkanRaytracingPipeline.h"

using namespace SmolEngine;
int main(int argc, char** argv)
{
	EditorCameraCreateInfo cameraCI = {};
	Camera* camera = new EditorCamera(&cameraCI);
	DebugLog* log = new DebugLog([&](const std::string&& msg, LogLevel level) { std::cout << msg << "\n"; });

	WindowCreateInfo windoInfo = {};
	{
		windoInfo.bFullscreen = false;
		windoInfo.bVSync = false;
		windoInfo.Height = 480;
		windoInfo.Width = 720;
		windoInfo.Title = "Raytracing";
	}

	GraphicsContextCreateInfo info = {};
	info.ResourcesFolder = "../resources/";
	info.pWindowCI = &windoInfo;
	info.eFeaturesFlags = {};

	auto context = GraphicsContext::Create(&info);
	context->SetEventCallback([&](Event& e) 
     { 
         camera->OnEvent(e);
     });

	BufferLayout mainLayout =
	{
		{ DataTypes::Float3, "aPos" },
		{ DataTypes::Float3, "aNormal" },
		{ DataTypes::Float3, "aTangent" },
		{ DataTypes::Float2, "aUV" },
		{ DataTypes::Int4,   "aBoneIDs"},
		{ DataTypes::Float4, "aWeight"}
	};

	RaytracingPipelineCreateInfo rtCreateInfo{};
	rtCreateInfo.ShaderCI.Stages[ShaderType::RayCloseHit] = info.ResourcesFolder + "Shaders/Raytracing/reflections.rchit";
	rtCreateInfo.ShaderCI.Stages[ShaderType::RayGen] = info.ResourcesFolder + "Shaders/Raytracing/reflections.rgen";
	rtCreateInfo.ShaderCI.Stages[ShaderType::RayMiss] = info.ResourcesFolder + "Shaders/Raytracing/reflections.rmiss";

	rtCreateInfo.ShaderCI.Buffers[2].bGlobal = false;
	rtCreateInfo.ShaderCI.Buffers[666].bGlobal = false;
	rtCreateInfo.ShaderCI.Buffers[666].Size = sizeof(VulkanRaytracingPipeline::ObjDesc) * 1000;
	rtCreateInfo.VertexInput = VertexInputInfo(sizeof(PBRVertex), mainLayout);

	Ref<RaytracingPipeline> rtPipeline = RaytracingPipeline::Create();
	rtPipeline->Build(&rtCreateInfo);

	glm::mat4 model;
	Utils::ComposeTransform(glm::vec3(0), glm::vec3(0), glm::vec3(1), model);

	Ref<Mesh> sponza = Mesh::Create();
	sponza->LoadFromFile("Assets/sponza_small.gltf");

	RaytracingPipelineSceneInfo sceneCI{};
	sceneCI.Transforms = { model };
	//                         mesh, instance count
	sceneCI.Scene.push_back({ sponza , 1 });

	rtPipeline->CreateScene(&sceneCI);

	Ref<Texture> storageTex = Texture::Create();
	{
		TextureCreateInfo texCI{};
		texCI.Width = 720;
		texCI.Height = 480;

		storageTex->LoadAsStorage(&texCI);
		rtPipeline->UpdateTexture(storageTex, 1);
	}

	while (context->IsOpen())
	{
		context->ProcessEvents();
		float deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		camera->OnUpdate(deltaTime);

		context->BeginFrame(deltaTime);
		{
			struct CameraProperties
			{
				glm::mat4 viewProj;
				glm::mat4 viewInverse;
				glm::mat4 projInverse;
			} camData;

			struct PC
			{
				glm::vec4  clearColor = glm::vec4(1);
				glm::vec3  lightPosition = glm::vec3(105.0f, 53.0f, 102.0f);
				float lightIntensity = 1;
				int   lightType = 1;
				int   maxDepth = 4;
			} static pc{};

			camData.viewProj = camera->GetProjection() * camera->GetViewMatrix();
			camData.projInverse = glm::inverse(camera->GetProjection());
			camData.viewInverse = glm::inverse(camera->GetViewMatrix());

			rtPipeline->UpdateBuffer(2, sizeof(CameraProperties), &camData);
			rtPipeline->SetCommandBuffer(); // default
			rtPipeline->SubmitPushConstant(ShaderType::RayCloseHit, sizeof(PC), &pc);
			rtPipeline->Dispatch(720, 480);

			VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

			auto& swapchain = VulkanContext::GetSwapchain();
			auto cmd = VulkanContext::GetCurrentVkCmdBuffer();

			// Prepare current swap chain image as transfer destination
			VulkanTexture::SetImageLayout(
				cmd,
				swapchain.GetCurrentImage(),
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

			// Prepare ray tracing output image as transfer source
			VulkanTexture::SetImageLayout(
				cmd,
				storageTex->Cast<VulkanTexture>()->GetVkImage(),
				VK_IMAGE_LAYOUT_GENERAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				subresourceRange);

			VkImageCopy copyRegion{};
			copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.srcOffset = { 0, 0, 0 };
			copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
			copyRegion.dstOffset = { 0, 0, 0 };
			copyRegion.extent = { 720, 480, 1 };
			vkCmdCopyImage(cmd, storageTex->Cast<VulkanTexture>()->GetVkImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, swapchain.GetCurrentImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			// Transition swap chain image back for presentation
			VulkanTexture::SetImageLayout(
				cmd,
				swapchain.GetCurrentImage(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				subresourceRange);

			// Transition ray tracing output image back to general layout
			VulkanTexture::SetImageLayout(
				cmd,
				storageTex->Cast<VulkanTexture>()->GetVkImage(),
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_GENERAL,
				subresourceRange);

		}
		context->SwapBuffers();
	}
}
