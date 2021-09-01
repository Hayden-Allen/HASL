#include "pch.h"
#include "assembler.h"
#include "script.h"
#include "util/string.h"

namespace hasl::sasm
{
	assembler::assembler(const char* fp, script* const script) :
		m_line(0),
		m_last_mem_write(c::mem_count),
		m_filepath(fp),
		m_file(fp),
		m_abort(false),
		m_script(script)
	{
		if (!m_file.is_open())
		{
			printf("Error opening script file '%s'\n", fp);
			m_abort = true;
		}
	}



	bool assembler::assemble()
	{
		// read the file
		std::string line;
		while (!m_abort && next_line(&line))
		{
			m_line++;

			if (string_trim(line).empty())
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
				m_script->m_instructions[ref.first].ii1 = it->second;
			}
		}

		// if a "main" label is provided, use it as the entry point
		const auto& it = m_labels.find(c::entry_point_token);
		if (it != m_labels.end())
			m_script->m_entry_point = it->second;

		return !m_abort;
	}



	void assembler::parse_line(std::string& line)
	{

	}
	args assembler::create(const std::string& command, const std::string& arglist)
	{

	}
	arg_type assembler::get_arg_type(const std::string& arg)
	{

	}
	bool assembler::next_line(std::string* const line)
	{
		if (m_file.eof())
			return false;
		getline(m_file, *line);
		return true;
	}
	size_t assembler::next_space(const std::string& s)
	{
		size_t space = s.find_first_of(hasl::c::whitespace_tokens);
		if (space == std::string::npos)
			space = s.size();
		return space;
	}
}
