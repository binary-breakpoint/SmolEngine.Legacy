#include "HelloTriangle.h"

#include <GraphicsCore.h>

using namespace SmolEngine;

int main(int argc, char** argv)
{
	WindowCreateInfo windoInfo = {};
	{
		windoInfo.bFullscreen = false;
		windoInfo.bVSync = false;
		windoInfo.Height = 480;
		windoInfo.Width = 720;
		windoInfo.Title = "Hello Triangle";
	}

	Camera* camera = nullptr;
	{
		EditorCameraCreateInfo cameraCI = {};
		cameraCI.Type = CameraType::Ortho;

		camera = new EditorCamera(&cameraCI);
	}

	GraphicsContextInitInfo info = {};
	{
		info.eMSAASamples = MSAASamples::SAMPLE_COUNT_1;
		info.ResourcesFolderPath = "../resources/";
		info.pWindowCI = &windoInfo;
		info.eFeaturesFlags = FeaturesFlags::Imgui;
	}
}
