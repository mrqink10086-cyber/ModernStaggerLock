#include "LoadGame.h"
#include "Runtime.h"

DLLEXPORT constinit auto SKSEPlugin_Version = []() noexcept {
	SKSE::PluginVersionData data{};
	data.PluginVersion(Plugin::Version);
	data.PluginName(Plugin::NAME);
	data.AuthorName(Plugin::AUTHOR);
	data.UsesAddressLibrary();
	data.UsesUpdatedStructs();
	data.CompatibleVersions({ SKSE::RUNTIME_SSE_1_6_1170 });
	data.MinimumRequiredXSEVersion({ 2, 2, 6, 0 });
	return data;
}();

DLLEXPORT bool SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->name = SKSEPlugin_Version.pluginName;
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->version = SKSEPlugin_Version.pluginVersion;
	return !a_skse->IsEditor() && ModernStaggerLock::Runtime::IsSupported(a_skse->RuntimeVersion());
}

DLLEXPORT bool SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	if (a_skse->IsEditor() || !ModernStaggerLock::Runtime::IsSupported(a_skse->RuntimeVersion())) {
		return false;
	}
	const REL::Version minimumSKSE = REL::Module::IsAE() ? REL::Version{ 2, 2, 6, 0 } : REL::Version{ 2, 0, 20, 0 };
	const auto skseVersion = REL::Version::unpack(a_skse->SKSEVersion());
	if (skseVersion < minimumSKSE) {
		return false;
	}
	SKSE::Init(a_skse);
	auto logPath = SKSE::log::log_directory();
	if (!logPath) {
		return false;
	}
	*logPath /= "ModernStaggerLock.log";
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logPath->string(), true);
	spdlog::set_default_logger(std::make_shared<spdlog::logger>("ModernStaggerLock", std::move(sink)));
	spdlog::set_level(spdlog::level::info);
	spdlog::flush_on(spdlog::level::info);
	INFO("{} v{} loaded on Skyrim {} (SKSE {})", Plugin::NAME, Plugin::Version.string(),
		a_skse->RuntimeVersion().string(), skseVersion.string());
	const auto messaging = SKSE::GetMessagingInterface();
	return messaging && messaging->RegisterListener(ModernStaggerLock::EventCallback);
}
