#include "pch.h"
#include "script.h"
#include "assembler.h"

namespace hasl::sasm
{
	// TODO
	script::script(const char* fp) :
		m_assembled(false),
		m_abort(false),
		m_sleeping(false),
		m_entry_point(0),
		m_sleep_end(0),
		m_filepath(fp)
	{
		m_assembled = assembler(fp, this).assemble();
	}
}
