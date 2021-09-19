#pragma once
#include "pch.h"
#include "hasl/util/vec.h"

namespace hasl::sasm
{
	enum class arg_type
	{
		// error type
		NONE = 0,
		// int register, float register, vec register, int immediate, float immediate, label, string literal, 16-bit int literal
		I = 1, F = 2, V = 4, MI = 8, MF = 16, L = 32, MS = 64, MIS = 128,
		// combo types (for when multiple argument types are allowed for a given instruction)
		I_F = I | F,
		I_MI = I | MI,
		I_MIS = I | MIS,
		F_MF = F | MF,
		V_MF = V | MF,
		L_MI = L | MI,
		I_F_V = I | F | V,
		I_F_MI = I | F | MI,
		I_F_MF = I | F | MF,
		F_V_MF = F | V | MF,
		I_MI_MS = I | MI | MS,
		I_F_V_MF = I | F | V | MF,
		I_F_V_MI_MF = I | F | V | MI | MF
	};
	static bool operator&(arg_type a, arg_type b)
	{
		return HASL_CAST(uint32_t, a) & HASL_CAST(uint32_t, b);
	}
	static bool is_int_immediate(arg_type a)
	{
		return a == arg_type::MI || a == arg_type::MS || a == arg_type::MIS;
	}
	static bool is_immediate(arg_type a)
	{
		return is_int_immediate(a) || a == arg_type::MF;
	}



	struct command_description
	{
		uint8_t opcode;
		std::vector<arg_type> args;
	};



	struct args
	{
		// register pointers
		i_t* i[c::command_reg_count] = { nullptr };
		f_t* f[c::command_reg_count] = { nullptr };
		v_t* v[c::command_reg_count] = { nullptr };

		uint8_t opcode;
		// immediate values
		i_t ii[2] = { 0 };
		f_t fi = 0.f;
	};
}
