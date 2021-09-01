#pragma once
#include "pch.h"

namespace hasl
{
	template<typename T>
	struct vec
	{
		T x, y;


		vec() : x(CAST(T, 0)), y(CAST(T, 0)) {}
		template<typename U>
		vec(const U& u) : x(CAST(T, u)), y(CAST(T, u)) {}
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
			return isZero(x - other.x) && isZero(y - other.y);
		}
		// short circuit if x == other.x
		bool operator!=(const vec<T>& other) const
		{
			return !isZero(x - other.x) || !isZero(y - other.y);
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
		T Magnitude() const
		{
			return dist(0.f, 0.f, x, y);
		}
		// return a new vector that is this vector normalized
		vec<T> Normalized() const
		{
			return operator/(Magnitude());
		}
		// normalize this vector and return it
		vec<T>& Normalize()
		{
			return operator/=(Magnitude());
		}
		// dot product of this vector and other
		T Dot(const vec<T>& other) const
		{
			return x * other.x + y * other.y;
		}
		// the angle (relative to the x-axis) of this vector
		T Angle() const
		{
			return atan(x, y);
		}
		// the angle between this vector and other
		T AngleBetween(const vec<T>& other) const
		{
			return acos(Dot(other) / (Magnitude() * other.Magnitude()));
		}
		vec<T>& Clamp(const T& lo, const T& hi)
		{
			const T mag = Magnitude();
			if (mag < lo)
				return Normalize() *= lo;
			if (mag > hi)
				return Normalize() *= hi;
			return *this;
		}
		vec<T> Unit() const
		{
			return { CAST(T, sign(x)), CAST(T, sign(y)) };
		}
		vec<T> Abs() const
		{
			return { abs(x), abs(y) };
		}
		bool IsZero() const
		{
			return isZero(x) && isZero(y);
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
