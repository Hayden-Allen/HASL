#pragma once
#define HASL_CAST(T, x) static_cast<T>(x)
#define HASL_PUN(T, x) *(T*)&x
#define HASL_DCM(x) \
	x(const x& other) = delete; \
	x(x&& other) = delete;
#define HASL_ASSERT(x, s) if(!(x)) { printf("HASL error: %s\n", s); __debugbreak(); }