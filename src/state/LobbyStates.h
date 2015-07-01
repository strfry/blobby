#ifndef LOBBYSTATES_H_INCLUDED
#define LOBBYSTATES_H_INCLUDED

#include <boost/shared_ptr.hpp>
#include "NetworkMessage.h"
#include "PlayerIdentity.h"
#include "State.h"
#include "GenericIOFwd.h"

class RakClient;

enum class ConnectionState
{
	CONNECTING,
	CONNECTED,
	DISCONNECTED,
	CONNECTION_FAILED
};

/// \todo this currently also contains server config data
struct ServerStatusData
{
	struct OpenGame
	{
		unsigned id;
		std::string name;
		unsigned rules;
		unsigned speed;
		unsigned score;
	};

	std::vector<OpenGame> mOpenGames;
	std::vector<unsigned int> mPossibleSpeeds;
	std::vector<std::string> mPossibleRules;
	std::vector<std::string> mPossibleRulesAuthor;
};

class LobbySubstate
{
public:
	void virtual step(const ServerStatusData& status ) = 0;
};

class LobbyState : public State
{
	public:
		LobbyState(ServerInfo info);
		virtual ~LobbyState();

		virtual void step_impl();
		virtual const char* getStateName() const;

	private:
		boost::shared_ptr<RakClient> mClient;
		PlayerIdentity mLocalPlayer;
		ServerInfo mInfo;

		ConnectionState mLobbyState;

		ServerStatusData mStatus;
		boost::shared_ptr<LobbySubstate> mSubState;
};


class LobbyMainSubstate : public LobbySubstate
{
public:
	LobbyMainSubstate(boost::shared_ptr<RakClient> client);
	virtual void step( const ServerStatusData& status );
private:
	boost::shared_ptr<RakClient> mClient;

	unsigned int mSelectedGame = 0;

	// temp variables for open game
	unsigned mChosenSpeed = 0;
	unsigned mChosenRules = 0;
	unsigned mChosenScore = 3;

	std::vector<unsigned> mPossibleScores{2, 5, 10, 15, 20, 25, 40, 50};
};


class LobbyGameSubstate : public LobbySubstate
{
public:
	LobbyGameSubstate(boost::shared_ptr<RakClient> client, boost::shared_ptr<GenericIn>);

	virtual void step( const ServerStatusData& status );
private:
	boost::shared_ptr<RakClient> mClient;

	unsigned mGameID = 0;
	std::string mGameName = "";
	bool mIsHost = false;
	unsigned mSpeed = 0;
	unsigned mRules = 0;
	unsigned mScore = 3;

	unsigned mSelectedPlayer = 0;

	std::vector<PlayerID> mOtherPlayers;
};

#endif // LOBBYSTATES_H_INCLUDED
