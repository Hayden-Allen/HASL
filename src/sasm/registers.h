#pragma once
#include "pch.h"

namespace hasl::sasm
{
	// contains all the registers for a VM instance
	struct registers
	{
		// indices 0-19
		i_t i[c::int_reg_count] = { 0 };
		// indices 20-35
		f_t f[c::float_reg_count] = { 0.f };
		// indices 36-51
		v_t v[c::vec_reg_count];
	};
}
