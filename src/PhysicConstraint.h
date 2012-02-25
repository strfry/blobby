#pragma once

#include "Vector.h"

class PhysicObject;

class IPhysicConstraint
{
	public:
		virtual ~IPhysicConstraint() {};
		virtual bool checkAndCorrectConstraint(PhysicObject& object) const = 0;
};

class HorizontalFieldBoundaryConstraint : public IPhysicConstraint
{
	public:
		HorizontalFieldBoundaryConstraint(float min, float max, float bounce = 0) : 
				mLowerBound(min), mUpperBound(max), mBouncing(bounce)
		{
		}
		
		virtual bool checkAndCorrectConstraint(PhysicObject& object) const;
	
	private:
		float mLowerBound;
		float mUpperBound;
		float mBouncing;
};

class GroundConstraint : public IPhysicConstraint
{
	public:
		GroundConstraint(float ground) : mGroundHeight(ground)
		{
		}
		
		virtual bool checkAndCorrectConstraint(PhysicObject& object) const;
	
	private:
		float mGroundHeight;
};

class FixationConstraint : public IPhysicConstraint
{
	public:
		FixationConstraint()
		{
		}
		
		virtual bool checkAndCorrectConstraint(PhysicObject& object) const;
};
