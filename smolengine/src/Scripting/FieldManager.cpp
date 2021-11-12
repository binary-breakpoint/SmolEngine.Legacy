#include "stdafx.h"
#include "Scripting/FieldManager.h"

namespace SmolEngine
{
	FieldManager::FieldManager()
		:
		m_Floats(std::make_shared< std::unordered_map<std::string, float>>()),
		m_Ints(std::make_shared< std::unordered_map<std::string, int32_t>>()),
		m_Strings(std::make_shared< std::unordered_map<std::string, std::string>>())
	{

	}

	FieldManager& FieldManager::operator=(const FieldManager& another)
	{
		*m_Floats = *another.m_Floats;
		*m_Ints = *another.m_Ints;
		*m_Strings = *another.m_Strings;

		m_Hash = another.m_Hash;
		return *this;
	}

	void FieldManager::FieldCopyOrReplace(FieldManager* another)
	{
		if (!AreEqual(*another))
		{
			for (auto& [name, value] : *m_Ints)
			{
				auto& it = another->m_Ints->find(name);
				if (it != another->m_Ints->end())
				{
					auto& map = *another->m_Ints;
					map[name] = value;
				}
			}

			for (auto& [name, value] : *m_Floats)
			{
				auto& it = another->m_Floats->find(name);
				if (it != another->m_Floats->end())
				{
					auto& map = *another->m_Floats;
					map[name] = value;
				}
			}

			for (auto& [name, value] : *m_Strings)
			{
				auto& it = another->m_Strings->find(name);
				if (it != another->m_Strings->end())
				{
					auto& map = *another->m_Strings;
					map[name] = value;
				}
			}

			m_Hash = another->m_Hash;
			*m_Floats = *another->m_Floats;
			*m_Ints = *another->m_Ints;
			*m_Strings = *another->m_Strings;
		}

		SubmitComplete();
	}

	bool FieldManager::AreEqual(const FieldManager& another)
	{
		size_t my_hash = GetHash();
		size_t another_hash = another.GetHash();

		return my_hash == another_hash;
	}

	void FieldManager::SubmitComplete()
	{
		m_RefData.resize(m_Ints->size() + m_Floats->size() + m_Strings->size());

		uint32_t index = 0;
		for (auto& [name, value] : *m_Ints)
		{
			m_RefData[index] = *GetVarriable<int32_t>(name).get();
			index++;
		}

		for (auto& [name, value] : *m_Floats)
		{
			m_RefData[index] = *GetVarriable<float>(name).get();
			index++;
		}

		for (auto& [name, value] : *m_Strings)
		{
			m_RefData[index] = *GetVarriable<std::string>(name).get();
			index++;
		}
	}

	size_t FieldManager::GetHash() const
	{
		return m_Hash;
	}

	uint32_t FieldManager::GetCount() const
	{
		return static_cast<uint32_t>(m_Ints->size() + m_Floats->size() + m_Strings->size());
	}

	const std::vector<FieldView>& FieldManager::GetFields() const
	{
		return m_RefData;
	}
}