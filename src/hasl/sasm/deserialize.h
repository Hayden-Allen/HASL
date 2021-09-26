#pragma once
#include "vm.h"

namespace hasl::sasm
{
	template<typename T, size_t STACK, size_t RAM>
	T* deserialize(const char* fp, vm<STACK, RAM>* const vm)
	{
		std::ifstream in(fp);
		if (!in.is_open())
		{
			printf("Error opening sasm file '%s'\n", fp);
			return nullptr;
		}

		const size_t entry_point = read_ulong(in);

		std::vector<uint64_t> byte_code;
		std::vector<args> instructions;

		while (true)
		{
			uint64_t instruction = read_ulong(in);
			if (in.eof())
				break;
			byte_code.push_back(instruction);
			instructions.push_back(vm->deserialize(instruction));
		}

		return new T(entry_point, byte_code, instructions);
	}
}
