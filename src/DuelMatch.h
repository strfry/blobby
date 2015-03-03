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
#include <thread>
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
class SpeedController;

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
		void setGameSpeed( int fps );

		/// \todo remove the necessity for this function!
		void reset();

		// this starts the match thread
		void run();

		// This steps through one frame
		void step();

		/// gets the score required to win the game
		/// as this is a read-only value, this function
		/// is thread_save
		int getScoreToWin() const;

		// pause control
		void pause();
		void unpause();
		bool isPaused() const;

		/// reads the current state in a non thread-save way
		const DuelMatchState& readCurrentState() const;

		//Input stuff for recording and playing replays
		boost::shared_ptr<InputSource> getInputSource(PlayerSide player) const;

		PlayerIdentity getPlayer(PlayerSide player) const;
		PlayerIdentity& getPlayer(PlayerSide player);

		// this prepares a state to be injected into the match before the next iteration starts
		void injectState( const DuelMatchState& state, std::vector<MatchEvent> events );

		// copies the physic events into the target and resets.
		std::vector<MatchEvent> fetchEvents( );
		std::vector<boost::shared_ptr<const DuelMatchState>> fetchStates( );
	private:
		void physicEventCallback( MatchEvent event );
		// update the internal last world state to the current physicworld
		void updateState();
		// resets the ball for next serve
		void resetBall(PlayerSide side);
		bool canStartRound(PlayerSide servingPlayer) const;


		/// gets the current state, not thread safe
		DuelMatchState getState() const;
		/// Set a new state using a saved DuelMatchState
		void setState(const DuelMatchState& state);

		boost::shared_ptr<InputSource> mInputSources[MAX_PLAYERS];
		PlayerIdentity mPlayers[MAX_PLAYERS];

		const bool mRemote;

		// data that is written to in the simulation thread
		boost::scoped_ptr<PhysicWorld> mPhysicWorld;
		GameLogic mLogic;
		PlayerInput mTransformedInput[MAX_PLAYERS];

		// accumulation of physic events in the current time step
		std::vector<MatchEvent> mStepPhysicEvents;
		// accumulation of physic events since they were fetched the last time
		std::vector<MatchEvent> mPhysicEvents;
		// ... and the same thing for states
		std::vector<boost::shared_ptr<const DuelMatchState>> mLastStates;
		boost::shared_ptr<const DuelMatchState> mLastState;
		std::mutex mEventMutex;

		std::thread mMatchThread;
		boost::shared_ptr<SpeedController> mSpeedController;

		// this variable is used to tell the run thread to exit
		std::atomic<bool> mIsRunning;

		// state injectsion
		boost::scoped_ptr<DuelMatchState> mInjectionState;
		std::vector<MatchEvent> mInjectionEvents;
		std::atomic<bool> mHasInjection;
		std::mutex mInjectionMutex;
};

