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

#include "IUserConfigReader.h"
#include "DuelMatch.h"
#include "UserConfig.h"
#include "GameConstants.h"

#include <cassert>
#include <iostream>

DuelMatch* DuelMatch::mMainGame = 0;

DuelMatch::DuelMatch(InputSource* linput, InputSource* rinput,
				bool global, bool remote):mLogic(createGameLogic("rules.lua")),
					mPaused(false), events(0), external_events(0), mRemote(remote)
{
	mGlobal = global;
	if (mGlobal)
	{
		assert(mMainGame == 0);
		mMainGame = this;
	}

	mLeftInput = linput;
	mRightInput = rinput;

	mBallDown = false;
	mIsGameRunning = false;

	mPhysicWorld.step();

	/// \todo we better pass this as a parameter so DuelMatch has no coupeling with UserConfigs...s
	mLogic->setScoreToWin(IUserConfigReader::createUserConfigReader("config.xml")->getInteger("scoretowin"));
}

DuelMatch::~DuelMatch()
{
	if (mGlobal)
	{
		mMainGame = 0;
	}
}

DuelMatch* DuelMatch::getMainGame()
{
	return mMainGame;
}

void DuelMatch::step()
{
	events = external_events;

	// do steps in physic an logic
	if (mLeftInput)
	{
		PhysicObject& blob = mPhysicWorld.getBlobReference(LEFT_PLAYER);
		Vector2 vel = blob.getVelocity();
		vel.x = mLeftInput->getInput().left ? -4.5 : 0 - mLeftInput->getInput().right ? 4.5 : 0;
		
		if(mLeftInput->getInput().up)
		{
			blob.addForce( Vector2(0, -BLOBBY_JUMP_BUFFER) );
			if(!getBlobJump(LEFT_PLAYER))
				vel.y = -BLOBBY_JUMP_ACCELERATION;
		}
		blob.setVelocity(vel);
	}
	
	if (mRightInput)
	{
		PhysicObject& blob = mPhysicWorld.getBlobReference(RIGHT_PLAYER);
		Vector2 vel = blob.getVelocity();
		vel.x = mRightInput->getInput().left ? -4.5 : 0 - mRightInput->getInput().right ? 4.5 : 0;
		
		if( mRightInput->getInput().up)
		{
			blob.addForce( Vector2(0, -BLOBBY_JUMP_BUFFER) );
			if(!getBlobJump(RIGHT_PLAYER) )
				vel.y = -BLOBBY_JUMP_ACCELERATION;
		}
		blob.setVelocity(vel);
	}
		
	// in pause mode, step does nothing except input being set
	/// \todo find out why we do set input in pause mode!
	if(mPaused)
		return;
	
	// update gravity
	std::cout << " run game: " << mIsGameRunning << " " << mBallDown << "\n";
	if(mIsGameRunning) 
	{
		mPhysicWorld.getBallReference().addForce( Vector2(0, BALL_GRAVITATION));
	}
	mPhysicWorld.getBlobReference(LEFT_PLAYER).addForce( Vector2(0, GRAVITATION));
	mPhysicWorld.getBlobReference(RIGHT_PLAYER).addForce( Vector2(0, GRAVITATION));
	
	mPhysicWorld.step();
	mLogic->step();
	
	// check for all hit events
	// don't process events if ball is already down
	if(!mRemote && !mBallDown)
	{
		// create game events
		// ball/player hit events:
		
		while( PhysicEvent evt = mPhysicWorld.getNextEvent()) 
		{
			switch(evt.type) {
				case PhysicEvent::PE_BALL_HIT_BLOBBY:
					mIsGameRunning = true;
					if(evt.position.x < 400) 
					{
						events |= ~(!mLogic->isCollisionValid(LEFT_PLAYER)) & EVENT_LEFT_BLOBBY_HIT;
						std::cout << "LEFT BLOBBY BALL HIT" << mLogic->isCollisionValid(LEFT_PLAYER) << "\n";
					}
					 else 
					{
						events |= ~(!mLogic->isCollisionValid(RIGHT_PLAYER)) & EVENT_RIGHT_BLOBBY_HIT;
						std::cout << "RIGHT BLOBBY BALL HIT "<< mLogic->isCollisionValid(RIGHT_PLAYER) << "\n";
					}
					break;
				case PhysicEvent::PE_BALL_HIT_GROUND:
					std::cout << "BALL HIT GROUND\n";
					if(evt.position.x < 400) 
					{
						events |= EVENT_BALL_HIT_LEFT_GROUND;
					}
					 else 
					{
						events |= EVENT_BALL_HIT_RIGHT_GROUND;
					}
					break;
			}
		}
		
	} else {
		// just clear the event queue
		while( PhysicEvent evt = mPhysicWorld.getNextEvent());
	}
	
	// process events
	if (events & EVENT_LEFT_BLOBBY_HIT)
		mLogic->onBallHitsPlayer(LEFT_PLAYER);
	
	if (events & EVENT_RIGHT_BLOBBY_HIT)
		mLogic->onBallHitsPlayer(RIGHT_PLAYER);
	
	if(events & EVENT_BALL_HIT_LEFT_GROUND)
		mLogic->onBallHitsGround(LEFT_PLAYER);
	
	if(events & EVENT_BALL_HIT_RIGHT_GROUND)
		mLogic->onBallHitsGround(RIGHT_PLAYER);
	
	

	switch(mLogic->getLastErrorSide()){
		case LEFT_PLAYER:
			events |= EVENT_ERROR_LEFT;
		case RIGHT_PLAYER:
			// if the error was caused by the right player
			// reset EVENT_ERROR_LEFT
			events &= ~EVENT_ERROR_LEFT;
			events |= EVENT_ERROR_RIGHT;
			if (!(events & EVENT_BALL_HIT_GROUND))
			{
				mPhysicWorld.getBallReference().setVelocity( getBallVelocity() * 0.6);
			}
			
			// now, the ball is not valid anymore
			/// \todo why do we set balldown?
			/// 		we could get here just
			///			by for hits
			mBallDown = true;
			break;
		
	}

	// if the round is finished, we 
	// reset BallDown, reset the World
	// to let the player serve
	// and trigger the EVENT_RESET
	if (roundFinished())
	{
		mBallDown = false;
		resetBall(mLogic->getServingPlayer());
		mIsGameRunning = false;
		events |= EVENT_RESET;
	}
	
	// reset external events
	external_events = 0;
}

void DuelMatch::setScore(int left, int right)
{
	mLogic->setScore(LEFT_PLAYER, left);
	mLogic->setScore(RIGHT_PLAYER, right);
}

void DuelMatch::trigger(int event)
{
	external_events |= event;
}

void DuelMatch::resetTriggeredEvents()
{
	external_events = 0;
}

void DuelMatch::pause()
{
	mLogic->onPause();
	mPaused = true;
}
void DuelMatch::unpause()
{
	mLogic->onUnPause();
	mPaused = false;
}

PlayerSide DuelMatch::winningPlayer()
{
	return mLogic->getWinningPlayer();
}

int DuelMatch::getHitcount(PlayerSide player) const
{
	if (player == LEFT_PLAYER)
		return mLogic->getHits(LEFT_PLAYER);
	else if (player == RIGHT_PLAYER)
		return mLogic->getHits(RIGHT_PLAYER);
	else
		return 0;
}

int DuelMatch::getScore(PlayerSide player) const
{
	return mLogic->getScore(player);
}

int DuelMatch::getScoreToWin() const 
{
	return mLogic->getScoreToWin();
}

bool DuelMatch::getBallDown() const
{
	return mBallDown;
}

bool DuelMatch::getBallActive() const 
{
	/// \todo i guess this needs to be reworked too
	return mIsGameRunning;
}


bool DuelMatch::getBlobJump(PlayerSide player) const
{
	/// \todo replace with more stable algorithm
	PhysicObject::MotionState state = mPhysicWorld.getBlob(player).getMotionState();
	return !(state.vel.y == 0 && state.pos.y > GROUND_PLANE_HEIGHT - 2);
}

Vector2 DuelMatch::getBlobPosition(PlayerSide player) const
{
	return mPhysicWorld.getBlob(player).getPosition();
}

Vector2 DuelMatch::getBallPosition() const
{
	return mPhysicWorld.getBall().getPosition();
}

Vector2 DuelMatch::getBallVelocity() const
{
	return mPhysicWorld.getBall().getVelocity();
}

PlayerSide DuelMatch::getServingPlayer() const
{	// NO_PLAYER hack was moved into ScriptedInpurSource.cpp
	return mLogic->getServingPlayer();
}

void DuelMatch::setState(RakNet::BitStream* stream)
{
	//mPhysicWorld.setState(stream);
}

const PlayerInput* DuelMatch::getPlayersInput() const
{
	return mInputs;
}

void DuelMatch::setPlayersInput(const PlayerInput& left, const PlayerInput& right)
{
	mInputs[0] = left;
	mInputs[1] = right;
}

void DuelMatch::setServingPlayer(PlayerSide side)
{
	mLogic->setServingPlayer(side);
	resetBall(side);
}

const Clock& DuelMatch::getClock() const
{
	return mLogic->getClock();
}

Clock& DuelMatch::getClock()
{
	return mLogic->getClock();
}

void DuelMatch::resetBall(PlayerSide player)
{
	Vector2 ballPosition = Vector2(400, 450);
	
	if (player == LEFT_PLAYER)
		ballPosition = Vector2(200, STANDARD_BALL_HEIGHT);
	else if (player == RIGHT_PLAYER)
		 ballPosition= Vector2(600, STANDARD_BALL_HEIGHT);
	//else
	/// \todo assert here?
	mPhysicWorld.getBallReference().setPosition( ballPosition );
	mPhysicWorld.getBallReference().setVelocity( Vector2(0,0) );

/*	mBallRotation = 0.0;
	mBallAngularVelocity = STANDARD_BALL_ANGULAR_VELOCITY;
	mBlobState[LEFT_PLAYER] = 0.0;
	mBlobState[RIGHT_PLAYER] = 0.0;

	mIsGameRunning = false;
	mIsBallValid = true;

	mLastHitIntensity = 0.0;
*/
}

bool DuelMatch::resetAreaClear() const
{
	/// \todo implement this
	//if (blobbyHitGround(LEFT_PLAYER) && blobbyHitGround(RIGHT_PLAYER))
		return true;
	return false;
}

bool DuelMatch::roundFinished() const
{
	if (resetAreaClear())
	{
		if (mBallDown)
			if (getBallVelocity().y < 1.5 &&
				getBallVelocity().y > -1.5 && getBallPosition().y > 430)
				return true;
	}
	
	/// \todo add a timeout timer
	/*if (mTimeSinceBallout > TIMEOUT_MAX)
		return true;
	*/
	return false;
}

float DuelMatch::getBallRotation() const
{
	return mPhysicWorld.getBall().getRotation();
}
