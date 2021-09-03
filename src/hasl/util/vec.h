#pragma once
#include "pch.h"
#include "functions.h"
#include "hasl/core.h"

namespace hasl
{
	template<typename T>
	struct vec
	{
		T x, y;


		vec() : x(HASL_CAST(T, 0)), y(HASL_CAST(T, 0)) {}
		template<typename U>
		vec(const U& u) : x(HASL_CAST(T, u)), y(HASL_CAST(T, u)) {}
		vec(const T& _x, const T& _y) : x(_x), y(_y) {}
		// copy constructor
		vec(const vec<T>& other)
		{
			operator=(other);
		}
		// move constructor (compiler wants "noexcept" even though it's irrelevant to us)
		vec(vec<T>&& other) noexcept
		{
			operator=(other);
		}


		bool operator==(const T& t) const
		{
			return x == t && y == t;
		}
		// short circuit if x != other.x
		bool operator==(const vec<T>& other) const
		{
			return x == other.x && y == other.y;
		}
		// short circuit if x == other.x
		bool operator!=(const vec<T>& other) const
		{
			return x != other.x || y != other.y;
		}
		// copy values from other into this
		vec<T>& operator=(const vec<T>& other)
		{
			x = other.x;
			y = other.y;
			return *this;
		}
		// return new vector with both of this vector's fields PLUS t
		vec<T> operator+(const T& t) const
		{
			return { x + t, y + t };
		}
		// return new vector with this vector's fields PLUS other's fields
		vec<T> operator+(const vec<T>& other) const
		{
			return { x + other.x, y + other.y };
		}
		// add t to both of this vector's fields
		vec<T>& operator+=(const T& t)
		{
			x += t;
			y += t;
			return *this;
		}
		// add other's fields to this vector's fields
		vec<T>& operator+=(const vec<T>& other)
		{
			x += other.x;
			y += other.y;
			return *this;
		}
		vec<T> operator-() const
		{
			return { -x, -y };
		}
		// return new vector with both of this vector's fields MINUS t
		vec<T> operator-(const T& t) const
		{
			return { x - t, y - t };
		}
		// return new vector with this vector's fields MINUS other's fields
		vec<T> operator-(const vec<T>& other) const
		{
			return { x - other.x, y - other.y };
		}
		// subtract t from both of this vector's fields
		vec<T>& operator-=(const T& t)
		{
			x -= t;
			y -= t;
			return *this;
		}
		// subtract other's fields from this vector's fields
		vec<T>& operator-=(const vec<T>& other)
		{
			x -= other.x;
			y -= other.y;
			return *this;
		}
		// return new vector with both of this vector's fields TIMES t
		vec<T> operator*(const T& t) const
		{
			return { x * t, y * t };
		}
		// return new vector with this vector's fields TIMES other's fields
		vec<T> operator*(const vec<T>& other) const
		{
			return { x * other.x, y * other.y };
		}
		// MULTIPLY this vector's fields by t
		vec<T>& operator*=(const T& t)
		{
			x *= t;
			y *= t;
			return *this;
		}
		// MULTIPLY this vector's fields by other's fields
		vec<T>& operator*=(const vec<T>& other)
		{
			x *= other.x;
			y *= other.y;
			return *this;
		}
		// return new vector with both of this vector's fields DIVIDED BY t
		vec<T> operator/(const T& t) const
		{
			return { x / t, y / t };
		}
		// return new vector with this vector's fields DIVIDED BY other's fields
		vec<T> operator/(const vec<T>& other) const
		{
			return { x / other.x, y / other.y };
		}
		// DIVIDE this vector's fields by t
		vec<T>& operator/=(const T& t)
		{
			x /= t;
			y /= t;
			return *this;
		}
		// DIVIDE this vector's fields by other's fields
		vec<T>& operator/=(const vec<T>& other)
		{
			x /= other.x;
			y /= other.y;
			return *this;
		}
		// the length of this vector
		T magnitude() const
		{
			return HASL_CAST(T, std::sqrt(x * x + y * y));
		}
		// return a new vector that is this vector normalized
		vec<T> normalized() const
		{
			return operator/(magnitude());
		}
		// normalize this vector and return it
		vec<T>& normalize()
		{
			return operator/=(magnitude());
		}
		// dot product of this vector and other
		T dot(const vec<T>& other) const
		{
			return x * other.x + y * other.y;
		}
		// the angle (relative to the x-axis) of this vector
		T angle() const
		{
			return std::atan2(y, x);
		}
		// the angle between this vector and other
		T angle_between(const vec<T>& other) const
		{
			return std::acos(dot(other) / (magnitude() * other.magnitude()));
		}
		vec<T>& clamp(const T& lo, const T& hi)
		{
			const T mag = magnitude();
			if (mag < lo)
				return normalize() *= lo;
			if (mag > hi)
				return normalize() *= hi;
			return *this;
		}
		vec<T> unit() const
		{
			return { HASL_CAST(T, sign(x)), HASL_CAST(T, sign(y)) };
		}
		vec<T> abs() const
		{
			return { std::abs(x), std::abs(y) };
		}
	};



	template<typename T>
	vec<T> operator+(const T& t, const vec<T>& v)
	{
		return { t + v.x, t + v.y };
	}
	template<typename T>
	vec<T> operator-(const T& t, const vec<T>& v)
	{
		return { t - v.x, t - v.y };
	}
	template<typename T>
	vec<T> operator*(const T& t, const vec<T>& v)
	{
		return { t * v.x, t * v.y };
	}
	template<typename T>
	vec<T> operator/(const T& t, const vec<T>& v)
	{
		return { t / v.x, t / v.y };
	}
}
