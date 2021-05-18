#pragma once
#include <GraphicsPipeline.h>
#include <MaterialLibrary.h>
#include <Common/Mesh.h>
#include <Common/Framebuffer.h>

int main(int argc, char** argv);

struct RendererData
{
	// Pipelines
	Frostium::GraphicsPipeline   MRT_Pipeline = {};
	Frostium::GraphicsPipeline   Comp_Pipeline = {};
	// Framebuffers			     
	Frostium::Framebuffer        MRT_Framebufer = {};
	/// Meshes
	Frostium::Mesh*              Box_Mesh = nullptr;
};

struct CameraUBO
{
	glm::mat4 projection{};
	glm::mat4 view{};
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
							     
	Frostium::GraphicsContext*   m_Context = nullptr;
	Frostium::EditorCamera*      m_Camera = nullptr;
	Frostium::MaterialLibrary*   m_MaterialLibrary = nullptr;
	bool                         m_Process = true;
	RendererData                 m_Storage = {};
	CameraUBO                    m_CameraUBO = {};
	std::vector<InstanceUBO>     m_Instances;
	std::vector<uint32_t>        m_MaterialsIDs;
};							   