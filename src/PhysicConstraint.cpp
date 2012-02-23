#include "PhysicConstraint.h"
#include "PhysicObject.h"

bool HorizontalFieldBoundaryConstraint::checkAndCorrectConstraint(PhysicObject& object) const 
{
	Vector2 pos = object.getPosition();
	Vector2 vel = object.getVelocity();
	vel.x *= -mBouncing;
	if( pos.x < mLowerBound ) {
		pos.x = mLowerBound;
		object.setPosition( pos );
		object.setVelocity( vel );
		return true;
	}
	
	if( pos.x > mUpperBound ) {
		pos.x = mUpperBound;
		object.setPosition( pos );
		object.setVelocity( vel );
		return true;
	}
	
	return false;
}

bool GroundConstraint::checkAndCorrectConstraint(PhysicObject& object) const
{
	Vector2 pos = object.getPosition();
	Vector2 vel = object.getVelocity();
	if( pos.y + object.getBoundingBox().lowerRight.y > mGroundHeight ) {
		pos.y = mGroundHeight - object.getBoundingBox().lowerRight.y;
		vel.y = 0;
		object.setPosition( pos );
		object.setVelocity( vel );
		return true;
	}
	
	return false;
}
