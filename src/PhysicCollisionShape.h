#pragma once

#include "Vector.h"
#include <algorithm>

struct AABBox
{
	AABBox() : upperLeft(), lowerRight()
	{
	}
	
	AABBox(Vector2 uL, Vector2 lR) : 
		upperLeft(uL), lowerRight(lR)	
	{
	};
	
	AABBox(Vector2 center, float width, float height) : 
		upperLeft(center - Vector2(width/2, height/2)), lowerRight(center + Vector2(width/2, height/2))	
	{
	};
	
	void clear()
	{
		upperLeft.clear();
		lowerRight.clear();
	}
	
	void merge(const AABBox& other)
	{
		upperLeft.x = std::min(upperLeft.x, other.upperLeft.x);
		upperLeft.y = std::min(upperLeft.y, other.upperLeft.y);
		lowerRight.x = std::max(lowerRight.x, other.lowerRight.x);
		lowerRight.y = std::max(lowerRight.y, other.lowerRight.y);
	}
	
	bool intersects(const AABBox& other) const
	{
		Vector2 d1 = other.upperLeft - lowerRight;
		Vector2 d2 = upperLeft - other.lowerRight;

		if (d1.x > 0.0f || d1.y > 0.0f)
			return false;

		if (d2.x > 0.0f || d2.y > 0.0f)
			return false;

		return true;

	}
	
	AABBox& operator+=(Vector2 vec)
	{
		upperLeft += vec;
		lowerRight += vec;
		return *this;
	}
	
	AABBox operator+(Vector2 vec) const
	{
		return AABBox(upperLeft + vec, lowerRight + vec);
	}
	
	Vector2 upperLeft;
	Vector2 lowerRight;
};

class ICollisionShape
{
	public:
		ICollisionShape(Vector2 rpos) :
			mRelativePosition(rpos)
		{
			
		}
	
		Vector2 getRelativePosition() const 
		{
			return mRelativePosition;
		}
		
		void setRelativePosition( Vector2 pos) 
		{
			mRelativePosition = pos;
		}
	
		virtual AABBox getBoundingBox() const = 0;
		virtual bool isPointInside(Vector2 point) const = 0;
		virtual unsigned int getShapeType() const = 0;
		
	private:
		Vector2 mRelativePosition;
};

enum ShapeTypes
{
	BOX,
	SPHERE
};

class CollisionShapeSphere : public ICollisionShape
{
	public:
		CollisionShapeSphere(float rad, Vector2 center = Vector2(0,0)) : 
				ICollisionShape(center), mRadius(rad)
		{
		}
	
		virtual AABBox getBoundingBox() const 
		{
			return AABBox(getRelativePosition(), mRadius, mRadius);
		}
		
		virtual bool isPointInside(Vector2 point) const 
		{
			/// \todo we need lengthSQ();
			return (point - getRelativePosition()).length() < mRadius;
		}
		virtual unsigned int getShapeType() const 
		{
			return SPHERE;
		}
		
		float getRadius() const
		{
			return mRadius;
		}
		
	private:
		float mRadius;
};
