#include "StaggerData.h"
#include "RuntimeLogic.h"

namespace ModernStaggerLock
{
	void from_json(const nlohmann::json& a_json, SpecialStaggerData& a_data)
	{
		const auto& name = a_json.at("SpecialStaggerName").get_ref<const std::string&>();
		a_data.animID_ = name.empty() ? 0 : static_cast<std::uint32_t>(Logic::HashName(name));
		a_data.ignoreStaggerLevel_ = a_json.value("IgnoreStaggerLevel", false);
	}

	bool ParseStaggerData(std::string_view a_text, SpecialStaggerData& a_data)
	{
		try {
			a_data = nlohmann::json::parse(a_text).get<SpecialStaggerData>();
			return true;
		} catch (const nlohmann::json::exception&) {
			return false;
		}
	}

	bool StaggerPayloadCache::Find(std::string_view a_text, SpecialStaggerData& a_data)
	{
		if (a_text.size() > kMaxKeyLength) {
			return ParseStaggerData(a_text, a_data);
		}

		std::uint64_t hash = 14695981039346656037ull;
		for (unsigned char ch : a_text) {
			hash = (hash ^ ch) * 1099511628211ull;
		}
		auto& entry = entries_[hash % kCapacity];
		if (!entry.occupied || entry.text != a_text) {
			entry.text.assign(a_text);
			entry.valid = ParseStaggerData(a_text, entry.data);
			entry.occupied = true;
		}
		if (entry.valid) {
			a_data = entry.data;
		}
		return entry.valid;
	}
}
