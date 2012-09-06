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
#include "DedicatedServer.h"

/* includes */
#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <ctime>

#include <errno.h>
#include <unistd.h>

#include "raknet/RakServer.h"
#include "raknet/PacketEnumerations.h"
#include "raknet/GetTime.h"

#include <SDL/SDL_timer.h>
#include <boost/lexical_cast.hpp>

#include "InputSource.h"
#include "PhysicWorld.h"
#include "NetworkGame.h"
#include "UserConfig.h"
#include "NetworkMessage.h"
#include "SpeedController.h"
#include "RakNetPacket.h"
#include "NetworkPlayer.h"
#include "FileSystem.h"
#include "PacketLogger.h"
#include "BlobbyDebug.h"

// platform specific
#ifndef WIN32
#include <syslog.h>
#include <sys/wait.h>
#else
#include <cstdarg>
#endif



/* implementation */

#ifdef WIN32
#undef main

// function for logging to replacing syslog
enum {
	LOG_ERR,
	LOG_NOTICE,
	LOG_DEBUG
};
void syslog(int pri, const char* format, ...);

#endif

static bool g_run_in_foreground = false;
static bool g_print_syslog_to_stderr = false;
static bool g_workaround_memleaks = false;

// ...
void printHelp();
void process_arguments(int argc, char** argv);
void fork_to_background();
void wait_and_restart_child();
void setup_physfs(char* argv0);

// server workload statistics
int SWLS_PacketCount = 0;
int SWLS_Connections = 0;
int SWLS_Games		 = 0;
int SWLS_GameSteps	 = 0;
int SWLS_RunningTime = 0;
int SWLS_IngamePacketsProcessed = 0;
int SWLS_IngameEventCounter = 0;
int SWLS_PhysicStateBroadcasts = 0;

// functions for processing certain network packets
void createNewGame();

#include <fstream>

int main(int argc, char** argv)
{
	try
	{
	process_arguments(argc, argv);
	FileSystem fileSys(argv[0]);
	
	if (!g_run_in_foreground)
	{
		fork_to_background();
	}

	if (g_workaround_memleaks)
	{
		wait_and_restart_child();
	}
	

	int startTime = SDL_GetTicks();

	#ifndef WIN32
	int syslog_options = LOG_CONS | LOG_PID | (g_print_syslog_to_stderr ? LOG_PERROR : 0);

	openlog("blobby-server", syslog_options, LOG_DAEMON);
	#endif
	
	setup_physfs(argv[0]);

	GameList gamelist;
	PlayerMap playermap;
	RakServer server;
	UserConfig config;

	NetworkPlayer firstPlayer;

	int port = BLOBBY_PORT;
	try 
	{
		config.loadFile("server.xml");
		port = config.getInteger("port");
	} 
	catch (std::exception& e) 
	{
		syslog(LOG_ERR, "server.xml not found. Falling back to default values.");
	}
	
	int clients = 0;

	ServerInfo myinfo(config);
	
	float speed = myinfo.gamespeed;

	if (!server.Start(150, 1, port))
	{
		syslog(LOG_ERR, "Couldn´t bind to port %i, exiting", port);
		return 2;
	}

	SpeedController scontroller(speed);
	SpeedController::setMainInstance(&scontroller);

	syslog(LOG_NOTICE, "Blobby Volley 2 dedicated server version %i.%i started", BLOBBY_VERSION_MAJOR, BLOBBY_VERSION_MINOR);

	packet_ptr packet;
	PacketLogger plogger;
	
	plogger.setHumanReadableType(ID_NEW_INCOMING_CONNECTION, "ID_NEW_INCOMING_CONNECTION");
	plogger.setHumanReadableType(ID_CONNECTION_LOST, 		 "ID_CONNECTION_LOST");
	plogger.setHumanReadableType(ID_DISCONNECTION_NOTIFICATION, "ID_DISCONNECTION_NOTIFICATION");
	plogger.setHumanReadableType(ID_INPUT_UPDATE, 			"ID_INPUT_UPDATE");
	plogger.setHumanReadableType(ID_PAUSE, 					"ID_PAUSE");
	plogger.setHumanReadableType(ID_UNPAUSE, 				"ID_UNPAUSE");
	plogger.setHumanReadableType(ID_CHAT_MESSAGE, 			"ID_CHAT_MESSAGE");
	plogger.setHumanReadableType(ID_REPLAY, 				"ID_REPLAY");
	plogger.setHumanReadableType(ID_ENTER_GAME, 			"ID_ENTER_GAME");
	plogger.setHumanReadableType(ID_PONG, 					"ID_PONG");
	plogger.setHumanReadableType(ID_BLOBBY_SERVER_PRESENT, 	"ID_BLOBBY_SERVER_PRESENT");
	plogger.setHumanReadableType(ID_RECEIVED_STATIC_DATA, 	"ID_RECEIVED_STATIC_DATA");
	//plogger.setHumanReadableType("ID_PONG", ID_PONG);
	//plogger.setHumanReadableType("ID_PONG", ID_PONG);

//	std::fstream packet_log("logs/packets.txt", std::fstream::out);
//	packet_log << "#time\torigen\tid\tcontent\n";
	while (1)
	{
		
		// -------------------------------------------------------------------------------
		// process all incoming packets , probably relay them to responsible network games
		// -------------------------------------------------------------------------------
		
		while ((packet = receivePacket(&server)))
		{
			
			// alle packete mitloggen
			//packet_log << SWLS_RunningTime << "\t" << packet->playerId.binaryAddress << ":" << packet->playerId.port << "\t" << (int)packet->data[0] << "\t" << packet->data << "\n";
			plogger << packet.get();
			SWLS_PacketCount++;
			
			switch(packet->data[0])
			{
				case ID_NEW_INCOMING_CONNECTION:
					clients++;
					SWLS_Connections++;
					syslog(LOG_DEBUG, "New incoming connection, %d clients connected now", clients);
					break;
				case ID_CONNECTION_LOST:
				case ID_DISCONNECTION_NOTIFICATION:
				{
					bool cond1 = firstPlayer.valid();
					bool cond2 = firstPlayer.getID() == packet->playerId;
					// if first player disconncted, reset
					if (cond1 && cond2)
						firstPlayer = NetworkPlayer();
					
					// delete the disconnectiong player
					if ( playermap.find(packet->playerId) != playermap.end() ) 
					{
						/// \todo what are we doing here???
						/// seems not a good idea to let injectPacket remove the game from the game list...
						/// maybe we should add a centralized way to delete unused games  and players!
						// inject the packet into the game
						/// strange, injectPacket just pushes the packet into a queue. That cannot delete 
						/// the game???
						playermap[packet->playerId]->injectPacket(packet);
						
						// if it was the last player, the game is removed from the game list.
						// thus, give the game a last chance to process the last
						// input
						
						// check, wether game was removed from game list (not a good idea!), in that case, process manually 
						if( std::find(gamelist.begin(), gamelist.end(), playermap[packet->playerId]) == gamelist.end())
						{
							playermap[packet->playerId]->step();
						}
						
						// then delete the player
						playermap.erase(packet->playerId);
					}
					
					clients--;
					syslog(LOG_DEBUG, "Connection closed, %d clients connected now", clients);
					break;
				}
				case ID_INPUT_UPDATE:
				case ID_PAUSE:
				case ID_UNPAUSE:
				case ID_CHAT_MESSAGE:
				case ID_REPLAY:
					if (playermap.find(packet->playerId) != playermap.end()){
						playermap[packet->playerId]->injectPacket(packet);
						
						// check, wether game was delete from this, in this case, process manually 
						/// \todo here again, injectPacket is not able to delete the game. So, what are we doing here?
						if( std::find(gamelist.begin(), gamelist.end(), playermap[packet->playerId]) == gamelist.end())
						{
							playermap[packet->playerId]->step();
						}
						
					} else {
						syslog(LOG_ERR, "player not found!");
						#ifdef DEBUG
						std::cout	<< " received game packet for no longer existing game! " 
									<< (int)packet->data[0] << " - "
									<< packet->playerId.binaryAddress << " : " << packet->playerId.port
									<< "\n";
						// only quit in debug mode as this is not a problem endangering the stability
						// of the running server, but a situation that should never occur.
						//return 3;
						#endif
					}
						
					break;
				case ID_ENTER_GAME:
				{
					RakNet::BitStream stream((char*)packet->data,
							packet->length, false);
					
					stream.IgnoreBytes(1);	//ID_ENTER_GAME

					if (!firstPlayer.valid())
					{
						/// \todo does the copy-ctor what i assume it does? deep copy?
						firstPlayer = NetworkPlayer(packet->playerId, stream);
					}
					else // We have two players now
					{
						NetworkPlayer secondPlayer = NetworkPlayer(packet->playerId, stream);
						/// \todo refactor this, this code is awful!
						///  one swap should be enough
						
						NetworkPlayer leftPlayer = firstPlayer;
						NetworkPlayer rightPlayer = secondPlayer;
						PlayerSide switchSide = NO_PLAYER;
						
						if(RIGHT_PLAYER == firstPlayer.getDesiredSide())
						{
							std::swap(leftPlayer, rightPlayer);
						} 
						if (secondPlayer.getDesiredSide() == firstPlayer.getDesiredSide())
						{
							if (secondPlayer.getDesiredSide() == LEFT_PLAYER)
								switchSide = RIGHT_PLAYER;
							if (secondPlayer.getDesiredSide() == RIGHT_PLAYER)
								switchSide = LEFT_PLAYER;
						}
						
						
						boost::shared_ptr<NetworkGame> newgame (new NetworkGame(
							server, leftPlayer.getID(), rightPlayer.getID(),
							leftPlayer.getName(), rightPlayer.getName(),
							leftPlayer.getColor(), rightPlayer.getColor(),
							switchSide) );
						
						playermap[leftPlayer.getID()] = newgame;
						playermap[rightPlayer.getID()] = newgame;
						gamelist.push_back(newgame);
						SWLS_Games++;
						
						#ifdef DEBUG
						std::cout 	<< "NEW GAME CREATED:\t"<<leftPlayer.getID().binaryAddress << " : " << leftPlayer.getID().port << "\n"
									<< "\t\t\t" << rightPlayer.getID().binaryAddress << " : " << rightPlayer.getID().port << "\n";
						#endif			

						firstPlayer = NetworkPlayer();
					}
					break;
				}
				case ID_PONG:
					break;
				case ID_BLOBBY_SERVER_PRESENT:
				{
					RakNet::BitStream stream((char*)packet->data,
							packet->length, false);

					// If the client knows nothing about versioning, the version is 0.0
					int major = 0;
					int minor = 0;
					bool wrongPackageSize = true;

					// actuel client has bytesize 72

					if(packet->bitSize == 72)
					{
						stream.IgnoreBytes(1);	//ID_BLOBBY_SERVER_PRESENT
						stream.Read(major);
						stream.Read(minor);
						wrongPackageSize = false;
					}

					RakNet::BitStream stream2;

					if (wrongPackageSize)
					{
						printf("major: %d minor: %d\n", major, minor);
						stream2.Write((unsigned char)ID_VERSION_MISMATCH);
						stream2.Write((int)BLOBBY_VERSION_MAJOR);
						stream2.Write((int)BLOBBY_VERSION_MINOR);
						server.Send(&stream2, LOW_PRIORITY,
									RELIABLE_ORDERED, 0, packet->playerId,
									false);
					}
					else if (major < BLOBBY_VERSION_MAJOR
						|| (major == BLOBBY_VERSION_MAJOR && minor < BLOBBY_VERSION_MINOR))
					// Check if the packet contains matching version numbers
					{
						stream2.Write((unsigned char)ID_VERSION_MISMATCH);
						stream2.Write((int)BLOBBY_VERSION_MAJOR);
						stream2.Write((int)BLOBBY_VERSION_MINOR);
												server.Send(&stream2, LOW_PRIORITY,
							RELIABLE_ORDERED, 0, packet->playerId,
							false);
					}
					else
					{
						myinfo.activegames = gamelist.size();
						if (!firstPlayer.valid())
						{
							myinfo.setWaitingPlayer("none");
						}
						else
						{
							myinfo.setWaitingPlayer(firstPlayer.getName());
						}

						stream2.Write((unsigned char)ID_BLOBBY_SERVER_PRESENT);
						myinfo.writeToBitstream(stream2);
						server.Send(&stream2, HIGH_PRIORITY,
							RELIABLE_ORDERED, 0,
							packet->playerId, false);
					}
					break;
				}
				case ID_RECEIVED_STATIC_DATA:
					break;
				default:
					syslog(LOG_DEBUG, "Unknown packet %d received\n", int(packet->data[0]));
			}
		}

		// -------------------------------------------------------------------------------
		// now, step through all network games and process input - if a game ended, delete it
		// -------------------------------------------------------------------------------
		
		SWLS_RunningTime++;
		
		if(SWLS_RunningTime % (75 * 6 /** 60 /*1h*/) == 0 )
		{
			std::fstream f((std::string("logs/server") + boost::lexical_cast<std::string>(SWLS_RunningTime/75) + ".txt").c_str(), std::fstream::out );
			f << "Blobby Server Status Report " << (SWLS_RunningTime / 75 / 60 / 60) << "h running \n";
			f << " packet count: " << SWLS_PacketCount << "\n";
			f << " accepted connections: " << SWLS_Connections << "\n";
			f << " started games: " << SWLS_Games << "\n";
			f << " ingame packets processed: " << SWLS_IngamePacketsProcessed << "\n";
			f << " ingame event counter: " << SWLS_IngameEventCounter << "\n";
			f << " physics broadcasts: " << SWLS_PhysicStateBroadcasts << "\n";
			f << " active games: " << gamelist.size() << "\n";
			f << " active players: " << playermap.size() << "\n";
			report(f);
		}
		
		for (GameList::iterator iter = gamelist.begin(); gamelist.end() != iter; ++iter)
		{
			SWLS_GameSteps++;
			if (!(*iter)->step())
			{
				iter = gamelist.erase(iter);
				// workarround to prevent increment of
				// past-end-iterator
				if(iter == gamelist.end())
					break;
			}
		}
		scontroller.update();

		if (g_workaround_memleaks)
		{
			// Workaround for memory leak
			// Restart the server after 1 hour if no player is
			// connected
			if ((SDL_GetTicks() - startTime) > 60 * 60 * 1000)
			{
				if (gamelist.empty() && !firstPlayer.valid())
				{
					exit(0);
				}
			}
		}
	}
	syslog(LOG_NOTICE, "Blobby Volley 2 dedicated server shutting down");
	#ifndef WIN32
	closelog();
	#endif
	}catch(...)
	{
		std::cout << "AN UNKNOWN EXCEPTION OCCURED\n";
	}
	
	
}

// -----------------------------------------------------------------------------------------

void printHelp()
{
	std::cout << "Usage: blobby-server [OPTION...]" << std::endl;
	std::cout << "  -m, --memleak-hack        Workaround memory leaks by restarting regularly" << std::endl;
	std::cout << "  -n, --no-daemon           Don´t run as background process" << std::endl;
	std::cout << "  -p, --print-msgs          Print messages to stderr" << std::endl;
	std::cout << "  -h, --help                This message" << std::endl;
}


void process_arguments(int argc, char** argv)
{
	if (argc > 1)
	{
		for (int i = 1; i < argc; ++i)
		{
			if (strcmp(argv[i], "--memleak-hack") == 0 || strcmp(argv[i], "-m") == 0)
			{
				g_workaround_memleaks = true;
				continue;
			}
			if (strcmp(argv[i], "--no-daemon") == 0 || strcmp(argv[i], "-n") == 0)
			{
				g_run_in_foreground = true;
				continue;
			}
			if (strcmp(argv[i], "--print-msgs") == 0 || strcmp(argv[i], "-p") == 0)
			{
				g_print_syslog_to_stderr = true;
				continue;
			}
			if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
			{
				printHelp();
				exit(3);
			}
			std::cout << "Unknown option \"" << argv[i] << "\"" << std::endl;
			printHelp();
			exit(1);
		}
	}
}

void fork_to_background()
{
	#ifndef WIN32
	pid_t f_return = fork();
	if (f_return == -1)
	{
		perror("fork");
		exit(1);
	}
	if (f_return != 0)
	{
		std::cout << "Running in background as PID " << f_return << std::endl;
		exit(0);
	}
	#else
	std::cerr<<"fork is not available under windows\n";
	#endif
}


void wait_and_restart_child()
{
	#ifndef WIN32
	pid_t leaking_server;
	while ((leaking_server = fork()) > 0)
	{
		int status;

		// Wait for server to quit and refork
		waitpid(leaking_server, &status, 0);
		// Error will propably occur again
		if (WEXITSTATUS(status) != 0)
		{
			exit(WEXITSTATUS(status));
		}
	}

	if (leaking_server == -1)
	{
		perror("fork");
		exit(1);
	}
	#else
	std::cerr<<"fork is not available under windows\n";
	#endif

}

void setup_physfs(char* argv0)
{
	FileSystem& fs = FileSystem::getSingleton();
	fs.addToSearchPath("data");
	
	#if defined(WIN32)
	// Just write in installation directory
	fs.setWriteDir("data");
	#else
	std::string userdir = fs.getUserDir();
	std::string userAppend = ".blobby";
	std::string homedir = userdir + userAppend;
	fs.setWriteDir(homedir);
	#endif

}


#ifdef WIN32
#undef main

void syslog(int pri, const char* format, ...) 
{
	// first, look where we want to send our message to
	FILE* target = stdout;
	switch(pri)
	{
		case LOG_ERR:
			target = stderr;
			break;
		case LOG_NOTICE:
		case LOG_DEBUG:
			target = stdout;
			break;
	}
	
	// create a string containing date and time
	std::time_t time_v = std::time(0);
	std::tm* time = localtime(&time_v);
	char buffer[128];
	std::strftime(buffer, sizeof(buffer), "%x - %X", time);
	
	// print it
	fprintf(target, "%s: ", buffer);
	
	// now relay the passed arguments and format string to vfprintf for output
	va_list args;
	va_start (args, format);
	vfprintf(target, format, args);
	va_end (args);
	
	// end finish with a newline
	fprintf(target, "\n");
}
#endif
