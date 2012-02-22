#include "CollisionDetector.h"
#include <boost/weak_ptr.hpp>
#include <iostream>

BroadphaseCollisonArray CollisionDetector::getCollisionEventsBroadphase(const std::vector<PhysicObject>& objects)
{
	BroadphaseCollisonArray collisions_found;
	
	// first, we have to compute future position
	std::vector<PhysicObject::MotionState> future_motion_states;
	future_motion_states.resize(objects.size());
	
	for(int i=0; i < objects.size(); ++i)
	{
		future_motion_states[i] = objects[i].getPredictedMotionState();
	}
	
	// now we check if any of the overlap
	for(auto i = future_motion_states.begin(); i != future_motion_states.end()-1; ++i)
	{
		for(auto j = i + 1; j < future_motion_states.end(); ++j)
		{
			if( (i->object->getBoundingBox() + i->pos).intersects(j->object->getBoundingBox() + j->pos))
			{
				collisions_found.push_back(BroadphaseCollisionEvent{i->object, j->object});
			}
		}
	}
	
	return collisions_found;
}

TimedCollisionEvent CollisionDetector::checkCollision(BroadphaseCollisionEvent broadphase)
{
	const PhysicObject* one = broadphase.first;
	const PhysicObject* two = broadphase.second;
	
	/// \todo speed up this with a binary search
	for(int substep = 0; substep < 16; ++substep)
	{
		PhysicObject::MotionState stateone = one->getPredictedMotionState( substep / 16.f );
		PhysicObject::MotionState statetwo = two->getPredictedMotionState( substep / 16.f );
		
		/// \todo use a precise hit test algorithm here
		// test each shape against every other one
		for(int i = 0; i < one->getCollisionShapeCount(); ++i)
		{
			auto shape1 = one->getCollisionShape(i);
			for(int j = 0; j < two->getCollisionShapeCount(); ++j)
			{
				auto shape2 = two->getCollisionShape(j);
				
				// collision algorithm
				const ICollisionShape* cshape1 = shape1.lock().get();
				const ICollisionShape* cshape2 = shape2.lock().get();
				
				const CollisionShapeSphere* sphere1 = (const CollisionShapeSphere*)cshape1;
				const CollisionShapeSphere* sphere2 = (const CollisionShapeSphere*)cshape2;
				
				if(	(	stateone.pos + sphere1->getRelativePosition() - 
						statetwo.pos - sphere2->getRelativePosition()		).length() <
										sphere1->getRadius() + sphere2->getRadius()) 
				{
					std::cout << stateone.pos.x << " " << one->getPosition().x << "\n";
					std::cout << statetwo.pos.x << " " << two->getPosition().x << "\n";
					
					std::cout << "SUB: " << substep / 16.f << "\n"; 
					std::cout << (	stateone.pos + sphere1->getRelativePosition() - 
						statetwo.pos - sphere2->getRelativePosition()		).length() << "\n";
					std::cout << sphere1->getRadius() + sphere2->getRadius() << "\n";
					return TimedCollisionEvent{one, two, substep / 16.f};
				}
			}
		}
		
	}
	
	return {0,0,-1};
}
