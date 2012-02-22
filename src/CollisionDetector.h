#pragma once

#include <vector>
#include "Vector.h"
#include "PhysicObject.h"

typedef std::pair<const PhysicObject*, const PhysicObject*> BroadphaseCollisionEvent;
typedef std::vector<BroadphaseCollisionEvent> BroadphaseCollisonArray;

struct TimedCollisionEvent
{
	// the objects
	const PhysicObject* first;
	const PhysicObject* second;

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
};
