#include "CSharpBinding.h"

#include <Common/Mesh.h>
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
		windoInfo.Title = "Frostium Example";
	}

	EditorCameraCreateInfo cameraCI = {};
	GraphicsContextInitInfo info = {};
	{
		info.Flags = Features_Renderer_3D_Flags | Features_HDR_Flags | Features_ImGui_Flags;
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
		info.pEditorCameraCI = &cameraCI;
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

	Mesh sword = {};
	Mesh::Create("Assets/sword.fbx", &sword);
	MaterialCreateInfo swordMat = {};
	{
		swordMat.SetTexture(MaterialTexture::Albedro, "Assets/materials/sword/SHD_greatsword_Base_Color.PNG");
		swordMat.SetTexture(MaterialTexture::Normal, "Assets/materials/sword/SHD_greatsword_Normal_OpenGL.PNG");
		swordMat.SetTexture(MaterialTexture::Roughness, "Assets/materials/sword/SHD_greatsword_Roughness.PNG");
		swordMat.SetTexture(MaterialTexture::Metallic, "Assets/materials/sword/SHD_greatsword_Metallic.PNG");
		swordMat.SetTexture(MaterialTexture::AO, "Assets/materials/sword/SHD_greatsword_Mixed_AO.PNG");
	}
	int32_t matID = MaterialLibrary::GetSinglenton()->Add(&swordMat, "Assets/materials/weapon.mat");
	Renderer::UpdateMaterials();
	static glm::vec3 lightDir = glm::vec3(105.0f, 53.0f, 102.0f);

	while (process)
	{
		context->ProcessEvents();
		DeltaTime deltaTime = context->CalculateDeltaTime();

		if (context->IsWindowMinimized())
			continue;

		context->BeginFrame(deltaTime);
		{
			Renderer::BeginScene(&clearInfo);
			Renderer::SetShadowLightDirection(lightDir);
			Renderer::SubmitDirectionalLight(lightDir, { 1, 1, 1, 1 });
			Renderer::SubmitMesh({ 0,0,0 }, { 0,0, 0 }, { 2, 2, 2 }, &sword, matID);
			Renderer::EndScene();

			ImGui::Begin("Debug Window");
			{
				ImGui::DragFloat3("LightDir", glm::value_ptr(lightDir));
			}
			ImGui::End();
		}
		context->SwapBuffers();
	}
}