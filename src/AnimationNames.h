#pragma once
#include <array>

namespace ModernStaggerLock
{
	struct AnimationNames
	{
		static const AnimationNames& Get()
		{
			static const AnimationNames names;
			return names;
		}

		RE::BSFixedString staggerStop{ "StaggerStop" };
		RE::BSFixedString staggerDirection{ "staggerDirection" };
		RE::BSFixedString staggerMagnitude{ "staggerMagnitude" };
		RE::BSFixedString mslDirection{ "msl_staggerDirection" };
		RE::BSFixedString mslLevel{ "msl_staggerLevel" };
		RE::BSFixedString recovery{ "MSL_IsStaggerRecovery" };
		RE::BSFixedString characterState{ "msl_CharacterStateType" };
		RE::BSFixedString specialID{ "msl_SpecialStaggerID" };
		RE::BSFixedString specialId{ "msl_specialStaggerId" };
		RE::BSFixedString specialFalling{ "msl_IsSpecialStaggerFalling" };
		RE::BSFixedString specialLandStart{ "MSL_SpecialStaggerLandStart" };
		std::array<RE::BSFixedString, 5> counters{
			"msl_SmallStaggerCounter", "msl_MediumStaggerCounter", "msl_LargeStaggerCounter",
			"msl_LargestStaggerCounter", "msl_SpecialStaggerCounter"
		};
	};
}
