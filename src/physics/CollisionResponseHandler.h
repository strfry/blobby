#pragma once

#include <algorithm>
#include <iostream>

class ICollisionResponseHandler
{
	public:
		virtual ~ICollisionResponseHandler() {};
		virtual Vector2 getImpulse(Vector2 normal, PhysicObject* obj1, PhysicObject* obj2) = 0;
};

class CollisionResponseHandlerBlobby : public ICollisionResponseHandler
{
	public:
	CollisionResponseHandlerBlobby() : savedImpulse(13)
	{
		
	}
	virtual Vector2 getImpulse(Vector2 normal, PhysicObject* obj1, PhysicObject* obj2)
	{
		// obj1 : Blobby
		// obj2 : Ball
		/*float incoming_vel = obj2->getVelocity().dotProduct(normal);
		if(incoming_vel > 1e-5) 
		{
			Vector2 impulse = obj2->getVelocity().normalise() * std::min(1.f, incoming_vel);
			return impulse;
		}
		else if(savedImpulse > 0)
		{
			//std::cout << "HANDLE: " << incoming_vel << " " << obj2->getVelocity().length() << " " << savedImpulse << "\n";
			Vector2 impulse = normal.normalise() * std::min(1.f, savedImpulse);
			savedImpulse -= std::min(1.f, savedImpulse);
			return impulse;
		}*/
		return 13 * normal + obj2->getVelocity();
	}
	
	float savedImpulse;
};

class CollisionResponseHandlerIdealElastic : public ICollisionResponseHandler
{
	public:
	CollisionResponseHandlerIdealElastic() : savedImpulse(Vector2(0,0))
	{
		
	}
	virtual Vector2 getImpulse(Vector2 normal, PhysicObject* obj1, PhysicObject* obj2)
	{
		float incoming_vel = obj1->getVelocity().dotProduct(-normal);
		/*if(incoming_vel > 0) 
		{
			Vector2 impulse = normal.normalise() * std::min(1.f, incoming_vel);
			savedImpulse += impulse;
			return impulse;
		}
		else
		{
			Vector2 impulse = savedImpulse.normalise() * std::min(1.f, savedImpulse.length());
			savedImpulse -= impulse;
			return impulse;
		}*/
		return normal * incoming_vel * 2; 
	}
	
	Vector2 savedImpulse;
};
