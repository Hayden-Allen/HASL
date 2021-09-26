#pragma once
#include "pch.h"

namespace hasl
{
	static std::string string_trim(const std::string& s)
	{
		const size_t start = s.find_first_not_of(c::whitespace_tokens);
		const size_t end = s.find_last_not_of(c::whitespace_tokens);
		if (start == std::string::npos || end == std::string::npos)
			return "";
		return s.substr(start, end - start + 1);
	}
	static std::vector<std::string> string_split(const std::string& s, char delim)
	{
		std::vector<std::string> result;
		std::string cur;
		size_t last = 0;
		bool in_string = false;

		for (size_t i = 0; i < s.size(); i++)
		{
			if (s[i] == c::string_token)
				in_string = !in_string;
			if (!in_string && s[i] == delim)
			{
				result.emplace_back(string_trim(s.substr(last, i - last)));
				last = i + 1;
			}
		}
		// end of the string may not have a delimiter, but should still be added
		result.emplace_back(string_trim(s.substr(last)));

		return result;
	}
	static size_t string_next_space(const std::string& s)
	{
		size_t space = s.find_first_of(hasl::c::whitespace_tokens);
		if (space == std::string::npos)
			space = s.size();
		return space;
	}
	template<typename T>
	static int sign(const T& t)
	{
		return (t < 0 ? -1 : (t > 0 ? 1 : 0));
	}
	template<typename T>
	static void arrprint(size_t count, T* arr, const std::string& fmt, const std::string& sep, size_t wrap = 10)
	{
		printf("[\n\t");
		const size_t bufferLength = HASL_CAST(size_t, fmt.length() + sep.length() + 2);
		char* buffer = new char[bufferLength];
		for (size_t i = 0; i < count; i++)
		{
			sprintf_s(buffer, bufferLength, "%s%s", fmt.c_str(), (i != count - 1 ? sep.c_str() : ""));
			printf(buffer, arr[i]);
			if (i != 0 && (i + 1) % wrap == 0)
				printf("\n\t");
		}
		printf("\n]\n");
		delete[] buffer;
	}
	template<typename T, size_t N>
	static void arrprint(const T(&arr)[N], const std::string& fmt, const std::string& sep, size_t wrap = 10)
	{
		arrprint(N, arr, fmt, sep, wrap);
	}
	// TODO properly with Squirrel3
	template<typename A, typename B>
	static A rand(A min, B max)
	{
		return HASL_CAST(A, HASL_CAST(double, std::rand()) / HASL_CAST(double, RAND_MAX) * (max - min) + min);
	}
	template<typename T>
	static T rad_to_deg(const T& t)
	{
		return HASL_CAST(T, HASL_CAST(double, t) * 180.0 / std::numbers::pi);
	}
	static void sleep(size_t ms)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(ms));
	}
	static void write_ulong(std::ofstream& out, uint64_t i)
	{
		out.put(HASL_CAST(char, (i & 0xff00000000000000) >> 56));
		out.put(HASL_CAST(char, (i & 0xff000000000000) >> 48));
		out.put(HASL_CAST(char, (i & 0xff0000000000) >> 40));
		out.put(HASL_CAST(char, (i & 0xff00000000) >> 32));
		out.put(HASL_CAST(char, (i & 0xff000000) >> 24));
		out.put(HASL_CAST(char, (i & 0xff0000) >> 16));
		out.put(HASL_CAST(char, (i & 0xff00) >> 8));
		out.put(HASL_CAST(char, (i & 0xff) >> 0));
	}
	static uint64_t read_ulong(std::ifstream& in)
	{
		return
			(HASL_CAST(uint64_t, in.get()) << 56) |
			(HASL_CAST(uint64_t, in.get()) << 48) |
			(HASL_CAST(uint64_t, in.get()) << 40) |
			(HASL_CAST(uint64_t, in.get()) << 32) |
			(HASL_CAST(uint64_t, in.get()) << 24) |
			(HASL_CAST(uint64_t, in.get()) << 16) |
			(HASL_CAST(uint64_t, in.get()) << 8) |
			(HASL_CAST(uint64_t, in.get()) << 0);
	}
}
