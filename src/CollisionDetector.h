#pragma once

#include <vector>
#include "Vector.h"
#include "PhysicObject.h"

typedef std::pair<const PhysicObject*, const PhysicObject*> BroadphaseCollisionEvent;
typedef std::vector<BroadphaseCollisionEvent> BroadphaseCollisonArray;

/// guarantees that first->getCollisionType() <= second->getCollisionType()
struct TimedCollisionEvent
{
	// the objects
	const PhysicObject* first;
	const PhysicObject* second;
	
	// hit information
	Vector2 impactNormal;

	// time when collision occurs
	float time;
	
	bool operator<(const TimedCollisionEvent& other) const
	{
		return time < other.time;
	}
};

class CollisionDetector
{
	public:
		BroadphaseCollisonArray getCollisionEventsBroadphase(const std::vector<PhysicObject>& objects);
		
		TimedCollisionEvent checkCollision(BroadphaseCollisionEvent broadphase);
		
	private:
		bool collisionTestSphereSphere(PhysicObject::MotionState state1, PhysicObject::MotionState state2,
										const CollisionShapeSphere* s1, const CollisionShapeSphere* s2, 
										Vector2& norm);
		bool collisionTestBoxSphere(PhysicObject::MotionState state1, PhysicObject::MotionState state2,
										const CollisionShapeBox* box, const CollisionShapeSphere* sp, 
										Vector2& norm);
		
		
};
