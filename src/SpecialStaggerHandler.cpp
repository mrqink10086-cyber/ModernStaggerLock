#include "SpecialStaggerHandler.h"
#include "Settings.h"
#include "Utils.h"
#include "RuntimeLogic.h"
#include "AnimationNames.h"

namespace ModernStaggerLock
{
	int32_t HashSpecialStaggerID(const char* data, size_t const size) noexcept
	{
		return Logic::HashName({ data, size });
	}

	bool SpecialStaggerHandler::FindSpecialStaggerData(RE::Actor* a_actor, SpecialStaggerData& a_data)
	{
		if (!a_actor) {
			return false;
		}

		if (!MSLSettings::ersh_Precision) {
			return false;
		}

		auto extraData = MSLSettings::ersh_Precision->GetCachedExtraData(static_cast<RE::TESObjectREFR*>(a_actor)->GetHandle(), Plugin::NAME.data());
		if (extraData) {
			thread_local StaggerPayloadCache cache;
			return cache.Find(extraData, a_data);
		}

		return false;
	}

	void SpecialStaggerHandler::ProcessSpeicalStagger(RE::Actor* a_actor, std::int32_t a_StaggerLevel)
	{
		if (a_actor) {
			SpecialStaggerData data;

			if (!FindSpecialStaggerData(a_actor, data) || (!data.IsIgnoreStaggerLevel() && a_StaggerLevel < 4)) {
				return;
			}

			std::int32_t nextSpecialStaggerId = data.GetNameHash();
			if (nextSpecialStaggerId == 0) {
				return;
			}

			std::int32_t curSpecialStaggerId = 0;
			const auto& names = AnimationNames::Get();
			if (a_actor->GetGraphVariableInt(names.specialId, curSpecialStaggerId) && CanTriggerTransaction(curSpecialStaggerId, nextSpecialStaggerId)) {
				a_actor->SetGraphVariableInt(names.mslLevel, 5);
				a_actor->SetGraphVariableInt(names.specialId, nextSpecialStaggerId);
			}
		}
	}

	bool SpecialStaggerHandler::CanTriggerTransaction(const std::int32_t a_curAnimID, const std::int32_t a_NextAnimID) const
	{
		if (a_curAnimID == 0 || a_curAnimID == a_NextAnimID) {
			DEBUG("Return True");
			return true;
		}

		auto it = transDataMap.find(a_curAnimID);
		if (it != transDataMap.end()) {
			const auto& transData = it->second;
			if (transData.Unrestricted) {
				DEBUG("Return Unrestricted True");
				return true;
			} else {
				DEBUG("Return List");
				return std::binary_search(transData.transAnimList.begin(), transData.transAnimList.end(), a_NextAnimID);
			}
		}

		return false;
	}

	void from_json(const json& j, SpecialStaggerTransactionData& a_data)
	{
		j.at("AnimName").get_to(a_data.animName);
		j.at("Unrestricted").get_to(a_data.Unrestricted);
		if (!a_data.Unrestricted) {
			std::vector<std::string> animNames;
			j.at("TransAnimList").get_to(animNames);
			a_data.transAnimList.reserve(animNames.size());
			for (auto const& animName : animNames) {
				a_data.transAnimList.push_back(HashSpecialStaggerID(animName.data(), animName.size()));
			}
			std::sort(a_data.transAnimList.begin(), a_data.transAnimList.end());
			a_data.transAnimList.erase(std::unique(a_data.transAnimList.begin(), a_data.transAnimList.end()), a_data.transAnimList.end());
		}
	}

}
