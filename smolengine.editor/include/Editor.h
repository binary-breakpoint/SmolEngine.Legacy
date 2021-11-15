#pragma once

#include "SmolEngineCore.h"

namespace SmolEngine
{
	class EditorCamera;

	class Editor : public Engine
	{
	public:

		~Editor();

		void OnEngineModuleCreation(EngineModuleCreateInfo* info) override;
		void OnLayerModuleCreation(LayerManager* layerManager) override;
		void OnGraphicsModuleCreation(GraphicsContextInitInfo* info) override;
		void OnPhysicsModuleCreation(PhysicsModuleCreateInfo* info) override;
		void OnScriptModuleCreation(ScriptingSystem* scriptingSytem) override;

	private:

		EditorCamera* m_Camera = nullptr;
	};

}

