#pragma once
#include "Runtime.h"

namespace ModernStaggerLock
{
	class StaggeredStateCheckPatch
	{
	public:
		static void Install()
		{
			REL::safe_fill(Runtime::StaggerCheck(), 0x90, 6);
			INFO("Staggered state check patched");
		}
	};
}
