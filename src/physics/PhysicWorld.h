#pragma once

#include "PhysicObject.h"
#include "CollisionDetector.h"

#include "Global.h"
#include "InputSource.h"
#include <vector>
#include <queue>

struct PhysicEvent
{
	enum Type {
		PE_NONE,
		PE_BALL_HIT_BLOBBY,
		PE_BALL_HIT_GROUND
	} type;
	
	float time_substep;	// when did it happen
	Vector2 position;	// where did it happen
	
	operator bool() const {
		return type != PE_NONE;
	}
};

class TimedCollisionEvent;
class ICollisionResponseHandler;

class PhysicWorld
{
	public:
		PhysicWorld();
		~PhysicWorld();
	
		void step();
		
		const PhysicObject& getBall() const;
		PhysicObject& getBallReference();
		const PhysicObject& getBlob(PlayerSide player) const;
		PhysicObject& getBlobReference(PlayerSide player);
		
		PhysicEvent getNextEvent();
		
	private:
		std::vector<PhysicObject> mObjects;
		std::queue<PhysicEvent> mEventQueue;
		CollisionDetector collisionDetector;
		
		ICollisionResponseHandler* handleCollision(TimedCollisionEvent event);
		void step_impl(float timestep);
};

