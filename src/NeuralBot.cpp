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
#include "NeuralBot.h"

/* includes */
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>

#include <SDL2/SDL.h>

extern "C"
{
#include "lua/lua.h"
#include "lua/lauxlib.h"
#include "lua/lualib.h"
#include "fann.h"
}

#include "DuelMatch.h"
#include "DuelMatchState.h"
#include "GameConstants.h"
#include "Benchmark.h"

/* implementation */

NeuralBot::NeuralBot(PlayerSide playerside, unsigned int difficulty)
: mDifficulty(difficulty)
, mSide(playerside),
mOwnCollector(mCollector, false),
mOppCollector(mCollector, true),
reference( "scripts/reduced.lua", playerside, difficulty), 
mExerciseThread( [this](){ exercise(); } )
{
	mStartTime = SDL_GetTicks();

    const unsigned int num_output = 3;
    const unsigned int num_layers = 4;

	mNetwork = fann_create_standard(num_layers, mCoder.getChannels(), 200, 100, num_output);
    fann_set_activation_function_hidden(mNetwork, FANN_SIGMOID_SYMMETRIC_STEPWISE);	// use stepwise to be faster
    fann_set_activation_function_output(mNetwork, FANN_SIGMOID_SYMMETRIC);	

	//mNetwork = fann_create_from_file("neuralbot.net");
	fann_set_training_algorithm( mNetwork, FANN_TRAIN_RPROP);
	fann_set_learning_rate( mNetwork, 0.7 );
	fann_set_learning_momentum( mNetwork, 0.1 );
	fann_set_activation_steepness_output( mNetwork, 1.0 );
}

NeuralBot::~NeuralBot()
{
    mRunThread = false;
    mExerciseThread.join();
}

PlayerInputAbs NeuralBot::getNextInput()
{
	bool serving = false;
	// reset input

	if (getMatch() == 0)
	{
		return PlayerInputAbs();
	}

	// collect match states
	auto matchstate = getMatch()->getState();
	if(mSide != LEFT_PLAYER)
	{
		if(!getMatch()->getBallDown())
			mOppCollector.record(matchstate);
		matchstate.swapSides();
		if(!getMatch()->getBallDown())
			mOwnCollector.record(matchstate);
	} 
	 else if(!getMatch()->getBallDown())
	{
		mOwnCollector.record(matchstate);
		matchstate.swapSides();
		mOppCollector.record(matchstate);
		matchstate.swapSides();
	}

	
	
	// do neural calculation
	auto state = mCoder.encode( matchstate.worldState );
	
	bool wantleft, wantright, wantjump;
	{
		std::lock_guard<std::mutex> l(mNetworkMutex);
		auto result = fann_run(mNetwork, &state[0]);
		wantleft = result[0] > 0;
		//std::cout << result[0] << " " << result[1] << " " << result[2] << "\n"; 
		wantright = result[1] > 0;
		wantjump = result[2] > 0;
	}
	
	bool LEARNING = true;
	if( LEARNING )
	{
		static bool init = false;
		if(!init)
		{
			reference.setMatch( getMatch() );
			init = true;
		}
		auto ref_in = reference.getNextInput().toPlayerInput( getMatch() );
		//if( rand() % 2 == 0)
		{
			bool dir = rand() % 2;
			wantleft = dir;//ref_in.left;
			wantright = !dir;//ref_in.right;
			wantjump = false;//ref_in.up;
		}
	}

	// swap left/right if side is swapped
	if ( mSide == RIGHT_PLAYER )
		std::swap(wantleft, wantright);

	if (!getMatch()->getBallActive() && mSide ==// if no player is serving player, assume the left one is
			(getMatch()->getServingPlayer() == NO_PLAYER ? LEFT_PLAYER : getMatch()->getServingPlayer() ))
	{
		wantright = matchstate.worldState.blobPosition[LEFT_PLAYER].x < 180;
		wantleft = matchstate.worldState.blobPosition[LEFT_PLAYER].x > 200;
		wantjump = rand() % 100 == 0;
	}


	if (mStartTime + 1500 > SDL_GetTicks() && serving)
		return PlayerInputAbs();
	return PlayerInputAbs(wantleft, wantright, wantjump);
}

void NeuralBot::exercise()
{
	fann_train_data* data_ptr = nullptr;
	fann_train_data* working_set = nullptr;
	fann* working_net = nullptr;
	while(mRunThread)
	{
		if( mCollector.has_new())
		{
			data_ptr = mCollector.getExerciseData();
			if(working_set)
				fann_destroy_train(working_set);
			working_set = fann_duplicate_train_data(data_ptr);
			std::cout << "csize: " << fann_length_train_data(working_set) << "\n";
		}
		
		if(working_set && fann_length_train_data(working_set) > 1000)
		{
			if(!working_net)
			{
				std::lock_guard<std::mutex> l(mNetworkMutex);
				working_net = fann_copy(mNetwork);
				fann_set_user_data(working_net, this);
			}
			
			//std::cout << fann_length_train_data(working_set) << "\n";
			// learn for some time
			fann_set_training_algorithm( working_net, FANN_TRAIN_INCREMENTAL);
			fann_set_learning_rate( working_net, 0.5 );
			fann_train_on_data(working_net, working_set, 50, 10, 0);
				
			{
				std::lock_guard<std::mutex> l(mNetworkMutex);
				fann_destroy(mNetwork);
				mNetwork = working_net;
				working_net = nullptr;
			}
		}
	}
	
	fann_save(mNetwork, "neuralbot.net");
	fann_destroy(mNetwork);
}
