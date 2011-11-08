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

#include "SpeedController.h"

#include <SDL/SDL.h>
#include <cassert>
#include <algorithm>

/// this makes it possible to run a thread with >= 1000fps, 
/// usefull for benchmarking bots
const int TICK_FACTOR = 100;

SpeedController* SpeedController::mMainInstance = NULL;

SpeedController::SpeedController(float FPS, unsigned int thread) : mCounter(0), mThread(thread),
																mFPS(FPS), mFramedrop(false)
{
	mLastTicks = SDL_GetTicks();
	mBeginSecond = mLastTicks;
}

SpeedController::~SpeedController()
{
}

void SpeedController::setSpeed(float fps)
{
	/// \todo do we still need this check?
	if (fps < 5)
		fps = 5;
	mFPS = fps;
}

bool SpeedController::requireFramedrop() const
{
	return mFramedrop;
}

void SpeedController::wait()
{
	
	assert(mThread == SDL_ThreadID());
	
	mFramedrop = false;
	
	// calculate how many ms per frame
	int rateTicks = std::max(int(TICK_FACTOR * 1000 / mFPS), 1);

	// when all steps are done for this second, wait till second is done
	if (mCounter == mFPS)
	{
		const int delta = SDL_GetTicks() - mBeginSecond;
		int wait = 1000 - delta;
		if (wait > 0)
			SDL_Delay(wait);
	}
	
	// look if next second has started
	if (mBeginSecond + 1000 <= SDL_GetTicks())
	{
		mBeginSecond = SDL_GetTicks();
		mCounter = 0;
	}

	const int delta = SDL_GetTicks() - mBeginSecond;
	
	// check if time/rate (desired number of frames) <= number of frames
	if (delta / rateTicks * TICK_FACTOR <= mCounter)
	{
		// find out when the next frame has to be done and calculate timedifference, use as wait
		int wait = ((mCounter+1)* rateTicks/TICK_FACTOR) - delta;
		
		// do wait
		if (wait > 0)
			SDL_Delay(wait);
	}
	
	// (draw) next frame
	mCounter++;

	//update for next call:
	mLastTicks = SDL_GetTicks();
}

unsigned int SpeedController::getLastFrameTime() const
{
	return mLastTicks;
}
