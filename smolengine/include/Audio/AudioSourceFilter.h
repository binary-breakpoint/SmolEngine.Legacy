#pragma once
#include "Core/Core.h"

namespace cereal
{
	class access;
}

namespace SmolEngine
{
	enum class AudioFilterFlags : int
	{
		Echo         = 1,
		Lofi         = 2,
		BQResonant   = 4,
		Bassboost    = 8,
		Freeverb     = 16,
		WaveShaper   = 32,

		MaxEnum      = 64
	};

	inline AudioFilterFlags operator~ (AudioFilterFlags a) { return (AudioFilterFlags)~(int)a; }
	inline AudioFilterFlags operator| (AudioFilterFlags a, AudioFilterFlags b) { return (AudioFilterFlags)((int)a | (int)b); }
	inline AudioFilterFlags operator& (AudioFilterFlags a, AudioFilterFlags b) { return (AudioFilterFlags)((int)a & (int)b); }
	inline AudioFilterFlags operator^ (AudioFilterFlags a, AudioFilterFlags b) { return (AudioFilterFlags)((int)a ^ (int)b); }
	inline AudioFilterFlags& operator|= (AudioFilterFlags& a, AudioFilterFlags b) { return (AudioFilterFlags&)((int&)a |= (int)b); }
	inline AudioFilterFlags& operator&= (AudioFilterFlags& a, AudioFilterFlags b) { return (AudioFilterFlags&)((int&)a &= (int)b); }
	inline AudioFilterFlags& operator^= (AudioFilterFlags& a, AudioFilterFlags b) { return (AudioFilterFlags&)((int&)a ^= (int)b); }

	struct WaveShaperFilterCreateInfo
	{
		float Amount = 0.1f;
	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(Amount);
		}
	};

	struct FreeverbFilterCreateInfo
	{
		float Mode = 0.0f;
		float RoomSize = 0.5f;
		float Damp = 0.5f;
		float Width = 1.0f;
	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(Mode, RoomSize, Damp, Width);
		}
	};

	struct BassboostFilterCreateInfo
	{
		float Boost = 0.1f;
	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(Boost);
		}
	};

	struct BQResonantFilterCreateInfo
	{
		int   Type = 0;
		float Frequency = 500.0f;
		float Resonance = 2.0f;

	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(Type, Frequency, Resonance);
		}
	};

	struct LofiFilterCreateInfo
	{
		float SampleRate = 8000.0f;
		float Bitdepth = 5.0f;
	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(SampleRate, Bitdepth);
		}
	};

	struct EchoFilterCreateInfo
	{
		float Delay = 0.0f;
		float Decay = 0.7f;
		float Filter = 0.0f;

	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(Delay, Decay, Filter);
		}
	};

	struct AudioSourceFilter
	{
		AudioFilterFlags             Flags = AudioFilterFlags::MaxEnum;

		EchoFilterCreateInfo         EchoCI;
		LofiFilterCreateInfo         LofiCI;
		BQResonantFilterCreateInfo   BQResonantCI;
		BassboostFilterCreateInfo    BassboostCI;
		FreeverbFilterCreateInfo     FreeverbCI;
		WaveShaperFilterCreateInfo   WaveShaperCI;

	private:
		friend class cereal::access;

		template<typename Archive>
		void serialize(Archive& archive)
		{
			archive(Flags, EchoCI, LofiCI, BQResonantCI, BassboostCI, FreeverbCI, WaveShaperCI);
		}
	};
}