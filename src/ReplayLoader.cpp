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
#include "IReplayLoader.h"

/* includes */
#include <cassert>
#include <algorithm>
#include <vector>
#include <ctime>

#include <boost/make_shared.hpp>

#include "InputSource.h"
#include "FileRead.h"
#include "GenericIO.h"
#include "AttributesInterface.h"

const SavepointIndex SavepointIndex::NO_SAVEPOINT;

/* implementation */
IReplayLoader* IReplayLoader::createReplayLoader(const std::string& filename)
{
	// do some generic loading stuff:
	// first, try to open the file
	boost::shared_ptr<FileRead> file = boost::make_shared<FileRead>(filename);

	// then, check the file length. We need at least 12 bytes.
	int fileLength = file->length();
	if (fileLength < 12)
	{
		/// \todo add some error handling here!
		return 0;
	}

	// check if file contains a valid BV2 header
	char header[4];
	file->readRawBytes(header, sizeof(header));
	if (memcmp(&header, &validHeader, 4) != 0)
	{
		/// \todo add some error handling here!
		return 0;
	}

	// now, find out which version we need!
	char version[4];
	file->readRawBytes(version, sizeof(version));

	// now we got our version number.
	int major = version[1];
	int minor = version[2];

	// read checksum
	uint32_t checksum;
	file->readRawBytes((char*)&checksum, 4);

	// calculate reference checksum
	uint32_t refchecksum = file->calcChecksum(file->tell());

	if(refchecksum != checksum && major > 0 && minor > 0)
	{
		BOOST_THROW_EXCEPTION (ChecksumException(file->getFileName(), checksum, refchecksum));
	}

	IReplayLoader* loader = createReplayLoader(major);
	boost::shared_ptr<GenericIn> in = createGenericReader(file);
	loader->initLoading(in, minor);

	return loader;
}

/***************************************************************************************************
			              R E P L A Y   L O A D E R    V 0.1
***************************************************************************************************/

// That version used different physics than we use now, and it does not include any save-points that would
// allow to extrapolate the match, so we could not play these matches, even if we had a loader for them

//
// -------------------------------------------------------------------------------------------------
//


/***************************************************************************************************
			              R E P L A Y   L O A D E R    V 1.x
***************************************************************************************************/


/*! \class ReplayLoader_V2X
	\brief Replay Loader V 2.x
	\details Replay Loader for 2.0 replays
*/
class ReplayLoader_V2X: public IReplayLoader
{
	public:
		ReplayLoader_V2X() {};

		virtual ~ReplayLoader_V2X() { };

		virtual int getVersionMajor() const { return 2; };
		virtual int getVersionMinor() const { return 0; };

		virtual std::string getPlayerName(PlayerSide player) const
		{
			if(player == LEFT_PLAYER)
				return mAttributes.getAttributeString("LeftPlayerName");
			if(player == RIGHT_PLAYER)
				return mAttributes.getAttributeString("RightPlayerName");

			assert(0);
		}

		virtual Color getBlobColor(PlayerSide player) const
		{
			if(player == LEFT_PLAYER)
				return mAttributes.getAttributeColor("LeftColor");
			if(player == RIGHT_PLAYER)
				return mAttributes.getAttributeColor("RightColor");

			assert(0);
		}


		virtual int getFinalScore(PlayerSide player) const
		{
			if(player == LEFT_PLAYER)
				return mAttributes.getAttributeInteger("LeftFinalScore");
			if(player == RIGHT_PLAYER)
				return mAttributes.getAttributeInteger("RightFinalScore");

			assert(0);
		}

		virtual int getSpeed() const
		{
			return mAttributes.getAttributeInteger("GameSpeed");
		};

		virtual int getDuration() const
		{
			return mAttributes.getAttributeInteger("GameDuration");
		};

		virtual int getLength()  const
		{
			return mGameLength;
		};

		virtual std::time_t getDate() const
		{
			return mAttributes.getAttributeInteger("GameDate");
		};

		virtual const AttributesInterface& getAttributes() const
		{
			return mAttributes;
		}


		virtual void getInputAt(unsigned int step, InputSource* left, InputSource* right)
		{
			assert( step  < mGameLength );

			// for now, we have only a linear sequence of InputPackets, so finding the right one is just
			// a matter of address arithmetics.

			// each packet has size 1 byte for now
			// so we find step at mReplayOffset + step
			char packet = mBuffer[mReplayOffset + step];

			// now read the packet data
			left->setInput(PlayerInput((bool)(packet & 32), (bool)(packet & 16), (bool)(packet & 8)));
			right->setInput(PlayerInput((bool)(packet & 4), (bool)(packet & 2), (bool)(packet & 1)));
		}

		virtual bool isSavePoint(int position, SavepointIndex& save_position) const
		{
			int foundPos;
			save_position = getSavePoint(position, foundPos);
			return save_position.isValid() && foundPos == position;
		}

		// TODO: add optional argument: int previous = 0;
		// 		so we can start from it when calling
		// 		getSavePoint in a row (without "jumping").
		// 		we can save this parameter in ReplayPlayer
		virtual SavepointIndex getSavePoint(int targetPosition, int& savepoint) const
		{
			// desired index can't be lower that this value,
			// cause additional savepoints could shift it only right
			unsigned int index = targetPosition / REPLAY_SAVEPOINT_PERIOD;

			if(index >= mSavePointsCount)
				return SavepointIndex::NO_SAVEPOINT;

			savepoint = mSavePoints[index].step;

			// watch right from initial index,
			// cause best savepoint could be there.
			// we have no much additional savepoints,
			// so this cycle would be fast,
			// maybe even faster than binary search.
			index -= 1;
			do
			{
				unsigned int nextIndex = index + 1;

				if (nextIndex >= mSavePointsCount)
					break;

				int nextPos = mSavePoints[nextIndex].step;

				if (nextPos > targetPosition)
					break;

				index = nextIndex;
				savepoint = nextPos;
			} while (true);

			return SavepointIndex(index);
		}

		virtual void readSavePoint(SavepointIndex index, ReplaySavePoint& state) const
		{
			state = mSavePoints.at(index.index);
		}

	private:
		virtual void initLoading(boost::shared_ptr<GenericIn> file, int minor_version)
		{
			mReplayFormatVersion = minor_version;
			mSavePoints.resize(0);
			/// \todo check if minor_version < getVersionMinor, otherwise issue a warning

			// we start with the replay header.
			uint32_t header_size, attr_ptr , attr_size ,
					jptb_ptr, jptb_size , data_ptr , data_size,
					states_ptr, states_size;

			file->uint32(header_size);
			file->uint32(attr_ptr);
			file->uint32(attr_size);
			file->uint32(jptb_ptr);
			file->uint32(jptb_size);
			file->uint32(data_ptr);
			file->uint32(data_size);
			file->uint32(states_ptr);
			file->uint32(states_size);

			mGameLength = data_size;

			// now, we read the attributes section
			//  jump over the attr - marker
			file->seek(attr_ptr + 4);
			// copy attributes into buffer

			file->generic<AttributesInterface> (mAttributes);

			// now, read the raw data
			file->seek(data_ptr + 8);		// jump over the dat marker and over the length value
			// read into buffer
			mBuffer = boost::shared_array<char>(new char[data_size]);
			file->array(mBuffer.get(), data_size);
			mReplayOffset = 0;

			// now read savepoints
			file->seek(states_ptr + 4);		// jump over the sta marker
			file->uint32(mSavePointsCount);
			std::cout << "SAVE POINT COUNT: " << mSavePointsCount << "\n";
			mSavePoints.reserve(mSavePointsCount);
			for(unsigned int i = 0; i < mSavePointsCount; ++i)
			{
				ReplaySavePoint sp;
				file->generic<ReplaySavePoint>(sp);
				mSavePoints.push_back(sp);
			}


			/// \todo check that mSavePointsCount and states_size match

		}


		boost::shared_array<char> mBuffer;
		uint32_t mReplayOffset;

		std::vector<ReplaySavePoint> mSavePoints;
		uint32_t mSavePointsCount;

		// specific data
		unsigned int mGameLength;

		AttributesInterface mAttributes;

		unsigned char mReplayFormatVersion;
};



IReplayLoader* IReplayLoader::createReplayLoader(int major)
{
	// we find a loader depending on major version
	/// \todo throw, when version is too old
	switch(major)
	{
		case 0:
			break;
		case 1:
			break;
		case 2:
			return new ReplayLoader_V2X();
			break;
	}

	// fallback
	return 0;
}
