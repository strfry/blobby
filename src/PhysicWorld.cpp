#include "PhysicWorld.h"
#include "PhysicWall.h"
#include "GameConstants.h"

PhysicWorld::PhysicWorld()
{
	mObjects.resize(3);
	mObjects[2].setDebugName("Ball");
	mObjects[2].setPosition( Vector2(400, 300) );
	mObjects[2].setAcceleration( Vector2(0, BALL_GRAVITATION) );
	mObjects[2].setVelocity( Vector2(10, -2) );
	mObjects[2].setRadius( BALL_RADIUS );
	
	mObjects[2].addWall(new PhysicWall(PhysicWall::HORIZONTAL, 500));
	mObjects[2].addWall(new PhysicWall(PhysicWall::VERTICAL, 800));
	mObjects[2].addWall(new PhysicWall(PhysicWall::VERTICAL, 0));
}

PhysicWorld::~PhysicWorld()
{
	
}

void PhysicWorld::step() 
{
	for(int i=0; i < mObjects.size(); ++i)
	{
		mObjects[i].step();	
	}
}

const PhysicObject& PhysicWorld::getBall() const
{
	return mObjects[2];
}

// -------------------------------------------------


Vector2 PhysicWorld::getBallVelocity() const
{
	
}
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

Vector2 PhysicWorld::getBlob(PlayerSide player) const
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

float PhysicWorld::getBallSpeed() const
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

// This resets the player to their starting positions
void PhysicWorld::resetPlayer()
{
	
}

// For reducing ball speed after rule violation
void PhysicWorld::dampBall()
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

