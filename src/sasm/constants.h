#pragma once
#include "pch.h"
#include "util/vec.h"

namespace hasl::sasm
{
	// each is 8 bytes
	typedef int64_t i_t;
	typedef double f_t;
	typedef vec<float> v_t;


	namespace c
	{
		constexpr static size_t command_reg_count = 3;

		// GENERAL	  ARG	 RETURN		EXTRA
		// $i0-$i7, $j0-$j3, $k0-$k3, ($obj, $hst, $oc, $flag)
		constexpr static size_t int_reg_count = 20;
		constexpr static size_t first_int_reg = 0, last_int_reg = int_register_count - 1;
		// $f0-$f7, $g0-$g3, $h0-$h3
		constexpr static size_t float_reg_count = 16;
		constexpr static size_t first_float_reg = last_int_reg + 1, last_float_reg = first_float_reg + float_reg_count - 1;
		// $v0-$v7, $w0-$w3, $x0-$x3
		constexpr static size_t vec_reg_count = 16;
		constexpr static size_t first_vec_reg = last_float_reg + 1, last_vec_reg = first_vec_reg + vec_reg_count - 1;
		// upper and lower 32 bits
		constexpr static int64_t reg_hi_mask = 0xffffffff00000000, reg_lo_mask = 0xffffffff;


		// special register indices
		constexpr static int64_t reg_obj = 16, reg_hst = 17, reg_oc = 18, reg_flag = 19;
		constexpr static int64_t host_index = -1;
		// 8KB, each element is 8 bytes
		constexpr static size_t stack_count = 8192 / (sizeof(int64_t) / sizeof(int8_t));
		// 4KB, each element is 1 byte
		constexpr static size_t mem_count = 4096;
		// one byte is used as the opcode, so up to 256 are supported
		constexpr static size_t max_op_count = 256;


		// language constants
		constexpr static char label_token = ':', comment_token = ';', reg_token = '$', entry_point_token[] = "main";
	}
}
