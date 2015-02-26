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

#pragma once

#include <string>
#include <atomic>
#include <mutex>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "GameLogic.h"
#include "Vector.h"
#include "PlayerInput.h"
#include "PlayerIdentity.h"
#include "BlobbyDebug.h"

class InputSource;
struct DuelMatchState;
class PhysicWorld;
class PhysicState;
class MatchEvent;

/*! \class DuelMatch
	\brief class representing a blobby game.
	\details
	This class represents a single game between two players
	It applys the rules itself and provides an interface for querying
	different parameters. For this purpose it is designed as something
	similar to a singleton, but it can be instantiated
	multiple times on a server or be completely unavailable
*/
class DuelMatch : public ObjectCounter<DuelMatch>
{
	public:
		// If remote is true, only physical responses will be calculated
		// but hit events and score events are received from network

		DuelMatch(bool remote, std::string rules);

		void setPlayers( PlayerIdentity lplayer, PlayerIdentity rplayer);
		void setInputSources(boost::shared_ptr<InputSource> linput, boost::shared_ptr<InputSource> rinput );

		~DuelMatch();

		void setRules(std::string rulesFile);

		void reset();

		// This steps through one frame
		void step();

		// this methods allow external input
		// events triggered by the network
		void setScore(int left, int right);
		void resetBall(PlayerSide side);

		// This reports the index of the winning player and -1 if the
		// game is still running
		/// \todo not thread save
		PlayerSide winningPlayer() const;

		// This methods report the current game state and a useful for
		// the input manager, which needs information about the blob
		// positions and for lua export, which makes them accessable
		// for scripted input sources

		int getScore(PlayerSide player) const;
		int getScoreToWin() const;
		PlayerSide getServingPlayer() const;

		int getHitcount(PlayerSide player) const;

		Vector2 getBallPosition() const;
		Vector2 getBallVelocity() const;
		float getBallRotation() const;
		Vector2 getBlobPosition(PlayerSide player) const;
		Vector2 getBlobVelocity(PlayerSide player) const;
		float getBlobState( PlayerSide player ) const;

		const Clock& getClock() const;
		Clock& getClock();

		bool getBallDown() const;
		bool getBallActive() const;
		bool canStartRound(PlayerSide servingPlayer) const;

		void pause();
		void unpause();

		bool isPaused() const{ return mPaused; }

		// This functions returns true if the player launched
		// and is jumping at the moment
		// the function directly accesses the world, and thus is dangerous
		bool getBlobJump(PlayerSide player) const __attribute__((deprecated));

		/// Set a new state using a saved DuelMatchState
		void setState(const DuelMatchState& state);

		/// gets the current state
		DuelMatchState getState() const;

		//Input stuff for recording and playing replays
		boost::shared_ptr<InputSource> getInputSource(PlayerSide player) const;

		PlayerIdentity getPlayer(PlayerSide player) const;
		PlayerIdentity& getPlayer(PlayerSide player);

		void setServingPlayer(PlayerSide side);

		// copies the physic events into the target and resets.
		void fetchEvents(std::vector<MatchEvent>& target);
	private:
		void physicEventCallback( MatchEvent event );
		// update the internal last world state to the current physicworld
		void updateState();

		boost::shared_ptr<InputSource> mInputSources[MAX_PLAYERS];
		PlayerIdentity mPlayers[MAX_PLAYERS];

		bool mPaused;
		bool mRemote;

		// data that is written to in the simulation thread
		boost::scoped_ptr<PhysicWorld> mPhysicWorld;
		GameLogic mLogic;
		PlayerInput mTransformedInput[MAX_PLAYERS];
		// accumulation of physic events in the current time step
		std::vector<MatchEvent> mStepPhysicEvents;
		// accumulation of physic events since they were fetched the last time
		std::vector<MatchEvent> mPhysicEvents;
		std::mutex mEventMutex;

		// we need two memory locations to store the world state.
		boost::scoped_ptr<PhysicState> mLastStateStorageA;
		boost::scoped_ptr<PhysicState> mLastStateStorageB;
		// we then can atomically set this pointer to the PhysicState
		// that is currently active;
		std::atomic<const PhysicState*> mLastState;

};
