#pragma once
#include "pch.h"
#include "script.h"
#include "script_runtime.h"
#include "scriptable.h"

namespace hasl::sasm
{
	struct mem_dump_options
	{
		bool ri = true, rf = true, rv = true;
		bool stack = false, ram = false;
	};


	template<size_t STACK, size_t RAM>
	class vm
	{
		friend class assembler<STACK, RAM>;
		friend class serializer;
	public:
		vm() :
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
					s_command_descriptions.emplace(cur.name, command_description(HASL_CAST(uint8_t, i), cur.desc));
					s_operations[i] = cur.op;
				}
			}
		}
		HASL_DCM(vm);
	public:
		void mem_dump(const mem_dump_options& options = {}) const
		{
			printf("\n\n====================\n= HASL VM MEM DUMP =\n====================\n\n");

			if(options.ri)
				arrprint(m_regs.i, "%lld", ", ", 8);

			if(options.rf)
				arrprint(m_regs.f, "%f", ", ", 8);

			if (options.rv)
			{
				printf("[\n\t");
				for (size_t i = 0; i < c::vec_reg_count; i++)
				{
					printf("<%f, %f>", m_regs.v[i].x, m_regs.v[i].y);
					if (i != c::vec_reg_count - 1)
						printf(", ");
					if ((i + 1) % 4 == 0)
					{
						printf("\n");
						if (i != c::vec_reg_count - 1)
							printf("\t");
					}
				}
				printf("]\n");
			}
			
			if(options.stack)
				arrprint(m_stack, "%llu", ", ", 16);
			if(options.ram)
				arrprint(m_memory, "%u", ", ", 16);
		}
		args deserialize(uint64_t i)
		{
			args a = deserialize_base(i);

			printf("%llx\n", i);
			printf("%u | %llx | %llx\n", a.opcode, (i & 0xff000000000000) >> 48, (i & 0xff0000000000) >> 40);
			deserialize_reg(a, (i & 0xff000000000000) >> 48, 0);
			deserialize_reg(a, (i & 0xff0000000000) >> 40, 2);

			uint64_t flags = (i & 0xff00000000) >> 32, reg = i & 0xff, imm = i & 0xffffffff;
			if (flags == 1)
				a.i[1] = get_int_reg(reg);
			else if (flags == 2)
				a.f[1] = get_float_reg(reg);
			else if (flags == 3)
				a.v[1] = get_vec_reg(reg);
			else if (flags == 4)
				a.fi = HASL_PUN(float, imm);
			else if (flags == 5)
				a.ii[0] = imm;
			else
				printf("Invalid second source flags %llx\n", flags);

			return a;
		}
	protected:
		// stack
		i_t m_stack[STACK];
		// RAM
		uint8_t m_memory[RAM];
		// registers
		registers m_regs;
		// program counter, stack pointer
		size_t m_pc, m_sp;
		std::vector<scriptable*> m_spawn_queue;
	protected:
		virtual scriptable* spawn(const char* s) = 0;
		virtual void process_spawn_queue(script_runtime& rt) = 0;
		virtual bool is_key_pressed(i_t key) const = 0;
		virtual bool is_mouse_pressed(i_t button) const = 0;
		virtual v_t get_mouse_pos() const = 0;
		virtual v_t get_mouse_scroll() const = 0;
		i_t run(script<STACK, RAM>& s, script_runtime& rt)
		{
			if (!s.m_assembled)
			{
				HASL_ASSERT(false, "Cannot run a script that failed to compile");
				return 0;
			}

			// script is still sleeping
			if (rt.current_time < s.m_sleep_end)
				return 0;

			// restart from entry point unless sleeping
			if (!s.m_sleeping)
				m_pc = s.m_entry_point;

			s.m_sleeping = false;
			s.m_abort = false;
			m_regs.i[c::reg_hst] = c::host_index;
			m_regs.i[c::reg_oc] = rt.env.size();
			m_regs.i[c::reg_flag] = 0;

			while (!s.m_abort && m_pc < s.m_instructions.size())
			{
				const args& cur = s.m_instructions[m_pc];
				(this->*(s_operations[cur.opcode]))(&s, cur, rt.current_time, rt.delta_time, rt.host, rt.env);
				m_pc++;
			}

			process_spawn_queue(rt);
			m_spawn_queue.clear();

			return m_regs.i[c::reg_flag];
		}
	private:
		i_t* const get_int_reg(size_t i)
		{
			HASL_ASSERT(i >= c::first_int_reg && i <= c::last_int_reg, "Invalid int register index");
			return &m_regs.i[i - c::first_int_reg];
		}
		f_t* const get_float_reg(size_t i)
		{
			HASL_ASSERT(i >= c::first_float_reg && i <= c::last_float_reg, "Invalid float register index");
			return &m_regs.f[i - c::first_float_reg];
		}
		v_t* const get_vec_reg(size_t i)
		{
			HASL_ASSERT(i >= c::first_vec_reg && i <= c::last_vec_reg, "Invalid vec register index");
			return &m_regs.v[i - c::first_vec_reg];
		}
		bool range_check(script<STACK, RAM>* const s, i_t i, i_t min, i_t max)
		{
			if (s->m_abort = (i < min || i >= max))
			{
				HASL_ASSERT(false, "Range check failed");
				return false;
			}
			return true;
		}
		template<typename T>
		void stack_push(script<STACK, RAM>* const s, const T& t)
		{
			if (s->m_abort = (m_sp >= STACK))
			{
				HASL_ASSERT(false, "Stack overflow");
				return;
			}
			m_stack[m_sp++] = HASL_PUN(i_t, t);
		}
		template<typename T>
		T stack_pop(script<STACK, RAM>* const s)
		{
			if (s->m_abort = (m_sp == 0))
			{
				HASL_ASSERT(false, "Stack underflow");
				return HASL_CAST(T, 0);
			}
			return HASL_PUN(T, m_stack[--m_sp]);
		}
		static args deserialize_base(uint64_t i)
		{
			args a;
			a.opcode = (i & 0xff00000000000000) >> 56;
			return a;
		}
		void deserialize_reg(args& a, uint64_t byte, size_t index)
		{
			uint64_t flags = (byte & 0xc0), reg = (byte & 0x3f);
			if (flags == 0x40)
				a.i[index] = get_int_reg(reg);
			else if (flags == 0x80)
				a.f[index] = get_float_reg(reg);
			else if (flags == 0xc0)
				a.v[index] = get_vec_reg(reg);
			else if (flags != 0)
				printf("Invalid reg %zu type %llx\n", index, flags);
		}
	private:
		// instruction signature
#define I(name, code) \
	void name(script<STACK, RAM>* const s, const args& a, float ct, float dt, scriptable* const host, std::vector<scriptable*>& env) { code }
		// register's value if it exists, OR something else
#define R(r, o) ((r) ? (*r) : (o))


		// math
		I(add,
			*a.i[2] = *a.i[0] + R(a.i[1], a.ii[0]);
		);
		I(addf,
			*a.f[2] = *a.f[0] + R(a.f[1], a.fi);
		);
		I(addv,
			*a.v[2] = *a.v[0] + R(a.v[1], R(a.f[1], a.fi));
		);
		I(sub,
			*a.i[2] = *a.i[0] - R(a.i[1], a.ii[0]);
		);
		I(subf,
			*a.f[2] = *a.f[0] - R(a.f[1], a.fi);
		);
		I(subv,
			*a.v[2] = *a.v[0] - R(a.v[1], R(a.f[1], a.fi));
		);
		I(mul,
			*a.i[2] = *a.i[0] * R(a.i[1], a.ii[0]);
		);
		I(mulf,
			*a.f[2] = *a.f[0] * R(a.f[1], a.fi);
		);
		I(mulv,
			*a.v[2] = *a.v[0] * R(a.v[1], R(a.f[1], a.fi));
		);
		I(div,
			*a.i[2] = *a.i[0] / R(a.i[1], a.ii[0]);
		);
		I(divf,
			*a.f[2] = *a.f[0] / R(a.f[1], a.fi);
		);
		I(divv,
			*a.v[2] = *a.v[0] / R(a.v[1], R(a.f[1], a.fi));
		);
		// math.bit
		I(band,
			*a.i[2] = *a.i[0] & R(a.i[1], a.ii[0]);
		);
		I(bxor,
			*a.i[2] = *a.i[0] ^ R(a.i[1], a.ii[0]);
		);
		I(bor,
			*a.i[2] = *a.i[0] | R(a.i[1], a.ii[0]);
		);
		I(bnot,
			*a.i[1] = ~R(a.i[0], a.ii[0]);
		);
		I(sl,
			*a.i[2] = (*a.i[0]) << R(a.i[1], a.ii[0]);
		);
		I(sr,
			*a.i[2] = (*a.i[0]) >> R(a.i[1], a.ii[0]);
		);
		// math.trig
		I(sine,
			*a.f[1] = std::sin(*a.f[0]);
		);
		I(cosine,
			*a.f[1] = std::cos(*a.f[0]);
		);
		I(tangent,
			*a.f[1] = std::tan(*a.f[0]);
		);
		I(arcsine,
			*a.f[1] = std::asin(*a.f[0]);
		);
		I(arccosine,
			*a.f[1] = std::acos(*a.f[0]);
		);
		I(arctangent,
			*a.f[2] = std::atan2(*a.f[1], *a.f[0]);
		);
		// math.fn
		I(min,
			*a.i[2] = std::min(*a.i[0], R(a.i[1], a.ii[0]));
		);
		I(max,
			*a.i[2] = std::max(*a.i[0], R(a.i[1], a.ii[0]));
		);
		I(minf,
			*a.f[2] = std::min(*a.f[0], R(a.f[1], a.fi));
		);
		I(maxf,
			*a.f[2] = std::max(*a.f[0], R(a.f[1], a.fi));
		);
		I(minv,
			*a.f[1] = std::min(a.v[0]->x, a.v[0]->y);
		);
		I(maxv,
			*a.f[1] = std::max(a.v[0]->x, a.v[0]->y);
		);
		I(power,
			*a.f[2] = std::pow(*a.f[0], R(a.f[1], a.fi));
		);
		I(squareroot,
			*a.f[1] = std::sqrt(R(a.f[0], a.fi));
		);
		I(absolute,
			*a.i[1] = std::abs(R(a.i[0], a.ii[0]));
		);
		I(absolutef,
			*a.f[1] = std::abs(R(a.f[0], a.fi));
		);
		I(absolutev,
			*a.v[1] = a.v[0]->abs();
		);
		I(random,
			*a.i[2] = rand(*a.i[0], R(a.i[1], a.ii[0]));
		);
		I(randomf,
			*a.f[2] = rand(*a.f[0], R(a.f[1], a.fi));
		);
		I(sign,
			*a.i[1] = hasl::sign(R(a.i[0], a.ii[0]));
		);
		I(signf,
			*a.f[1] = hasl::sign(R(a.f[0], a.fi));
		);
		I(signv,
			*a.v[1] = a.v[0]->unit();
		);
		// math.v_t
		I(dot,
			*a.f[2] = a.v[0]->dot(*a.v[1]);
		);
		I(mag,
			*a.f[1] = a.v[0]->magnitude();
		);
		I(ang,
			*a.f[1] = rad_to_deg(a.v[0]->angle());
		);
		I(angv,
			*a.f[2] = rad_to_deg(a.v[0]->angle_between(*a.v[1]));
		);
		I(norm,
			*a.v[1] = a.v[0]->normalized();
		);
		// mem
		I(psh,
			if (a.i[0])
				stack_push(s, *a.i[0]);
			else if (a.f[0])
				stack_push(s, *a.f[0]);
			else
				stack_push(s, *a.v[0]);
		);
		I(pop,
			if (a.i[0])
				*a.i[0] = stack_pop<i_t>(s);
			else if (a.f[0])
				*a.f[0] = stack_pop<f_t>(s);
			else
				*a.v[0] = stack_pop<v_t>(s);
		);
		I(mov,
			*a.i[1] = HASL_CAST(i_t, R(a.f[0], R(a.i[0], a.ii[0])));
		);
		I(movl,
			*a.i[1] &= c::reg_hi_mask;
			*a.i[1] |= R(a.i[0], a.ii[0]);
		);
		I(movh,
			*a.i[1] &= c::reg_lo_mask;
			*a.i[1] |= (R(a.i[0], a.ii[0]) << 32);
		);
		I(movf,
			*a.f[1] = HASL_CAST(f_t, R(a.f[0], R(a.i[0], a.fi)));
		);
		I(movv,
			*a.v[1] = R(a.v[0], R(a.i[0], R(a.f[0], a.fi)));
		);
		I(movx,
			if (a.v[0])
				a.v[1]->x = a.v[0]->x;
			else
				a.v[1]->x = HASL_CAST(float, R(a.i[0], R(a.f[0], a.fi)));
		);
		I(movy,
			if (a.v[0])
				a.v[1]->y = a.v[0]->y;
			else
				a.v[1]->y = HASL_CAST(float, R(a.i[0], R(a.f[0], a.fi)));
		);
		I(stm,
			const i_t index = R(a.i[1], a.ii[0]);
			if (!range_check(s, index, 0, RAM))
				return;

			// write to memory in chunks of 8 bytes by HASL_CASTing to a uint64_t pointer
			if (a.i[0])
				*((uint64_t*)(&m_memory[index])) = *(uint64_t*)a.i[0];
			else if (a.f[0])
				*((uint64_t*)(&m_memory[index])) = *(uint64_t*)a.f[0];
			else if (a.v[0])
				*((uint64_t*)(&m_memory[index])) = *(uint64_t*)a.v[0];
		);
		I(ldm,
			const i_t index = R(a.i[1], a.ii[0]);
			if (!range_check(s, index, 0, RAM))
				return;

			if (a.i[1])
				*a.i[1] = HASL_PUN(i_t, m_memory[index]);
			else if (a.f[1])
				*a.f[1] = HASL_PUN(f_t, m_memory[index]);
			else if (a.v[1])
				*a.v[1] = HASL_PUN(v_t, m_memory[index]);
		);
		// ctrl
		I(beq,
			if (!range_check(s, a.ii[0], 0, s->m_instructions.size()))
				return;

			if (R(a.i[0], R(a.f[0], *a.v[0])) == R(a.i[1], R(a.f[1], *a.v[1])))
				m_pc = HASL_CAST(size_t, a.ii[0]) - 1;
		);
		I(beqz,
			if (!range_check(s, a.ii[0], 0, s->m_instructions.size()))
				return;

			if (R(a.i[0], R(a.f[0], *a.v[0])) == 0)
				m_pc = HASL_CAST(size_t, a.ii[0]) - 1;
		);
		I(bne,
			if (!range_check(s, a.ii[0], 0, s->m_instructions.size()))
				return;

			if (R(a.i[0], R(a.f[0], *a.v[0])) != R(a.i[1], R(a.f[1], *a.v[1])))
				m_pc = HASL_CAST(size_t, a.ii[0]) - 1;
		);
		I(blt,
			if (!range_check(s, a.ii[0], 0, s->m_instructions.size()))
				return;

			if (R(a.i[0], *a.f[0]) < R(a.i[1], *a.f[1]))
				m_pc = HASL_CAST(size_t, a.ii[0]) - 1;
		);
		I(bgt,
			if (!range_check(s, a.ii[0], 0, s->m_instructions.size()))
				return;

			if (R(a.i[0], *a.f[0]) > R(a.i[1], *a.f[1]))
				m_pc = HASL_CAST(size_t, a.ii[0]) - 1;
		);
		I(ble,
			if (!range_check(s, a.ii[0], 0, s->m_instructions.size()))
				return;

			if (R(a.i[0], *a.f[0]) <= R(a.i[1], *a.f[1]))
				m_pc = HASL_CAST(size_t, a.ii[0]) - 1;
		);
		I(bge,
			if (!range_check(s, a.ii[0], 0, s->m_instructions.size()))
				return;

			if (R(a.i[0], *a.f[0]) >= R(a.i[1], *a.f[1]))
				m_pc = HASL_CAST(size_t, a.ii[0]) - 1;
		);
		I(j,
			if (!range_check(s, a.ii[0], 0, s->m_instructions.size()))
				return;
			m_pc = HASL_CAST(size_t, a.ii[0]) - 1;
		);
		I(call,
			if (!range_check(s, a.ii[0], 0, s->m_instructions.size()))
				return;
			stack_push(s, m_pc);
			m_pc = HASL_CAST(size_t, a.ii[0]) - 1;
		);
		I(ret,
			m_pc = stack_pop<size_t>(s);
		);
		I(end,
			s->m_abort = true;
		);
		I(slp,
			s->m_sleep_end = ct + R(a.i[0], a.ii[0]);
			s->m_sleeping = true;
			s->m_abort = true;
		);
		I(blk,
			sleep(HASL_CAST(size_t, R(a.i[0], a.ii[0])));
		);
		// debug
		I(dbg,
			printf("[HASL@%s]: %lld\n", s->m_filepath.c_str(), R(a.i[0], a.ii[0]));
		);
		I(dbgf,
			printf("[HASL@%s]: %f\n", s->m_filepath.c_str(), R(a.f[0], a.fi));
		);
		I(dbgv,
			printf("[HASL@%s]: <%f, %f>\n", s->m_filepath.c_str(), a.v[0]->x, a.v[0]->y);
		);
		I(dbgs,
			printf("[HASL@%s]: %s\n", s->m_filepath.c_str(), (char*)(m_memory + R(a.i[0], a.ii[0])));
		);
		// engine
		I(gettime,
			*a.f[0] = ct;
		);
		// engine.input
		I(imp,
			*a.v[0] = get_mouse_pos();
		);
		I(ims,
			*a.v[0] = get_mouse_scroll();
		);
		I(imb,
			*a.i[1] = HASL_CAST(i_t, is_mouse_pressed(R(a.i[0], a.ii[0])));
		);
		I(ikp,
			*a.i[1] = HASL_CAST(i_t, is_key_pressed(R(a.i[0], a.ii[0])));
		);
		I(ikd,
			const i_t x = HASL_CAST(i_t, is_key_pressed(R(a.i[0], a.ii[0])));
			const i_t y = HASL_CAST(i_t, is_key_pressed(R(a.i[1], a.ii[1])));
			*a.i[2] = x - y;
		);
		// engine.obj
#define CS (m_regs.i[c::reg_obj] == c::host_index ? host : env[m_regs.i[c::reg_obj]])
		I(ogp,
			*a.v[0] = CS->get_pos();
		);
		I(osp,
			CS->set_pos(*a.v[0]);
		);
		I(ogv,
			*a.v[0] = CS->get_vel();
		);
		I(osv,
			CS->set_vel(*a.v[0]);
		);
		I(ogd,
			*a.v[0] = CS->get_dims();
		);
		I(ogs,
			*a.f[0] = CS->get_speed();
		);
		I(oss,
			CS->set_state((char*)(m_memory + R(a.i[0], a.ii[0])));
		);
		I(spn,
			scriptable* spawned = spawn((char*)(m_memory + R(a.i[0], a.ii[0])));
			// add for processing at the end of the current execution
			m_spawn_queue.push_back(spawned);
			// increment current object count
			m_regs.i[c::reg_oc]++;
			// add to current environment
			env.push_back(spawned);
			*a.i[1] = env.size() - 1;
		);

#undef R
#undef I
	private:
		typedef void(vm::* operation)(script<STACK, RAM>* const, const args&, float, float, scriptable* const, std::vector<scriptable*>&);
		static inline operation s_operations[c::max_op_count];
		static inline std::unordered_map<size_t, std::string> s_command_names;
		static inline std::unordered_map<std::string, command_description> s_command_descriptions;


		struct instruction
		{
			std::string name;
			std::vector<arg_type> desc;
			// TODO remove
			size_t group;
			operation op;
		};
		const static inline std::vector<instruction> s_instructions =
		{
			// math
			{ "add",	{ arg_type::I, arg_type::I_MI, arg_type::I }, 0, &vm::add },
			{ "addf",	{ arg_type::F, arg_type::F_MF, arg_type::F }, 1, &vm::addf },
			{ "addv",	{ arg_type::V, arg_type::F_V_MF, arg_type::V }, 2, &vm::addv },
			{ "sub",	{ arg_type::I, arg_type::I_MI, arg_type::I }, 0, &vm::sub },
			{ "subf",	{ arg_type::F, arg_type::F_MF, arg_type::F }, 1, &vm::subf },
			{ "subv",	{ arg_type::V, arg_type::F_V_MF, arg_type::V }, 2, &vm::subv },
			{ "mul",	{ arg_type::I, arg_type::I_MI, arg_type::I }, 0, &vm::mul },
			{ "mulf",	{ arg_type::F, arg_type::F_MF, arg_type::F }, 1, &vm::mulf },
			{ "mulv",	{ arg_type::V, arg_type::F_V_MF, arg_type::V }, 2, &vm::mulv },
			{ "div",	{ arg_type::I, arg_type::I_MI, arg_type::I }, 0, &vm::div },
			{ "divf",	{ arg_type::F, arg_type::F_MF, arg_type::F }, 1, &vm::divf },
			{ "divv",	{ arg_type::V, arg_type::F_V_MF, arg_type::V }, 2, &vm::divv },
			// math.bit
			{ "and",	{ arg_type::I, arg_type::I_MI, arg_type::I }, 0, &vm::band },
			{ "xor",	{ arg_type::I, arg_type::I_MI, arg_type::I }, 0, &vm::bxor },
			{ "or",		{ arg_type::I, arg_type::I_MI, arg_type::I }, 0, &vm::bor },
			{ "not",	{ arg_type::I_MI, arg_type::I }, 3, &vm::bnot },
			{ "sl",		{ arg_type::I, arg_type::I_MI, arg_type::I }, 0, &vm::sl },
			{ "sr",		{ arg_type::I, arg_type::I_MI, arg_type::I }, 0, &vm::sr },
			// math.trig
			{ "sin",	{ arg_type::F, arg_type::F }, 4, &vm::sine },
			{ "cos",	{ arg_type::F, arg_type::F }, 4, &vm::cosine },
			{ "tan",	{ arg_type::F, arg_type::F }, 4, &vm::tangent },
			{ "asin",	{ arg_type::F, arg_type::F }, 4, &vm::arcsine },
			{ "acos",	{ arg_type::F, arg_type::F }, 4, &vm::arccosine },
			{ "atan",	{ arg_type::F, arg_type::F }, 4, &vm::arctangent },
			// math.fn
			{ "min",	{ arg_type::I, arg_type::I_MI, arg_type::I }, 0, &vm::min },
			{ "max",	{ arg_type::I, arg_type::I_MI, arg_type::I }, 0, &vm::max },
			{ "minf",	{ arg_type::F, arg_type::F_MF, arg_type::F }, 1, &vm::minf },
			{ "maxf",	{ arg_type::F, arg_type::F_MF, arg_type::F }, 1, &vm::maxf },
			{ "minv",	{ arg_type::V, arg_type::F }, 5, &vm::minv },
			{ "maxv",	{ arg_type::V, arg_type::F }, 5, &vm::maxv },
			{ "pow",	{ arg_type::F, arg_type::F_MF, arg_type::F }, 1, &vm::power },
			{ "sqrt",	{ arg_type::F_MF, arg_type::F }, 6, &vm::squareroot },
			{ "abs",	{ arg_type::I_MI, arg_type::I }, 3, &vm::absolute },
			{ "absf",	{ arg_type::F_MF, arg_type::F }, 6, &vm::absolutef },
			{ "absv",	{ arg_type::V, arg_type::V }, 7, &vm::absolutev },
			{ "rand",	{ arg_type::I, arg_type::I_MI, arg_type::I }, 0, &vm::random },
			{ "randf",	{ arg_type::F, arg_type::F_MF, arg_type::F }, 1, &vm::randomf },
			{ "sign",	{ arg_type::I_MI, arg_type::I }, 3, &vm::sign },
			{ "signf",	{ arg_type::F_MF, arg_type::F }, 6, &vm::signf },
			{ "signv",	{ arg_type::V, arg_type::V }, 7, &vm::signv },
			// math.vec
			{ "dot",	{ arg_type::V, arg_type::V, arg_type::F }, 8, &vm::dot },
			{ "mag",	{ arg_type::V, arg_type::F }, 5, &vm::mag },
			{ "ang",	{ arg_type::V, arg_type::F }, 5, &vm::ang },
			{ "angv",	{ arg_type::V, arg_type::V, arg_type::F }, 8, &vm::angv },
			{ "norm",	{ arg_type::V, arg_type::V }, 7, &vm::norm },
			// mem
			{ "psh",	{ arg_type::I_F_V }, 9, &vm::psh },
			{ "pop",	{ arg_type::I_F_V }, 9, &vm::pop },
			{ "mov",	{ arg_type::I_F_MI, arg_type::I }, 10, &vm::mov },
			{ "movl",	{ arg_type::I_MI, arg_type::I }, 3, &vm::movl },
			{ "movh",	{ arg_type::I_MI, arg_type::I }, 3, &vm::movh },
			{ "movf",	{ arg_type::I_F_MF, arg_type::F }, 11, &vm::movf },
			{ "movv",	{ arg_type::I_F_V_MF, arg_type::V }, 12, &vm::movv },
			{ "movx",	{ arg_type::I_F_V_MF, arg_type::V }, 12, &vm::movx },
			{ "movy",	{ arg_type::I_F_V_MF, arg_type::V }, 12, &vm::movy },
			{ "stm",	{ arg_type::I_F_V, arg_type::I_MI }, 13, &vm::stm },
			{ "ldm",	{ arg_type::I_MI, arg_type::I_F_V }, 14, &vm::ldm },
			// ctrl
			{ "beq",	{ arg_type::I_F_V, arg_type::I_F_V, arg_type::L_MI }, 15, &vm::beq },
			{ "beqz",	{ arg_type::I_F_V, arg_type::L_MI }, 16, &vm::beqz },
			{ "bne",	{ arg_type::I_F_V, arg_type::I_F_V, arg_type::L_MI }, 15, &vm::bne },
			{ "blt",	{ arg_type::I_F, arg_type::I_F, arg_type::L_MI }, 17, &vm::blt },
			{ "bgt",	{ arg_type::I_F, arg_type::I_F, arg_type::L_MI }, 17, &vm::bgt },
			{ "ble",	{ arg_type::I_F, arg_type::I_F, arg_type::L_MI }, 17, &vm::ble },
			{ "bge",	{ arg_type::I_F, arg_type::I_F, arg_type::L_MI }, 17, &vm::bge },
			{ "j",		{ arg_type::L_MI }, 18, &vm::j },
			{ "call",	{ arg_type::L_MI }, 18, &vm::call },
			{ "ret",	{ }, 26, &vm::ret },
			{ "end",	{ }, 26, &vm::end },
			{ "slp",	{ arg_type::I_MI }, 19, &vm::slp },
			{ "blk",	{ arg_type::I_MI }, 19, &vm::blk },
			// debug
			{ "dbg",	{ arg_type::I_MI }, 19, &vm::dbg },
			{ "dbgf",	{ arg_type::F_MF }, 20, &vm::dbgf },
			{ "dbgv",	{ arg_type::V }, 21, &vm::dbgv },
			{ "dbgs",	{ arg_type::I_MI_MS }, 22, &vm::dbgs },
			// engine
			{ "time",	{ arg_type::F }, 23, &vm::gettime },
			// engine.input
			{ "imp",	{ arg_type::V }, 21, &vm::imp },
			{ "ims",	{ arg_type::V }, 21, &vm::ims },
			{ "imb",	{ arg_type::I_MI, arg_type::I }, 3, &vm::imb },
			{ "ikp",	{ arg_type::I_MI, arg_type::I }, 3, &vm::ikp },
			{ "ikd",	{ arg_type::I_MIS, arg_type::I_MIS, arg_type::I }, 24, &vm::ikd },
			// engine.obj
			{ "ogp",	{ arg_type::V }, 21, &vm::ogp },
			{ "osp",	{ arg_type::V }, 21, &vm::osp },
			{ "ogv",	{ arg_type::V }, 21, &vm::ogv },
			{ "osv",	{ arg_type::V }, 21, &vm::osv },
			{ "ogd",	{ arg_type::V }, 21, &vm::ogd },
			{ "ogs",	{ arg_type::F }, 23, &vm::ogs },
			{ "oss",	{ arg_type::I_MI_MS }, 22, &vm::oss },
			{ "spn",	{ arg_type::I_MI_MS, arg_type::I }, 25, &vm::spn }
		};
	};
}
