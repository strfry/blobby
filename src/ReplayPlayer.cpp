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
#include "ReplayPlayer.h"

/* includes */
#include <cassert>
#include <iostream>

#include "IReplayLoader.h"
#include "DuelMatch.h"
#include "MatchEvents.h"
#include "InputSource.h" // for debug

/* implementation */
ReplayPlayer::ReplayPlayer( boost::shared_ptr<DuelMatch> match ) : mMatch( match )
{
}

ReplayPlayer::~ReplayPlayer()
{
}

bool ReplayPlayer::endOfFile() const
{
	return (mPosition >= mLength);
}

void ReplayPlayer::load(const std::string& filename)
{
	loader.reset(IReplayLoader::createReplayLoader(filename));

	mPlayerNames[LEFT_PLAYER] = loader->getPlayerName(LEFT_PLAYER);
	mPlayerNames[RIGHT_PLAYER] = loader->getPlayerName(RIGHT_PLAYER);

	mPosition = 0;
	mLength = loader->getLength();

	// set data in match
	mMatch->setPlayers(getPlayerName(LEFT_PLAYER), getPlayerName(RIGHT_PLAYER));

	mMatch->getPlayer(LEFT_PLAYER).setStaticColor(getBlobColor(LEFT_PLAYER));
	mMatch->getPlayer(RIGHT_PLAYER).setStaticColor(getBlobColor(RIGHT_PLAYER));
}

std::string ReplayPlayer::getPlayerName(const PlayerSide side) const
{
	return mPlayerNames[side];
}

Color ReplayPlayer::getBlobColor(const PlayerSide side) const
{
	return loader->getBlobColor(side);
}

int ReplayPlayer::getGameSpeed() const
{
	return loader->getSpeed();
}

float ReplayPlayer::getPlayProgress() const
{
	return (float)mPosition / mLength;
}

int ReplayPlayer::getReplayPosition() const
{
	return mPosition;
}

int ReplayPlayer::getReplayLength() const
{
	return mLength;
}

void ReplayPlayer::play()
{;
	mMatch->setStepCallback( std::bind( &ReplayPlayer::onMatchStep, this, std::placeholders::_1, std::placeholders::_2) );
	mLeftInput = mMatch->getInputSource( LEFT_PLAYER );
	mRightInput = mMatch->getInputSource( RIGHT_PLAYER );

	// initial input
	loader->getInputAt(mPosition, mLeftInput.get(), mRightInput.get() );
	mMatch->setGameSpeed( loader->getSpeed() );
	mMatch->run();
}

void ReplayPlayer::setReplaySpeed( int fps )
{
	mMatch->setGameSpeed( fps );
}

void ReplayPlayer::onMatchStep(const DuelMatchState& state, const std::vector<MatchEvent>& events)
{
	std::lock_guard<std::mutex> lock(mExclusive);
	// now the old implementation would perform a step. we would have to wait for mPosition to increase once more
	// to fulfill the condition for having a safepoint reached
	int point;
	if(loader->isSavePoint(mPosition, point))
	{
		ReplaySavePoint reference;
		loader->readSavePoint(point, reference);
		// current state and savepoint should always be identical
		// this would override our next input. Thus we make sure the next input actually is what we want
		mMatch->injectState(reference.state, std::vector<MatchEvent>());	// inject state is synchronized... deadlock?
	}

	// mPosition is atomic, so this is save
	++mPosition;
	if( mPosition >= mLength )
	{
		mPosition = mLength;
		return;
	}
	// update input
	// only read accesses to loader, so this is save
	loader->getInputAt(mPosition, mLeftInput.get(), mRightInput.get() );
}

void ReplayPlayer::gotoPlayingPosition(int rep_position)
{
	std::lock_guard<std::mutex> lock(mExclusive);

	assert( rep_position >= 0 );

	/// \todo not thread safe with onMatchStep
	/// \todo add validity check for rep_position
	/// \todo replay clock does not work!

	// find next safepoint
	int save_position = -1;
	int savepoint = loader->getSavePoint(rep_position, save_position);
	// save position contains game step at which the save point is
	// savepoint is index of save point in array

	// now compare safepoint and actual position
	// if we have to forward and save_position is nearer than current position, jump
	if( (rep_position < mPosition || save_position > mPosition) && savepoint >= 0)
	{
		// can't use mPosition
		// set match to safepoint
		ReplaySavePoint state;
		loader->readSavePoint(savepoint, state);

		// set position and update match
		mPosition = save_position;
		mMatch->injectState(state.state, std::vector<MatchEvent>());
	}
	// otherwise, use current position

	// this is legacy code which will make fast forwarding possible even
	// when we have no savepoint and have to go back
	if(rep_position < mPosition)
	{
		// reset the match and simulate from start!
		mMatch->reset();
		mPosition = 0;
		gotoPlayingPosition( rep_position );
	}

	/// \todo re-enable bulk simulation
	// in the end, simulate the remaining steps
	// maximum: 100 steps
	// this is currently not possible
	/*for(int i = 0; i < 100; ++i)
	{
		// check if we have to do another step
		if(endOfFile() || rep_position == mPosition)
			return true;

		// do one play step
		play(virtual_match);
	}

	return false;*/
}
