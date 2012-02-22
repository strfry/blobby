#include "PhysicObject.h"
#include <boost/weak_ptr.hpp>
#include <iostream>

PhysicObject::PhysicObject(const Vector2& p, const Vector2& v):
	mPosition(p),
	mVelocity(v),
	mConnectedWorld(0)
{
}

// ----------------------------
// motion state getters/setters

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

// ---------------------------------

void PhysicObject::setDebugName(const std::string& name)
{
	mDebugName = name;
}

const std::string& PhysicObject::getDebugName() const
{
	return mDebugName;
}

void PhysicObject::setWorld(const PhysicWorld* world)
{
	mConnectedWorld = world;
}

const PhysicWorld* PhysicObject::getWorld() const
{
	return mConnectedWorld;	
}

void PhysicObject::addCollisionShape( boost::shared_ptr<ICollisionShape> newshape )
{
	mCollisionShapes.push_back(newshape);
	mBoundingBox.merge(newshape->getBoundingBox());
}

void PhysicObject::clearCollisionShapes()
{
	mCollisionShapes.clear();
	mBoundingBox.clear();
}

int PhysicObject::getCollisionShapeCount() const
{
	return mCollisionShapes.size();
}

boost::weak_ptr<const ICollisionShape> PhysicObject::getCollisionShape(int i) const
{
	return mCollisionShapes[i];
}

AABBox PhysicObject::getBoundingBox() const
{
	return mBoundingBox;
}

PhysicObject::MotionState PhysicObject::getMotionState() const
{
	return MotionState{mPosition, mVelocity, this};
}

PhysicObject::MotionState PhysicObject::getPredictedMotionState(float time) const
{
	return MotionState{mPosition + mVelocity * time + mAcceleration / 2 * time * time, mVelocity + mAcceleration * time, this};
}

void PhysicObject::step(float time)
{
	mPosition += mVelocity * time + mAcceleration / 2 * time * time;
	mVelocity += mAcceleration * time;
	
	/*if(mPosition.x < 0 || mPosition.x > 800 )
	{
		mVelocity.x *= -1;
	}
	if(mPosition.y > 500)
	{
		mVelocity.y *= -1;
	}*/
	/*
	HitData hitInfo;
	
	// now, check for collisions with walls
	for(int i = 0; i < mWalls.size(); ++i)
	{
		if(mWalls[i].wall->hitTest(mPosition, mVelocity, mRadius, hitInfo))
		{
			mWalls[i].handler->onHit(*this, *mWalls[i].wall, hitInfo);
		}
	}*/
}
