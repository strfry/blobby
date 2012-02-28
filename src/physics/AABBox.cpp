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

#include "AABBox.h"

#include <algorithm>

AABBox::AABBox() : upperLeft(), lowerRight()
{
}

AABBox::AABBox(Vector2 uL, Vector2 lR) : 
	upperLeft(uL), lowerRight(lR)	
{
};

AABBox::AABBox(Vector2 center, float half_width, float half_height) : 
	upperLeft(center - Vector2(half_width, half_height)), 
	lowerRight(center + Vector2(half_width, half_height))	
{
};

void AABBox::clear()
{
	upperLeft.clear();
	lowerRight.clear();
}

AABBox& AABBox::merge(const AABBox& other)
{
	upperLeft.x = std::min(upperLeft.x, other.upperLeft.x);
	upperLeft.y = std::min(upperLeft.y, other.upperLeft.y);
	lowerRight.x = std::max(lowerRight.x, other.lowerRight.x);
	lowerRight.y = std::max(lowerRight.y, other.lowerRight.y);
	
	return *this;
}

Vector2 AABBox::getCenter() const
{
	return 0.5 * (upperLeft + lowerRight);
}


bool AABBox::intersects(const AABBox& other) const
{
	Vector2 d1 = other.upperLeft - lowerRight;
	Vector2 d2 = upperLeft - other.lowerRight;

	if (d1.x > 0.0f || d1.y > 0.0f)
		return false;

	if (d2.x > 0.0f || d2.y > 0.0f)
		return false;

	return true;

}

bool AABBox::isBoxInside(const AABBox& other) const
{
	return other.upperLeft.x >= upperLeft.x && 
			other.upperLeft.y >= upperLeft.y && 
			other.lowerRight.x <= lowerRight.x &&
			other.lowerRight.y <= lowerRight.y;
}

bool AABBox::isPointInside(Vector2 point) const {
	return point > upperLeft && point < lowerRight;
}

AABBox& AABBox::operator+=(Vector2 vec)
{
	upperLeft += vec;
	lowerRight += vec;
	return *this;
}

AABBox AABBox::operator+(Vector2 vec) const
{
	return AABBox(upperLeft + vec, lowerRight + vec);
}

