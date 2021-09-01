#pragma once
#include "pch.h"

namespace hasl
{
	std::string string_trim(const std::string& s)
	{
		const size_t start = s.find_first_not_of(c::whitespace_tokens);
		const size_t end = s.find_last_not_of(c::whitespace_tokens);
		if (start == std::string::npos || end == std::string::npos)
			return "";
		return s.substr(start, end - start + 1);
	}
	std::vector<std::string> string_split(const std::string& s, char delim)
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
}
