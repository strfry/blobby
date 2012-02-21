#include "PhysicObject.h"

PhysicObject::PhysicObject(const Vector2& p, const Vector2& v):
	mPosition(p),
	mVelocity(v)
{
	
}

void PhysicObject::setPosition(const Vector2& np)
{
	mPosition = np;
}

void PhysicObject::setVelocity(const Vector2& nv)
{
	mVelocity = nv;
}

void PhysicObject::setAcceleration( const Vector2& na)
{
	mAcceleration = na;
}

const Vector2& PhysicObject::getPosition() const
{
	return mPosition;
}

const Vector2& PhysicObject::getVelocity() const
{
	return mVelocity;
}

const Vector2& PhysicObject::getAcceleration() const
{
	return mAcceleration;
}

Vector2& PhysicObject::getPosition()
{
	return mPosition;
}

Vector2& PhysicObject::getVelocity() 
{
	return mVelocity;
}

void PhysicObject::setDebugName(const std::string& name)
{
	mDebugName = name;
}

const std::string& PhysicObject::getDebugName() const
{
	return mDebugName;
}

void PhysicObject::addWall(PhysicWall* pw)
{
	mWalls.push_back(pw);
}

void PhysicObject::step()
{
	mPosition += mVelocity + mAcceleration / 2;
	mVelocity += mAcceleration;
}
