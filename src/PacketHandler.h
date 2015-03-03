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

#include "raknet/NetworkTypes.h"
#include <array>
#include <functional>
#include <limits>
#include <initializer_list>

class PacketHandler
{
	/// \todo add sender handling
	typedef std::function<void(packet_ptr)> handler_fn;
public:
	// default c'tor, sets the error function to print to cerr
	PacketHandler();

	// handle packet
	void handlePacket( packet_ptr packet );

	// register packet handlers
	void registerHandler( std::initializer_list<unsigned char> packet_ids, std::function<void(const RakNet::BitStream&)> handler);
	void registerHandler( std::initializer_list<unsigned char> packet_ids, handler_fn handler );
	void ignorePackets ( std::initializer_list<unsigned char> packet_ids );

private:
	std::array<handler_fn, std::numeric_limits<unsigned char>::max()> mHandlers;

	/// this function is called whenever a packet is not handled by the handler list.
	/// we do not set the mHandlers entries to mErrorHandler to make it easier to change
	/// them afterwards, and also to catch cases when the user supplies a non.callable function.
	handler_fn mErrorHandler;
};

