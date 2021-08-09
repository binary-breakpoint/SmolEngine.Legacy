#include "stdafx.h"
#include "Animation/AnimationClip.h"
#include "Import/glTFImporter.h"
#include "GraphicsContext.h"

#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/skeleton_utils.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/maths/math_ex.h>
#include <ozz/base/containers/vector.h>
#include <ozz-animation/include/ozz/animation/runtime/local_to_model_job.h>

#ifdef FROSTIUM_SMOLENGINE_IMPL
namespace SmolEngine
#else
namespace Frostium
#endif
{
	class PlaybackController {
	public:
		PlaybackController()
			: time_ratio_(0.f),
			previous_time_ratio_(0.f),
			playback_speed_(1.f),
			play_(true),
			loop_(true) {}

		void set_time_ratio(float _ratio)
		{
			previous_time_ratio_ = time_ratio_;
			if (loop_) 
			{
				// Wraps in the unit interval [0:1], even for negative values (the reason
				// for using floorf).
				time_ratio_ = _ratio - floorf(_ratio);
			}
			else 
			{
				// Clamps in the unit interval [0:1].
				time_ratio_ = ozz::math::Clamp(0.f, _ratio, 1.f);
			}
		}

		// Updates animation time if in "play" state, according to playback speed and
		// given frame time _dt.
		// Returns true if animation has looped during update
		void Update(const ozz::animation::Animation& _animation, float _dt)
		{
			float new_time = time_ratio_;

			if (play_) {
				new_time = time_ratio_ + _dt * playback_speed_ / _animation.duration();
			}

			// Must be called even if time doesn't change, in order to update previous
			// frame time ratio. Uses set_time_ratio function in order to update
			// previous_time_ an wrap time value in the unit interval (depending on loop
			// mode).
			set_time_ratio(new_time);
		}

		// Resets all parameters to their default value.
		void Reset()
		{
			previous_time_ratio_ = time_ratio_ = 0.f;
			playback_speed_ = 1.f;
			play_ = true;
		}

		// Current animation time ratio, in the unit interval [0,1], where 0 is the
		// beginning of the animation, 1 is the end.
		float time_ratio_;

		// Time ratio of the previous update.
		float previous_time_ratio_;

		// Playback speed, can be negative in order to play the animation backward.
		float playback_speed_;

		// Animation play mode state: play/pause.
		bool play_;

		// Animation loop mode.
		bool loop_;
	};

	struct AnimationClipStorage
	{
		PlaybackController                    m_Controller;
		ImportedAnimationGlTF                 m_Import;
		ozz::animation::Skeleton              m_Skeleton;
		ozz::animation::Animation             m_Clip;
		ozz::animation::SamplingCache         m_Cache;
		ozz::vector<ozz::math::SoaTransform>  m_Locals;
		ozz::vector<ozz::math::Float4x4>      m_Models;
	};

	bool LoadSkeleton(const char* _filename, ozz::animation::Skeleton* _skeleton) 
	{
		ozz::io::File file(_filename, "rb");
		if (!file.opened()) 
		{
			NATIVE_ERROR("Failed to open skeleton file: {}", _filename);
			return false;
		}

		ozz::io::IArchive archive(&file);
		if (!archive.TestTag<ozz::animation::Skeleton>()) 
		{
			NATIVE_ERROR("Failed to load skeleton instance from file: {}", _filename);
			return false;
		}

		archive >> *_skeleton;
		return true;
	}

	bool LoadMeshData(const std::string& filePath, ImportedAnimationGlTF* anim)
	{
		return glTFImporter::ImportAnimation(filePath, anim);
	}

	bool LoadAnimation(const char* _filename, ozz::animation::Animation* _animation) 
	{
		ozz::io::File file(_filename, "rb");
		if (!file.opened()) 
		{
			NATIVE_ERROR("Failed to open animation file: {}", _filename);
			return false;
		}

		ozz::io::IArchive archive(&file);
		if (!archive.TestTag<ozz::animation::Animation>()) 
		{
			NATIVE_ERROR("Failed to load animation instance from file: {}", _filename);
			return false;
		}

		archive >> *_animation;
		return true;
	}

	bool AnimationClip::Create(const AnimationClipCreateInfo& createInfo)
	{
		m_Storage = std::make_shared<AnimationClipStorage>();

		if (LoadAnimation(createInfo.AnimationPath.c_str(), &m_Storage->m_Clip) &&
			LoadSkeleton(createInfo.SkeletonPath.c_str(), &m_Storage->m_Skeleton) &&
			LoadMeshData(createInfo.ModelPath.c_str(), &m_Storage->m_Import))
		{
			const int numSoaJoints = m_Storage->m_Skeleton.num_soa_joints();
			const int numTracks = m_Storage->m_Clip.num_tracks();
			const int numJoints = m_Storage->m_Skeleton.num_joints();

			if (numTracks == numJoints)
			{
				m_Storage->m_Models.resize(numJoints);
				m_Storage->m_Cache.Resize(numJoints);
				m_Storage->m_Locals.resize(numSoaJoints);

				return true;
			}
		}
		
		return false;
	}

	bool AnimationClip::IsGood() const
	{
		return m_Storage != nullptr;
	}

	void AnimationClip::SetLoop(bool value)
	{
		m_Storage->m_Controller.loop_ = value;
	}

	void AnimationClip::SetSpeed(float value)
	{
		m_Storage->m_Controller.playback_speed_ = value;
	}

	bool AnimationClip::IsLooping() const
	{
		return m_Storage->m_Controller.loop_;
	}

	float AnimationClip::GetSpeed() const
	{
		return m_Storage->m_Controller.playback_speed_;
	}

	std::vector<glm::mat4>& AnimationClip::GetJoints() const
	{
		return m_Storage->m_Import.Joints;
	}

	bool AnimationClip::Update()
	{
		const float time = GraphicsContext::GetSingleton()->GetDeltaTime();
		const auto& storage = m_Storage;

		if (storage)
		{
			storage->m_Controller.Update(storage->m_Clip, time);

			// Samples optimized animation at t = animation_time_.
			ozz::animation::SamplingJob sampling_job;
			sampling_job.animation = &storage->m_Clip;
			sampling_job.cache = &storage->m_Cache;
			sampling_job.ratio = storage->m_Controller.time_ratio_;
			sampling_job.output = make_span(storage->m_Locals);
			if (!sampling_job.Run()) 
			{
				return false;
			}

			// Converts from local space to model space matrices.

			ozz::animation::LocalToModelJob ltm_job;
			ltm_job.skeleton = &storage->m_Skeleton;
			ltm_job.input = make_span(storage->m_Locals);
			ltm_job.output = make_span(storage->m_Models);
			if (!ltm_job.Run()) 
			{
				return false;
			}

			for (uint32_t i = 0; i < static_cast<uint32_t>(m_Storage->m_Models.size()); ++i)
			{
				glm::mat4& mBone = m_Storage->m_Import.Joints[i];
				ozz::math::StorePtr(m_Storage->m_Models[i].cols[0],  glm::value_ptr(mBone[0]));
				ozz::math::StorePtr(m_Storage->m_Models[i].cols[1],  glm::value_ptr(mBone[1]));
				ozz::math::StorePtr(m_Storage->m_Models[i].cols[2],  glm::value_ptr(mBone[2]));
				ozz::math::StorePtr(m_Storage->m_Models[i].cols[3],  glm::value_ptr(mBone[3]));

				mBone *= m_Storage->m_Import.Skins[0].InverseBindMatrices[i];
			}

			return true;
		}

		return false;
	}
}