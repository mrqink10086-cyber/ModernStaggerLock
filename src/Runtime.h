#pragma once

namespace ModernStaggerLock::Runtime
{
	inline bool IsSupported(REL::Version a_version)
	{
		return (REL::Module::IsSE() && a_version == SKSE::RUNTIME_SSE_1_5_97) ||
		       (REL::Module::IsAE() && a_version == SKSE::RUNTIME_SSE_1_6_1170);
	}

	inline std::uintptr_t NPCLandCall()
	{
		return REL::Relocation<std::uintptr_t>{ REL::RelocationID(36507, 37507), REL::Relocate(0x89D, 0x9BB) }.address();
	}

	inline std::uintptr_t PlayerLandCall()
	{
		return REL::Relocation<std::uintptr_t>{ REL::RelocationID(41271, 42350), REL::Relocate(0x241, 0x179) }.address();
	}

	inline std::uintptr_t StaggerCheck()
	{
		return REL::Relocation<std::uintptr_t>{ REL::RelocationID(36700, 37710), REL::Relocate(0x55, 0x54) }.address();
	}

	inline bool ValidatePatchSites()
	{
		constexpr std::array<std::uint8_t, 6> seStagger{ 0x0F, 0x85, 0xD6, 0x01, 0x00, 0x00 };
		constexpr std::array<std::uint8_t, 6> aeStagger{ 0x0F, 0x85, 0xD1, 0x01, 0x00, 0x00 };
		const auto& expected = REL::Module::IsAE() ? aeStagger : seStagger;
		if (std::memcmp(reinterpret_cast<const void*>(StaggerCheck()), expected.data(), expected.size()) != 0) {
			SKSE::log::critical("Stagger state branch differs from the verified executable");
			return false;
		}
		const REL::Relocation<std::uintptr_t> performAction{ REL::RelocationID(40551, 41557) };
		for (const auto site : { NPCLandCall(), PlayerLandCall() }) {
			std::int32_t displacement{};
			std::memcpy(&displacement, reinterpret_cast<const void*>(site + 1), sizeof(displacement));
			if (*reinterpret_cast<const std::uint8_t*>(site) != 0xE8 || site + 5 + displacement != performAction.address()) {
				SKSE::log::critical("Landing call at {:X} differs from the verified executable", site);
				return false;
			}
		}
		return true;
	}
}
