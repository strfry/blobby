#pragma once

#include "Vector.h"
#include <cmath>
#include <iostream>
class PhysicWall
{
	public:
	
		enum Orientation
		{
			HORIZONTAL,
			VERTICAL
		};
		
		PhysicWall(Orientation o, float position, float min = -100000, float max = 1000000);
	
		bool hitTest(const Vector2& center, float radius) const;
		Vector2 getNormal() const;
		
	private:
		
		Orientation mOrientation;
		
		float mPosition;
		float mMinBoundary;
		float mMaxBoundary;
};
