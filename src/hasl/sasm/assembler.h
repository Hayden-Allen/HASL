#pragma once
#include "pch.h"
#include "command.h"

namespace hasl::sasm
{
	template<size_t, size_t>
	class script;
	template<size_t, size_t>
	class vm;

	template<size_t STACK, size_t RAM>
	class assembler
	{
	public:
		assembler(const char* fp, script<STACK, RAM>* const s, vm<STACK, RAM>* const vm) :
			m_line(0),
			m_last_mem_write(RAM),
			m_filepath(fp),
			m_file(fp),
			m_abort(false),
			m_script(s),
			m_vm(vm)
		{
			if (!m_file.is_open())
			{
				printf("Error opening script file '%s'\n", fp);
				m_abort = true;
			}
		}
		HASL_DCM(assembler);
	public:
		bool assemble()
		{
			// read the file
			std::string line;
			while (!m_abort && next_line(&line))
			{
				m_line++;

				line = string_trim(line);
				if (line.empty())
					continue;

				parse_line(line);
			}

			// resolve label references
			if (!m_abort)
			{
				for (const auto& ref : m_references)
				{
					const auto& it = m_labels.find(ref.second);
					// a non-existent label was referenced
					if (it == m_labels.end())
					{
						err(ref.first, "Unresolved label '%s'", ref.second);
						break;
					}
					// label value is always in this spot
					m_script->m_instructions[ref.first].ii[0] = it->second;
				}
			}

			// if a "main" label is provided, use it as the entry point
			const auto& it = m_labels.find(c::entry_point_token);
			if (it != m_labels.end())
				m_script->m_entry_point = it->second;

			return !m_abort;
		}
	private:
		size_t m_line, m_last_mem_write;
		std::unordered_map<std::string, size_t> m_labels;
		std::unordered_map<size_t, std::string> m_references;
		std::string m_filepath;
		std::ifstream m_file;
		bool m_abort;
		script<STACK, RAM>* m_script;
		vm<STACK, RAM>* m_vm;
	private:
		struct reg_indices
		{
			size_t i[3] = { 0 };
		};
	private:
		static uint64_t serialize_args(const args& a, const reg_indices& r)
		{
			return s_serialization_functions[s_instruction_groups[a.opcode]](a, r);
		}
		static uint64_t serialize_base(const args& a)
		{
			return HASL_CAST(uint64_t, a.opcode) << 56;
		}
		// 7: opcode
		// 6: src reg 1
		// 5: dst reg
		// 4: flags for 3-0
		//	1 if int reg
		//	2 if float reg
		//	3 if vec reg
		//  4 if float immediate
		//  5 if int immediate
		// 3-0: src reg 2 / immediate
		static uint64_t serialize23r1i(const args& a, const reg_indices& r, bool float_immediate)
		{
			uint64_t i = serialize_base(a);
			if (a.i[0])
				i |= 0x40000000000000 | (r.i[0] << 48);
			else if (a.f[0])
				i |= 0x80000000000000 | (r.i[0] << 48);
			else if (a.v[0])
				i |= 0xc0000000000000 | (r.i[0] << 48);

			if (a.i[2])
				i |= 0x400000000000 | (r.i[2] << 40);
			else if (a.f[2])
				i |= 0x800000000000 | (r.i[2] << 40);
			else if (a.v[2])
				i |= 0xc00000000000 | (r.i[2] << 40);

			if (a.i[1])
				i |= 0x100000000 | r.i[1];
			else if (a.f[1])
				i |= 0x200000000 | r.i[1];
			else if (a.v[1])
				i |= 0x300000000 | r.i[1];
			else
			{
				if (float_immediate)
					i |= 0x400000000 | HASL_PUN(uint64_t, a.f);
				else
					i |= 0x500000000 | a.ii[0];
			}

			return i;
		}
	private:
		bool next_line(std::string* const line)
		{
			if (m_file.eof())
				return false;
			getline(m_file, *line);
			return true;
		}
		void parse_line(std::string& line)
		{
			// separate line into instruction and arguments
			const size_t space = string_next_space(line);
			// `line` is already trimmed, so this won't have surrounding whitespace
			std::string command = line.substr(0, space);
			// there may be multiple spaces between the command and the args, so we need to trim this
			std::string args = string_trim(line.substr(space));

			// this line is commented out, ignore it
			if (command[0] == c::comment_token)
				return;

			// this line is a label definition
			if (command.back() == c::label_token)
				parse_label(command, args);
			// this line is an instruction
			else
				create(command, args);
		}
		void parse_label(const std::string& cmd, const std::string& args)
		{
			// check that label is on its own line
			const std::string label = cmd.substr(0, cmd.size() - 1);
			if (!args.empty())
			{
				err(m_line, "Label definition '%s' is not on its own line", label.c_str());
				return;
			}

			// check if this label is being redefined
			const auto& it = m_labels.find(label);
			if (it != m_labels.end())
			{
				err(m_line, "Label '%s' is defined twice", label.c_str());
				return;
			}

			// add the label
			m_labels.emplace(label, m_script->m_instructions.size());
		}
		void create(const std::string& command, const std::string& arglist)
		{
			// get info about the given command
			args args;
			const auto& desc = vm<STACK, RAM>::s_command_descriptions.find(command);
			if (desc == vm<STACK, RAM>::s_command_descriptions.end())
			{
				err(m_line, "Invalid command '%s'", command.c_str());
				return;
			}
			args.opcode = desc->second.opcode;

			// check that correct number of args were given
			const auto& expected = desc->second.args;
			std::vector<std::string> list = string_split(arglist, hasl::c::list_separator_token);
			// remove empty args
			list.erase(std::remove_if(list.begin(), list.end(), [](const std::string& s) { return string_trim(s).size() == 0; }), list.end());
			if (list.size() != expected.size())
			{
				err(m_line, "Instruction '%s' expects %zu arguments but %zu were given", desc->first.c_str(), expected.size(), list.size());
				return;
			}

			// parse each argument
			size_t imm_count = 0;
			reg_indices regs;
			for (size_t i = 0; i < list.size(); i++)
			{
				// check that current arg matches the expected type
				arg_type cur = get_arg_type(list[i]);
				if (!(expected[i] & cur))
				{
					err(m_line, "Invalid argument %u for instruction '%s'", i, desc->first.c_str());
					return;
				}

				// this argument is a label reference, which must be resolved at the end
				if (cur == arg_type::L)
					continue;

				// whatever this argument is, get its bits in an integer
				auto result = resolve_int(list[i]);

				// this argument is an immediate
				if (is_immediate(cur))
				{
					// int immediate
					if (!result.second)
					{
						if(cur == arg_type::MIS && (result.first <= c::small_int_min || result.first >= c::small_int_max))
							err(m_line, "Invalid 16-bit integer literal %d (must be in [%d, %d])", c::small_int_min, c::small_int_max);
						
						args.ii[imm_count++] = result.first;
					}
					// float immediate
					else
						args.fi = HASL_PUN(f_t, result.first);
				}
				// this argument is a register
				else
				{
					regs.i[i] = result.first;
					if (cur == arg_type::I)
						args.i[i] = m_vm->get_int_reg(result.first);
					else if (cur == arg_type::F)
						args.f[i] = m_vm->get_float_reg(result.first);
					else
						args.v[i] = m_vm->get_vec_reg(result.first);
				}
			}

			m_script->m_instructions.emplace_back(args);
			m_script->m_byte_code.emplace_back(serialize_args(args, regs));
		}
		arg_type get_arg_type(const std::string& arg)
		{
			// string literal
			if (arg[0] == hasl::c::string_token)
				return arg_type::MS;

			// label reference
			if (std::isalpha(arg[0]))
			{
				m_references.emplace(m_script->m_instructions.size(), arg);
				return arg_type::L;
			}

			// special register
			const auto& it = c::special_regs.find(arg);
			if (it != c::special_regs.end())
				return arg_type::I;

			// register
			if (arg[0] == c::reg_token)
			{
				if (arg.size() != 3)
					goto get_arg_type_end;

				const char c = arg[1], n = arg[2];
				if ((c == 'i' && (n >= '0' && n <= '7')) ||
					((c == 'j' || c == 'k') && (n >= '0' && n <= '3')))
					return arg_type::I;
				if ((c == 'f' && (n >= '0' && n <= '7')) ||
					((c == 'g' || c == 'h') && (n >= '0' && n <= '3')))
					return arg_type::F;
				if ((c == 'v' && (n >= '0' && n <= '7')) ||
					((c == 'w' || c == 'x') && (n >= '0' && n <= '3')))
					return arg_type::V;
			}

			// float immediate (must contain a '.')
			if (arg.find(c::float_token) != std::string::npos)
				return arg_type::MF;
			// int immediate
			if (isdigit(arg[0]) || arg[0] == '-')
			{
				int64_t i = parse_int(arg);
				if (i >= c::small_int_min && i <= c::small_int_max)
					return arg_type::MI | arg_type::MIS;
				return arg_type::MI;
			}

		get_arg_type_end:
			err(m_line, "Invalid argument '%s'", arg.c_str());
			return arg_type::NONE;
		}
		std::pair<i_t, bool> resolve_int(const std::string& arg)
		{
			// string literal
			if (arg[0] == hasl::c::string_token)
			{
				// make sure the last character is also a "
				if (arg.back() != hasl::c::string_token)
				{
					err(m_line, "String literals must be enclosed in '\"'");
					return { 0, false };
				}
				// write string into RAM
				const size_t length = arg.size() - 1;
				m_last_mem_write = m_last_mem_write - length;
				for (size_t i = 0; i < length; i++)
					m_vm->m_memory[m_last_mem_write + i] = (i == length - 1 ? 0 : arg[i + 1]);
				// return pointer to the string
				return { HASL_CAST(int64_t, m_last_mem_write), false };
			}

			// get special register index from map
			const auto& it = c::special_regs.find(arg);
			if (it != c::special_regs.end())
				return { it->second, false };

			// $<c><n>
			if (arg[0] == c::reg_token)
			{
				// group base index + number
				char group = arg[1], n = arg[2] - '0';
				return { c::reg_offsets.at(group) + n, false };
			}

			// floats are still returned as "ints" but with the flag set
			if (arg.find('.') != std::string::npos)
			{
				f_t d = std::stod(arg);
				return { HASL_PUN(int64_t, d), true };
			}

			return { parse_int(arg), false };
		}
		int64_t parse_int(const std::string& arg)
		{
			// support bin, dec, and hex
			int base = 10;
			if (arg.find("0x") == 0)
				base = 16;
			if (arg.find("0b") == 0)
				base = 2;
			return std::stoll((base != 10 ? arg.substr(2) : arg), nullptr, base);
		}
		template<typename ... ARGS>
		void err(size_t line, const char* fmt, const ARGS& ... args)
		{
			char buf[1024];
			sprintf_s(buf, "[%s:%llu] %s\n", m_filepath.c_str(), m_line, fmt);
			printf(buf, args...);
			m_abort = true;
		}
	private:
		constexpr static size_t s_instruction_groups[] =
		{
			// math
			0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2,
			// bit
			0, 0, 0, 0, 3, 0, 0,
			// trig
			4, 4, 4, 4, 4, 4,
			// fn
			0, 0, 1, 1, 5, 5, 1, 6, 3, 6, 7, 0, 1, 3, 6, 7,
			// vec
			8, 5, 5, 8, 7,
			// mem
			9, 9, 10, 3, 3, 11, 12, 12, 12, 13, 14,
			// ctrl
			15, 16, 15, 17, 17, 17, 17, 18, 18, 26, 26, 19, 19,
			// debug
			19, 20, 21, 22,
			// engine
			23,
			// input
			21, 21, 3, 3, 24,
			// obj
			21, 21, 21, 21 ,21, 23, 22, 25
		};
		const static inline std::vector<std::function<uint64_t(const args&, const reg_indices&)>> s_serialization_functions =
		{
			// I, I_MI, I
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// F, F_MF, F
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, true); },
			// V, F_V_MF
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, true); },
			// I_MI, I
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// F, F
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },

			// V, F
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// F_MF, F
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// V, V
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// V, V, F
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },

			// I_F_V
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// I_F_MI, I
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// I_F_MF, F
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, true); },
			// I_F_V, I_MI
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// I_MI, I_F_V
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },


			
			// I_F_V, I_F_V, L_MI
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// I_F_V, L_MI
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// I_F, I_F, L_MI
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// L_MI
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// I_MI
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },



			// F_MF
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, true); },
			// V
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// I_MI_MS
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// F
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
			// I_MIS, I_MIS, I
			[](const args& a, const reg_indices& r)
			{
				uint64_t i = serialize_base(a);
				// TODO
				return i;
			},

			// I_MI_MS, I
			[](const args& a, const reg_indices& r) { return serialize23r1i(a, r, false); },
		};
	};
}
