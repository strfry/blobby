#pragma once

#include "Vector.h"
#include <cmath>

class PhysicWall
{
	public:
	
		enum Orientation
		{
			HORIZONTAL,
			VERTICAL
		};
		
		PhysicWall(Orientation o, float position, float min = -100000, float max = 1000000) : 
				mOrientation(o), 
				mPosition(position), 
				mMinBoundary(min),
				mMaxBoundary(max)
		{
			
		}
	
		bool hitTest(const Vector2& center, float radius)
		{
			float tp = (mOrientation == HORIZONTAL) ? center.x : center.y;
			float sp = (mOrientation == HORIZONTAL) ? center.y : center.x;
			
			if( std::abs(tp - mPosition) < radius )
			{
				if( sp > mMinBoundary && sp < mMaxBoundary)
				{
					return true;
				}
			}
			
			return false;
		}
		
	private:
		
		Orientation mOrientation;
		
		float mPosition;
		float mMinBoundary;
		float mMaxBoundary;
};
