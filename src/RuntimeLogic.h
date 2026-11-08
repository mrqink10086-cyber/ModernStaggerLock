#pragma once

#include "Utils.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ModernStaggerLock::Logic
{
	constexpr std::int32_t HashName(std::string_view a_name) noexcept
	{
		std::uint32_t hash = 5381;
		for (char ch : a_name) {
			hash = hash * 33 + static_cast<std::uint32_t>(charToLower(ch));
		}
		return std::bit_cast<std::int32_t>(hash);
	}

	constexpr std::int32_t NextCounter(std::int32_t a_value) noexcept
	{
		return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(a_value) + std::uint32_t{ 1 });
	}

	constexpr int CompareInsensitive(std::string_view a_left, std::string_view a_right) noexcept
	{
		const auto size = std::min(a_left.size(), a_right.size());
		for (std::size_t i = 0; i < size; ++i) {
			const auto left = static_cast<unsigned char>(charToLower(a_left[i]));
			const auto right = static_cast<unsigned char>(charToLower(a_right[i]));
			if (left != right) {
				return left < right ? -1 : 1;
			}
		}
		return a_left.size() == a_right.size() ? 0 : (a_left.size() < a_right.size() ? -1 : 1);
	}

	constexpr bool EqualInsensitive(std::string_view a_left, std::string_view a_right) noexcept
	{
		return a_left.size() == a_right.size() && CompareInsensitive(a_left, a_right) == 0;
	}

	enum class StaggerEvent : std::int8_t
	{
		kOther = -2,
		kStop = -1,
		kSmall = 0,
		kMedium,
		kLarge,
		kLargest,
		kSpecial
	};

	constexpr StaggerEvent ClassifyEvent(std::string_view a_event) noexcept
	{
		switch (a_event.size()) {
		case sizeof("staggerStop") - 1:
			if (EqualInsensitive(a_event, "staggerStop"))
				return StaggerEvent::kStop;
			break;
		case sizeof("MSL_SmallStaggerStart") - 1:
			if (EqualInsensitive(a_event, "MSL_SmallStaggerStart"))
				return StaggerEvent::kSmall;
			if (EqualInsensitive(a_event, "MSL_LargeStaggerStart"))
				return StaggerEvent::kLarge;
			break;
		case sizeof("MSL_MediumStaggerStart") - 1:
			if (EqualInsensitive(a_event, "MSL_MediumStaggerStart"))
				return StaggerEvent::kMedium;
			break;
		case sizeof("MSL_LargestStaggerStart") - 1:
			if (EqualInsensitive(a_event, "MSL_LargestStaggerStart"))
				return StaggerEvent::kLargest;
			if (EqualInsensitive(a_event, "MSL_SpecialStaggerStart"))
				return StaggerEvent::kSpecial;
			break;
		}
		return StaggerEvent::kOther;
	}

	inline std::int32_t StaggerLevel(float a_magnitude, const std::array<float, 4>& a_thresholds) noexcept
	{
		for (std::int32_t i = 3; i >= 0; --i) {
			if (a_magnitude >= a_thresholds[i])
				return i + 1;
		}
		return 1;
	}

	class RecoveryEvents
	{
	public:
		void Assign(std::vector<std::string> a_events)
		{
			std::sort(a_events.begin(), a_events.end(), Less);
			a_events.erase(std::unique(a_events.begin(), a_events.end(), EqualInsensitive), a_events.end());
			events_ = std::move(a_events);
		}

		bool Contains(std::string_view a_event) const noexcept
		{
			if (events_.size() <= 8) {
				return std::any_of(events_.begin(), events_.end(), [&](const auto& name) { return EqualInsensitive(name, a_event); });
			}
			const auto it = std::lower_bound(events_.begin(), events_.end(), a_event, Less);
			return it != events_.end() && EqualInsensitive(*it, a_event);
		}

		const std::vector<std::string>& Values() const noexcept { return events_; }

	private:
		static bool Less(std::string_view a_left, std::string_view a_right) noexcept
		{
			return CompareInsensitive(a_left, a_right) < 0;
		}

		std::vector<std::string> events_;
	};
}
