#pragma once
#include <GraphicsPipeline.h>
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
	glm::mat4 model{};
};
							     
class CustomRenderer		     
{							     
public:						     
							     
	void Init();			         
	void BuildPipelines();	    
	void BuildFramebuffers();
	void BuildObjects();
	void Render();

	void OnResize(uint32_t width, uint32_t height);
							     
private:					     
							     
	Frostium::GraphicsContext*   m_Context = nullptr;
	Frostium::EditorCamera*      m_Camera = nullptr;
	bool                         m_Process = true;
	RendererData                 m_Storage = {};
	CameraUBO                    m_CameraUBO = {};
	std::vector<InstanceUBO>     m_Instances;
};							   