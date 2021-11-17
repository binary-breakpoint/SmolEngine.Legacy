#include "stdafx.h"
#include "Scripting/CSharp/MonoContext.h"
#include "Scripting/CSharp/CSharpAPI.h"
#include "ECS/Components/ScriptComponent.h"

#include "Pools/PrefabPool.h"

#include <mono/metadata/assembly.h>
#include <mono/jit/jit.h>
#include <mono/utils/mono-counters.h>
#include <mono/utils/mono-logger.h>

#include <mono/metadata/environment.h>
#include <mono/metadata/exception.h>

#include <mono/metadata/appdomain.h>
#include <mono/metadata/object.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/threads.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/mono-config.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/profiler.h>
#include <mono/metadata/debug-helpers.h>

namespace SmolEngine
{
	void LogHook(const char* log_domain, const char* log_level, const char* message, mono_bool fatal, void* user_data)
	{
		DebugLog::LogInfo(message);
	}

	void PrintHook(const char* string, mono_bool is_stdout)
	{
		DebugLog::LogInfo(string);
	}

	MonoContext::MonoContext()
	{
		mono_set_dirs("../vendor/mono/lib", "../vendor/mono/etc");
		mono_debug_init(MONO_DEBUG_FORMAT_MONO);
		mono_config_parse(NULL);
		mono_set_signal_chaining(true);
		mono_trace_set_log_handler(LogHook, nullptr);
		mono_trace_set_print_handler(PrintHook);
		mono_trace_set_printerr_handler(PrintHook);

		s_Instance = this;
		m_RootDomain = mono_jit_init("CSharp_Domain");
		Create();
	}

	void MonoContext::Create()
	{
		LoadDomain();
		LoadMonoImage();
		LoadAssembly();
	}

	void MonoContext::Shutdown()
	{
		if (m_CSharpAssembly != nullptr && m_Domain != m_RootDomain)
		{
			mono_domain_set(m_RootDomain, false);

			mono_image_close(m_Image);
			mono_domain_finalize(m_Domain, 2000);
			mono_gc_collect(mono_gc_max_generation());
			mono_domain_unload(m_Domain);

			m_InternalClasses.clear();
			m_MetaMap.clear();
			m_Domain = nullptr;
			m_CSharpAssembly = nullptr;
			m_Image = nullptr;
		}
	}

	bool MonoContext::IsRunning()
	{
		return m_CSharpAssembly != nullptr;
	}

	MonoContext* MonoContext::GetSingleton()
	{
		return s_Instance;
	}

	MonoDomain* MonoContext::GetDomain()
	{
		return m_Domain;
	}

	void MonoContext::SetOnReloadCallback(const std::function<void()>& callback)
	{
		m_Callback = callback;
	}

	void MonoContext::Track()
	{
		std::filesystem::path p(m_DLLPath);
		if (std::filesystem::exists(p))
		{
			auto time = std::filesystem::last_write_time(p);
			if (time != m_LastWriteTime)
			{
				DebugLog::LogWarn("[C# module]: Reloading...");
				Shutdown();
				Create();

				if (m_Callback != nullptr)
					m_Callback();

				OnRecompilation();
				DebugLog::LogWarn("[C# module]: Reloading complete!");
			}
		}
	}

	void MonoContext::RunTest()
	{
		{
			MonoClass* m_class = m_InternalClasses[InternalClassType::UnitTests];
			MonoMethod* method = mono_class_get_method_from_name(m_class, "CallMe", 2);
			MonoObject* instance = mono_object_new(m_Domain, m_class);
			mono_runtime_object_init(instance);

			if (method)
			{
				uint32_t arg1 = 266;
				uint32_t arg2 = 55;

				void* args[2];
				args[0] = &arg1;
				args[1] = &arg2;

				mono_runtime_invoke(method, instance, args, NULL);
			}
		}
	}

	void MonoContext::LoadMonoImage()
	{
		MonoImageOpenStatus status;
		std::ifstream instream(m_DLLPath.c_str(), std::ios::in | std::ios::binary);
		std::vector<uint8_t> data((std::istreambuf_iterator<char>(instream)), std::istreambuf_iterator<char>());

		m_Image = mono_image_open_from_data_with_name(
			(char*)&data[0], static_cast<uint32_t>(data.size()),
			true, &status, false,
			"SmolEngine");

		if (status != MONO_IMAGE_OK)
		{
			DebugLog::LogError("Failed to create mono context"); abort();
		}

		// debug symbols
		std::filesystem::path p(m_DLLPath);
		std::string pdbPath = p.parent_path().u8string() + "/" + p.filename().stem().u8string() + ".pdb";
		if (std::filesystem::exists(pdbPath))
		{
			instream = std::ifstream(pdbPath.c_str(), std::ios::in | std::ios::binary);
			data = std::vector<uint8_t>((std::istreambuf_iterator<char>(instream)), std::istreambuf_iterator<char>());
			mono_debug_open_image_from_memory(m_Image, &data[0], static_cast<uint32_t>(data.size()));
		}

	}

	void MonoContext::LoadDomain()
	{
		m_Domain = mono_domain_create_appdomain("SmolEngine.ScriptsDomain", NULL);
		mono_domain_set(m_Domain, false);
	}

	void* MonoContext::CreateClassInstance(const std::string& class_name, const Ref<Actor>& actor)
	{
		auto& it = m_MetaMap.find(class_name);
		if (it != m_MetaMap.end())
		{
			const auto& meta = it->second;
			MonoClassField* id_field = mono_class_get_field_from_name(meta.pClass, "MyEntityID");
			if (id_field)
			{
				if (mono_type_get_type(mono_field_get_type(id_field)) == MONO_TYPE_U4) // uint32
				{
					MonoObject* instance = mono_object_new(m_Domain, meta.pClass);
					mono_runtime_object_init(instance);

					uint32_t id = (uint32_t)actor->m_Entity;
					mono_field_set_value(instance, id_field, &id);
					return instance;
				}
			}
		}

		return nullptr;
	}

	void MonoContext::UpdateFields(void* script_)
	{
		auto script = (ScriptComponent::CSharpInstance*)script_;
		MonoObject* instance = (MonoObject*)script->Instance;

		if (!instance)
			return;

		MonoClass* mono_class = m_MetaMap[script->Name].pClass;
		for (auto& field : script->Fields.GetFields())
		{
			MonoClassField* id_field = mono_class_get_field_from_name(mono_class, field.name.c_str());

			if (field.type == FieldDataFlags::Int32 || field.type == FieldDataFlags::Float)
			{
				mono_field_set_value(instance, id_field, field.ptr);
			}

			if (field.type == FieldDataFlags::Actor)
			{
				int32_t* id = (int32_t*)field.ptr;
				Ref<Actor> actor = WorldAdmin::GetSingleton()->GetActiveScene()->FindActorByID(*id);
				if (actor)
				{
					uint32_t    enttID = (uint32_t)actor->m_Entity;
					MonoClass*  actor_class = m_InternalClasses[InternalClassType::Actor];
					MonoObject* actor_instance = mono_object_new(m_Domain, actor_class);

					MonoMethodDesc* desc = mono_method_desc_new(":.ctor (uint)", FALSE);
					MonoMethod* ctor = mono_method_desc_search_in_class(desc, actor_class);
					mono_method_desc_free(desc);

					void* args[1];
					args[0] = &enttID;
					mono_runtime_invoke(ctor, actor_instance, args, NULL);
					mono_field_set_value(instance, id_field, actor_instance);
				}
			}

			if (field.type == FieldDataFlags::Prefab)
			{
				std::string* str = (std::string*)field.ptr;
				if (std::filesystem::exists(*str))
				{
					Ref<Prefab> prefab = PrefabPool::ConstructFromPath(*str);
					if (prefab)
					{
						
						MonoClass* prefab_class = m_InternalClasses[InternalClassType::Prefab];
						MonoObject* prefab_instance = mono_object_new(m_Domain, prefab_class);
					
						MonoMethodDesc* desc = mono_method_desc_new(":.ctor (ulong)", FALSE);
						MonoMethod* ctor = mono_method_desc_search_in_class(desc, prefab_class);
						mono_method_desc_free(desc);
					
						size_t id = prefab->GetUUID();
						void* args[1];
						args[0] = &id;
						mono_runtime_invoke(ctor, prefab_instance, args, NULL);
						mono_field_set_value(instance, id_field, prefab_instance);
					}
				}
			}

			if (field.type == FieldDataFlags::String)
			{
				std::string* str = (std::string*)field.ptr;
				MonoString* mono_str = mono_string_new(m_Domain, str->c_str());
				mono_field_set_value(instance, id_field, mono_str);
			}
		}
	}

	void* MonoContext::GetMethod(const char* signature, const char* class_name, MonoClass* p_class)
	{
		std::stringstream ss;
		ss << class_name << ":" << signature;
		MonoMethodDesc* desc = mono_method_desc_new(ss.str().c_str(), NULL);
		MonoMethod* method = mono_method_desc_search_in_class(desc, p_class);

		mono_method_desc_free(desc);
		return method;
	}

	void MonoContext::OnBegin(ScriptComponent* comp)
	{
		for (auto& script : comp->CSharpScripts)
		{
			const MonoContext::MetaData* meta = GetMeta(comp, script.Name);
			if (meta != nullptr)
			{
				MonoObject* instance = (MonoObject*)CreateClassInstance(script.Name, comp->pActor);
				if (instance)
				{
					script.Instance = instance;
					UpdateFields(&script);

					MonoObject* result = mono_runtime_invoke(meta->pOnBegin, instance, NULL, NULL);
				}
			}
		}
	}

	void MonoContext::OnUpdate(const ScriptComponent* comp)
	{
		for (auto& script : comp->CSharpScripts)
		{
			const MonoContext::MetaData* meta = GetMeta(comp, script.Name);
			if (meta != nullptr && script.Instance)
			{
				MonoObject* instance = (MonoObject*)script.Instance;
				MonoObject* result = mono_runtime_invoke(meta->pOnUpdate, instance, NULL, NULL);
			}
		}
	}

	void MonoContext::OnInternalUpdate(float delta)
	{

	}

	void MonoContext::OnDestroy(ScriptComponent* comp)
	{
		for (auto& script : comp->CSharpScripts)
		{
			const MonoContext::MetaData* meta = GetMeta(comp, script.Name);
			if (meta != nullptr && script.Instance)
			{
				MonoObject* instance = (MonoObject*)script.Instance;
				MonoObject* result = mono_runtime_invoke(meta->pOnDestroy, instance, NULL, NULL);

				script.Instance = nullptr;
			}
		}
	}

	void MonoContext::OnCollisionBegin(const ScriptComponent* comp, Actor* another, bool isTrigger)
	{

	}

	void MonoContext::OnCollisionEnd(const ScriptComponent* comp, Actor* another, bool isTrigger)
	{

	}

	void MonoContext::OnConstruct(ScriptComponent* comp)
	{
		for (auto& script: comp->CSharpScripts)
		{
			auto& it = m_MetaMap.find(script.Name);
			if (it != m_MetaMap.end())
			{
				script.Fields.FieldCopyOrReplace(&it->second.Fields);
			}
			else
			{
				DebugLog::LogWarn("[MonoContext]: C# Script {} not found!", script.Name);
				script.Name = "";
			}
		}
	}

	void MonoContext::OnRecompilation()
	{
		auto& reg = WorldAdmin::GetSingleton()->GetActiveScene()->GetRegistry();
		const auto& view = reg.view<ScriptComponent>();
		for (const auto& entity : view)
		{
			auto& component = view.get<ScriptComponent>(entity);
			OnConstruct(&component);
		}
	}

	const MonoContext::MetaData* MonoContext::GetMeta(const ScriptComponent* comp, const std::string& class_name) const
	{
		auto& it = m_MetaMap.find(class_name);
		if (it != m_MetaMap.end())
		{
			return &it->second;
		}

		return nullptr;
	}

	void MonoContext::LoadAssembly(bool is_initialization)
	{
		MonoImageOpenStatus status;
		m_CSharpAssembly = mono_assembly_load_from_full(m_Image, "SmolEngine", &status, false);
		m_LastWriteTime = std::filesystem::last_write_time(m_DLLPath);

		ResolveFunctions();
		ResolveClasses();
		ResolveMeta();

		// temp
		RunTest();
	}

	void MonoContext::ResolveFunctions()
	{
		// SetUp Internal Calls called from CSharp
		// Namespace.Class::Method + a Function pointer with the actual definition

		// Actor
		mono_add_internal_call("SmolEngine.Actor::GetComponent_EX", &CSharpAPi::GetComponent_CSharpAPI);
		mono_add_internal_call("SmolEngine.Actor::SetComponent_EX", &CSharpAPi::SetComponent_CSharpAPI);
		mono_add_internal_call("SmolEngine.Actor::HasComponent_EX", &CSharpAPi::HasComponent_CSharpAPI);
		mono_add_internal_call("SmolEngine.Actor::AddComponent_EX", &CSharpAPi::AddComponent_CSharpAPI);
		mono_add_internal_call("SmolEngine.Actor::GetEntityName_EX", &CSharpAPi::GetEntityName_CSharpAPI);
		mono_add_internal_call("SmolEngine.Actor::GetEntityTag_EX", &CSharpAPi::GetEntityTag_CSharpAPI);
		mono_add_internal_call("SmolEngine.Actor::GetActorID_EX", &CSharpAPi::GetActorID_CSharpAPI);
		mono_add_internal_call("SmolEngine.Actor::DestroyComponent_EX", &CSharpAPi::DestroyComponent_CSharpAPI);

		// Scene
		mono_add_internal_call("SmolEngine.SceneManager::FindActorByName_EX", &CSharpAPi::FindActorByName_CSharpAPI);
		mono_add_internal_call("SmolEngine.SceneManager::FindActorByTag_EX", &CSharpAPi::FindActorByTag_CSharpAPI);
		mono_add_internal_call("SmolEngine.SceneManager::IsActorExistsID_EX", &CSharpAPi::IsActorExistsID_CSharpAPI);
		mono_add_internal_call("SmolEngine.SceneManager::CreateActor_EX", &CSharpAPi::CreateActor_CSharpAPI);
		mono_add_internal_call("SmolEngine.SceneManager::DestroyActor_EX", &CSharpAPi::DestroyActor_CSharpAPI);

		// Prefab
		mono_add_internal_call("SmolEngine.Prefab::Instantiate_EX",&CSharpAPi::PrefabInstantiate_CSharpAPI);

		// Assets
		mono_add_internal_call("SmolEngine.AssetManager::LoadAsset_EX", &CSharpAPi::LoadAsset_CSharpAPI);

		// Utils
		mono_add_internal_call("SmolEngine.Input::IsKeyPressed_EX", &CSharpAPi::IsKeyInput_CSharpAPI);
		mono_add_internal_call("SmolEngine.Input::IsMouseButtonPressed_EX", &CSharpAPi::IsMouseInput_CSharpAPI);
		mono_add_internal_call("SmolEngine.Input::GetMousePos_EX", &CSharpAPi::GetMousePos_CSharpAPI);
		mono_add_internal_call("SmolEngine.SLog::WriteLine_EX", &CSharpAPi::AddMessage_CSharpAPI);

		// Mesh
		mono_add_internal_call("SmolEngine.MeshComponent::SetMaterial_EX", &CSharpAPi::MeshSetMaterial_CSharpAPI);
		mono_add_internal_call("SmolEngine.MeshComponent::SetMesh_EX", &CSharpAPi::SetMesh_CSharpAPI);
		mono_add_internal_call("SmolEngine.Mesh::GetChildsCount_EX", &CSharpAPi::MeshGetChildsCount_CSharpAPI);

		// Rigidbody
		mono_add_internal_call("SmolEngine.RigidBodyComponent::CreateRigidBody_EX", &CSharpAPi::RigidBodyCreate_CSharpAPI);
		mono_add_internal_call("SmolEngine.RigidBodyComponent::SetImpact_EX", &CSharpAPi::RigidBodySetImpact_CSharpAPI);
	}

	void MonoContext::ResolveClasses()
	{
		m_InternalClasses[InternalClassType::Actor] = mono_class_from_name(m_Image, "SmolEngine", "Actor");
		m_InternalClasses[InternalClassType::Prefab] = mono_class_from_name(m_Image, "SmolEngine", "Prefab");
		m_InternalClasses[InternalClassType::BehaviourPrimitive] = mono_class_from_name(m_Image, "SmolEngine", "BehaviourPrimitive");

		m_InternalClasses[InternalClassType::UnitTests] = mono_class_from_name(m_Image, "SmolEngine", "Tests");
	}

	void MonoContext::ResolveMeta()
	{
		MonoClass* b_class = m_InternalClasses[InternalClassType::BehaviourPrimitive];
		const char* b_class_name = mono_class_get_name(b_class);
		const char* b_class_name_space = mono_class_get_namespace(b_class);
		 
		const MonoTableInfo* table_info = mono_image_get_table_info(m_Image, MONO_TABLE_TYPEDEF);
		int rows = mono_table_info_get_rows(table_info);

		/* For each row, get some of its values */
		for (int i = 0; i < rows; i++)
		{
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(table_info, i, cols, MONO_TYPEDEF_SIZE);
			const char* name = mono_metadata_string_heap(m_Image, cols[MONO_TYPEDEF_NAME]);
			const char* name_space = mono_metadata_string_heap(m_Image, cols[MONO_TYPEDEF_NAMESPACE]);
			bool is_same = strcmp(name, b_class_name) == 0 && strcmp(name_space, b_class_name_space) == 0;

			if (is_same == false)
			{
				MonoClass* mono_class = mono_class_from_name(m_Image, name_space, name);
				if (!mono_class)
					continue;

				if (!mono_class_is_subclass_of(mono_class, b_class, false))
					continue;

				MonoContext::MetaData meta = {};

				meta.pClass = mono_class;
				meta.pOnBegin = (MonoMethod*)GetMethod("OnBegin()", name, mono_class);
				meta.pOnDestroy = (MonoMethod*)GetMethod("OnDestroy()", name, mono_class);
				meta.pOnUpdate = (MonoMethod*)GetMethod("OnUpdate()", name, mono_class);
				meta.pOnCollisionBegin = (MonoMethod*)GetMethod("OnCollisionBegin (uint,bool)", name, mono_class);
				meta.pOnCollisionEnd = (MonoMethod*)GetMethod("OnCollisionEnd (uint,bool)", name, mono_class);

				if (!meta.pOnBegin && !meta.pOnDestroy && !meta.pOnUpdate)
					continue;

				// Get supported public fileds
				{
					void* iter = nullptr;
					MonoClassField* field = nullptr;

					while ((field = mono_class_get_fields(mono_class, &iter)))
					{
						uint32_t flags = mono_field_get_flags(field) & MONO_FIELD_ATTR_FIELD_ACCESS_MASK;
						if (flags == MONO_FIELD_ATTR_PUBLIC)
						{
							if (strcmp(mono_type_full_name(mono_field_get_type(field)), "SmolEngine.Prefab") == 0)
							{
								std::string str = "";
								std::string name = std::string(mono_field_get_name(field)) + "##InternalPrefabFlag";
								meta.Fields.PushVariable<std::string>(&str, name);
							}

							if (strcmp(mono_type_full_name(mono_field_get_type(field)), "SmolEngine.Actor") == 0)
							{
								int32_t value = 0;
								std::string name = std::string(mono_field_get_name(field)) + "##InternalActorFlag";
								meta.Fields.PushVariable<int32_t>(&value, name);
							}

							if (mono_type_get_type(mono_field_get_type(field)) == MONO_TYPE_I4)
							{
								int32_t value = 0;
								const char* name = mono_field_get_name(field);
								meta.Fields.PushVariable<int32_t>(&value, name);
							}

							if (mono_type_get_type(mono_field_get_type(field)) == MONO_TYPE_R4)
							{
								float value = 0;
								const char* name = mono_field_get_name(field);
								meta.Fields.PushVariable<float>(&value, name);
							}

							if (mono_type_get_type(mono_field_get_type(field)) == MONO_TYPE_STRING)
							{
								std::string str = "";
								const char* name = mono_field_get_name(field);
								meta.Fields.PushVariable<std::string>(&str, name);
							}
						}
					}
				}


				m_MetaMap[name] = std::move(meta);
			}
		}
	}

}