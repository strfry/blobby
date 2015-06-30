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

#include "LobbyState.h"

#include <set>
#include <stdexcept>
#include <iostream>

#include <boost/lexical_cast.hpp>

#include "IMGUI.h"
#include "NetworkState.h"
#include "UserConfig.h"
#include "GenericIO.h"

LobbyState::LobbyState(ServerInfo info) : mClient(new RakClient(), [](RakClient* client) { client->Disconnect(25); }), mInfo(info), mSelectedGame(0)
{
	if (!mClient->Connect(mInfo.hostname, mInfo.port, 0, 0, RAKNET_THREAD_SLEEP_TIME))
		throw( std::runtime_error(std::string("Could not connect to server ") + mInfo.hostname) );

	mLobbyState = CONNECTING;

	// send an ENTER_SERVER packet with name and side preference
	RenderManager::getSingleton().redraw();

	/// \todo we need read-only access here!
	UserConfig config;
	config.loadFile("config.xml");
	PlayerSide side = (PlayerSide)config.getInteger("network_side");

	// load player identity
	if(side == LEFT_PLAYER)
	{
		mLocalPlayer = config.loadPlayerIdentity(LEFT_PLAYER, true);
	}
	 else
	{
		mLocalPlayer = config.loadPlayerIdentity(RIGHT_PLAYER, true);
	}

	mMainSubstate = new LobbyMainSubstate(mClient, mInfo);
}

LobbyState::~LobbyState()
{
	// we should properly disconnect if we do not connect to server!
	//mClient->Disconnect(50);
}

void LobbyState::step_impl()
{
	// process packets
	packet_ptr packet;
	while (packet = mClient->Receive())
	{
		switch(packet->data[0])
		{
			case ID_CONNECTION_REQUEST_ACCEPTED:
			{
				RakNet::BitStream stream;
				stream.Write((unsigned char)ID_ENTER_SERVER);

				// Send preferred side
				stream.Write( mLocalPlayer.getPreferredSide() );

				// Send playername
				char myname[16];
				strncpy(myname, mLocalPlayer.getName().c_str(), sizeof(myname));
				stream.Write(myname, sizeof(myname));

				// send color settings
				stream.Write(mLocalPlayer.getStaticColor().toInt());

				mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);

				mLobbyState = CONNECTED;
				break;
			}
			case ID_CONNECTION_ATTEMPT_FAILED:
			{
				mLobbyState = CONNECTION_FAILED;
				break;
			}

			case ID_CONNECTION_LOST:
			case ID_DISCONNECTION_NOTIFICATION:
			{
				mLobbyState = DISCONNECTED;
				break;
			};
			case ID_LOBBY:
			{
				RakNet::BitStream stream = packet->getStream();
				auto in = createGenericReader( &stream );
				unsigned char t;
				in->byte(t);
				in->byte(t);
				if((LobbyPacketType)t == LobbyPacketType::SERVER_STATUS)
				{
					uint32_t player_count;
					in->uint32( player_count );
					in->generic<std::vector<unsigned int>>( mPossibleSpeeds );
					in->generic<std::vector<std::string>>( mPossibleRules );
					std::cout << mPossibleSpeeds.size() << "\n";
					
					std::vector<unsigned int> gameids;
					std::vector<std::string> gamenames;
					std::vector<unsigned char> gamespeeds;
					std::vector<unsigned char> gamerules;
					std::vector<unsigned char> gamescores;
					in->generic<std::vector<unsigned int>>( gameids );
					in->generic<std::vector<std::string>>( gamenames );
					in->generic<std::vector<unsigned char>>( gamespeeds );
					in->generic<std::vector<unsigned char>>( gamerules );
					in->generic<std::vector<unsigned char>>( gamescores );
					
					mOpenGames.clear();
					for( unsigned i = 0; i < gameids.size(); ++i)
					{
						mOpenGames.push_back( OpenGame{ gameids.at(i), gamenames.at(i), gamerules.at(i), gamespeeds.at(i), gamescores.at(i)});
					}
				} else if((LobbyPacketType)t == LobbyPacketType::GAME_STATUS)
				{
					in->uint32( mOwnGame );
					PlayerID creator;
					std::string name;
					in->generic<PlayerID>(creator);
					in->string(name);
					in->uint32(mChosenSpeed);
					in->uint32(mChosenRules);
					in->uint32(mChosenScore);
					in->generic<std::vector<PlayerID>>(mOtherPlayers);
				}
			}
			break;
			// we ignore these packets. they tell us about remote connections, which we handle manually with ID_SERVER_STATUS packets.
			case ID_REMOTE_EXISTING_CONNECTION:
			case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			case ID_REMOTE_NEW_INCOMING_CONNECTION:
				// not surprising, but we are not interested in his packet
				break;
			default:
				std::cout << "Unknown packet " << int(packet->data[0]) << " received\n";
		}
	}


	IMGUI& imgui = IMGUI::getSingleton();

	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	imgui.doInactiveMode(false);

	// server name
	imgui.doText(GEN_ID, Vector2(400 - 12 * std::strlen(mInfo.name), 20), mInfo.name);

	// server description
	if (mLobbyState != CONNECTED )
	{
		mMainSubstate->step();
	}
	else
	{
		std::string description = mInfo.description;
		for (unsigned int i = 0; i < description.length(); i += 63)
		{
			imgui.doText(GEN_ID, Vector2(25, 55 + i / 63 * 15), description.substr(i, 63), TF_SMALL_FONT);
		}
	}

	// player list

	std::vector<std::string> gamelist;
	if( mLobbyState == CONNECTED )
	{
		gamelist.push_back( TextManager::getSingleton()->getString(TextManager::NET_RANDOM_OPPONENT) );
		gamelist.push_back( TextManager::getSingleton()->getString(TextManager::NET_OPEN_GAME) );
		for ( const auto& game : mOpenGames)
		{
			gamelist.push_back( game.name );
		}
	}

	bool doEnterGame = false;
	if( imgui.doSelectbox(GEN_ID, Vector2(25.0, 90.0), Vector2(375.0, 470.0), gamelist, mSelectedGame) == SBA_DBL_CLICK )
	{
		doEnterGame = true;
	}

	if(mSelectedGame > 1)
	{
		// info panel
		imgui.doOverlay(GEN_ID, Vector2(425.0, 90.0), Vector2(775.0, 470.0));

		// info panel contents:
		//  * gamespeed
		imgui.doText(GEN_ID, Vector2(435, 100), TextManager::getSingleton()->getString(TextManager::NET_SPEED) +
					 boost::lexical_cast<std::string>(int(0.5 + 100.0 / 75.0 * mPossibleSpeeds.at(mOpenGames.at(mSelectedGame-2).speed))) + "%");
		//  * points
		imgui.doText(GEN_ID, Vector2(435, 135), TextManager::getSingleton()->getString(TextManager::NET_POINTS) +
											 boost::lexical_cast<std::string>(mOpenGames.at(mSelectedGame-2).score) );
		
		//  * rulesfile
		imgui.doText(GEN_ID, Vector2(435, 170), TextManager::getSingleton()->getString(TextManager::NET_RULES_TITLE) );
		std::string rulesstring = mPossibleRules.at(mOpenGames.at(mSelectedGame-2).rules) + TextManager::getSingleton()->getString(TextManager::NET_RULES_BY) + mInfo.rulesauthor;
		for (unsigned int i = 0; i < rulesstring.length(); i += 25)
		{
			imgui.doText(GEN_ID, Vector2(445, 205 + i / 25 * 15), rulesstring.substr(i, 25), TF_SMALL_FONT);
		}
		
		// open game button
		if( imgui.doButton(GEN_ID, Vector2(435, 430), TextManager::getSingleton()->getString(TextManager::NET_JOIN) ))
		{
			// send open game packet to server
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_LOBBY);
			stream.Write((unsigned char)LobbyPacketType::JOIN_GAME);
			stream.Write( mOpenGames.at(mSelectedGame-2).id );
			/// \todo add a name
			
			mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
		}
	} 
	// open game
	else if (mSelectedGame == 1)
	{
		// info panel
		imgui.doOverlay(GEN_ID, Vector2(425.0, 90.0), Vector2(775.0, 470.0));

		// info panel contents:
		//  * gamespeed
		if(imgui.doButton(GEN_ID, Vector2(435, 100), TextManager::getSingleton()->getString(TextManager::NET_SPEED) +
					 boost::lexical_cast<std::string>(int(0.5 + 100.0 / 75.0 * mPossibleSpeeds.at(mChosenSpeed))) + "%"))
		{
			mChosenSpeed = (mChosenSpeed + 1) % mPossibleSpeeds.size();
		}
		//  * points
		if(imgui.doButton(GEN_ID, Vector2(435, 135), TextManager::getSingleton()->getString(TextManager::NET_POINTS) +
											 boost::lexical_cast<std::string>( mPossibleScores.at(mChosenScore) ) ))
		{
			mChosenScore = (mChosenScore + 1) % mPossibleScores.size();
		}
		
		//  * rulesfile
		if(imgui.doButton(GEN_ID, Vector2(435, 170), TextManager::getSingleton()->getString(TextManager::NET_RULES_TITLE) ))
		{
			mChosenRules = (mChosenRules + 1) % mPossibleRules.size();
		}
		std::string rulesstring = mPossibleRules.at(mChosenRules) + TextManager::getSingleton()->getString(TextManager::NET_RULES_BY) + mInfo.rulesauthor;
		for (unsigned int i = 0; i < rulesstring.length(); i += 25)
		{
			imgui.doText(GEN_ID, Vector2(445, 205 + i / 25 * 15), rulesstring.substr(i, 25), TF_SMALL_FONT);
		}
		
		// open game button
		if( imgui.doButton(GEN_ID, Vector2(435, 430), TextManager::getSingleton()->getString(TextManager::NET_OPEN_GAME) ))
		{
			// send open game packet to server
			RakNet::BitStream stream;
			stream.Write((unsigned char)ID_LOBBY);
			stream.Write((unsigned char)LobbyPacketType::OPEN_GAME);
			stream.Write( mChosenSpeed );
			stream.Write( mPossibleScores.at(mChosenScore) );
			stream.Write( mChosenRules );
			/// \todo add a name
			
			mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);
		}
	}



	// back button
	if (imgui.doButton(GEN_ID, Vector2(480, 530), TextManager::LBL_CANCEL))
	{
		switchState( new MainMenuState );
	}

	// ok button
	/*if (mLobbyState == CONNECTED && (imgui.doButton(GEN_ID, Vector2(230, 530), TextManager::LBL_OK) || doEnterGame))
	{
		RakNet::BitStream stream;
		stream.Write((char)ID_CHALLENGE);
		auto writer = createGenericWriter(&stream);
		if( mSelectedPlayer != 0 )
		{
			writer->generic<PlayerID>( mConnectedPlayers[mSelectedPlayer-1].id );
		}
		 else
		{
			writer->generic<PlayerID>( UNASSIGNED_PLAYER_ID );
		}

		mClient->Send(&stream, HIGH_PRIORITY, RELIABLE_ORDERED, 0);

		switchState( new NetworkGameState(mClient) );
	}*/
}

const char* LobbyState::getStateName() const
{
	return "LobbyState";
}

// ******************************************************************************
// 						Lobby in main state
// ******************************************************************************

void LobbyMainSubstate::step()
{
	IMGUI& imgui = IMGUI::getSingleton();

	imgui.doCursor();
	imgui.doImage(GEN_ID, Vector2(400.0, 300.0), "background");
	imgui.doOverlay(GEN_ID, Vector2(0.0, 0.0), Vector2(800.0, 600.0));
	imgui.doInactiveMode(false);

	// server name
	imgui.doText(GEN_ID, Vector2(400 - 12 * std::strlen(mInfo.name), 20), mInfo.name);

	// server description
	if (mLobbyStatus == CONNECTING )
	{
		imgui.doText(GEN_ID, Vector2( 100, 55 ), TextManager::NET_CONNECTING);
	}
	else if (mLobbyStatus == DISCONNECTED )
	{
		imgui.doText(GEN_ID, Vector2( 100, 55 ), TextManager::NET_DISCONNECT);
	}
	else if (mLobbyStatus == CONNECTION_FAILED )
	{
		imgui.doText(GEN_ID, Vector2( 100, 55 ), TextManager::NET_CON_FAILED);
	}
	
	// player list
	unsigned int s;
	imgui.doSelectbox(GEN_ID, Vector2(25.0, 90.0), Vector2(375.0, 470.0), std::vector<std::string>{}, s);

	// info panel
	imgui.doOverlay(GEN_ID, Vector2(425.0, 90.0), Vector2(775.0, 470.0));
}
