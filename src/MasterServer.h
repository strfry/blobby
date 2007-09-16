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

#include <list>
#include <string>
#include <boost/cstdint.hpp>

#include <RakNetTypes.h>

class RakPeer;

class MasterServer
{
public:
	MasterServer(uint16_t port);
	~MasterServer();

	void run();

private:

	// RPC Functions
	//
	// Note: Described parameters and return values have nothing to do with
	// the function signatures, these functions are called by RakNet over network

	static void loginUser(RPCParameters* rpcparam);
	static void registerNewUser(RPCParameters* rpcparam);
	static void createGame(RPCParameters* rpcparam);
	static void joinGame(RPCParameters* rpcparam);

	SystemAddress createGame(SystemAddress creator);

	RakPeer* mServer;
	static MasterServer* mSingleton;

	struct Player
	{
		SystemAddress address;
		std::string name;
	};

	std::list<Player> mLoggedInPlayers;
	std::list<Player> mWaitingServers;
};

