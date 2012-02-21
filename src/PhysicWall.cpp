#include "PhysicWall.h"

PhysicWall::PhysicWall(Orientation o, float position, float min, float max) : 
		mOrientation(o), 
		mPosition(position), 
		mMinBoundary(min),
		mMaxBoundary(max)
{
	
}

bool PhysicWall::hitTest(const Vector2& center, float radius) const
{
	float tp = (mOrientation == HORIZONTAL) ? center.y : center.x;
	float sp = (mOrientation == HORIZONTAL) ? center.x : center.y;
	if( std::abs(tp - mPosition) < radius )
	{
		if( sp > mMinBoundary && sp < mMaxBoundary)
		{
			return true;
		}
	}
	
	return false;
}

Vector2 PhysicWall::getNormal() const
{
	return mOrientation == HORIZONTAL ? Vector2(0,1) : Vector2(1,0);
}
