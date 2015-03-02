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

#include "PacketHandler.h"
#include "raknet/BitStream.h"
#include <iostream>

PacketHandler::PacketHandler()
{
	mErrorHandler= []( packet_ptr packet )
	{
		/// \todo print more info!
		std::cerr << "Received unknown Packet " << (int)packet->data[0] << "\n";
	};
}

void PacketHandler::handlePacket( packet_ptr packet )
{
    unsigned char pid = packet->data[0];
    auto handler = mHandlers[pid];
    // if handler is undefined
    if( handler )
		handler( packet );
	else
		mErrorHandler( packet );
}

void PacketHandler::registerHandler( std::initializer_list<unsigned char> packet_ids, std::function<void(const RakNet::BitStream&)> handler)
{
	auto hf = [handler]( packet_ptr p )
	{
		RakNet::BitStream stream( p->getStream() );
		stream.IgnoreBytes(1);	//ID_BLOBBY_SERVER_PRESENT
		handler( stream );
	};
	registerHandler( packet_ids, hf );
}

void PacketHandler::ignorePackets ( std::initializer_list<unsigned char> packet_ids )
{
	registerHandler( packet_ids, [](packet_ptr) { });
}

void PacketHandler::registerHandler( std::initializer_list<unsigned char> packet_ids, handler_fn handler )
{
	for(auto packet_id : packet_ids)
		mHandlers[packet_id] = handler;
}
