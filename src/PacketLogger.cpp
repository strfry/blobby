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

#include "PacketLogger.h"

#include <map>
#include <fstream>

#include "RakNet/NetworkTypes.h"

// implementation details

struct PacketLogger::PacketLoggerImpl
{
	std::map<int, std::string> translation;
	std::fstream file;
};


// PacketLogger class


PacketLogger::PacketLogger() : p(new PacketLoggerImpl)
{
	p->file.open("logs/packets.txt", std::fstream::out);
};

PacketLogger::~PacketLogger()
{
	
}


void PacketLogger::setHumanReadableType(int id, const std::string& name)
{
	p->translation[id] = name;
}

const std::string& PacketLogger::getHumanReadableType(int id) const
{
	return p->translation[id];
}

int PacketLogger::getPacketID(const std::string& type) const
{
	
}
	
const PacketLogger& PacketLogger::operator<<(Packet* packet) const
{
	int id = (int)packet->data[0];
	p->file << packet->playerId.binaryAddress << ":" << packet->playerId.port << "\t" << getHumanReadableType(id) << " " << id << "\t" << packet->data << "\n"; 
}

