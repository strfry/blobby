/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

#pragma once

#include <cassert>
#include <cmath>

/// \brief class for repesenting 2d vectors
/// \details e.g. positions, velocities.
class Vector2
{
	public:
		union
		{
			struct 
			{
				float x;
				float y;		
			};
			float val[2];
		};
	
	/// \brief default c'tor
	/// \details does initialise to (0,0)
	Vector2();
	
	/// \brief component c'tor
	/// \details initialises to (\p x,\p y)
	Vector2(float x, float y);
	
	/// \brief difference c'tor
	/// \details initialises the new vector with the difference v2 - v1
	Vector2(const Vector2& v1, const Vector2& v2);

	/// resets the vector to (0,0)
	void clear();
	
	Vector2 reflectedX() const;
	Vector2 reflectedY() const;
	Vector2 scaled(float factor) const;
	Vector2 scaledX(float factor) const;
	Vector2 scaledY(float factor) const;
	float length() const;
	Vector2 normalise() const;
	Vector2 contraVector() const ;
	
	inline Vector2 getHalfVector(const Vector2& vec) const
	{
		return Vector2(x + (vec.x - x) / 2, y + (vec.y - y) / 2);
	}

	inline Vector2& operator = (const Vector2& newVector)
	{
		x = newVector.x;
		y = newVector.y;	
		return *this;
	}
	
	inline bool operator == (const Vector2& vector) const
	{
		return (x == vector.x && y == vector.y);
	}
	
	inline bool operator != (const Vector2& vector) const
	{
		return (x != vector.x || y != vector.y);
	}
	
	inline Vector2 operator + (const Vector2& vector) const
	{
		return Vector2(x + vector.x, y + vector.y);
	}
	
	inline Vector2 operator - (const Vector2& vector) const
	{
		return Vector2(x - vector.x, y - vector.y);
	}
	
	inline Vector2 operator * (float scalar) const
	{
		return Vector2(x * scalar, y * scalar);
	}
	
	inline Vector2 operator * (const Vector2& vector) const
	{
		return Vector2(x * vector.x, y * vector.y);
	}
	
	inline Vector2 operator / (float scalar) const
	{
		assert(scalar != 0.0);
		float invert = 1.0 / scalar;
		return Vector2(x * invert, y * invert);
	}
	
	inline Vector2 operator - () const
	{
		return Vector2(-x, -y);
	}
	
	inline Vector2& operator += (const Vector2& vector)
	{
		x += vector.x;
		y += vector.y;
		return *this;
	}
	
	inline Vector2& operator -= (const Vector2& vector)
	{
		x -= vector.x;
		y -= vector.y;
		return *this;
	}
	
	inline Vector2& operator *= (const Vector2& vector)
	{
		x *= vector.x;
		y *= vector.y;
		return *this;
	}


	inline float dotProduct(const Vector2& vector) const
	{
		return x * vector.x + y * vector.y;
	}
	
	inline float crossProduct(const Vector2& vector) const
	{
		return x * vector.y - y * vector.x;
	}
	
	inline Vector2 reflect(const Vector2& normal) const
	{
		return Vector2(*this - (normal * 2 * dotProduct(normal)));
	}

	Vector2 decompose(Vector2 line) const
	{
		//line = line.normalise();
		Vector2 perp = Vector2(-line.y, line.x);
		
		return Vector2 (dotProduct(line), dotProduct(perp))/line.length()/line.length();
	}
	
	Vector2 recompose(Vector2 line) const
	{
		Vector2 perp = Vector2(-line.y, line.x);
		
		return line * x + perp * y;
	}
};

inline Vector2::Vector2() : x(0), y(0)
{
}

inline Vector2::Vector2(float a, float b) : x(a), y(b)
{
}

inline Vector2::Vector2(const Vector2& v1, const Vector2& v2) : x(v2.x - v1.x), y(v2.y - v1.y)
{

}

inline Vector2 Vector2::reflectedX() const
{
	return Vector2(-x, y);
}

inline Vector2 Vector2::reflectedY() const
{
	return Vector2(x, -y);
}

inline Vector2 Vector2::scaled(float factor) const
{
	return Vector2(x * factor, y * factor);
}

inline Vector2 Vector2::scaledX(float factor) const
{
	return Vector2(x * factor, y);
}

inline Vector2 Vector2::scaledY(float factor) const
{
	return Vector2(x, y * factor);
}

inline float Vector2::length() const
{
	return sqrt(this->x * this->x + this->y * this->y);
}

inline Vector2 Vector2::normalise() const
{
	float fLength = length();
	if (fLength > 1e-08)
		return Vector2(x / fLength, y / fLength);	
	return *this;
}

inline Vector2 Vector2::contraVector() const
{
	return Vector2(-x, -y);
}

inline void Vector2::clear()
{
	x = 0.0;
	y = 0.0;
}

inline bool operator < (const Vector2& v1, const Vector2& v2)
{
	if (v1.x < v2.x)
	if (v1.y < v2.y)
		return true;
	return false;
	
}

inline bool operator > (const Vector2& v1, const Vector2& v2)
{
	if (v1.x > v2.x)
	if (v1.y > v2.y)
		return true;
	return false;
}

inline Vector2 operator*(float f, const Vector2& vec)
{
	return vec * f;
}
