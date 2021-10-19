#pragma once

int main(int argc, char** argv)
{

}

#if 0

#ifdef FROSTIUM_SMOLENGINE_IMPL
using namespace SmolEngine;
#else
using namespace Frostium;
#endif

#include <FrostiumCore.h>


struct RendererData
{
	// Pipelines
	GraphicsPipeline   MRT_Pipeline = {};
	GraphicsPipeline   Comp_Pipeline = {};
	// Framebuffers			     
	Framebuffer        MRT_Framebufer = {};
	/// Meshes
	Mesh*              Box_Mesh = nullptr;
};

struct CameraUBO
{
	glm::mat4 projection{};
	glm::mat4 view{};
	glm::vec4 camPos{};
};

struct InstanceUBO
{
	glm::vec4 pos;
	glm::vec4 scale;
};
							     
class CustomRenderer		     
{							     
public:						     
							     
	void Init();			         
	void BuildPipelines();	    
	void BuildFramebuffers();
	void BuildMaterials();
	void BuildObjects();
	void Render();

	void OnResize(uint32_t width, uint32_t height);
							     
private:					     
							     
	GraphicsContext*             m_Context = nullptr;
	EditorCamera*                m_Camera = nullptr;
	MaterialPool*             m_MaterialLibrary = nullptr;
	bool                         m_Process = true;
	RendererData                 m_Storage = {};
	CameraUBO                    m_CameraUBO = {};
	std::vector<InstanceUBO>     m_Instances;
	std::vector<uint32_t>        m_MaterialsIDs;
};		

#endif