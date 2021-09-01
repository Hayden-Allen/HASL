#include "pch.h"
#include "hasl.h"

using namespace hasl;

int main()
{
	sasm::script script("res/test.sasm");
	sasm::vm* vm = new sasm::vm();

	vm->run(script, 0.f);

	delete vm;
	return 0;
}
