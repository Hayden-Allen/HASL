#pragma once
#include "pch.h"
#include "script.h"

namespace hasl::sasm
{
	class vm
	{
	public:
		vm();
		HASL_DCM(vm);
	public:
		i_t run(script& s, float time)
		{
			if (!s.m_assembled)
			{
				HASL_ASSERT(false, "Cannot run a script that failed to compile");
				return 0;
			}

			// script is still sleeping
			if (time < s.m_sleep_end)
				return 0;

			// restart from entry point unless sleeping
			if (!s.m_sleeping)
				m_pc = s.m_entry_point;

			s.m_sleeping = false;
			s.m_abort = false;
			m_reg.i[c::reg_hst] = c::host_index;
			// TODO
			// m_reg.i[c::reg_oc] = 0;
			m_reg.i[c::reg_flag] = 0;

			while (!s.m_abort && m_pc < s.m_instructions.size())
			{
				const args& cur = s.m_instructions[m_pc];
				(this->*(s_operations[cur.opcode]))(s, cur, time, 0.f);
			}


			return m_reg.i[c::reg_flag];
		}
	private:
		// stack
		i_t m_stack[c::stack_count];
		// RAM
		uint8_t m_memory[c::mem_count];
		// registers
		registers m_reg;
		// program counter, stack pointer
		size_t m_pc, m_sp;
	private:
		i_t* const get_int_reg(size_t i);
		f_t* const get_float_reg(size_t i);
		v_t* const get_vec_reg(size_t i);
		bool range_check(script& s, i_t i, i_t min, i_t max);
		template<typename T>
		void stack_push(script& s, const T& t)
		{
			if (s.m_abort = m_sp >= c::stack_count)
			{
				HASL_ASSERT(false, "Stack overflow");
				return HASL_CAST(T, 0);
			}
			m_stack[m_sp++] = HASL_PUN(i_t, t);
		}
		template<typename T>
		T stack_pop(script& s)
		{
			if (s.m_abort = (m_sp == 0))
			{
				HASL_ASSERT(false, "Stack underflow");
				return CAST(T, 0);
			}
			return HASL_PUN(T, m_stack[--m_sp]);
		}
	private:
		// instruction signature
#define I(name, code) \
	void name(script& script, const args& args, float time, float dt) { code }
		// register's value if it exists, OR something else
#define R(r, o) ((r) ? (*r) : (o));
		// math.int
		I(add,
			*args.i[2] = *args.i[0] + R(args.i[1], args.ii1);
		);
		I(sub,
			*args.i[2] = *args.i[0] - R(args.i[1], args.ii1);
		);
		I(mul,
			*args.i[2] = *args.i[0] * R(args.i[1], args.ii1);
		);
		I(div,
			*args.i[2] = *args.i[0] / R(args.i[1], args.ii1);
		);
#undef R
#undef I
	private:
		typedef void(vm::* operation)(script&, const args&, float, float);
		static inline operation s_operations[c::max_op_count];
		static inline std::unordered_map<size_t, std::string> s_command_names;
		static inline std::unordered_map<std::string, command_description> s_command_descriptions;


		struct instruction
		{
			std::string name;
			std::vector<arg_type> desc;
			operation op;
		};
		const static inline std::vector<instruction> s_instructions
		{
			// math.int
			{ "add",		{ arg_type::I, arg_type::I_MI, arg_type::I }, &vm::add },
			{ "sub",		{ arg_type::I, arg_type::I_MI, arg_type::I }, &vm::sub },
			{ "mul",		{ arg_type::I, arg_type::I_MI, arg_type::I }, &vm::mul },
			{ "div",		{ arg_type::I, arg_type::I_MI, arg_type::I }, &vm::div },
		};
	};
}
