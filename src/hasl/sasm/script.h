#pragma once
#include "pch.h"
#include "registers.h"
#include "command.h"
#include "assembler.h"

namespace hasl::sasm
{
	template<size_t, size_t>
	class vm;

	template<size_t STACK, size_t RAM>
	class script
	{
		friend class assembler<STACK, RAM>;
		friend class vm<STACK, RAM>;
	public:
		script(const char* fp, vm<STACK, RAM>* const vm) :
			m_assembled(false),
			m_abort(false),
			m_sleeping(false),
			m_entry_point(0),
			m_sleep_end(0),
			m_filepath(fp),
			m_vm(vm)
		{
			m_assembled = assembler<STACK, RAM>(fp, this, vm).assemble();
		}
		script(uint64_t entry_point, const std::vector<uint64_t>& byte_code, const std::vector<args>& instructions) :
			m_assembled(true),
			m_abort(false),
			m_entry_point(entry_point),
			m_sleep_end(0),
			m_filepath(""),
			m_byte_code(byte_code),
			m_instructions(instructions),
			m_vm(nullptr)
		{}
		HASL_DCM(script);
	public:
		void serialize(std::ofstream& out)
		{
			write_ulong(out, m_entry_point);
			for (const auto& i : m_byte_code)
				write_ulong(out, i);
		}
	private:
		bool m_assembled, m_abort, m_sleeping;
		size_t m_entry_point;
		float m_sleep_end;
		std::string m_filepath;
		std::vector<uint64_t> m_byte_code;
		// resolved commands (do this ahead of time so they don't have to be created from the byte code each time a command is run).
		std::vector<args> m_instructions;
		vm<STACK, RAM>* m_vm;
	};
}
