#include "CSharpBinding.h"

#include <Common/Mesh.h>
#include <Common/Input.h>
#include <GraphicsContext.h>
#include <MaterialLibrary.h>
#include <Renderer.h>
#include <ImGUI/ImGuiExtension.h>

#include <mono/metadata/assembly.h>
#include <mono/jit/jit.h>


using namespace Frostium;

GraphicsContext* context = nullptr;

void PrintMethod(MonoString* string)
{
	char* cppString = mono_string_to_utf8(string);
	NATIVE_INFO(cppString);
	mono_free(cppString);
}

void SetWindow(MonoString* title, int32_t width, int32_t height)
{
	std::string str(mono_string_to_utf8(title));
	context->GetWindow()->SetTitle(str);
	context->GetWindow()->SetWidth(width);
	context->GetWindow()->SetHeight(height);
}

void InitMono()
{
	//Indicate Mono where you installed the lib and etc folders
	mono_set_dirs("../vendor/mono/lib", "../vendor/mono/etc");
	//Create the main CSharp domain
	MonoDomain* domain = mono_jit_init("CSharp_Domain");
	//Load the binary file as an Assembly
	MonoAssembly* csharpAssembly = mono_domain_assembly_open(domain, "../vendor/mono/CSharp/Debug/CSharp.exe");
	if (!csharpAssembly)
	{
		//Error detected
		return;
	}

	//SetUp Internal Calls called from CSharp

	//Namespace.Class::Method + a Function pointer with the actual definition
	mono_add_internal_call("Frostium.FrostiumEngine::PrintMethod", &PrintMethod);
	//Namespace.Class::Method + a Function pointer with the actual definition
	mono_add_internal_call("Frostium.FrostiumEngine::SetWindow", &SetWindow);

	int argc = 1;
	char* argv[1] = { (char*)"CSharp" };

	//Call the main method in this code
	mono_jit_exec(domain, csharpAssembly, argc, argv);
}

int main(int argc, char** argv)
{
	WindowCreateInfo windoInfo = {};
	{
		windoInfo.bFullscreen = false;
		windoInfo.bVSync = false;
		windoInfo.Height = 480;
		windoInfo.Width = 720;
		windoInfo.Title = "Frostium Skinning";
	}

	EditorCamera* camera = nullptr;
	{
		EditorCameraCreateInfo cameraCI = {};
		camera = new EditorCamera(&cameraCI);
	}

	GraphicsContextInitInfo info = {};
	{
		info.Flags = Features_Renderer_3D_Flags | Features_ImGui_Flags;
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
		info.pDefaultCamera = camera;
		info.eShadowMapSize = ShadowMapSize::SIZE_16;
	}

	bool process = true;
	ClearInfo clearInfo = {};
	context = new GraphicsContext(&info);
	context->SetEventCallback([&](Event& e)
		{
			if (e.IsType(EventType::WINDOW_CLOSE))
				process = false;
		});

	InitMono();
	static glm::vec3 lightDir = glm::vec3(105.0f, 53.0f, 102.0f);
	Mesh* cube = GraphicsContext::GetSingleton()->GetBoxMesh();

	// Load assets
	MaterialCreateInfo materialCI = {};
	materialCI.SetTexture(MaterialTexture::Albedro, "Assets/materials/stone/Tiles087_1K_Color.png");
	materialCI.SetTexture(MaterialTexture::Normal, "Assets/materials/stone/Tiles087_1K_Normal.png");
	materialCI.SetTexture(MaterialTexture::Roughness, "Assets/materials/stone/Tiles087_1K_Roughness.png");
	materialCI.SetTexture(MaterialTexture::AO, "Assets/materials/stone/Tiles087_1K_AmbientOcclusion.png");
	materialCI.SetMetalness(0.1f);
	uint32_t stoneMat = MaterialLibrary::GetSinglenton()->Add(&materialCI);
	Renderer::UpdateMaterials();

	while (process)
	{
		context->ProcessEvents();
		DeltaTime deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		context->UpdateSceneInfo(); // uses default camera - updates automatically
		context->BeginFrame(deltaTime);
		{
			ImGui::Begin("Debug Window");
			{

			}
			ImGui::End();

			Renderer::BeginScene(&clearInfo);
			Renderer::SubmitMesh({ 0, -5.0, 0 }, { 0, 0, 0 }, { 10, 1, 10, }, cube, stoneMat);
			Renderer::EndScene();
		}
		context->SwapBuffers();
	}
}