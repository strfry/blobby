#include "PhysicObject.h"

void PhysicObject::setPosition(const Vector2& np)
{
	mPosition = np;
}

void PhysicObject::setVelocity(const Vector2& nv)
{
	mVelocity = nv;
}

const Vector2& PhysicObject::getPosition() const
{
	return mPosition;
}

const Vector2& PhysicObject::getVelocity() const
{
	return mVelocity;
}

Vector2& PhysicObject::getPosition()
{
	return mPosition;
}

Vector2& PhysicObject::getVelocity() 
{
	return mVelocity;
}
