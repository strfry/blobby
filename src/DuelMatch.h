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

#pragma once

#include "PhysicWorld.h"
#include "GameLogic.h"
#include "Vector.h"
#include "BlobbyThread.h"
//#include "IDuelMatch.h"

class InputSource;
class ReplayRecorder;

class DuelMatch;

class DuelMatchThread : public BlobbyThread//, public IDuelMatch
{
	public:
		DuelMatchThread(InputSource* linput, InputSource* rinput, int speed, 
						bool global, bool remote, ReplayRecorder* rec);
		~DuelMatchThread();
		
		virtual int getScore(PlayerSide player) const 	; 
		virtual const Clock& getClock() const			;
		virtual PlayerSide getServingPlayer() const;
		bool isPaused() const;
		PlayerSide getWinningPlayer() const;
		const PlayerInput* getPlayersInput() const;
		const DuelMatch* getMatchRepresentation() const;
		
		void pause();
		void unpause();
		
		virtual const PhysicWorld& getWorld() const;
		virtual int getEvents() const;
		virtual void step() 
		{
			
		}
		
	private:
		struct MatchData;
		MatchData* data;
		
		MatchData* initData(InputSource* linput, InputSource* rinput, bool global, bool remote, ReplayRecorder*);
		
		static int threadInit(ThreadRunParams<MatchData> data);
		static int threadMain(ThreadRunParams<MatchData> data);
};

/// This class represents a single game between two players
/// It applys the rules itself and provides an interface for querying
/// different parameters. For this purpose it is designed as something
/// similar to a singleton, but it can be instantiated
/// multiple times on a server or be completely unavailable
/*! \class DuelMatch
	\brief class representing a blobby game.
	\details
	This class represents a single game between two players
	It applys the rules itself and provides an interface for querying
	different parameters. For this purpose it is designed as something
	similar to a singleton, but it can be instantiated
	multiple times on a server or be completely unavailable
*/
/// \todo Move ReplayRecorder into DuelMatch!
class DuelMatch//: public IDuelMatch
{
public:
	// This constructor takes the input sources used to get player input
	// The parameter output tells DuelMatch if it should report its
	// results to the user through RenderManager and SoundManager.
	// A deacivation of the output is useful on dedicated servers

	// If global is true, the instance registered as the main
	// game and can be accessed from everywhere. There can only
	// be one global game at a time, otherwise an assertion fails.
	
	// If remote is true, only physical responses will be calculated
	// but hit events and score events are received from network

	DuelMatch(InputSource* linput, InputSource* rinput, bool global, bool remote, ReplayRecorder* recorder);

	~DuelMatch();

	// Allthough DuelMatch can be instantiated multiple times, a
	// singleton may be registered for the purpose of scripted or
	// interactive input. Note this can return 0.
	static DuelMatch* getMainGame();
	
	void reset();

	// This steps through one frame
	void step();
	
	// this methods allow external input 
	// events triggered by the network
	void setScore(int left, int right);
	
	void trigger(int event);
	void resetTriggeredEvents();

	// This reports the index of the winning player and -1 if the
	// game is still running
	PlayerSide getWinningPlayer() const;

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
	Vector2 getBlobPosition(PlayerSide player) const;
	
	const PhysicWorld& getWorld() const{ return mPhysicWorld; };
	const Clock& getClock() const;
	Clock& getClock();

	bool getBallDown() const;
	bool getBallActive() const;
	
	void pause();
	void unpause();
	
	bool isPaused() const{ return mPaused; }

	// This functions returns true if the player launched
	// and is jumping at the moment
	bool getBlobJump(PlayerSide player) const;

	// Set a new state received from server over a RakNet BitStream
	void setState(RakNet::BitStream* stream);

	//Input stuff for recording and playing replays
	const PlayerInput* getPlayersInput() const;
	void setPlayersInput(const PlayerInput& left, const PlayerInput& right);
	
	void setServingPlayer(PlayerSide side);
	
	enum {
		EVENT_LEFT_BLOBBY_HIT = 1,
		EVENT_RIGHT_BLOBBY_HIT = 2,
		EVENT_BALL_HIT_LEFT_GROUND = 4,
		EVENT_BALL_HIT_RIGHT_GROUND = 8,
		EVENT_BALL_HIT_GROUND = EVENT_BALL_HIT_LEFT_GROUND | EVENT_BALL_HIT_RIGHT_GROUND,
		EVENT_ERROR_LEFT = 16,
		EVENT_ERROR_RIGHT = 32,
		EVENT_ERROR = EVENT_ERROR_LEFT | EVENT_ERROR_RIGHT,
		EVENT_RESET = 64
	};
	
	int getEvents() const { return events; }

private:
	static DuelMatch* mMainGame;
	bool mGlobal;

	PhysicWorld mPhysicWorld;

	InputSource* mLeftInput;
	InputSource* mRightInput;

	GameLogic mLogic;
	
	// we need the replay recorder here, cause it must record each step
	// and thus be executed in the same thread as DuelMatch together with
	// the step function.
	// as we use DuelMatch for shwoing replays, too, it is possible to
	// disable recplay recording for a match. then, mRecorder is null.
	// it is not possible to record a math partially!
	ReplayRecorder* mRecorder;

	bool mBallDown;
	
	bool mPaused;

	int events;
	int external_events;
	bool mRemote;
};
