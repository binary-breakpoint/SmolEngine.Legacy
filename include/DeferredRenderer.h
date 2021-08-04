#pragma once
#include "Common/Core.h"
#include "Common/RendererShared.h"
#include "Common/RendererStorage.h"

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	class Mesh;
	class Framebuffer;
	class CubeMap;
	struct MeshComponent;
	enum class ShadowMapSize : uint16_t;

	class DeferredRenderer
	{
	public:
		static void Init(RendererStorage* storage);
		static void Shutdown();
		static void BeginScene(const ClearInfo* clearInfo);
		static void DrawOffscreen(Framebuffer* fb, BeginSceneInfo* sceneInfo, RendererStateEX* state);
		static void EndScene();

		// SUbmit
		static void SubmitMesh(const glm::vec3& pos, const glm::vec3& rotation, const glm::vec3& scale, Mesh* mesh, const uint32_t& PBRmaterialID = 0);
		static void SubmitMeshEx(const glm::vec3& pos, const glm::vec3& rotation, const glm::vec3& scale, Mesh* mesh, const uint32_t& PBRmaterialID);
		static void SubmitDirLight(DirectionalLight* light);
		static void SubmitPointLight(PointLight* light);
		static void SubmitSpotLight(SpotLight* light);

		// States
		static void SetDirtMask(Texture* mask, float intensity, float baseIntensity);
		static void SetEnvironmentCube(CubeMap* cube);
		static void SetDynamicSkyboxProperties(DynamicSkyProperties& properties, bool regeneratePBRmaps = true);
		static void SetDebugView(DebugViewFlags flag);
		static void SetGridActive(bool active);
		static void SetBloomActive(bool active);
		static void SetFxaaActive(bool active);
		static void SetIblActive(bool active);
		static void SetFrustumRadius(float radius);
		static void SetIBLProperties(IBLProperties& properties);
		static void SetBloomProperties(BloomProperties& properties);
		static void SetFxaaProperties(FXAAProperties& properties);

		static RendererStateEX& GetState();
		static Framebuffer* GetFramebuffer();
		static uint32_t GetNumObjects();
		static bool UpdateMaterials();
		static bool ResetStates();

	private:
		static void Reset();
		static void Flush();
		static void Render();
		static void StartNewBacth();
		static void BuildDrawList();
		static void UpdateUniforms();
		static void OnResize(uint32_t width, uint32_t height);

		static void InitPBR();
		static void InitPipelines();
		static void InitFramebuffers();
		static void CalculateDepthMVP(glm::mat4& out_mvp);

		static void AddMesh(const glm::vec3& pos, const glm::vec3& rotation,
			const glm::vec3& scale, Mesh* mesh, const uint32_t& materialID);

	private:
		friend class GraphicsContext;
	};
}
