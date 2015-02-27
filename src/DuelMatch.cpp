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
#include "SpeedController.h"

// helper macro to check that non-threadsafe functions are only called from
// within the match thread
#define thread_assert()	assert( std::this_thread::get_id() == mMatchThread.get_id() || !mMatchThread.joinable() );
/* implementation */

DuelMatch::DuelMatch(bool remote, std::string rules) :
		// we send a pointer to an unconstructed object here!
		mLogic(createGameLogic(rules, this)),
		mPaused(false),
		mRemote(remote),
		mPhysicWorld( new PhysicWorld() ),
		mIsRunning( true ),
		mHasInjection( false ),
		mInjectionState( new DuelMatchState )
{
	mPhysicWorld->setEventCallback( std::bind(&DuelMatch::physicEventCallback, this, std::placeholders::_1) );

	setInputSources(boost::make_shared<InputSource>(), boost::make_shared<InputSource>());

	// init
	updateState();
}

DuelMatch::~DuelMatch()
{
	mIsRunning = false;
	if( mMatchThread.joinable() )
		mMatchThread.join();
}

void DuelMatch::run()
{
	// end old thread if still running
	mIsRunning = false;
	if( mMatchThread.joinable() )
		mMatchThread.join();

	// start new thread
	mIsRunning = true;
	// start match thread
	std::cout << "START THREAD "<<mMatchThread.joinable() << "\n";
	mMatchThread = std::thread([this]()
	{
		SpeedController ctrl(1000);
		while(mIsRunning)
		{
			step();
			ctrl.update();
		}
	}
	);
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

void DuelMatch::setRules(std::string rulesFile)
{
	/// \todo review thread safety
	mLogic = createGameLogic(rulesFile, this);
}


void DuelMatch::step()
{
	/// \todo this should no longer be public, it is called only from the internal thread
	// in pause mode, step does nothing. this should go to the framerate computation
	if(mPaused || mLogic->getWinningPlayer() != NO_PLAYER)
		return;

	// get external events
	if( mHasInjection )
	{
		std::lock_guard<std::mutex> lock(mInjectionMutex);
		setState( *mInjectionState );
		mStepPhysicEvents = mInjectionEvents;
		mInjectionEvents.clear();
		mHasInjection = false;
	}

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

int DuelMatch::getScoreToWin() const
{
	/// \todo evaluate thread safety
	return mLogic->getScoreToWin();
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
}

DuelMatchState DuelMatch::getState() const
{
	/// \todo review thread safety
	DuelMatchState state;
	state.worldState = mPhysicWorld->getState();
	state.logicState = mLogic->getState();
	state.playerInput[LEFT_PLAYER] = mTransformedInput[LEFT_PLAYER];
	state.playerInput[RIGHT_PLAYER] = mTransformedInput[RIGHT_PLAYER];

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
	// lock
	std::lock_guard<std::mutex> lock(mEventMutex);
	// move all events to accumulating vector
	for(auto&& data : mStepPhysicEvents)
		mPhysicEvents.push_back( data );
	mStepPhysicEvents.clear();

	// update the state
	mLastStates.push_back( boost::make_shared<DuelMatchState>(getState()) );
	mLastState = mLastStates.back();
}

std::vector<MatchEvent> DuelMatch::fetchEvents()
{
	std::lock_guard<std::mutex> lock(mEventMutex);
	auto copy = mPhysicEvents;
	mPhysicEvents.clear();

	return copy;
}

std::vector<boost::shared_ptr<const DuelMatchState>> DuelMatch::fetchStates( )
{
	std::lock_guard<std::mutex> lock(mEventMutex);
	auto copy = mLastStates;
	mLastStates.clear();

	return copy;
}

const DuelMatchState& DuelMatch::readCurrentState() const
{
	assert( mLastState );
	return *mLastState;
}

void DuelMatch::injectState( const DuelMatchState& state, std::vector<MatchEvent> events )
{
	std::lock_guard<std::mutex> lock(mInjectionMutex);
	*mInjectionState = state;
	for(auto& e : events)
		mInjectionEvents.push_back(e);
	mHasInjection = true;
}
