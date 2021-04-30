#pragma once

#include <cfloat>

namespace Frostium
{
	struct Animation
	{
		// Setters
		void SetSpeed(float speed) { Speed = speed; }
		void SetStartTime(float start) { Start = start; }
		void SetEndTime(float end) { End = end; }
		void SetLoop(bool loop) { Loop = loop; }

		// Getters
		float GetStartTime() const { return Start; }
		float GetEndTime() const { return End; }
		float GetSpeed() const { return Speed; }
		bool IsLooping() const { return Loop; }

	private:

		float Start = FLT_MAX;
		float End = FLT_MIN;
		float Speed = 1.0f;
		bool  Loop = true;

		friend class Mesh;
	};
}