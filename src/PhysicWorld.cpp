#include "PhysicWorld.h"
#include "CollisionDetector.h"
#include "GameConstants.h"

#include <algorithm>
#include <iostream>

PhysicWorld::PhysicWorld()
{
	mObjects.resize(3);
	mObjects[0].setDebugName("Left Blobby");
	mObjects[0].setPosition( Vector2(200, 300) );
	mObjects[0].setAcceleration( Vector2(0, 0*GRAVITATION) );
	mObjects[0].setVelocity( Vector2(5, 0) );
	boost::shared_ptr<ICollisionShape> body (new CollisionShapeSphere(BLOBBY_LOWER_RADIUS, Vector2(0, BLOBBY_LOWER_SPHERE)));	
	boost::shared_ptr<ICollisionShape> head (new CollisionShapeSphere(BLOBBY_UPPER_RADIUS, Vector2(0, -BLOBBY_UPPER_SPHERE)));	
	mObjects[0].addCollisionShape( body );
	mObjects[0].addCollisionShape( head );
	mObjects[0].setCollisionType(1);
	
	mObjects[1].setDebugName("Right Blobby");
	mObjects[1].setPosition( Vector2(600, 400) );
	mObjects[1].setVelocity( Vector2(3, -4) );
	mObjects[1].setAcceleration( Vector2(0, GRAVITATION) );	
	mObjects[1].addCollisionShape( body );
	mObjects[1].addCollisionShape( head );
	mObjects[1].setCollisionType(1);
	
	mObjects[2].setDebugName("Ball");
	mObjects[2].setPosition( Vector2(300, 300) );
	mObjects[2].setAcceleration( Vector2(0, 0*BALL_GRAVITATION) );
	mObjects[2].setVelocity( Vector2(0, 0) );
	boost::shared_ptr<ICollisionShape> sp2 (new CollisionShapeSphere(BALL_RADIUS));
	mObjects[2].addCollisionShape( sp2 );
	mObjects[2].setCollisionType(2);
}

PhysicWorld::~PhysicWorld()
{
}

void PhysicWorld::step() 
{
	static CollisionDetector collisionDetector;

	// at first, do some broadphase collision detection:
	//  we determine all pairs of objects that will intersec if the objects move without interacting
	//  with each other.
	
	float curtime = 0;
	
	BroadphaseCollisonArray col_candidates = collisionDetector.getCollisionEventsBroadphase(mObjects);
	
	if(!col_candidates.empty())
	{
		// now we've got an array of possible collision pairs. Now we need some more precise checks.
		// we want to determine in which order the collisions happen and when exactly the will occur.
		// with that information, we are able to sort the collision events

		std::vector<TimedCollisionEvent> timed_hits;
		for(auto i = col_candidates.begin(); i != col_candidates.end(); ++i)
		{
			TimedCollisionEvent evt = collisionDetector.checkCollision(*i);
			// if its real
			if(evt.first) 
			{
				timed_hits.push_back(evt);
			}
		}

		std::sort(timed_hits.begin(), timed_hits.end());

		// now we have the hit events in chronological order.
		// let's handle them.
		
		for(auto i = timed_hits.begin(); i != timed_hits.end(); ++i)
		{
			// simulate 
			for(auto j = mObjects.begin(); j != mObjects.end(); ++j)
			{
				j->step(i->time - curtime);
			}
			
			// no we need to do the corresponding collision response handler
			
			std::cout << i->time << "\n";
			//const_cast<PhysicObject*>(i->second)->setVelocity( -i->second->getVelocity() );
			if(i->second->getCollisionType() == 2) {
				const_cast<PhysicObject*>(i->second)->setVelocity( Vector2(13,0) );
			}
			//const_cast<PhysicObject*>(i->first)->setAcceleration( Vector2(0,0) );
			//const_cast<PhysicObject*>(i->second)->setAcceleration( Vector2(0,0) );
		}
	}
	
	for(auto i = mObjects.begin(); i != mObjects.end(); ++i)
	{
		i->step(1 - curtime);
	}
}

const PhysicObject& PhysicWorld::getBall() const
{
	return mObjects[2];
}

PhysicObject& PhysicWorld::getBallReference()
{
	return mObjects[2];
}

const PhysicObject& PhysicWorld::getBlob(PlayerSide side) const
{
	assert(side == LEFT_PLAYER || side == RIGHT_PLAYER);
	switch(side)
	{
		case LEFT_PLAYER:
			return mObjects[0];
		case RIGHT_PLAYER:
			return mObjects[1];
	}
	assert(0);
}

// -------------------------------------------------

bool PhysicWorld::getBlobJump(PlayerSide player) const
{
	
}
bool PhysicWorld::getBallActive() const
{
	
}

void PhysicWorld::setLeftInput(const PlayerInput& input)
{
	
}
void PhysicWorld::setRightInput(const PlayerInput& input)
{
	
}

float PhysicWorld::getBlobState(PlayerSide player) const
{
	return 0;
}
float PhysicWorld::getBallRotation() const
{
	return 0;
}

// These functions tell about ball collisions for game logic and sound
bool PhysicWorld::ballHitLeftPlayer() const
{
	return false;
}
bool PhysicWorld::ballHitRightPlayer() const
{
	return false;
}
bool PhysicWorld::ballHitLeftGround() const
{
	return false;
}
bool PhysicWorld::ballHitRightGround() const
{
	return false;
}

bool PhysicWorld::blobbyHitGround(PlayerSide player) const
{
	return false;
}

// Blobby animation methods
void PhysicWorld::blobbyAnimationStep(PlayerSide player)
{
	
}
void PhysicWorld::blobbyStartAnimation(PlayerSide player)
{
	
}

// This reports the intensity of the collision
// which was detected and also queried last.
float PhysicWorld::lastHitIntensity() const
{
	return 0;
}

// Here the game logic can decide whether the ball is valid.
// If not, no ball to player collision checking is done,
// the input is ignored an the ball experiences a strong damping
void PhysicWorld::setBallValidity(bool validity)
{
	
}

// This returns true if the ball is not valid and the ball is steady
bool PhysicWorld::roundFinished() const
{
	return false;
}

// This resets everything to the starting situation and
// wants to know, which player begins.
void PhysicWorld::reset(PlayerSide player)
{
	
}

// Set a new state received from server over a RakNet BitStream
void PhysicWorld::setState(RakNet::BitStream* stream)
{
	
}

// Fill a Bitstream with the state
void PhysicWorld::getState(RakNet::BitStream* stream) const
{
	
}

// Fill a Bitstream with a side reversed state
void PhysicWorld::getSwappedState(RakNet::BitStream* stream) const
{
	
}

