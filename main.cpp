#include "pch.h"
#include "hasl.h"

using namespace hasl;

namespace hasl::sasm
{
	class test_vm final : public vm<1024, 4096>
	{
	public:
		test_vm() : vm<1024, 4096>() {}
		HASL_DCM(test_vm);
	private:
		bool is_key_pressed(i_t key) const override
		{
			return false;
		}
		bool is_mouse_pressed(i_t button) const override
		{
			return true;
		}
		v_t get_mouse_pos() const override
		{
			return { 0.f, 0.f };
		}
		v_t get_mouse_scroll() const override
		{
			return { 0.f, 0.f };
		}
		scriptable* spawn(const char* s) override
		{
			printf("SPAWN '%s'\n", s);
			return nullptr;
		}
		void process_spawn_queue(script_runtime& rt) override
		{
			for (scriptable* s : m_spawn_queue)
				printf("PROCESS '%p'\n", s);
		}
	};



	class test_scriptable final : public scriptable
	{
	public:
		test_scriptable(const std::unordered_map<std::string, void*>& states, const std::string& state) :
			scriptable(states, state)
		{}
		HASL_DCM(test_scriptable);
	public:
		v_t get_dims() const override
		{
			return { 0.f, 0.f };
		}
	};
}

int main()
{
	// 8KB, each element is 8 bytes
	constexpr static size_t stack_count = 8192 / (sizeof(int64_t) / sizeof(int8_t));
	// 4KB, each element is 1 byte
	constexpr static size_t mem_count = 4096;

	sasm::test_vm* vm = new sasm::test_vm();
	sasm::script<stack_count, mem_count> script("res/test.sasm", vm);
	sasm::test_scriptable* host = new sasm::test_scriptable({ {"a", nullptr } }, "a");

	sasm::script_runtime rt = { 0.f, 0.f, host, {} };
	vm->run(script, rt);
	vm->mem_dump();

	delete vm;
	return 0;
}
