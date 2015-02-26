/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

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

/* header include */
#include "DuelMatch.h"

/* includes */
#include <cassert>
#include <iostream>

#include <boost/make_shared.hpp>

#include "DuelMatchState.h"
#include "MatchEvents.h"
#include "PhysicWorld.h"
#include "GenericIO.h"
#include "GameConstants.h"
#include "InputSource.h"

#define thread_assert(x)

/* implementation */

DuelMatch::DuelMatch(bool remote, std::string rules) :
		// we send a pointer to an unconstructed object here!
		mLogic(createGameLogic(rules, this)),
		mPaused(false),
		mRemote(remote),
		mLastStateStorageA( new PhysicState ),
		mLastStateStorageB( new PhysicState ),
		mPhysicWorld( new PhysicWorld() ),
		mIsRunning( true )
{
	mPhysicWorld->setEventCallback( std::bind(&DuelMatch::physicEventCallback, this, std::placeholders::_1) );

	setInputSources(boost::make_shared<InputSource>(), boost::make_shared<InputSource>());

	// init
	updateState();
}

void DuelMatch::setPlayers( PlayerIdentity lplayer, PlayerIdentity rplayer)
{
	/// \todo review thread safety
	mPlayers[LEFT_PLAYER] = lplayer;
	mPlayers[RIGHT_PLAYER] = rplayer;
}

void DuelMatch::setInputSources(boost::shared_ptr<InputSource> linput, boost::shared_ptr<InputSource> rinput )
{
	/// \todo review thread safety
	if(linput)
		mInputSources[LEFT_PLAYER] = linput;

	if(rinput)
		mInputSources[RIGHT_PLAYER] = rinput;

	mInputSources[LEFT_PLAYER]->setMatch(this);
	mInputSources[RIGHT_PLAYER]->setMatch(this);
}

void DuelMatch::reset()
{
	/// \todo review thread safety
	mPhysicWorld.reset(new PhysicWorld());
	mPhysicWorld->setEventCallback( std::bind(&DuelMatch::physicEventCallback, this, std::placeholders::_1) );

	mLogic = mLogic->clone();
}

DuelMatch::~DuelMatch()
{
}

void DuelMatch::setRules(std::string rulesFile)
{
	/// \todo review thread safety
	mLogic = createGameLogic(rulesFile, this);
}


void DuelMatch::step()
{
	/// \todo this should no longer be public, it is called only from the internal thread
	// in pause mode, step does nothing. this should go to the framerate computation
	if(mPaused || winningPlayer() != NO_PLAYER)
		return;

	// this calculates the new input, i.e. the input source functions are called from
	// within the match thread.
	mTransformedInput[LEFT_PLAYER] = mInputSources[LEFT_PLAYER]->updateInput();
	mTransformedInput[RIGHT_PLAYER] = mInputSources[RIGHT_PLAYER]->updateInput();

	if(!mRemote)
	{
		mTransformedInput[LEFT_PLAYER] = mLogic->transformInput( mTransformedInput[LEFT_PLAYER], LEFT_PLAYER );
		mTransformedInput[RIGHT_PLAYER] = mLogic->transformInput( mTransformedInput[RIGHT_PLAYER], RIGHT_PLAYER );
	}

	// do steps in physic an logic.
	mLogic->step();
	// the callback also adds the events
	mPhysicWorld->step( mTransformedInput[LEFT_PLAYER], mTransformedInput[RIGHT_PLAYER],
											mLogic->isBallValid(), mLogic->isGameRunning() );

	// process all physics events and relay them to logic
	for( const auto& event : mStepPhysicEvents )
	{
		switch( event.event )
		{
		case MatchEvent::BALL_HIT_BLOB:
			mLogic->onBallHitsPlayer( event.side );
			break;
		case MatchEvent::BALL_HIT_GROUND:
			mLogic->onBallHitsGround( event.side );
			// if not valid, reduce velocity
			if(!mLogic->isBallValid())
				mPhysicWorld->setBallVelocity( mPhysicWorld->getBallVelocity().scale(0.6) );
			break;
		case MatchEvent::BALL_HIT_NET:
			mLogic->onBallHitsNet( event.side );
			break;
		case MatchEvent::BALL_HIT_NET_TOP:
			mLogic->onBallHitsNet( NO_PLAYER );
			break;
		case MatchEvent::BALL_HIT_WALL:
			mLogic->onBallHitsWall( event.side );
			break;
		}
	}

	auto errorside = mLogic->getLastErrorSide();
	if(errorside != NO_PLAYER)
	{
		physicEventCallback( MatchEvent{MatchEvent::PLAYER_ERROR, errorside, 0});
		mPhysicWorld->setBallVelocity( mPhysicWorld->getBallVelocity().scale(0.6) );
	}

	// if the round is finished, we
	// reset BallDown, reset the World
	// to let the player serve
	// and trigger the EVENT_RESET
	if (!mLogic->isBallValid() && canStartRound(mLogic->getServingPlayer()))
	{
		resetBall( mLogic->getServingPlayer() );
		mLogic->onServe();
		physicEventCallback( MatchEvent{MatchEvent::RESET_BALL, NO_PLAYER, 0});
	}

	// update to new world state
	updateState();
}

void DuelMatch::setScore(int left, int right)
{
	/// \todo review thread safety
	mLogic->setScore(LEFT_PLAYER, left);
	mLogic->setScore(RIGHT_PLAYER, right);
}

void DuelMatch::pause()
{
	/// \todo review thread safety
	mLogic->onPause();
	mPaused = true;
}

void DuelMatch::unpause()
{
	/// \todo review thread safety
	mLogic->onUnPause();
	mPaused = false;
}

PlayerSide DuelMatch::winningPlayer() const
{
	thread_assert();
	return mLogic->getWinningPlayer();
}

int DuelMatch::getHitcount(PlayerSide player) const
{
	thread_assert();
	if (player == LEFT_PLAYER)
		return mLogic->getTouches(LEFT_PLAYER);
	else if (player == RIGHT_PLAYER)
		return mLogic->getTouches(RIGHT_PLAYER);
	else
		return 0;
}

int DuelMatch::getScore(PlayerSide player) const
{
	thread_assert();
	return mLogic->getScore(player);
}

int DuelMatch::getScoreToWin() const
{
	thread_assert();
	return mLogic->getScoreToWin();
}

bool DuelMatch::getBallDown() const
{
	thread_assert();
	return !mLogic->isBallValid();
}

bool DuelMatch::getBallActive() const
{
	thread_assert();
	return mLogic->isGameRunning();
}

bool DuelMatch::getBlobJump(PlayerSide player) const
{
	thread_assert();
	return !mPhysicWorld->blobHitGround(player);
}

Vector2 DuelMatch::getBlobPosition(PlayerSide player) const
{
	return mLastState.load()->blobPosition[player];
}

float DuelMatch::getBlobState( PlayerSide player ) const
{
	return mLastState.load()->blobState[player];
}

Vector2 DuelMatch::getBlobVelocity(PlayerSide player) const
{
	return mLastState.load()->blobVelocity[player];
}

Vector2 DuelMatch::getBallPosition() const
{
	return mLastState.load()->ballPosition;
}

Vector2 DuelMatch::getBallVelocity() const
{
	return mLastState.load()->ballVelocity;
}

float DuelMatch::getBallRotation() const
{
	return mLastState.load()->ballRotation;
}

PlayerSide DuelMatch::getServingPlayer() const
{
	thread_assert();
	// NO_PLAYER hack was moved into ScriptedInpurSource.cpp
	return mLogic->getServingPlayer();
}

void DuelMatch::setState(const DuelMatchState& state)
{
	/// \todo review thread safety
	mPhysicWorld->setState(state.worldState);
	mLogic->setState(state.logicState);

	mTransformedInput[LEFT_PLAYER] = state.playerInput[LEFT_PLAYER];
	mTransformedInput[RIGHT_PLAYER] = state.playerInput[RIGHT_PLAYER];

	mInputSources[LEFT_PLAYER]->setInput( mTransformedInput[LEFT_PLAYER] );
	mInputSources[RIGHT_PLAYER]->setInput( mTransformedInput[RIGHT_PLAYER] );

	/// \todo define loading events
/*	events &= ~EVENT_ERROR;
	switch (state.errorSide)
	{
		case LEFT_PLAYER:
			events |= EVENT_ERROR_LEFT;
			break;
		case RIGHT_PLAYER:
			events |= EVENT_ERROR_RIGHT;
			break;
	}
*/
}

DuelMatchState DuelMatch::getState() const
{
	/// \todo review thread safety
	DuelMatchState state;
	state.worldState = mPhysicWorld->getState();
	state.logicState = mLogic->getState();
	state.playerInput[LEFT_PLAYER] = mTransformedInput[LEFT_PLAYER];
	state.playerInput[RIGHT_PLAYER] = mTransformedInput[RIGHT_PLAYER];

/// \todo reenable
//	state.errorSide = (events & EVENT_ERROR_LEFT) ? LEFT_PLAYER : (events & EVENT_ERROR_RIGHT) ? RIGHT_PLAYER : NO_PLAYER;

	return state;
}

void DuelMatch::setServingPlayer(PlayerSide side)
{
	/// \todo review thread safety
	mLogic->setServingPlayer( side );
	resetBall( side );
	mLogic->onServe( );
}

const Clock& DuelMatch::getClock() const
{
	/// \todo review thread safety
	return mLogic->getClock();
}

Clock& DuelMatch::getClock()
{
	/// \todo review thread safety
	return mLogic->getClock();
}

boost::shared_ptr<InputSource> DuelMatch::getInputSource(PlayerSide player) const
{
	/// \todo review thread safety
	return mInputSources[player];
}

void DuelMatch::resetBall( PlayerSide side )
{
	thread_assert();
	if (side == LEFT_PLAYER)
		mPhysicWorld->setBallPosition( Vector2(200, STANDARD_BALL_HEIGHT) );
	else if (side == RIGHT_PLAYER)
		mPhysicWorld->setBallPosition( Vector2(600, STANDARD_BALL_HEIGHT) );
	else
		mPhysicWorld->setBallPosition( Vector2(400, 450) );

	mPhysicWorld->setBallVelocity( Vector2(0, 0) );
	mPhysicWorld->setBallAngularVelocity( (side == RIGHT_PLAYER ? -1 : 1) * STANDARD_BALL_ANGULAR_VELOCITY );
	mPhysicWorld->setLastHitIntensity(0.0);
}

bool DuelMatch::canStartRound(PlayerSide servingPlayer) const
{
	thread_assert();
	Vector2 ballVelocity = mPhysicWorld->getBallVelocity();
	return (mPhysicWorld->blobHitGround(servingPlayer) && ballVelocity.y < 1.5 &&
				ballVelocity.y > -1.5 && mPhysicWorld->getBallPosition().y > 430);
}

PlayerIdentity DuelMatch::getPlayer(PlayerSide player) const
{
	/// \todo review thread safety
	return mPlayers[player];
}

PlayerIdentity& DuelMatch::getPlayer(PlayerSide player)
{
	/// \todo review thread safety
	return mPlayers[player];
}

void DuelMatch::physicEventCallback( MatchEvent event )
{
	thread_assert();
	if( mRemote )
		return;

	mStepPhysicEvents.push_back( event );
}

void DuelMatch::updateState()
{
	// cannot thread assert, because it is called once in the ctor
	//thread_assert();

	// update physic state, alternate storage positions
	if(mLastState == mLastStateStorageB.get() )
	{
		*mLastStateStorageA = mPhysicWorld->getState();
		mLastState.store( mLastStateStorageA.get() );
	} else
	{
		*mLastStateStorageB = mPhysicWorld->getState();
		mLastState.store( mLastStateStorageB.get() );
	}

	// lock
	std::lock_guard<std::mutex> lock(mEventMutex);
	// move all events to accumulating vector
	for(auto&& data : mStepPhysicEvents)
		mPhysicEvents.push_back( data );
	mStepPhysicEvents.clear();
}

void DuelMatch::fetchEvents(std::vector<MatchEvent>& target)
{
	std::lock_guard<std::mutex> lock(mEventMutex);
	target.swap( mPhysicEvents );

	mPhysicEvents.clear();
}
