#pragma once
#include "pch.h"

namespace hasl::sasm
{
	class scriptable;

	struct script_runtime
	{
		float current_time, delta_time;
		scriptable* const host;
		std::vector<scriptable*> env;
	};
}
