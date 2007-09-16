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

#include <iostream>
#include <unistd.h>

#include <RakPeer.h>

#include "NetworkGameServer.h"

NetworkGameServer::NetworkGameServer()
	: mServer(0)
{
}

NetworkGameServer::~NetworkGameServer()
{
	if (mServer)
	{
		delete mServer;
	}
}

SystemAddress NetworkGameServer::getServerAddress()
{
	return mServerAddress;
}

void NetworkGameServer::createProcess()
{
	int filedes[2];
	pipe(filedes);

	pid_t pid = fork();

	if (pid > 0) {
		close(filedes[1]);
		read(filedes[0], &mServerAddress, sizeof(SystemAddress));
		close(filedes[0]);
	} else if (pid < 0) {
		perror("fork");
		std::abort();
	} else {
		close(filedes[0]);
		SocketDescriptor sockdesc(0, 0);

		mServer = new RakPeer();
		mServer->Startup(2, 0, &sockdesc, 1);
		mServer->SetMaximumIncomingConnections(2);

		mServerAddress = mServer->GetInternalID();
		write(filedes[1], &mServerAddress, sizeof(SystemAddress));
		close(filedes[1]);

		step();
	}
}

void NetworkGameServer::step()
{
	bool running = true;

	while (running)
	{
		mServer->DeallocatePacket(mServer->Receive());
		usleep(1);
	}
}
