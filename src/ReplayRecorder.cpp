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

/* header include */
#include "ReplayRecorder.h"

/* includes */
#include <algorithm>
#include <iostream>
#include <sstream>
#include <ctime>

#include <boost/crc.hpp>

#include "tinyxml/tinyxml.h"

#include "raknet/BitStream.h"

#include <SDL/SDL.h>

#include "Global.h"
#include "IReplayLoader.h"
#include "PhysicState.h"
#include "GenericIO.h"
#include "FileRead.h"
#include "FileWrite.h"

/* implementation */
ChecksumException::ChecksumException(std::string filename, uint32_t expected, uint32_t real)
{
	std::stringstream errorstr;

	errorstr << "Error: Corrupted replay file: " << filename <<
		std::endl << "real crc: " << real <<
		" crc in file: " << expected;
	error = errorstr.str();
}

ChecksumException::~ChecksumException() throw()
{
}

const char* ChecksumException::what() const throw()
{
	return error.c_str();
}

VersionMismatchException::VersionMismatchException(const std::string& filename, uint8_t major, uint8_t minor)
{
	std::stringstream errorstr;

	errorstr << "Error: Outdated replay file: " << filename <<
		std::endl << "expected version: " << (int)REPLAY_FILE_VERSION_MAJOR << "."
				<< (int)REPLAY_FILE_VERSION_MINOR <<
		std::endl << "got: " << (int)major << "." << (int)minor << " instead!" << std::endl;
	error = errorstr.str();
}

VersionMismatchException::~VersionMismatchException() throw()
{
}

const char* VersionMismatchException::what() const throw()
{
	return error.c_str();
}



ReplayRecorder::ReplayRecorder()
{
	mAttributes.setAttributeInteger("LeftEndScore", 0);
	mAttributes.setAttributeInteger("RightEndScore", 0);
}

ReplayRecorder::~ReplayRecorder()
{
}

void ReplayRecorder::save( boost::shared_ptr<FileWrite> file) const
{
	boost::shared_ptr<GenericOut> target = createGenericWriter(file);

	writeFileHeader(target, 0);

	uint32_t replayHeaderStart = target->tell();
	writeReplayHeader(target);
	writeAttributesSection(target);
	writeJumpTable(target);
	writeInputSection(target);
	writeStatesSection(target);

	// the last thing we write is the header again, so
	// we can fill in all data we gathered during the
	// rest of the writing process
	target->seek(replayHeaderStart);
	writeReplayHeader(target);

	target->seek(0);
	FileRead checksum_calculator(file->getFileName());
	/// \todo how can we make sure that we open the right file?

	uint32_t checksum = checksum_calculator.calcChecksum(replayHeaderStart);
	writeFileHeader(target, checksum);
}

void ReplayRecorder::send(boost::shared_ptr<GenericOut> target) const
{
	target->string(mAttributes.getAttributeString("LeftPlayerName"));
	target->string(mAttributes.getAttributeString("RightPlayerName"));

	target->generic<Color> (mAttributes.getAttributeColor("LeftColor"));
	target->generic<Color> (mAttributes.getAttributeColor("RightColor"));

	target->uint32( mAttributes.getAttributeInteger("GameSpeed") );
	target->uint32( mAttributes.getAttributeInteger("LeftFinalScore") );
	target->uint32( mAttributes.getAttributeInteger("RightFinalScore") );

	target->generic<std::vector<unsigned char> > (mSaveData);
	target->generic<std::vector<ReplaySavePoint> > (mSavePoints);
}

void ReplayRecorder::receive(boost::shared_ptr<GenericIn> source)
{
	std::string temp;
	source->string(temp);
	mAttributes.setAttributeString("LeftPlayerName", temp);
	source->string(temp);
	mAttributes.setAttributeString("RightPlayerName", temp);

	Color tempc;
	source->generic<Color> (tempc);
	mAttributes.setAttributeColor("LeftColor", tempc);
	source->generic<Color> (tempc);
	mAttributes.setAttributeColor("RightColor", tempc);

	unsigned int tempi;
	source->uint32( tempi );
	mAttributes.setAttributeInteger("GameSpeed", tempi);
	source->uint32(tempi);
	mAttributes.setAttributeInteger("LeftFinalScore", tempi);
	source->uint32(tempi);
	mAttributes.setAttributeInteger("RightFinalScore", tempi);



	source->generic<std::vector<unsigned char> >(mSaveData);
	source->generic<std::vector<ReplaySavePoint> > (mSavePoints);
}

void ReplayRecorder::writeFileHeader(boost::shared_ptr<GenericOut> file, uint32_t checksum) const
{
	file->array(validHeader, sizeof(validHeader));

	// after the header, we write the replay version
	// first, write zero. leading zero indicates that the following value
	// really is a version number (and not a checksum of an older replay!)
	file->byte(0);
	file->byte(REPLAY_FILE_VERSION_MAJOR);
	file->byte(REPLAY_FILE_VERSION_MINOR);
	file->byte(0);

	file->uint32(checksum);
}

void ReplayRecorder::writeReplayHeader(boost::shared_ptr<GenericOut> file) const
{
	/// for now, this are fixed numbers
	/// we have to make sure they are right!
	uint32_t header_ptr = file->tell();
	uint32_t header_size =  9*sizeof(header_ptr);

	uint32_t jptb_size = 128;											/// for now, we reserve 128 bytes!
	uint32_t data_size = mSaveData.size();								/// assumes 1 byte per data record!
	uint32_t states_size = mSavePoints.size() * sizeof(ReplaySavePoint);

	file->uint32(header_size);
	file->uint32(attr_ptr);
	file->uint32(attr_size);
	file->uint32(jptb_ptr);
	file->uint32(jptb_size);
	file->uint32(data_ptr);
	file->uint32(data_size);
	file->uint32(states_ptr);
	file->uint32(states_size);

	// check that we really needed header_size space
	assert( file->tell() - header_size );
}

void ReplayRecorder::writeAttributesSection(boost::shared_ptr<GenericOut> file) const
{
	attr_ptr = file->tell();

	// we have to check that we are at attr_ptr!
	char attr_header[4] = {'a', 't', 'r', '\n'};

	// check that we can really safe time in GameDate. ideally, we should use a static assertion here
	//static_assert (sizeof(uint32_t) >= sizeof(time_t), "time_t does not fit into 32bit" );

	file->array(attr_header, sizeof(attr_header));
	file->generic<AttributesInterface> ( mAttributes );

	// we need to check that we don't use more space than we got!

	// set up writing for next section. not good!
	attr_size = file->tell() - attr_ptr;
}

void ReplayRecorder::writeJumpTable(boost::shared_ptr<GenericOut> file) const
{
	jptb_ptr = file->tell();

	// we have to check that we are at attr_ptr!
	char jtbl_header[4] = {'j', 'p', 't', '\n'};

	file->array(jtbl_header, sizeof(jtbl_header));

	file->seek(jptb_ptr + 128);
}

void ReplayRecorder::writeInputSection(boost::shared_ptr<GenericOut> file) const
{
	data_ptr = file->tell();

	// we have to check that we are at attr_ptr!
	char data_header[4] = {'i', 'p', 't', '\n'};
	file->array(data_header, sizeof(data_header));

	file->generic<std::vector<unsigned char> > (mSaveData);

	/// \todo why don't we zip it? even though it's quite compact,
	/// 		we still save a lot of redundant information.

}

void ReplayRecorder::writeStatesSection(boost::shared_ptr<GenericOut> file) const
{
	states_ptr = file->tell();

	// we have to check that we are at attr_ptr!
	char states_header[4] = {'s', 't', 'a', '\n'};
	file->array(states_header, sizeof(states_header));

	file->generic<std::vector<ReplaySavePoint> > (mSavePoints);
}

void ReplayRecorder::record(const DuelMatchState& state)
{
	// save the state every REPLAY_SAVEPOINT_PERIOD frames
	// or when something interesting occurs
	/// \todo what is the purpose of this end score checks?
	if(mSaveData.size() % REPLAY_SAVEPOINT_PERIOD == 0 ||
		mAttributes.getAttributeInteger("LeftEndScore") != state.logicState.leftScore ||
		mAttributes.getAttributeInteger("RightEndScore") != state.logicState.rightScore ||
		state.errorSide != (unsigned char)NO_PLAYER)
	{
		ReplaySavePoint sp;
		sp.state = state;
		sp.step = mSaveData.size();
		mSavePoints.push_back(sp);
	}

	// we save this 1 here just for compatibility
	// set highest bit to 1
	unsigned char packet = 1 << 7;
	packet |= (state.playerInput[LEFT_PLAYER].getAll() & 7) << 3;
	packet |= (state.playerInput[RIGHT_PLAYER].getAll() & 7) ;
	mSaveData.push_back(packet);

	// update the score
	mAttributes.setAttributeInteger("LeftEndScore", state.logicState.leftScore);
	mAttributes.setAttributeInteger("LeftEndScore", state.logicState.rightScore);
}

void ReplayRecorder::setPlayerNames(const std::string& left, const std::string& right)
{
	mAttributes.setAttributeString("LeftPlayerName", left);
	mAttributes.setAttributeString("RightPlayerName", right);
}

void ReplayRecorder::setPlayerColors(Color left, Color right)
{
	mAttributes.setAttributeColor("LeftColor", left);
	mAttributes.setAttributeColor("RightColor", right);
}

void ReplayRecorder::setGameSpeed(int fps)
{
	mAttributes.setAttributeInteger( "GameSpeed", fps);
}

void ReplayRecorder::finalize(unsigned int left, unsigned int right)
{
	mAttributes.setAttributeInteger("LeftEndScore", left);
	mAttributes.setAttributeInteger("RightEndScore", right);

	// fill with one second of do nothing
	for(int i = 0; i < 75; ++i)
	{
		unsigned char packet = 0;
		mSaveData.push_back(packet);
	}

	mAttributes.setAttributeInteger("GameDuration", mSaveData.size() / mAttributes.getAttributeInteger("GameSpeed") );
	mAttributes.setAttributeInteger("GameDate", std::time(0));
}
