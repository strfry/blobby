#pragma once

#include "Vector.h"
#include "AABBox.h"

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
			return AABBox(getRelativePosition(), 2*mRadius, 2*mRadius);
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

class CollisionShapeBox : public ICollisionShape
{
	public:
		CollisionShapeBox(Vector2 size, Vector2 center = Vector2(0,0)) : 
				ICollisionShape(center), mBox(Vector2(0,0), size.x, size.y)
		{
			/// \todo that's not really efficient, using AABBox with arbitrary center
			// but using relativePosition too
		}
	
		virtual AABBox getBoundingBox() const 
		{
			return mBox + getRelativePosition();
		}
		
		virtual bool isPointInside(Vector2 point) const 
		{
			/// \todo we need lengthSQ();
			return mBox.isPointInside(point - getRelativePosition());
		}
		
		virtual unsigned int getShapeType() const 
		{
			return BOX;
		}
		
	private:
		AABBox mBox;
};
