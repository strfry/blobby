#include "CollisionDetector.h"
#include <boost/weak_ptr.hpp>
#include <algorithm>
#include <iostream>

BroadphaseCollisonArray CollisionDetector::getCollisionEventsBroadphase(const std::vector<PhysicObject>& objects, float timestep)
{
	BroadphaseCollisonArray collisions_found;
	
	// first, we have to compute future position
	std::vector<PhysicObject::MotionState> future_motion_states;
	future_motion_states.resize(objects.size());
	
	for(int i=0; i < objects.size(); ++i)
	{
		future_motion_states[i] = objects[i].getPredictedMotionState(timestep);
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

TimedCollisionEvent CollisionDetector::checkCollision(BroadphaseCollisionEvent broadphase, float timestep)
{
	const PhysicObject* one = broadphase.first;
	const PhysicObject* two = broadphase.second;
	
	// ensure that one and two are correctly ordered
	if( one->getCollisionType() > two->getCollisionType() ) 
		std::swap(one, two);
	
	/// \todo speed up this with a binary search
	for(int substep = 0; substep < 4; ++substep)
	{
		PhysicObject::MotionState stateone = one->getPredictedMotionState( timestep * substep / 4.f );
		PhysicObject::MotionState statetwo = two->getPredictedMotionState( timestep * substep / 4.f );
		
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
					
					if(collisionTestBoxSphere(statetwo.pos, stateone.pos, box, sphere, normal)) 
					{
						
						
						return TimedCollisionEvent{one, two, -normal, substep / 16.f};
					}
				} else if(cshape1->getShapeType() == BOX && cshape2->getShapeType() == SPHERE) 
				{
					const CollisionShapeSphere* sphere = (const CollisionShapeSphere*)cshape2;
					const CollisionShapeBox* box = (const CollisionShapeBox*)cshape1;
					
					Vector2 normal;
					
					if(collisionTestBoxSphere(stateone.pos, statetwo.pos, box, sphere, normal)) 
					{
						return TimedCollisionEvent{one, two, normal, substep / 16.f};
					}
				} 
					else if(cshape1->getShapeType() == SPHERE && cshape2->getShapeType() == SPHERE) 
				{
					const CollisionShapeSphere* sphere1 = (const CollisionShapeSphere*)cshape1;
					const CollisionShapeSphere* sphere2 = (const CollisionShapeSphere*)cshape2;
					
					Vector2 normal;
					
					if(collisionTestSphereSphere(stateone.pos, statetwo.pos, sphere1, sphere2, normal)) 
					{
						
						
						return TimedCollisionEvent{one, two, normal, substep / 16.f};
					}
					
				}
			}
		}
		
	}
	
	return {0,0,Vector2(), -1};
}

bool CollisionDetector::hitTest(const PhysicObject& one, const PhysicObject& two)
{
	PhysicObject::MotionState stateone = one.getMotionState( );
	PhysicObject::MotionState statetwo = two.getMotionState( );
	
	// test each shape against every other one
	for(int i = 0; i < one.getCollisionShapeCount(); ++i)
	{
		auto shape1 = one.getCollisionShape(i);
		for(int j = 0; j < two.getCollisionShapeCount(); ++j)
		{
			auto shape2 = two.getCollisionShape(j);
			
			// collision algorithm
			const ICollisionShape* cshape1 = shape1.lock().get();
			const ICollisionShape* cshape2 = shape2.lock().get();
			
			if(cshape2->getShapeType() == BOX && cshape1->getShapeType() == SPHERE) 
			{
				const CollisionShapeSphere* sphere = (const CollisionShapeSphere*)cshape1;
				const CollisionShapeBox* box = (const CollisionShapeBox*)cshape2;
				
				Vector2 normal;
				
				if(collisionTestBoxSphere(statetwo.pos, stateone.pos, box, sphere, normal)) 
				{
					return true;
				}
			} else if(cshape1->getShapeType() == BOX && cshape2->getShapeType() == SPHERE) 
			{
				const CollisionShapeSphere* sphere = (const CollisionShapeSphere*)cshape2;
				const CollisionShapeBox* box = (const CollisionShapeBox*)cshape1;
				
				Vector2 normal;
				
				if(collisionTestBoxSphere(stateone.pos, statetwo.pos, box, sphere, normal)) 
				{
					return true;
				}
			} 
				else if(cshape1->getShapeType() == SPHERE && cshape2->getShapeType() == SPHERE) 
			{
				const CollisionShapeSphere* sphere1 = (const CollisionShapeSphere*)cshape1;
				const CollisionShapeSphere* sphere2 = (const CollisionShapeSphere*)cshape2;
				
				Vector2 normal;
				
				if(collisionTestSphereSphere(stateone.pos, statetwo.pos, sphere1, sphere2, normal)) 
				{
					return true;
				}
				
			}
		}
	}
	
	return false;
}

bool CollisionDetector::collisionTestSphereSphere(Vector2 pos1, 
												Vector2 pos2,
												const CollisionShapeSphere* s1, 
												const CollisionShapeSphere* s2, 
												Vector2& norm)
{
	
	if(	(	pos1 + s1->getRelativePosition() - 
			pos2 - s2->getRelativePosition()	).length() <
							s1->getRadius() + s2->getRadius()) 
	{			
		norm = (pos1 + s1->getRelativePosition() - pos2 - s2->getRelativePosition()).normalise();
		
		return true;
	}
	
	return false;
}

float clamp(float val, float min, float max) 
{
	return std::max(std::min(max, val), min);
}

bool CollisionDetector::collisionTestBoxSphere(Vector2 pos1, 
											Vector2 pos2,
											const CollisionShapeBox* boxshape, 
											const CollisionShapeSphere* sp, 
											Vector2& normal)
{
	AABBox box = boxshape->getBoundingBox() + pos2;
	Vector2 circle = sp->getRelativePosition() + pos1;
	
	// closest point to circle within box
	float nearx = clamp(circle.x, box.upperLeft.x, box.lowerRight.x);
	float neary = clamp(circle.y, box.upperLeft.y, box.lowerRight.y);

	// distance between circle center and closest point
	float dx = circle.x - nearx;
	float dy = circle.y - neary;
	
	// If the distance is less than the circle's radius, an intersection occurs
	if(dx*dx + dy*dy > (sp->getRadius() * sp->getRadius()))
		return false;
	
	// if sphere center is inside box, we need a little more work for calculating the normal
	if( circle.x != nearx || circle.y != neary )
	{
		normal = (circle - Vector2(nearx, neary)).normalise();
		return true;
	}
	
	// normal has to point from cicle center to nearest surface point
	/// \todo optimize
	if( std::min(std::abs(circle.x - box.upperLeft.x), std::abs(circle.x - box.lowerRight.x)) < 
			std::min(std::abs(circle.y - box.upperLeft.y), std::abs(circle.y - box.lowerRight.y)))
	{
		if(2 * circle.x > box.upperLeft.x + box.lowerRight.x)
			normal = Vector2(1, 0);
		else
			normal = Vector2(-1, 0);
	} else {
		if(2 * circle.y > box.upperLeft.y + box.lowerRight.y)
			normal = Vector2(0, 1);
		else
			normal = Vector2(0, -1);
	}
}
