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

#include "MasterServer.h"

#include "NetworkGameServer.h"

#include <iostream>
#include <RakPeer.h>
#include <StringCompressor.h>

#include "Global.h"

MasterServer* MasterServer::mSingleton = 0;

const int MAX_PEERS = 1024;

int main()
{
	MasterServer server(MASTER_SERVER_PORT);
	server.run();
}

MasterServer::MasterServer(uint16_t port)
	: mServer(new RakPeer)
{
	assert (mSingleton == 0);
	mSingleton = this;

	REGISTER_STATIC_RPC(mServer, loginUser);
	REGISTER_STATIC_RPC(mServer, createGame);

	SocketDescriptor serversock(port, 0);
	bool success = mServer->Startup(MAX_PEERS, 30, &serversock, 1);
	if (!success)
	{
		perror("RakPeer::Startup");
		exit(EXIT_FAILURE);
	}
	mServer->SetMaximumIncomingConnections(MAX_PEERS);
}

MasterServer::~MasterServer()
{
	mServer->Shutdown(1, 50);
	delete mServer;
}

void MasterServer::run()
{
	bool running = true;
	while (running)
	{
		Packet* packet;
		while (packet = mServer->Receive())
		{
			mServer->DeallocatePacket(packet);
		}

		usleep(1);
	}
}

void MasterServer::loginUser(RPCParameters* rpcparams)
{
	RakNet::BitStream input(rpcparams->input,
			rpcparams->numberOfBitsOfData, false);

	char username[16];
	char password[16];

	StringCompressor::Instance()->DecodeString(username, 16, &input);
	StringCompressor::Instance()->DecodeString(password, 16, &input);

	std::cout << "loginUser: Username: " << username << " Password: " << password << std::endl;

	Player newplayer = {
		rpcparams->sender,
		username
	};

	mSingleton->mLoggedInPlayers.push_back(newplayer);

	rpcparams->replyToSender->Write(true);
}

void MasterServer::createGame(RPCParameters* rpcparams)
{
	SystemAddress address = mSingleton->createGame(rpcparams->sender);
	rpcparams->replyToSender->Write(address.binaryAddress);
	rpcparams->replyToSender->Write(address.port);
}

SystemAddress MasterServer::createGame(SystemAddress creator)
{
	std::string playername;

	typedef std::list<Player>::iterator PlayerListIterator;
	for (PlayerListIterator iter = mLoggedInPlayers.begin();
			iter != mLoggedInPlayers.end(); iter++)
	{
		if (creator == iter->address)
		{
			playername = iter->name;
			break;
		}
	}

	NetworkGameServer server;
	SystemAddress address = server.getServerAddress();
	
	server.createProcess();

	Player newserver = {
		address,
		playername
	};
	
	mWaitingServers.push_back(newserver);

	return address;
}

