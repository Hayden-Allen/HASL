#pragma once
#include "pch.h"
#include "command.h"

namespace hasl::sasm
{
	class script;

	class assembler
	{
	public:
		assembler(const char* fp, script* const s);
		HASL_DCM(assembler);
	public:
		bool assemble();
	private:
		size_t m_line, m_last_mem_write;
		std::unordered_map<std::string, size_t> m_labels;
		std::unordered_map<size_t, std::string> m_references;
		std::string m_filepath;
		std::ifstream m_file;
		bool m_abort;
		script* m_script;
	private:
		void parse_line(std::string& line);
		args create(const std::string& command, const std::string& arglist);
		arg_type get_arg_type(const std::string& arg);
		bool next_line(std::string* const line);
		size_t next_space(const std::string& s);
		template<typename ... ARGS>
		void err(size_t line, const char* fmt, const ARGS& ... args)
		{
			char buf[1024];
			sprintf_s(buf, "[%s:%u] %s\n", m_filepath.c_str(), m_line, fmt);
			printf(buf, args...);
			m_abort = true;
		}
	};
}
