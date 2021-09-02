#include "pch.h"
#include "hasl.h"

using namespace hasl;

int main()
{
	// 8KB, each element is 8 bytes
	constexpr static size_t stack_count = 8192 / (sizeof(int64_t) / sizeof(int8_t));
	// 4KB, each element is 1 byte
	constexpr static size_t mem_count = 4096;

	sasm::vm<stack_count, mem_count>* vm = new sasm::vm<stack_count, mem_count>();
	sasm::script<stack_count, mem_count> script("res/test.sasm", vm);

	vm->run(script, 0.f);
	vm->mem_dump();

	delete vm;
	return 0;
}
