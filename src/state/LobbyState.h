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

#include "State.h"

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

#include "raknet/RakClient.h"

#include "NetworkMessage.h"
#include "PlayerIdentity.h"

class LobbySubstate;

enum ConnectionState
{
	CONNECTING,
	CONNECTED,
	DISCONNECTED,
	CONNECTION_FAILED
};

class LobbyState : public State
{
	public:
		LobbyState(ServerInfo info);
		virtual ~LobbyState();

		virtual void step_impl();
		virtual const char* getStateName() const;

	private:
		boost::shared_ptr<RakClient> mClient;
		PlayerIdentity mLocalPlayer;
		ServerInfo mInfo;
		unsigned mSelectedGame;

		struct OpenGame
		{
			unsigned id;
			std::string name;
			unsigned rules;
			unsigned speed;
			unsigned score;
		};

		std::vector<OpenGame> mOpenGames;
		std::vector<unsigned int> mPossibleSpeeds;
		std::vector<std::string> mPossibleRules;
		std::vector<unsigned> mPossibleScores{2, 5, 10, 15, 20, 25, 40, 50};
		
		// temp variables for open game
		unsigned mChosenSpeed = 0;
		unsigned mChosenRules = 0;
		unsigned mChosenScore = 3;
		unsigned mOwnGame = 0;
		std::vector<PlayerID> mOtherPlayers;

		ConnectionState mLobbyState;
		
		LobbyMainSubstate* mMainSubstate;
};

// Lobby Substates
class LobbyMainSubstate : public LobbySubstate
{
public:
	LobbyMainSubstate(boost::shared_ptr<RakClient> c, ServerInfo info) : mClient(c), mInfo(info)
	{
		
	}
	
	void step();
	void setConnectionState();
	
private:
	boost::shared_ptr<RakClient> mClient;
	ServerInfo mInfo;
	ConnectionState mLobbyStatus;
};
