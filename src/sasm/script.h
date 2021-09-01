#pragma once
#include "pch.h"
#include "registers.h"
#include "command.h"

namespace hasl::sasm
{
	class script
	{
		friend class assembler;
		friend class vm;
	public:
		script(const char* fp);
		// TODO temp
		script(const std::vector<args>& instructions);
		HASL_DCM(script);
	private:
		bool m_assembled, m_abort, m_sleeping;
		size_t m_entry_point;
		float m_sleep_end;
		std::string m_filepath;
		// resolved commands (do this ahead of time so they don't have to be created from the byte code each time a command is run).
		std::vector<args> m_instructions;
	};
}
