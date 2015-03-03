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
#include "Clock.h"

/* includes */
#include "SDL2/SDL.h"

/* implementation */

Clock::Clock() : mRunning(false), mGameTime(0), mLastTime(0)
{

}

void Clock::reset()
{
	// set all variables to their default values
	mRunning = false;
	mGameTime = 0;
	mTimeSteps = 0;
	mLastTime = SDL_GetTicks();
}

void Clock::start()
{
	mLastTime = SDL_GetTicks();
	mRunning = true;
}

void Clock::stop()
{
	mRunning = false;
}

bool Clock::isRunning() const
{
	return mRunning;
}

int Clock::getTime() const
{
	return mGameTime / 1000;
}
void Clock::setTime(int newTime)
{
	mGameTime = newTime * 1000;
}

int Clock::getTimeSteps() const
{
	return mTimeSteps;
}

void Clock::setTimeSteps( int steps )
{
	mTimeSteps = steps;
}

void Clock::step()
{
	if(mRunning)
	{
		int newTime = SDL_GetTicks();
		if(newTime > mLastTime)
		{
			mGameTime += newTime - mLastTime;
		}
		mLastTime = newTime;
		++mTimeSteps;
	}
}
