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
				m_script->m_instructions.emplace_back(create(command, args));
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
		args create(const std::string& command, const std::string& arglist)
		{
			// get info about the given command
			args args;
			const auto& desc = vm<STACK, RAM>::s_command_descriptions.find(command);
			if (desc == vm<STACK, RAM>::s_command_descriptions.end())
			{
				err(m_line, "Invalid command '%s'", command.c_str());
				return args;
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
				return args;
			}

			// parse each argument
			size_t imm_count = 0;
			for (size_t i = 0; i < list.size(); i++)
			{
				// check that current arg matches the expected type
				arg_type cur = get_arg_type(list[i]);
				if (!(expected[i] & cur))
				{
					err(m_line, "Invalid argument %u for instruction '%s'", i, desc->first.c_str());
					return args;
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
						args.ii[imm_count++] = result.first;
					// float immediate
					else
						args.fi = HASL_PUN(f_t, result.first);
				}
				// this argument is a register
				else
				{
					if (cur == arg_type::I)
						args.i[i] = m_vm->get_int_reg(result.first);
					else if (cur == arg_type::F)
						args.f[i] = m_vm->get_float_reg(result.first);
					else
						args.v[i] = m_vm->get_vec_reg(result.first);
				}
			}

			return args;
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
				return arg_type::MI;

		get_arg_type_end:
			err(m_line, "Invalid argument '%s'", arg.c_str());
			return arg_type::NONE;
		}
		bool next_line(std::string* const line)
		{
			if (m_file.eof())
				return false;
			getline(m_file, *line);
			return true;
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

			// support bin, dec, and hex
			int base = 10;
			if (arg.find("0x") == 0)
				base = 16;
			if (arg.find("0b") == 0)
				base = 2;
			return { std::stoll((base != 10 ? arg.substr(2) : arg), nullptr, base), false };
		}
		template<typename ... ARGS>
		void err(size_t line, const char* fmt, const ARGS& ... args)
		{
			char buf[1024];
			sprintf_s(buf, "[%s:%llu] %s\n", m_filepath.c_str(), m_line, fmt);
			printf(buf, args...);
			m_abort = true;
		}
	};
}
