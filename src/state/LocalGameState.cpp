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
#include "LocalGameState.h"

/* includes */
#include <ctime>
#include <iostream>

#include <boost/make_shared.hpp>

#include "DuelMatch.h"
#include "InputManager.h"
#include "IMGUI.h"
#include "ReplayRecorder.h"
#include "SoundManager.h"
#include "TextManager.h"
#include "SpeedController.h"
#include "IUserConfigReader.h"
#include "input/InputSourceFactory.h"

/* implementation */
LocalGameState::~LocalGameState()
{
}

LocalGameState::LocalGameState()
	: GameState( boost::make_shared<DuelMatch>(false, "") ), mRecorder(new ReplayRecorder())
{
	boost::shared_ptr<IUserConfigReader> config = IUserConfigReader::createUserConfigReader("config.xml");
	PlayerIdentity leftPlayer = config->loadPlayerIdentity(LEFT_PLAYER, false);
	PlayerIdentity rightPlayer = config->loadPlayerIdentity(RIGHT_PLAYER, false);

	boost::shared_ptr<InputSource> leftInput = InputSourceFactory::createInputSource( config, LEFT_PLAYER);
	boost::shared_ptr<InputSource> rightInput = InputSourceFactory::createInputSource( config, RIGHT_PLAYER);

	// create default replay name
	setDefaultReplayName(leftPlayer.getName(), rightPlayer.getName());

	SoundManager::getSingleton().playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);

	mMatch->setRules( config->getString("rules") );
	mMatch->setPlayers(leftPlayer, rightPlayer);
	mMatch->setInputSources(leftInput, rightInput);
	mMatch->setGameSpeed( (float)config->getInteger("gamefps") );
	mMatch->setStepCallback( std::bind(&LocalGameState::onMatchStep, this, std::placeholders::_1, std::placeholders::_2) );

	mRecorder->setPlayerNames(leftPlayer.getName(), rightPlayer.getName());
	mRecorder->setPlayerColors( leftPlayer.getStaticColor(), rightPlayer.getStaticColor() );
	mRecorder->setGameSpeed((float)config->getInteger("gamefps"));

	mMatch->run();
}

void LocalGameState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();
	if(mErrorMessage != "")
	{
		displayErrorMessageBox();
	}
	else if (mSaveReplay)
	{
		if ( displaySaveReplayPrompt() )
		{
			saveReplay( *mRecorder.get() );
		}
	}
	else if (mMatch->isPaused())
	{
		imgui.doOverlay(GEN_ID, Vector2(180, 200), Vector2(670, 400));
		imgui.doText(GEN_ID, Vector2(281, 260), TextManager::LBL_CONF_QUIT);
		if (imgui.doButton(GEN_ID, Vector2(530, 300), TextManager::LBL_NO)){
			mMatch->unpause();
		}
		if (imgui.doButton(GEN_ID, Vector2(260, 300), TextManager::LBL_YES))
		{
			switchState(new MainMenuState);
		}
		if (imgui.doButton(GEN_ID, Vector2(293, 340), TextManager::RP_SAVE))
		{
			mSaveReplay = true;
			imgui.resetSelection();
		}
		imgui.doCursor();
	}
	else if (mLastState->getWinningPlayer() != NO_PLAYER )
	{
		displayWinningPlayerScreen( mLastState->getWinningPlayer() );
		if (imgui.doButton(GEN_ID, Vector2(290, 350), TextManager::LBL_OK))
		{
			switchState(new MainMenuState());
		}
		if (imgui.doButton(GEN_ID, Vector2(400, 350), TextManager::GAME_TRY_AGAIN))
		{
			switchState(new LocalGameState());
		}
		if (imgui.doButton(GEN_ID, Vector2(320, 390), TextManager::RP_SAVE))
		{
			mSaveReplay = true;
			imgui.resetSelection();
		}
	}
	else if (InputManager::getSingleton()->exit())
	{
		if (mSaveReplay)
		{
			mSaveReplay = false;
			IMGUI::getSingleton().resetSelection();
		}
		else if (mMatch->isPaused())
		{
			switchState(new MainMenuState);
		}
		else
		{
			RenderManager::getSingleton().redraw();
			mMatch->pause();
		}
	}
	else
	{
		presentGame( );
	}

	presentGameUI( );
}

const char* LocalGameState::getStateName() const
{
	return "LocalGameState";
}

void LocalGameState::onMatchStep(const DuelMatchState& state, const std::vector<MatchEvent>& events)
{
	mRecorder->record( state );
	if (state.getWinningPlayer() != NO_PLAYER)
	{
		// duplicate last state
		/// \todo why is this necessary?
		mRecorder->record( state );
		mRecorder->finalize( state.getScore(LEFT_PLAYER), state.getScore(RIGHT_PLAYER) );
	}
}

