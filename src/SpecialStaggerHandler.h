#pragma once
#include "DKUtil/Config.hpp"
#include "DKUtil/Utility.hpp"
#include "PrecisionAPI.h"
#include "nlohmann/json.hpp"
#include "StaggerData.h"

namespace ModernStaggerLock
{
	using namespace DKUtil::Alias;
	using json = nlohmann::json;

	int32_t HashSpecialStaggerID(const char* data, size_t const size) noexcept;

	class SpecialStaggerHandler : public DKUtil::model::Singleton<SpecialStaggerHandler>
	{
		friend DKUtil::model::Singleton<SpecialStaggerHandler>;

	public:
		struct SpecialStaggerTransactionData
		{
			std::string animName = "";
			bool Unrestricted = false;
			std::vector<std::int32_t> transAnimList = {};
		};

		using SpecialStaggerTransMap = std::unordered_map<std::int32_t, SpecialStaggerTransactionData>;

		SpecialStaggerHandler() = default;

		bool FindSpecialStaggerData(RE::Actor* a_actor, SpecialStaggerData& a_data);
		void ProcessSpeicalStagger(RE::Actor* a_actor, std::int32_t a_StaggerLevel);

		SpecialStaggerTransMap transDataMap;

	private:
		bool CanTriggerTransaction(const std::int32_t a_curAnimID, const std::int32_t a_NextAnimID) const;
	};
	using SpecialStaggerTransactionData = SpecialStaggerHandler::SpecialStaggerTransactionData;

	void from_json(const json& j, SpecialStaggerTransactionData& a_data);
}
