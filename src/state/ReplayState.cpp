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
#include "ReplayState.h"

/* includes */
#include <sstream>
#include <iostream>

#include <boost/make_shared.hpp>

#include "IMGUI.h"
#include "ReplayPlayer.h"
#include "DuelMatch.h"
#include "SoundManager.h"
#include "TextManager.h"
#include "IUserConfigReader.h"
#include "ReplaySelectionState.h"
#include "InputManager.h"

/* implementation */

extern const std::string DUMMY_RULES_NAME;

ReplayState::ReplayState() : GameState( boost::make_shared<DuelMatch>(false, DUMMY_RULES_NAME) )
{
	IMGUI::getSingleton().resetSelection();

	mPositionJump = -1;

	mSpeedValue = 1.0;
}

ReplayState::~ReplayState()
{

}

void ReplayState::loadReplay(const std::string& file)
{
	mReplayPlayer.reset( new ReplayPlayer( mMatch ) );

	//try
	//{
		mReplayPlayer->load(std::string("replays/" + file + ".bvr"));

		SoundManager::getSingleton().playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);
		mReplayPlayer->play();

	//}
	/*catch (ChecksumException& e)
	{
		delete mReplayRecorder;
		mReplayRecorder = 0;
		mChecksumError = true;
	}
	catch (VersionMismatchException& e)
	{
		delete mReplayRecorder;
		mReplayRecorder = 0;
		mVersionError = true;
	}*/
	/// \todo reintroduce error handling
}

void ReplayState::step_impl()
{
	IMGUI& imgui = IMGUI::getSingleton();

	// only draw cursor when mouse moved or clicked in the last second
	if(mLastMousePosition != InputManager::getSingleton()->position() || InputManager::getSingleton()->click())
	{
		/// \todo we must do this framerate independent
		mMouseShowTimer = 75;
	}

	if(mMouseShowTimer > 0)
	{
		imgui.doCursor();
		mMouseShowTimer--;
	}

	mLastMousePosition = InputManager::getSingleton()->position();


	if(mPositionJump >= 0)
	{
		mReplayPlayer->gotoPlayingPosition(mPositionJump);
		mPositionJump = -1;
	}

	// draw the progress bar
	Vector2 prog_pos = Vector2(50, 600-22);
	imgui.doOverlay(GEN_ID, prog_pos, Vector2(750, 600-3), Color(0,0,0));
	imgui.doOverlay(GEN_ID, prog_pos, Vector2(700*mReplayPlayer->getPlayProgress()+50, 600-3), Color(0,255,0));

	PlayerSide side = NO_PLAYER;
	if (mReplayPlayer->endOfFile())
	{
		int diff = mMatch->readCurrentState().getScore(LEFT_PLAYER) - mMatch->readCurrentState().getScore(RIGHT_PLAYER);
		if (diff > 0)
		{
			side = LEFT_PLAYER;
		}
		else if (diff < 0)
		{
			side = RIGHT_PLAYER;
		}
	}

	// play/pause button
	imgui.doOverlay(GEN_ID, Vector2(350, 535.0), Vector2(450, 575.0));
	bool pause_click = imgui.doImageButton(GEN_ID, Vector2(400, 555), Vector2(24, 24), mMatch->isPaused() ? "gfx/btn_play.bmp" : "gfx/btn_pause.bmp");
	bool fast_click = imgui.doImageButton(GEN_ID, Vector2(430, 555), Vector2(24, 24),  "gfx/btn_fast.bmp");
	bool slow_click = imgui.doImageButton(GEN_ID, Vector2(370, 555), Vector2(24, 24),  "gfx/btn_slow.bmp");

	// handle these image buttons. IMGUI is not capable of doing this.
	if(side == NO_PLAYER)
	{
		// control replay position
		Vector2 mousepos = InputManager::getSingleton()->position();
		if (mousepos.x + 5 > prog_pos.x && mousepos.y > prog_pos.y &&
			mousepos.x < prog_pos.x + 700 && mousepos.y < prog_pos.y + 24.0)
		{

			if (InputManager::getSingleton()->click())
			{
				float pos = (mousepos.x - prog_pos.x) / 700.0;
				mPositionJump = pos * mReplayPlayer->getReplayLength();
			}
		}

		if (pause_click)
		{
			if(mMatch->isPaused())
			{
				mMatch->unpause();
				if(mReplayPlayer->endOfFile())
					mPositionJump = 0;
			} else
			{
				mMatch->pause();
			}
		}

		if (fast_click)
		{
			mSpeedValue *= 2;
			if(mSpeedValue > 8)
				mSpeedValue = 8;
			mReplayPlayer->setReplaySpeed( mReplayPlayer->getGameSpeed() * mSpeedValue  );
		}

		if (slow_click)
		{
			mSpeedValue /= 2;
			if(mSpeedValue < 1.0/8)
				mSpeedValue = 1.0/8;
			mReplayPlayer->setReplaySpeed( mReplayPlayer->getGameSpeed() * mSpeedValue  );
		}

		if ((InputManager::getSingleton()->exit()))
		{
			switchState(new ReplaySelectionState());
			return;
		}
	}
	else
	{
		displayWinningPlayerScreen(side);
		mMatch->pause();

		if (imgui.doButton(GEN_ID, Vector2(290, 350), TextManager::LBL_OK))
		{
			switchState(new ReplaySelectionState());
			return;
		}
		if (imgui.doButton(GEN_ID, Vector2(400, 350), TextManager::RP_SHOW_AGAIN))
		{
			// we don't have to reset the match, cause we don't use lua rules
			// we just have to jump to the beginning
			SoundManager::getSingleton().playSound("sounds/pfiff.wav", ROUND_START_SOUND_VOLUME);

			mMatch->unpause();
			mPositionJump = 0;
		}
		imgui.doCursor();
	}

	presentGame();
	// show the game ui
	presentGameUI();
}

const char* ReplayState::getStateName() const
{
	return "ReplayState";
}

