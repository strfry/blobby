#include "CollisionDetector.h"
#include <boost/weak_ptr.hpp>
#include <algorithm>
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
	
	// ensure that one and two are correctly ordered
	if( one->getCollisionType() > two->getCollisionType() ) 
		std::swap(one, two);
	
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
				
				if(cshape2->getShapeType() == BOX && cshape1->getShapeType() == SPHERE) 
				{
					const CollisionShapeSphere* sphere = (const CollisionShapeSphere*)cshape1;
					const CollisionShapeBox* box = (const CollisionShapeBox*)cshape2;
					
					Vector2 normal;
					
					if(collisionTestBoxSphere(statetwo, stateone, box, sphere, normal)) 
					{
						
						
						return TimedCollisionEvent{one, two, normal, substep / 16.f};
					}
				} else if(cshape1->getShapeType() == BOX && cshape2->getShapeType() == SPHERE) 
				{
					const CollisionShapeSphere* sphere = (const CollisionShapeSphere*)cshape2;
					const CollisionShapeBox* box = (const CollisionShapeBox*)cshape1;
					
					Vector2 normal;
					
					if(collisionTestBoxSphere(stateone, statetwo, box, sphere, normal)) 
					{
						
						
						return TimedCollisionEvent{one, two, normal, substep / 16.f};
					}
				} 
					else if(cshape1->getShapeType() == SPHERE && cshape2->getShapeType() == SPHERE) 
				{
					const CollisionShapeSphere* sphere1 = (const CollisionShapeSphere*)cshape1;
					const CollisionShapeSphere* sphere2 = (const CollisionShapeSphere*)cshape2;
					
					Vector2 normal;
					
					if(collisionTestSphereSphere(stateone, statetwo, sphere1, sphere2, normal)) 
					{
						
						
						return TimedCollisionEvent{one, two, normal, substep / 16.f};
					}
					
				}
			}
		}
		
	}
	
	return {0,0,Vector2(), -1};
}

bool CollisionDetector::collisionTestSphereSphere(PhysicObject::MotionState state1, 
												PhysicObject::MotionState state2,
												const CollisionShapeSphere* s1, 
												const CollisionShapeSphere* s2, 
												Vector2& norm)
{
	
	if(	(	state1.pos + s1->getRelativePosition() - 
			state2.pos - s2->getRelativePosition()	).length() <
							s1->getRadius() + s2->getRadius()) 
	{			
		norm = (state1.pos + s1->getRelativePosition() - 
					state2.pos - s2->getRelativePosition()).normalise();
		
		return true;
	}
	
	return false;
}

float clamp(float val, float min, float max) 
{
	return std::max(std::min(max, val), min);
}

bool CollisionDetector::collisionTestBoxSphere(PhysicObject::MotionState state1, 
											PhysicObject::MotionState state2,
											const CollisionShapeBox* boxshape, 
											const CollisionShapeSphere* sp, 
											Vector2& normal)
{
	AABBox box = boxshape->getBoundingBox() + state2.pos;
	Vector2 circle = sp->getRelativePosition() + state1.pos;
	
	// closest point to circle within box
	float nearx = clamp(circle.x, box.upperLeft.x, box.lowerRight.x);
	float neary = clamp(circle.y, box.upperLeft.y, box.lowerRight.y);

	// distance between circle center and closest point
	float dx = circle.x - nearx;
	float dy = circle.y - neary;
	
	// If the distance is less than the circle's radius, an intersection occurs
	if(dx*dx + dy*dy > (sp->getRadius() * sp->getRadius()))
		return false;
	
	normal = (circle - Vector2(nearx, neary)).normalise();
	return true;
}
