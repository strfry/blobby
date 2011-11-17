/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

#include "PhysicWorld.h"
#include "GameConstants.h"
#include "raknet/BitStream.h"

#include <limits>

const int TIMESTEP = 5; // calculations per frame

const float TIMEOUT_MAX = 2.5;

// Gamefeeling relevant constants:
const float BLOBBY_ANIMATION_SPEED = 0.5;

const float STANDARD_BALL_ANGULAR_VELOCITY = 0.1;
const float STANDARD_BALL_HEIGHT = 269 + BALL_RADIUS;

PhysicWorld::PhysicWorld() : mWorkingState(new PhysicWorldState()), mCurrentState(new PhysicWorldState())
{
	reset(LEFT_PLAYER);
	mWorkingState->mCurrentBlobbyAnimationSpeed[LEFT_PLAYER] = 0.0;
	mWorkingState->mCurrentBlobbyAnimationSpeed[RIGHT_PLAYER] = 0.0;
	mTimeSinceBallout = 0.0;
	
	finishState();
}

PhysicWorld::~PhysicWorld()
{
	delete mWorkingState;
	delete mCurrentState;
}

void PhysicWorld::finishState()
{
	// swap states
	std::swap(mCurrentState, mWorkingState);
	// copy data
	*mWorkingState = *mCurrentState;
}

bool PhysicWorld::resetAreaClear() const
{
	if (blobbyHitGround(LEFT_PLAYER) && blobbyHitGround(RIGHT_PLAYER))
		return true;
	return false;
}

void PhysicWorld::reset(PlayerSide player)
{
	if (player == LEFT_PLAYER)
		mWorkingState->mBallPosition = Vector2(200, STANDARD_BALL_HEIGHT);
	else if (player == RIGHT_PLAYER)
		mWorkingState->mBallPosition = Vector2(600, STANDARD_BALL_HEIGHT);
	else
		mWorkingState->mBallPosition = Vector2(400, 450);

	mWorkingState->mBallVelocity.clear();

	mWorkingState->mBallRotation = 0.0;
	mWorkingState->mBallAngularVelocity = STANDARD_BALL_ANGULAR_VELOCITY;
	mWorkingState->mBlobState[LEFT_PLAYER] = 0.0;
	mWorkingState->mBlobState[RIGHT_PLAYER] = 0.0;

	mIsGameRunning = false;
	mIsBallValid = true;

	mLastHitIntensity = 0.0;
}

void PhysicWorld::resetPlayer()
{
	mWorkingState->mBlobPosition[LEFT_PLAYER] = Vector2( 200,
		GROUND_PLANE_HEIGHT);
	mWorkingState->mBlobPosition[RIGHT_PLAYER] = Vector2(600,
		GROUND_PLANE_HEIGHT);
}

bool PhysicWorld::ballHitRightGround() const
{
	if (mIsBallValid)
		if (mWorkingState->mBallPosition.y > GROUND_PLANE_HEIGHT &&
			mWorkingState->mBallPosition.x > NET_POSITION_X)
			return true;
	return false;
}

bool PhysicWorld::ballHitLeftGround() const
{
	if (mIsBallValid)
		if (mWorkingState->mBallPosition.y > GROUND_PLANE_HEIGHT &&
			mWorkingState->mBallPosition.x < NET_POSITION_X)
			return true;
	return false;
}

bool PhysicWorld::blobbyHitGround(PlayerSide player) const
{
	if (player == LEFT_PLAYER)
	{
		if (getBlob(LEFT_PLAYER).y >= GROUND_PLANE_HEIGHT)
			return true;
		else
			return false;
	}
	else if (player == RIGHT_PLAYER)
	{
		if (getBlob(RIGHT_PLAYER).y >= GROUND_PLANE_HEIGHT)
			return true;
		else
			return false;
	}
	else
		return false;
}

void PhysicWorld::setBallValidity(bool validity)
{
	mIsBallValid = validity;
}

bool PhysicWorld::roundFinished() const
{
	if (resetAreaClear())
	{
		if (!mIsBallValid)
			if (mWorkingState->mBallVelocity.y < 1.5 &&
				mWorkingState->mBallVelocity.y > -1.5 && mWorkingState->mBallPosition.y > 430)
				return true;
	}
	if (mTimeSinceBallout > TIMEOUT_MAX)
		return true;
	return false;
}

float PhysicWorld::lastHitIntensity() const
{
	float intensity = mLastHitIntensity / 25.0;
	return intensity < 1.0 ? intensity : 1.0;
}

bool PhysicWorld::playerTopBallCollision(int player) const
{
	if (Vector2(mWorkingState->mBallPosition,
		Vector2(mWorkingState->mBlobPosition[player].x,
			mWorkingState->mBlobPosition[player].y - BLOBBY_UPPER_SPHERE)
			).length() <= BALL_RADIUS + BLOBBY_UPPER_RADIUS)
		return true;
	return false;
}

inline bool PhysicWorld::playerBottomBallCollision(int player) const
{
	if (Vector2(mWorkingState->mBallPosition,
		Vector2(mWorkingState->mBlobPosition[player].x,
			mWorkingState->mBlobPosition[player].y + BLOBBY_LOWER_SPHERE)
			).length() <= BALL_RADIUS + BLOBBY_LOWER_RADIUS)
		return true;
	return false;
}

bool PhysicWorld::ballHitLeftPlayer() const
{
	return mBallHitByBlob[LEFT_PLAYER];
}

bool PhysicWorld::ballHitRightPlayer() const 
{
	return mBallHitByBlob[RIGHT_PLAYER];
}

Vector2 PhysicWorld::getBall() const
{
	return mWorkingState->mBallPosition;
}

float PhysicWorld::getBallRotation() const
{
	return mWorkingState->mBallRotation;
}

float PhysicWorld::getBallSpeed() const
{
	return mWorkingState->mBallVelocity.length();
}

Vector2 PhysicWorld::getBlob(PlayerSide player) const
{
	return mWorkingState->mBlobPosition[player];
}

float PhysicWorld::getBlobState(PlayerSide player) const
{
	return mWorkingState->mBlobState[player];
}

void PhysicWorld::setLeftInput(const PlayerInput& input)
{
	mPlayerInput[LEFT_PLAYER] = input;
}

void PhysicWorld::setRightInput(const PlayerInput& input)
{
	mPlayerInput[RIGHT_PLAYER] = input;
}

// Blobby animation methods
void PhysicWorld::blobbyAnimationStep(PlayerSide player)
{
	if (mWorkingState->mBlobState[player] < 0.0)
	{
		mWorkingState->mCurrentBlobbyAnimationSpeed[player] = 0;
		mWorkingState->mBlobState[player] = 0;
	}
	if (mWorkingState->mBlobState[player] >= 4.5)
	{
		mWorkingState->mCurrentBlobbyAnimationSpeed[player]
			=- BLOBBY_ANIMATION_SPEED;
	}

	mWorkingState->mBlobState[player] += mWorkingState->mCurrentBlobbyAnimationSpeed[player];

	if (mWorkingState->mBlobState[player] >= 5)
	{
		mWorkingState->mBlobState[player] = 4.99;
	}
}

void PhysicWorld::blobbyStartAnimation(PlayerSide player)
{
	if (mWorkingState->mCurrentBlobbyAnimationSpeed[player] == 0)
		mWorkingState->mCurrentBlobbyAnimationSpeed[player] =
			BLOBBY_ANIMATION_SPEED;
}

void PhysicWorld::handleBlob(PlayerSide player)
{
	// Reset ball to blobby collision
	mBallHitByBlob[player] = false;

	if (mPlayerInput[player].up)
	{
		if (blobbyHitGround(player))
		{
			mWorkingState->mBlobVelocity[player].y = -BLOBBY_JUMP_ACCELERATION;
			blobbyStartAnimation(PlayerSide(player));
		}
		mWorkingState->mBlobVelocity[player].y -= BLOBBY_JUMP_BUFFER;
	}

	if ((mPlayerInput[player].left || mPlayerInput[player].right)
			&& blobbyHitGround(player))
	{
		blobbyStartAnimation(player);
	}

	mWorkingState->mBlobVelocity[player].x =
		(mPlayerInput[player].right ? BLOBBY_SPEED : 0) -
		(mPlayerInput[player].left ? BLOBBY_SPEED : 0);

	// Acceleration Integration
	mWorkingState->mBlobVelocity[player].y += GRAVITATION;

	// Compute new position
	mWorkingState->mBlobPosition[player] += mWorkingState->mBlobVelocity[player];

	if (mWorkingState->mBlobPosition[player].y > GROUND_PLANE_HEIGHT)
	{
		if(mWorkingState->mBlobVelocity[player].y > 3.5)
		{
			blobbyStartAnimation(player);
		}

		mWorkingState->mBlobPosition[player].y = GROUND_PLANE_HEIGHT;
		mWorkingState->mBlobVelocity[player].y = 0.0;
	}

	blobbyAnimationStep(player);
}

void PhysicWorld::checkBlobbyBallCollision(PlayerSide player)
{
	// Check for bottom circles
	if(playerBottomBallCollision(player))
	{
		mLastHitIntensity = Vector2(mWorkingState->mBallVelocity, mWorkingState->mBlobVelocity[player]).length();

		const Vector2& blobpos = mWorkingState->mBlobPosition[player];
		const Vector2 circlepos = Vector2(blobpos.x, blobpos.y + BLOBBY_LOWER_SPHERE);
		
		mWorkingState->mBallVelocity = -Vector2(mWorkingState->mBallPosition, circlepos);
		mWorkingState->mBallVelocity = mWorkingState->mBallVelocity.normalise();
		mWorkingState->mBallVelocity = mWorkingState->mBallVelocity.scale(BALL_COLLISION_VELOCITY);
		mWorkingState->mBallPosition += mWorkingState->mBallVelocity;
		mBallHitByBlob[player] = true;
	}
	else if(playerTopBallCollision(player))
	{
		mLastHitIntensity = Vector2(mWorkingState->mBallVelocity, mWorkingState->mBlobVelocity[player]).length();

		const Vector2& blobpos = mWorkingState->mBlobPosition[player];
		const Vector2 circlepos = Vector2(blobpos.x, blobpos.y - BLOBBY_UPPER_SPHERE);

		mWorkingState->mBallVelocity = -Vector2(mWorkingState->mBallPosition, circlepos);
		mWorkingState->mBallVelocity = mWorkingState->mBallVelocity.normalise();
		mWorkingState->mBallVelocity = mWorkingState->mBallVelocity.scale(BALL_COLLISION_VELOCITY);
		mWorkingState->mBallPosition += mWorkingState->mBallVelocity;
		mBallHitByBlob[player] = true;
	}

}

void PhysicWorld::step()
{
	// Determistic IEEE 754 floating point computations
	set_fpu_single_precision();

	// Compute independent actions
	handleBlob(LEFT_PLAYER);
	handleBlob(RIGHT_PLAYER);

	// Ball Gravitation
	if (mIsGameRunning)
		mWorkingState->mBallVelocity.y += BALL_GRAVITATION;

	// move ball
	mWorkingState->mBallPosition += mWorkingState->mBallVelocity;

	// Collision detection
	if(mIsBallValid)
	{
		checkBlobbyBallCollision(LEFT_PLAYER);
		checkBlobbyBallCollision(RIGHT_PLAYER);
	}
	// Ball to ground Collision
	else if (mWorkingState->mBallPosition.y + BALL_RADIUS > 500.0)
	{
		mWorkingState->mBallVelocity = mWorkingState->mBallVelocity.reflectY().scaleY(0.5);
		mWorkingState->mBallVelocity = mWorkingState->mBallVelocity.scaleX(0.55);
		mWorkingState->mBallPosition.y = 500 - BALL_RADIUS;
	}

	if (ballHitLeftPlayer() || ballHitRightPlayer())
		mIsGameRunning = true;
	
	// Border Collision
	if (mWorkingState->mBallPosition.x - BALL_RADIUS <= LEFT_PLANE && mWorkingState->mBallVelocity.x < 0.0)
	{
		mWorkingState->mBallVelocity = mWorkingState->mBallVelocity.reflectX();
		// set the ball's position
		mWorkingState->mBallPosition.x = LEFT_PLANE + BALL_RADIUS;
	}
	else if (mWorkingState->mBallPosition.x + BALL_RADIUS >= RIGHT_PLANE && mWorkingState->mBallVelocity.x > 0.0)
	{
		mWorkingState->mBallVelocity = mWorkingState->mBallVelocity.reflectX();
		// set the ball's position
		mWorkingState->mBallPosition.x = RIGHT_PLANE - BALL_RADIUS;
	}
	else if (mWorkingState->mBallPosition.y > NET_SPHERE_POSITION &&
			fabs(mWorkingState->mBallPosition.x - NET_POSITION_X) < BALL_RADIUS + NET_RADIUS)
	{
		mWorkingState->mBallVelocity = mWorkingState->mBallVelocity.reflectX();
		// set the ball's position so that it touches the net
		mWorkingState->mBallPosition.x = NET_POSITION_X + ((mWorkingState->mBallPosition.x - NET_POSITION_X > 0) ?( BALL_RADIUS + NET_RADIUS) : (- BALL_RADIUS - NET_RADIUS));
	}
	else
	{
		// Net Collisions

		float ballNetDistance = Vector2(mWorkingState->mBallPosition, Vector2(NET_POSITION_X, NET_SPHERE_POSITION)).length();

		if (ballNetDistance < NET_RADIUS + BALL_RADIUS)
		{ 
			// calculate
			Vector2 normal = Vector2(mWorkingState->mBallPosition,	Vector2(NET_POSITION_X, NET_SPHERE_POSITION)).normalise();
					
			// normal component of kinetic energy
			float perp_ekin = normal.dotProduct(mWorkingState->mBallVelocity);
			perp_ekin *= perp_ekin;
			// parallel component of kinetic energy
			float para_ekin = mWorkingState->mBallVelocity.length() * mWorkingState->mBallVelocity.length() - perp_ekin;
			
			// the normal component is damped stronger than the parallel component
			// the values are ~ 0.85² and ca. 0.95², because speed is sqrt(ekin)
			perp_ekin *= 0.7;
			para_ekin *= 0.9;
			
			float nspeed = sqrt(perp_ekin + para_ekin);
			
			mWorkingState->mBallVelocity = Vector2(mWorkingState->mBallVelocity.reflect(normal).normalise().scale(nspeed));
			
			// pushes the ball out of the net
			mWorkingState->mBallPosition = (Vector2(NET_POSITION_X, NET_SPHERE_POSITION) - normal * (NET_RADIUS + BALL_RADIUS));
		}
		// mBallVelocity = mBallVelocity.reflect( Vector2( mBallPosition, Vector2 (NET_POSITION_X, temp) ).normalise()).scale(0.75);
	}

	// Collision between blobby and the net
	if (mWorkingState->mBlobPosition[LEFT_PLAYER].x + BLOBBY_LOWER_RADIUS > NET_POSITION_X - NET_RADIUS) // Collision with the net
		mWorkingState->mBlobPosition[LEFT_PLAYER].x = NET_POSITION_X - NET_RADIUS - BLOBBY_LOWER_RADIUS;

	if (mWorkingState->mBlobPosition[RIGHT_PLAYER].x - BLOBBY_LOWER_RADIUS < NET_POSITION_X + NET_RADIUS)
		mWorkingState->mBlobPosition[RIGHT_PLAYER].x = NET_POSITION_X + NET_RADIUS + BLOBBY_LOWER_RADIUS;

	// Collision between blobby and the border
	if (mWorkingState->mBlobPosition[LEFT_PLAYER].x < LEFT_PLANE)
		mWorkingState->mBlobPosition[LEFT_PLAYER].x = LEFT_PLANE;

	if (mWorkingState->mBlobPosition[RIGHT_PLAYER].x > RIGHT_PLANE)
		mWorkingState->mBlobPosition[RIGHT_PLAYER].x = RIGHT_PLANE;

	// Velocity Integration
	if (mWorkingState->mBallVelocity.x > 0.0)
		mWorkingState->mBallRotation += mWorkingState->mBallAngularVelocity * (getBallSpeed() / 6);
	else if (mWorkingState->mBallVelocity.x < 0.0)
		mWorkingState->mBallRotation -= mWorkingState->mBallAngularVelocity * (getBallSpeed() / 6);
	else
		mWorkingState->mBallRotation -= mWorkingState->mBallAngularVelocity;

	// Overflow-Protection
	if (mWorkingState->mBallRotation <= 0)
		mWorkingState->mBallRotation = 6.25 + mWorkingState->mBallRotation;
	else if (mWorkingState->mBallRotation >= 6.25)
		mWorkingState->mBallRotation = mWorkingState->mBallRotation - 6.25;

	mTimeSinceBallout = mIsBallValid ? 0.0 :
		mTimeSinceBallout + 1.0 / 60;
		
	finishState();
}

void PhysicWorld::dampBall()
{
	mWorkingState->mBallVelocity = mWorkingState->mBallVelocity.scale(0.6);
}

Vector2 PhysicWorld::getBallVelocity() const
{
	return mWorkingState->mBallVelocity;
}

bool PhysicWorld::getBlobJump(PlayerSide player) const
{
	return !blobbyHitGround(player);
}

bool PhysicWorld::getBallActive() const
{
	return mIsGameRunning;
}

void writeCompressedToBitStream(RakNet::BitStream* stream, float value, float min, float max)
{
	assert(min <= value && value <= max);
	assert(stream);
	unsigned short only2bytes = static_cast<unsigned short>((value - min) / (max - min) * std::numeric_limits<unsigned short>::max());
	stream->Write(only2bytes);
}

void readCompressedFromBitStream(RakNet::BitStream* stream, float& value, float min, float max)
{
	unsigned short only2bytes;
	stream->Read(only2bytes);
	value = static_cast<float>(only2bytes) / static_cast<float>(std::numeric_limits<unsigned short>::max()) * (max - min) + min;
}


void PhysicWorld::setState(RakNet::BitStream* stream)
{
	bool leftGround;
	bool rightGround;
	stream->Read(leftGround);
	stream->Read(rightGround);
	if(leftGround){
		mWorkingState->mBlobPosition[LEFT_PLAYER].y = GROUND_PLANE_HEIGHT;
		mWorkingState->mBlobVelocity[LEFT_PLAYER].y = 0;
	}else{
		readCompressedFromBitStream(stream, mWorkingState->mBlobPosition[LEFT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		readCompressedFromBitStream(stream, mWorkingState->mBlobVelocity[LEFT_PLAYER].y, -30, 30);
	}
	
	if(rightGround){
		mWorkingState->mBlobPosition[RIGHT_PLAYER].y = GROUND_PLANE_HEIGHT;
		mWorkingState->mBlobVelocity[RIGHT_PLAYER].y = 0;
	}else{
		readCompressedFromBitStream(stream, mWorkingState->mBlobPosition[RIGHT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		readCompressedFromBitStream(stream, mWorkingState->mBlobVelocity[RIGHT_PLAYER].y, -30, 30);
	}
	
	readCompressedFromBitStream(stream, mWorkingState->mBlobPosition[LEFT_PLAYER].x, LEFT_PLANE, NET_POSITION_X);
	readCompressedFromBitStream(stream, mWorkingState->mBlobPosition[RIGHT_PLAYER].x, NET_POSITION_X, RIGHT_PLANE);
	
	readCompressedFromBitStream(stream, mWorkingState->mBallPosition.x, LEFT_PLANE, RIGHT_PLANE);
	// maybe these values is a bit too pessimistic...
	// but we have 65535 values, hence it should be precise enough
	readCompressedFromBitStream(stream, mWorkingState->mBallPosition.y, -500, GROUND_PLANE_HEIGHT_MAX);

	readCompressedFromBitStream(stream, mWorkingState->mBallVelocity.x, -30, 30);
	readCompressedFromBitStream(stream, mWorkingState->mBallVelocity.y, -30, 30);
	
	// if ball velocity not zero, we must assume that the game is active
	// i'm not sure if this would be set correctly otherwise...
	// we must use this check with 0.1f because of precision loss when velocities are transmitted
	// wo prevent setting a false value when the ball is at the parabels top, we check also if the 
	// y - position is the starting y position
	/// \todo maybe we should simply send a bit which contains this information? 
	if( std::abs(mWorkingState->mBallVelocity.x) > 0.1f || std::abs(mWorkingState->mBallVelocity.y) > 0.1f || std::abs(mWorkingState->mBallPosition.y - STANDARD_BALL_HEIGHT) > 0.1f) {
		mIsGameRunning = true;
	} else {
		mIsGameRunning = false;
	}

	stream->Read(mPlayerInput[LEFT_PLAYER].left);
	stream->Read(mPlayerInput[LEFT_PLAYER].right);
	stream->Read(mPlayerInput[LEFT_PLAYER].up);
	stream->Read(mPlayerInput[RIGHT_PLAYER].left);
	stream->Read(mPlayerInput[RIGHT_PLAYER].right);
	stream->Read(mPlayerInput[RIGHT_PLAYER].up);
}

void PhysicWorld::getState(RakNet::BitStream* stream) const
{
	// if the blobbys are standing on the ground, we need not send
	// y position and velocity
	stream->Write(blobbyHitGround(LEFT_PLAYER));
	stream->Write(blobbyHitGround(RIGHT_PLAYER));
	
	if(!blobbyHitGround(LEFT_PLAYER)){
		writeCompressedToBitStream(stream, mWorkingState->mBlobPosition[LEFT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		writeCompressedToBitStream(stream, mWorkingState->mBlobVelocity[LEFT_PLAYER].y, -30, 30);
	}
	
	if(!blobbyHitGround(RIGHT_PLAYER)){
		writeCompressedToBitStream(stream, mWorkingState->mBlobPosition[RIGHT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		writeCompressedToBitStream(stream, mWorkingState->mBlobVelocity[RIGHT_PLAYER].y, -30, 30);
	}
	
	writeCompressedToBitStream(stream, mWorkingState->mBlobPosition[LEFT_PLAYER].x, LEFT_PLANE, NET_POSITION_X);
	writeCompressedToBitStream(stream, mWorkingState->mBlobPosition[RIGHT_PLAYER].x, NET_POSITION_X, RIGHT_PLANE);
	
	writeCompressedToBitStream(stream, mWorkingState->mBallPosition.x, LEFT_PLANE, RIGHT_PLANE);
	writeCompressedToBitStream(stream, mWorkingState->mBallPosition.y, -500, GROUND_PLANE_HEIGHT_MAX);

	writeCompressedToBitStream(stream, mWorkingState->mBallVelocity.x, -30, 30);
	writeCompressedToBitStream(stream, mWorkingState->mBallVelocity.y, -30, 30);

	stream->Write(mPlayerInput[LEFT_PLAYER].left);
	stream->Write(mPlayerInput[LEFT_PLAYER].right);
	stream->Write(mPlayerInput[LEFT_PLAYER].up);
	stream->Write(mPlayerInput[RIGHT_PLAYER].left);
	stream->Write(mPlayerInput[RIGHT_PLAYER].right);
	stream->Write(mPlayerInput[RIGHT_PLAYER].up);

}

void PhysicWorld::getSwappedState(RakNet::BitStream* stream) const
{
	// if the blobbys are standing on the ground, we need not send
	// y position and velocity
	stream->Write(blobbyHitGround(RIGHT_PLAYER));
	stream->Write(blobbyHitGround(LEFT_PLAYER));
	
	if(!blobbyHitGround(RIGHT_PLAYER)){
		writeCompressedToBitStream(stream, mWorkingState->mBlobPosition[RIGHT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		writeCompressedToBitStream(stream, mWorkingState->mBlobVelocity[RIGHT_PLAYER].y, -30, 30);
	}
	
	if(!blobbyHitGround(LEFT_PLAYER)){
		writeCompressedToBitStream(stream, mWorkingState->mBlobPosition[LEFT_PLAYER].y, 0, GROUND_PLANE_HEIGHT);
		writeCompressedToBitStream(stream, mWorkingState->mBlobVelocity[LEFT_PLAYER].y, -30, 30);
	}
	
	writeCompressedToBitStream(stream, 800 - mWorkingState->mBlobPosition[RIGHT_PLAYER].x, LEFT_PLANE, NET_POSITION_X);
	writeCompressedToBitStream(stream, 800 - mWorkingState->mBlobPosition[LEFT_PLAYER].x, NET_POSITION_X, RIGHT_PLANE);
	
	writeCompressedToBitStream(stream, 800 - mWorkingState->mBallPosition.x, LEFT_PLANE, RIGHT_PLANE);
	writeCompressedToBitStream(stream, mWorkingState->mBallPosition.y, -500, GROUND_PLANE_HEIGHT_MAX);

	writeCompressedToBitStream(stream, -mWorkingState->mBallVelocity.x, -30, 30);
	writeCompressedToBitStream(stream, mWorkingState->mBallVelocity.y, -30, 30);

	stream->Write(mPlayerInput[RIGHT_PLAYER].right);
	stream->Write(mPlayerInput[RIGHT_PLAYER].left);
	stream->Write(mPlayerInput[RIGHT_PLAYER].up);
	stream->Write(mPlayerInput[LEFT_PLAYER].right);
	stream->Write(mPlayerInput[LEFT_PLAYER].left);
	stream->Write(mPlayerInput[LEFT_PLAYER].up);
}

const PlayerInput* PhysicWorld::getPlayersInput() const
{
	return mPlayerInput;
}

PhysicWorldState* PhysicWorld::getWorldState() const
{
	return mCurrentState;
}
