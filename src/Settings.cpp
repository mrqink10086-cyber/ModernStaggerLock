#include "Settings.h"
#include "SpecialStaggerHandler.h"
#include "ConfigInput.h"

namespace ModernStaggerLock
{
	using json = nlohmann::json;

	static auto MainConfig = COMPILE_PROXY("ModernStaggerLock.ini"sv);

	MSLSettings::MSLSettings()
	{
		MainConfig.Bind<0.0, 1.0>(SmallStaggerMagnitude, 0.0);
		MainConfig.Bind<0.0, 1.0>(MediumStaggerMagnitude, 0.5);
		MainConfig.Bind<0.0, 1.0>(LargeStaggerMagnitude, 0.75);
		MainConfig.Bind<0.0, 1.0>(LargestStaggerMagnitude, 1.0);
		MainConfig.Bind(DisableJumpWhenStagger, true);

		MainConfig.Load();
		staggerThresholds_ = { static_cast<float>(*SmallStaggerMagnitude), static_cast<float>(*MediumStaggerMagnitude),
			static_cast<float>(*LargeStaggerMagnitude), static_cast<float>(*LargestStaggerMagnitude) };
		disableJumpWhenStagger_ = *DisableJumpWhenStagger;

		PrintSettingValue(SmallStaggerMagnitude);
		PrintSettingValue(MediumStaggerMagnitude);
		PrintSettingValue(LargeStaggerMagnitude);
		PrintSettingValue(LargestStaggerMagnitude);

		auto MSLFilesName = DKUtil::Config::GetAllFiles<false>("Data\\SKSE\\Plugins\\ModernStaggerLock"sv, ".json"sv, {}, "_MSL"sv);
		auto SpecialStaggerHandler = SpecialStaggerHandler::GetSingleton();
		std::vector<std::string> recoveryEvents;
		std::size_t remainingBytes = kMaxConfigTotalBytes;
		for (const auto& fileName : MSLFilesName) {
			if (remainingBytes == 0) {
				ERROR("MSL JSON configuration budget exhausted ({} bytes); remaining files skipped", kMaxConfigTotalBytes);
				break;
			}
			try {
				std::ifstream input(fileName, std::ios::binary);
				std::string text;
				if (!ReadConfigFile(input, text, remainingBytes)) {
					ERROR("Cannot read MSL JSON file or input limit exceeded: {}", fileName);
					continue;
				}
				json js = json::parse(text);
				for (auto& element : js) {
					if (element.contains("QuickRecoveryEvents")) {
						auto eventLists = element.at("QuickRecoveryEvents").get<std::vector<std::string>>();
						recoveryEvents.insert(recoveryEvents.end(), std::make_move_iterator(eventLists.begin()), std::make_move_iterator(eventLists.end()));
					} else if (element.contains("SpecialStaggerTransData")) {
						auto transData = element.at("SpecialStaggerTransData").get<SpecialStaggerTransactionData>();
						auto animID = HashSpecialStaggerID(transData.animName.data(), transData.animName.size());
						SpecialStaggerHandler->transDataMap.emplace(animID, std::move(transData));
					}
				}
			} catch (const json::exception& ex) {
				ERROR("Caught expection when parsing file \"{}\":\n {}", fileName, ex.what());
			} catch (const std::bad_alloc&) {
				ERROR("Insufficient memory loading MSL JSON configuration; remaining files skipped");
				break;
			}
		}

		for (const auto& eventName : recoveryEvents) {
			INFO("Get a Quick Recovery Event Name: \"{}\"", eventName);
		}

		quickRecoveryEvents.Assign(std::move(recoveryEvents));

		for (const auto& transData : SpecialStaggerHandler->transDataMap) {
			INFO("Get a Special Stagger Anim Name: \"{}\"", transData.second.animName);
		}
	}

}
