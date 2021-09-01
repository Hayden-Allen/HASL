#include "pch.h"
#include "vm.h"

namespace hasl::sasm
{
	vm::vm() :
		m_stack{ 0 },
		m_memory{ 0 },
		m_pc(0),
		m_sp(0)
	{
		// static structures haven't been initialized yet
		if (s_command_names.empty())
		{
			for (size_t i = 0; i < s_instructions.size(); i++)
			{
				const instruction& cur = s_instructions[i];
				s_command_names.emplace(i, cur.name);
				s_command_descriptions.emplace(cur.name, command_description(i, cur.desc));
				s_operations[i] = cur.op;
			}
		}
	}



	i_t* const vm::get_int_reg(size_t i)
	{
		HASL_ASSERT(i >= c::first_int_reg && i <= c::last_int_reg, "Invalid int register index");
		return &m_reg.i[i - c::first_int_reg];
	}
	f_t* const vm::get_float_reg(size_t i)
	{
		HASL_ASSERT(i >= c::first_float_reg && i <= c::last_float_reg, "Invalid float register index");
		return &m_reg.f[i - c::first_float_reg];
	}
	v_t* const vm::get_vec_reg(size_t i)
	{
		HASL_ASSERT(i >= c::first_vec_reg && i <= c::last_vec_reg, "Invalid int register index");
		return &m_reg.v[i - c::first_vec_reg];
	}
	bool vm::range_check(script& s, i_t i, i_t min, i_t max)
	{
		if (s.m_abort = (i < min || i >= max))
		{
			HASL_ASSERT(false, "Range check failed");
			return false;
		}
		return true;
	}
}
