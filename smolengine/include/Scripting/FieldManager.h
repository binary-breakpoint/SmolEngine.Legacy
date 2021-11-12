#pragma once
#include "Core/Core.h"

#include <unordered_map>
#include <string>

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	enum class FieldDataFlags
	{
		Int32,
		Float,
		String,
		Prefab,
		Actor,

		MaxEnum
	};

	struct FieldView
	{
		void*          ptr = nullptr;
		std::string    name = "";
		std::string    description = "";
		FieldDataFlags type = FieldDataFlags::MaxEnum;
	};

	class FieldManager
	{
	public:
		FieldManager();

		template<typename T>
		bool PushVariable(void* ptr, const std::string& name)
		{
			std::hash<std::string> hasher{};

			if (std::is_same<int32_t, T>::value)
			{
				if (m_Ints->find(name) == m_Ints->end())
				{
					m_Hash += hasher(name);
					auto& map = *m_Ints;
					map[name] = *(int32_t*)ptr;
					return true;
				}
			}

			if (std::is_same<float, T>::value)
			{
				if (m_Floats->find(name) == m_Floats->end())
				{
					m_Hash += hasher(name);
					auto& map = *m_Floats;
					map[name] = *(float*)ptr;
					return true;
				}
			}

			if (std::is_same<std::string, T>::value)
			{
				if (m_Strings->find(name) == m_Strings->end())
				{
					m_Hash += hasher(name);
					auto& map = *m_Strings;
					map[name] = *(std::string*)ptr;
					return true;
				}
			}

			return false;
		}

		template<typename T>
		Ref<FieldView> GetVarriable(const std::string& name)
		{
			if (std::is_same<int32_t, T>::value)
			{
				auto& it = m_Ints->find(name);
				if (it != m_Ints->end())
				{
					auto pos = it->first.find("##InternalActorFlag");

					Ref<FieldView> view = std::make_shared<FieldView>();
					view->ptr = &it->second;
					view->type = pos != std::string::npos ? FieldDataFlags::Actor : FieldDataFlags::Int32;
					view->name = pos != std::string::npos ? name.substr(0, pos) : name;
					return view;
				}
			}

			if (std::is_same<float, T>::value)
			{
				auto& it = m_Floats->find(name);
				if (it != m_Floats->end())
				{
					Ref<FieldView> view = std::make_shared<FieldView>();
					view->ptr = &it->second;
					view->type = FieldDataFlags::Float;
					view->name = name;
					return view;
				}
			}

			if (std::is_same<std::string, T>::value)
			{
				auto& it = m_Strings->find(name);
				if (it != m_Strings->end())
				{
					auto pos = it->first.find("##InternalPrefabFlag");

					Ref<FieldView> view = std::make_shared<FieldView>();
					view->ptr = &it->second;
					view->type = pos != std::string::npos ? FieldDataFlags::Prefab : FieldDataFlags::String;
					view->name = pos != std::string::npos ? name.substr(0, pos) : name;
					return view;
				}
			}

			return nullptr;
		}

		FieldManager& operator= (const FieldManager& another);

		void                                 FieldCopyOrReplace(FieldManager* another);
		bool                                 AreEqual(const FieldManager& another);
		void                                 SubmitComplete();
		size_t                               GetHash() const;
		uint32_t                             GetCount() const;
		const std::vector<FieldView>&        GetFields() const;

	private:

		size_t                                              m_Hash = 0;
		Ref<std::unordered_map<std::string, int32_t>>       m_Ints = nullptr;
		Ref<std::unordered_map<std::string, float>>         m_Floats = nullptr;
		Ref<std::unordered_map<std::string, std::string>>   m_Strings = nullptr;
	    std::vector<FieldView>                              m_RefData;

		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(m_Ints, m_Floats, m_Strings, m_Hash);
		}
	};
}