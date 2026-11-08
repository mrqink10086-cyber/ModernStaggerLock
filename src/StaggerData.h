#pragma once

#include <array>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

namespace ModernStaggerLock
{
	struct SpecialStaggerData
	{
		std::uint32_t GetNameHash() const noexcept { return animID_; }
		bool IsIgnoreStaggerLevel() const noexcept { return ignoreStaggerLevel_; }
		friend void from_json(const nlohmann::json& a_json, SpecialStaggerData& a_data);

	private:
		std::uint32_t animID_ = 0;
		bool ignoreStaggerLevel_ = false;
	};

	void from_json(const nlohmann::json& a_json, SpecialStaggerData& a_data);
	bool ParseStaggerData(std::string_view a_text, SpecialStaggerData& a_data);

	class StaggerPayloadCache
	{
	public:
		static constexpr std::size_t kCapacity = 32;
		static constexpr std::size_t kMaxKeyLength = 4096;

		bool Find(std::string_view a_text, SpecialStaggerData& a_data);

	private:
		struct Entry
		{
			std::string text;
			SpecialStaggerData data;
			bool occupied = false;
			bool valid = false;
		};

		std::array<Entry, kCapacity> entries_;
	};
}
