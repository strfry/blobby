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

void PhysicObject::addConstraint( boost::shared_ptr<IPhysicConstraint> cst )
{
	mConstraints.push_back(cst);
}

void PhysicObject::clearConstraints()
{
	mConstraints.clear();
}

int PhysicObject::getConstraintCount() const
{
	return mConstraints.size();
}

boost::weak_ptr<const IPhysicConstraint> PhysicObject::getConstraint(int i) const
{
	return mConstraints[i];
}

void PhysicObject::setCollisionType(unsigned int ct)
{
	mCollisionType = ct;
}
unsigned int PhysicObject::getCollisionType() const
{
	return mCollisionType;
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
	
	for(auto i = mConstraints.begin(); i != mConstraints.end(); ++i)
	{
		(*i)->checkAndCorrectConstraint(*this);
	}
}
