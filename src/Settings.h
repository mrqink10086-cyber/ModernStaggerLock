#pragma once
#include "DKUtil/Config.hpp"
#include "DKUtil/Utility.hpp"
#include "PrecisionAPI.h"
#include "RuntimeLogic.h"
#include <atomic>

namespace ModernStaggerLock
{
	using namespace DKUtil::Alias;

	class MSLSettings : public DKUtil::model::Singleton<MSLSettings>
	{
		friend DKUtil::model::Singleton<MSLSettings>;

	private:
		MSLSettings();

		template <class T>
		static void PrintSettingValue(const T& a_setting)
		{
			INFO("Setting:\"{}\" is {}", a_setting.get_key(), a_setting.get_data());
		}

	public:
		Boolean DisableJumpWhenStagger{ "DisableJumpWhenStagger", "Control" };

		Double SmallStaggerMagnitude{ "SmallStaggerMagnitude", "StaggerLevel" };
		Double MediumStaggerMagnitude{ "MediumStaggerMagnitude", "StaggerLevel" };
		Double LargeStaggerMagnitude{ "LargeStaggerMagnitude", "StaggerLevel" };
		Double LargestStaggerMagnitude{ "LargestStaggerMagnitude", "StaggerLevel" };

		enum StaggerType : std::int32_t
		{
			kSmall = 0,
			kMedium,
			kLarge,
			kLargest,
			kTotal
		};

		static inline Logic::RecoveryEvents quickRecoveryEvents;

		static inline PRECISION_API::IVPrecision5* ersh_Precision = nullptr;

		static bool IsReady() noexcept { return ready_.load(std::memory_order_acquire); }
		static void PublishReady() noexcept { ready_.store(true, std::memory_order_release); }

		std::int32_t GetStaggerLevel(float a_magnitude) const noexcept { return Logic::StaggerLevel(a_magnitude, staggerThresholds_); }
		bool IsJumpDisabled() const noexcept { return disableJumpWhenStagger_; }

	private:
		static inline std::atomic<bool> ready_{ false };
		std::array<float, 4> staggerThresholds_{};
		bool disableJumpWhenStagger_ = true;
	};
}
