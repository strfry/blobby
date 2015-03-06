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
#include "NetworkGame.h"

/* includes */
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cassert>

#include <boost/make_shared.hpp>

#include "raknet/RakServer.h"
#include "raknet/BitStream.h"


#include "NetworkMessage.h"
#include "ReplayRecorder.h"
#include "FileRead.h"
#include "FileSystem.h"
#include "GenericIO.h"
#include "MatchEvents.h"
#include "NetworkPlayer.h"
#include "PacketHandler.h"
#include "input/AsyncInputSource.h"

// macro to bind functions for packet handler
#define BIND_MF(F) std::bind(&NetworkGame::F, this, std::placeholders::_1)

/* implementation */

NetworkGame::NetworkGame(RakServer& server, boost::shared_ptr<NetworkPlayer> leftPlayer, boost::shared_ptr<NetworkPlayer> rightPlayer,
			PlayerSide switchedSide, std::string rules)
: mServer(server)
, mMatch(new DuelMatch(false, rules))
, mLeftInput (new AsyncInputSource())
, mRightInput(new AsyncInputSource())
, mRecorder(new ReplayRecorder())
, mPausing(false)
, mGameValid(true)
, mHandler( new PacketHandler )
{
	// check that both players don't have an active game
	if(leftPlayer->getGame())
	{
		BOOST_THROW_EXCEPTION( std::runtime_error("Trying to start a game with player already in another game!") );
	}

	if(rightPlayer->getGame())
	{
		BOOST_THROW_EXCEPTION( std::runtime_error("Trying to start a game with player already in another game!") );
	}

	mMatch->setPlayers( leftPlayer->getIdentity(), rightPlayer->getIdentity() );
	mMatch->setInputSources(mLeftInput, mRightInput);

	mLeftPlayer = leftPlayer->getID();
	mRightPlayer = rightPlayer->getID();
	mSwitchedSide = switchedSide;

	mRecorder->setPlayerNames(leftPlayer->getName(), rightPlayer->getName());
	mRecorder->setPlayerColors(leftPlayer->getColor(), rightPlayer->getColor());
	mRecorder->setGameSpeed(SpeedController::getMainInstance()->getGameSpeed());

	// read rulesfile into a string
	int checksum = 0;
	mRulesLength = 0;
	mRulesSent[0] = false;
	mRulesSent[1] = false;

	FileRead file(std::string("rules/") + rules);
	checksum = file.calcChecksum(0);
	mRulesLength = file.length();
	mRulesString = file.readRawBytes(mRulesLength);

	// writing rules checksum
	RakNet::BitStream stream;
	stream.Write((unsigned char)ID_RULES_CHECKSUM);
	stream.Write(checksum);
	/// \todo write file author and title, too; maybe add a version number in scripts, too.
	broadcastBitstream(stream);

	mMatch->setStepCallback( std::bind(&NetworkGame::onMatchStep, this, std::placeholders::_1, std::placeholders::_2) );
	mMatch->run();

	// basic packet handling functions
	mHandler->registerHandler({ID_CONNECTION_LOST, ID_DISCONNECTION_NOTIFICATION}, [this](packet_ptr){h_disconnect();});
	mHandler->registerHandler({ID_REPLAY}, BIND_MF(h_replay));
	mHandler->registerHandler({ID_PAUSE, ID_UNPAUSE}, BIND_MF(h_pause));
	mHandler->registerHandler({ID_CHAT_MESSAGE}, BIND_MF(h_chat) );
	mHandler->registerHandler({ID_RULES}, BIND_MF(h_rules) );
	mHandler->registerHandler({ID_INPUT_UPDATE}, BIND_MF(h_input) );
}

NetworkGame::~NetworkGame()
{
}

void NetworkGame::injectPacket(const packet_ptr& packet)
{
	mPacketQueue.push_back(packet);
}

void NetworkGame::broadcastBitstream(const RakNet::BitStream& stream, const RakNet::BitStream& switchedstream) const
{
	// checks that stream and switchedstream don't have the same content.
	// this is a common mistake that arises from constructs like:
	//		BitStream stream
	//		... fill common data into stream
	//		BitStream switchedstream
	//		.. fill data depending on side in both streams
	//		broadcastBistream(stream, switchedstream)
	//
	//	here, the internal data of switchedstream is the same as stream so all
	//	changes made with switchedstream are done with stream alike. this was not
	//  the intention of this construct so it should be caught by this assertion.
	/// NEVER USE THIS FUNCTION LIKE broadcastBitstream(str, str), use, broadcastBitstream(str) instead
	/// this function is intended for sending two different streams to the two clients

	assert( &stream != &switchedstream );
	assert( stream.GetData() != switchedstream.GetData() );
	const RakNet::BitStream& leftStream = mSwitchedSide == LEFT_PLAYER ? switchedstream : stream;
	const RakNet::BitStream& rightStream = mSwitchedSide == RIGHT_PLAYER ? switchedstream : stream;

	mServer.Send(&leftStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mLeftPlayer, false);
	mServer.Send(&rightStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mRightPlayer, false);
}

void NetworkGame::broadcastBitstream(const RakNet::BitStream& stream) const
{

	mServer.Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mLeftPlayer, false);
	mServer.Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mRightPlayer, false);
}

void NetworkGame::processPackets()
{
	while (!mPacketQueue.empty())
	{
		packet_ptr packet = mPacketQueue.front();
		mPacketQueue.pop_front();
		mHandler->handlePacket( packet );
	}
}

bool NetworkGame::isGameValid() const
{
	return mGameValid;
}


void NetworkGame::step()
{
	if (!isGameStarted())
		return;

	broadcastGameEvents();
}

void NetworkGame::onMatchStep(const DuelMatchState& state, const std::vector<MatchEvent>& events)
{
	mRecorder->record( state );
	if (state.getWinningPlayer() != NO_PLAYER)
	{
		// duplicate last state
		/// \todo why is this necessary?
		mRecorder->record( state );
		mRecorder->finalize( state.getScore(LEFT_PLAYER), state.getScore(RIGHT_PLAYER) );
	}

	// don't record the pauses
	if(!mMatch->isPaused())
	{
		// we can do that here, because RakNet adds packets to the send list in a thread save way (i hope it works^^)
		broadcastPhysicState( state );
	}
}

void NetworkGame::writeEventToStream(RakNet::BitStream& stream, MatchEvent e, bool switchSides ) const
{
	stream.Write((unsigned char)e.event);
	if( switchSides )
		stream.Write((unsigned char)(e.side == LEFT_PLAYER ? RIGHT_PLAYER : LEFT_PLAYER ) );
	else
		stream.Write((unsigned char)e.side);
	if( e.event == MatchEvent::BALL_HIT_BLOB )
		stream.Write( e.intensity );
}

void NetworkGame::broadcastPhysicState(const DuelMatchState& state) const
{
	DuelMatchState ms = state;	// modifyable copy

	RakNet::BitStream stream;
	stream.Write((unsigned char)ID_GAME_UPDATE);

	/// \todo this required dynamic memory allocation! not good!
	boost::shared_ptr<GenericOut> out = createGenericWriter( &stream );

	if (mSwitchedSide == LEFT_PLAYER)
		ms.swapSides();

	out->generic<DuelMatchState> (ms);

	mServer.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0, mLeftPlayer, false);

	// reset state and stream
	stream.Reset();
	stream.Write((unsigned char)ID_GAME_UPDATE);

	out = createGenericWriter( &stream );

	// either switch back, or perform switching for right side
	if (mSwitchedSide == LEFT_PLAYER || mSwitchedSide == RIGHT_PLAYER)
		ms.swapSides();

	out->generic<DuelMatchState> (ms);

	mServer.Send(&stream, HIGH_PRIORITY, UNRELIABLE_SEQUENCED, 0, mRightPlayer, false);
}

void NetworkGame::broadcastGameEvents() const
{
	RakNet::BitStream stream;

	auto events = mMatch->fetchEvents();
	// send the events
	if( events.empty() )
		return;
	// add all the events to the stream
	stream.Write( (unsigned char)ID_GAME_EVENTS );
	for(auto& e : events)
		writeEventToStream(stream, e, mSwitchedSide == LEFT_PLAYER );
	stream.Write((char)0);
	mServer.Send( &stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mLeftPlayer, false);

	stream.Reset();
	for(auto& e : events)
		writeEventToStream(stream, e, mSwitchedSide == RIGHT_PLAYER );
	stream.Write((char)0);
	mServer.Send( &stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mRightPlayer, false);
}

PlayerID NetworkGame::getPlayerID( PlayerSide side ) const
{
	if( side == LEFT_PLAYER )
	{
		return mLeftPlayer;
	}
	else if(side == RIGHT_PLAYER)
	{
		return mRightPlayer;
	}

	assert(0);
}

// -----------------------------------------------------------------------------------------
//							packet handling functions
// -----------------------------------------------------------------------------------------
void NetworkGame::h_disconnect( )
{
	RakNet::BitStream stream;
	stream.Write((unsigned char)ID_OPPONENT_DISCONNECTED);
	broadcastBitstream(stream);
	mMatch->pause();
	mGameValid = false;
}

void NetworkGame::h_replay( packet_ptr packet )
{
	RakNet::BitStream stream = RakNet::BitStream();
	stream.Write((unsigned char)ID_REPLAY);
	boost::shared_ptr<GenericOut> out = createGenericWriter( &stream );
	mRecorder->send( out );
	assert( stream.GetData()[0] == ID_REPLAY );

	mServer.Send(&stream, LOW_PRIORITY, RELIABLE_ORDERED, 0, packet->playerId, false);
}

void NetworkGame::h_pause( packet_ptr packet )
{
	unsigned char pause = packet->data[0];
	assert( pause == ID_PAUSE || pause == ID_UNPAUSE );

	// set pause mode
	if( pause == ID_PAUSE )
		mMatch->pause();
	else
		mMatch->unpause();

	// and broadcast to the clients
	RakNet::BitStream stream;
	stream.Write( pause );
	broadcastBitstream( stream );
}

void NetworkGame::h_chat( packet_ptr packet )
{
	RakNet::BitStream stream((char*)packet->data, packet->length, false);

	stream.IgnoreBytes(1); // ID_CHAT_MESSAGE
	char message[31];
	/// \todo we need to acertain that this package contains at least 31 bytes!
	///			otherwise, we send just uninitialized memory to the client
	///			thats no real security problem but i think we should address
	///			this nonetheless
	stream.Read(message, sizeof(message));

	RakNet::BitStream stream2;
	stream2.Write((unsigned char)ID_CHAT_MESSAGE);
	stream2.Write(message, sizeof(message));
	if (mLeftPlayer == packet->playerId)
		mServer.Send(&stream2, LOW_PRIORITY, RELIABLE_ORDERED, 0, mRightPlayer, false);
	else
		mServer.Send(&stream2, LOW_PRIORITY, RELIABLE_ORDERED, 0, mLeftPlayer, false);
}

void NetworkGame::h_rules( packet_ptr packet )
{
	boost::shared_ptr<RakNet::BitStream> stream = boost::make_shared<RakNet::BitStream>();
	bool needRules;
	stream->Read(needRules);
	mRulesSent[mLeftPlayer == packet->playerId ? LEFT_PLAYER : RIGHT_PLAYER] = true;

	if (needRules)
	{
		stream = boost::make_shared<RakNet::BitStream>();
		stream->Write((unsigned char)ID_RULES);
		stream->Write(mRulesLength);
		stream->Write(mRulesString.get(), mRulesLength);
		assert( stream->GetData()[0] == ID_RULES );

		mServer.Send(stream.get(), HIGH_PRIORITY, RELIABLE_ORDERED, 0, packet->playerId, false);
	}

	if (isGameStarted())
	{
		/// \param side: data of the player to write into the stream
		auto makeStream = [this](PlayerSide side)
		{
			// buffer for playernames
			char name[16];

			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_GAME_READY);
			stream.Write((int)SpeedController::getMainInstance()->getGameSpeed());
			strncpy(name, mMatch->getPlayer(side).getName().c_str(), sizeof(name));
			stream.Write(name, sizeof(name));
			stream.Write(mMatch->getPlayer(side).getStaticColor().toInt());
			return stream;
		};

		// writing data into leftStream
		RakNet::BitStream leftStream = makeStream( RIGHT_PLAYER );
		// writing data into rightStream
		RakNet::BitStream rightStream = makeStream( LEFT_PLAYER );

		mServer.Send(&leftStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mLeftPlayer, false);
		mServer.Send(&rightStream, HIGH_PRIORITY, RELIABLE_ORDERED, 0, mRightPlayer, false);
	}
}

void NetworkGame::h_input( packet_ptr packet )
{
	RakNet::BitStream stream((char*)packet->data, packet->length, false);

	// ignore ID_INPUT_UPDATE
	stream.IgnoreBytes(1);
	PlayerInputAbs newInput(stream);

	/// \todo this accesses the input sources for the game,
	/// thus it is currently not thread-safe
	if (packet->playerId == mLeftPlayer)
	{
		if (mSwitchedSide == LEFT_PLAYER)
			newInput.swapSides();
		mLeftInput->setInput(newInput);
	}
	if (packet->playerId == mRightPlayer)
	{
		if (mSwitchedSide == RIGHT_PLAYER)
			newInput.swapSides();
		mRightInput->setInput(newInput);
	}
}
