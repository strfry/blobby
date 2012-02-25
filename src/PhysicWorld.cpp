#include "PhysicWorld.h"
#include "CollisionDetector.h"
#include "PhysicConstraint.h"
#include "GameConstants.h"
#include "SDL/SDL.h"

#include <algorithm>
#include <deque>
#include <boost/weak_ptr.hpp>
#include <iostream>

#include "BlobbyDebug.h"

PhysicWorld::PhysicWorld()
{
	mObjects.resize(5);
	mObjects[0].setDebugName("Left Blobby");
	mObjects[0].setPosition( Vector2(200, 400) );
	mObjects[0].setAcceleration( Vector2(0, GRAVITATION) );
	mObjects[0].setVelocity( Vector2(0, 0) );
	boost::shared_ptr<ICollisionShape> body (new CollisionShapeSphere(BLOBBY_LOWER_RADIUS, Vector2(0, BLOBBY_LOWER_SPHERE)));	
	boost::shared_ptr<ICollisionShape> head (new CollisionShapeSphere(BLOBBY_UPPER_RADIUS, Vector2(0, -BLOBBY_UPPER_SPHERE)));	
	mObjects[0].addCollisionShape( body );
	mObjects[0].addCollisionShape( head );
	mObjects[0].setCollisionType(1);
	mObjects[0].setInverseMass(0.01);
	boost::shared_ptr<IPhysicConstraint> con1 (new HorizontalFieldBoundaryConstraint(0, 400 - NET_RADIUS - BLOBBY_LOWER_RADIUS));
	boost::shared_ptr<IPhysicConstraint> ground (new GroundConstraint(500));
	mObjects[0].addConstraint (con1);
	mObjects[0].addConstraint (ground);
	mObjects[0].setWorld(this);
	
	mObjects[1].setDebugName("Right Blobby");
	mObjects[1].setPosition( Vector2(600, 400) );
	mObjects[1].setVelocity( Vector2(0, 0) );
	mObjects[1].setAcceleration( Vector2(0, GRAVITATION) );	
	mObjects[1].addCollisionShape( body );
	mObjects[1].addCollisionShape( head );
	mObjects[1].setCollisionType(1);
	boost::shared_ptr<IPhysicConstraint> con2 (new HorizontalFieldBoundaryConstraint(400 + NET_RADIUS + BLOBBY_LOWER_RADIUS, 800));
	mObjects[1].addConstraint (con2);
	mObjects[1].addConstraint (ground);
	mObjects[1].setInverseMass(0.01);
	mObjects[1].setWorld(this);
	
	mObjects[2].setDebugName("Ball");
	mObjects[2].setPosition( Vector2(700, 454 + 13) );
	mObjects[2].setRotation( 0 );
	mObjects[2].setAngularVelocity( 0.1 );
	mObjects[2].setAcceleration( Vector2(0, 0) );
	mObjects[2].setVelocity( Vector2(0, 0) );
	boost::shared_ptr<ICollisionShape> sp2 (new CollisionShapeSphere(BALL_RADIUS));
	mObjects[2].addCollisionShape( sp2 );
	mObjects[2].setCollisionType(2);
	boost::shared_ptr<IPhysicConstraint> con3 (new HorizontalFieldBoundaryConstraint(BALL_RADIUS, 800 - BALL_RADIUS, 1));
	mObjects[2].addConstraint (con3);
	mObjects[2].setInverseMass(0.1);
	mObjects[2].setWorld(this);
	
	mObjects[3].setDebugName("Net");
	mObjects[3].setPosition( Vector2(400, 500) );
	boost::shared_ptr<ICollisionShape> nc (new CollisionShapeBox( Vector2( 2*NET_RADIUS, 2*(500-NET_SPHERE_POSITION))));
	mObjects[3].addCollisionShape( nc );
	boost::shared_ptr<ICollisionShape> nt (new CollisionShapeSphere( NET_RADIUS, Vector2(0, 500-NET_SPHERE_POSITION)) );
	mObjects[3].addCollisionShape( nt );
	mObjects[3].setCollisionType(3);
	mObjects[3].setWorld(this);
	mObjects[3].setInverseMass(0);
	
	mObjects[4].setDebugName("Ground");
	mObjects[4].setPosition( Vector2(400, 700) );
	boost::shared_ptr<ICollisionShape> gr (new CollisionShapeBox( Vector2( 1000, 400)));
	mObjects[4].addCollisionShape( gr );
	mObjects[4].setCollisionType(4);
	mObjects[4].setWorld(this);
	mObjects[4].setInverseMass(0);
}

PhysicWorld::~PhysicWorld()
{
}

void PhysicWorld::step() 
{
	// at first, do some broadphase collision detection:
	//  we determine all pairs of objects that will intersec if the objects move without interacting
	//  with each other.
	
	float curtime = 0;
	
	BroadphaseCollisonArray col_candidates = collisionDetector.getCollisionEventsBroadphase(mObjects);
	
	int hc = 0;
	if(!col_candidates.empty())
	{
		// now we've got an array of possible collision pairs. Now we need some more precise checks.
		// we want to determine in which order the collisions happen and when exactly the will occur.
		// with that information, we are able to sort the collision events

		std::deque<TimedCollisionEvent> timed_hits;
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

		while(!timed_hits.empty())
		{
			hc++;
			// now we have the hit events in chronological order.
			// let's handle them.
			
			
			// simulate till first impact
			for(auto j = mObjects.begin(); j != mObjects.end(); ++j)
			{
				j->step(timed_hits.begin()->time - curtime);
			}
			
			// no we need to do the corresponding collision response handler
			handleCollision(*timed_hits.begin());
			
			curtime = timed_hits.begin()->time;
			timed_hits.pop_front();
			
			/// \todo actually, we should look here if we can find a new collision which happens
			/// 		due to the changes in trajectories
		}
	}
	
	for(auto i = mObjects.begin(); i != mObjects.end(); ++i)
	{
		i->step(1 - curtime);
	}
	
	std::cout << "HC: " << hc << "\n";
	
	// reset all forces
	for(auto i = mObjects.begin(); i != mObjects.end(); ++i)
		i->clearForces();
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

PhysicObject& PhysicWorld::getBlobReference(PlayerSide side)
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

PhysicEvent PhysicWorld::getNextEvent()
{
	if(mEventQueue.empty()) 
	{
		return PhysicEvent{PhysicEvent::PE_NONE, 0, Vector2()};
	} 
	else 
	{
		PhysicEvent r = mEventQueue.front();
		mEventQueue.pop();
		return r;
	}
}

// -------------------------------------------------

float PhysicWorld::getBlobState(PlayerSide player) const
{
	return 0;
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

void PhysicWorld::handleCollision(TimedCollisionEvent event)
{
	if(event.second->getCollisionType() == 2) 
	{
		const_cast<PhysicObject*>(event.second)->setVelocity( -13 * event.impactNormal );
		mEventQueue.push(PhysicEvent{PhysicEvent::PE_BALL_HIT_BLOBBY, event.time, event.second->getPosition()});
	}
	else if(event.first->getCollisionType() == 2 && event.second->getCollisionType() == 3) 
	{
		//nethit
		const_cast<PhysicObject*>(event.first)->setVelocity( 
				event.first->getVelocity().reflect(event.impactNormal)) ;
	}
	else if
	(event.first->getCollisionType() == 2 && event.second->getCollisionType() == 4) 
	{
		PhysicObject* ball = const_cast<PhysicObject*>(event.first);
		// groundhit
		Vector2 vel = ball->getVelocity();
		Vector2 dc = vel.decompose(-event.impactNormal);
		float e1 = vel.length() * vel.length() +  ball->getAngularVelocity() * ball->getAngularVelocity() * BALL_RADIUS;
		float dp = dc.x * 1.5;
		dc.x *= -0.5;
		float mu = 1.0;
		/*
		for(int i=0; i < 100; ++i)
		{
			float cg = 0.01 * mu * dp;
			if(dc.y + BALL_RADIUS * ball->getAngularVelocity() > cg)
			{
				dc.y -= cg;
				ball->setAngularVelocity( ball->getAngularVelocity() - cg/BALL_RADIUS );
			}
			else if (dc.y + BALL_RADIUS * ball->getAngularVelocity() < -mu*dp)
			{
				dc.y += cg;
				ball->setAngularVelocity( ball->getAngularVelocity() + cg/BALL_RADIUS );
			} 
			else 
			{
				dc.y = BALL_RADIUS * ball->getAngularVelocity();
			}
		}*/
		//std::cout << ball->getAngularVelocity() << "\n";
		vel = dc.recompose(-event.impactNormal);
		
		float e2 = vel.length() * vel.length() +  ball->getAngularVelocity() * ball->getAngularVelocity() * BALL_RADIUS;
		//std::cout << e1 << " -> " << e2 << "\n";
		//assert( e2 <= e1 );
		
		ball->setVelocity( vel ) ;
		mEventQueue.push(PhysicEvent{PhysicEvent::PE_BALL_HIT_GROUND, 0, event.first->getPosition()});
	}
	
	// deintersect
	int dbg_counter = 0;
	while(collisionDetector.hitTest(*event.first, *event.second))
	{
		const_cast<PhysicObject*>(event.first)->setPosition( event.first->getPosition() 
									+ event.impactNormal * event.first->getInverseMass() ) ;
		
		const_cast<PhysicObject*>(event.second)->setPosition( event.second->getPosition() 
									- event.impactNormal * event.second->getInverseMass() ) ;
		
		dbg_counter++;
		if(dbg_counter == 10000) {
			std::cout << "PROBLEM: " << event.impactNormal << "\n";
			break;
		}
	}
}
