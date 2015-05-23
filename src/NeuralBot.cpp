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
, mLastBallSpeed(0)
, mSide(playerside)
, mPositionErrorDistribution( 0.0, difficulty / 75.0 * BALL_RADIUS ),
reference( "scripts/reduced.lua", playerside, difficulty), 
mTrainingData( 7500, mCoder.getChannels())
{
	mStartTime = SDL_GetTicks();

    const unsigned int num_output = 3;
    const unsigned int num_layers = 5;

    mNetwork = //fann_create_shortcut(2, 6, 3);
    fann_create_sparse(0.5, num_layers, mCoder.getChannels(), 150, 100, 50, num_output);
    fann_set_activation_function_hidden(mNetwork, FANN_SIGMOID_SYMMETRIC_STEPWISE);	// use stepwise to be faster
    fann_set_activation_function_output(mNetwork, FANN_SIGMOID_SYMMETRIC);	

	mNetwork = fann_create_from_file("neuralbot.net");
	fann_set_training_algorithm( mNetwork, FANN_TRAIN_RPROP);
	fann_set_learning_rate( mNetwork, 0.7 );
	fann_set_learning_momentum( mNetwork, 0.1 );
	fann_set_activation_steepness_output( mNetwork, 1.0 );
	
	mTrainingData.loadData();
}

NeuralBot::~NeuralBot()
{
	if( mLearnFuture.valid() )
	{
		fann_destroy(mNetwork);
		mNetwork = mLearnFuture.get();
	}
	
	fann_save(mNetwork, "neuralbot.net");
	mTrainingData.saveData();
    fann_destroy(mNetwork);
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
		matchstate.swapSides();
		
	// when the score changes
	if( mLastScore != matchstate.logicState.rightScore )
	{
		// limit maximum data
		if( mTrainingData.getFailureDataCount() < 500 )
			mLoses.addFail(mOldState.worldState.ballPosition, mOldState.worldState.ballVelocity, mOldState.worldState.blobPosition[LEFT_PLAYER] );
		mLastScore = matchstate.logicState.rightScore;
	}
		
	// whenever somebody touches the ball
	if(matchstate.logicState.hitCount[RIGHT_PLAYER] != mLastHitCount)
	{
		// if it was the ai
		if(matchstate.logicState.hitCount[RIGHT_PLAYER] != 0)
		{
			mOldState = matchstate;
		}
		mLastHitCount = matchstate.logicState.hitCount[RIGHT_PLAYER];
	}

	//matchstate.worldState.ballPosition += mBallPosError;
	//matchstate.worldState.ballVelocity *= Vector2(mBallVelError, mBallVelError);

	// do neural calculation
	auto state = mCoder.encode( matchstate.worldState );
	//mSituationCache.push_back(state);

	// switch to trained network
	if( mLearnFuture.valid() && std::future_status::ready == mLearnFuture.wait_for( std::chrono::milliseconds(0) ) )
	{
		fann_destroy(mNetwork);
		mNetwork = mLearnFuture.get();
		fann_save(mNetwork, "neuralbot.net");
	}
	
	// reset bench object
	if( mBenchFuture.valid() && std::future_status::ready == mBenchFuture.wait_for( std::chrono::milliseconds(0) ) )
	{
		mBenchFuture.get();
	}

	//float result[3];
	//std::cout << state[0] << " " << state[1] << " " << state[2] <<  " " << state[3] << "\n";
	auto result = fann_run(mNetwork, &state[0]);
	static bool init = false;
	if(!init)
	{
		reference.setMatch( getMatch() );
		init = true;
	}
	auto ref_in = reference.getNextInput().toPlayerInput( getMatch());
	//std::cout << result[0] << " " << result[1] << " " << result[2] << "\n";

	bool wantleft = result[0] > 0;
	bool wantright = result[1] > 0;
	mLastJump = result[2] > 0;

	//std::cout << mNet.getLayer(0).getWeights()[0] << "\n";
	mLearningPhase = 0;
	if( mLearningPhase == 0 && mStartTime + 1500 < SDL_GetTicks())
	{
		static int timer = 0;
		
		std::array<float, 3> out{ref_in.left ? 1 : -1, ref_in.right ? 1 : -1, ref_in.up ? 1 : -1} ;

		// learn every ten steps, or when the ball is close to the blobby
		if( rand() % 3 == 0 || ref_in.left != wantleft ||ref_in.right == wantright || mLastJump != ref_in.up )
			mTrainingData.addDefaultBotDataPoint( state, out );
		
		// switch between playing styles if no new data is available
		if(!mTrainingData.isNewDataReady() && (timer / 100) % 2 == 0 && mLastSuccessQuota < 0.95 )
		{
			wantleft = ref_in.left;
			wantright = ref_in.right;
			mLastJump = ref_in.up;
		}
		
		++timer;
	}
	
	// try to learn (non neuronal) from own mistakes
	if( (mLoses.ballpos.size() >= 50 || (!mLearnFuture.valid() && !mLoses.ballpos.empty())) && !mBenchFuture.valid() )
	{	
		// capture by copy, because we will change mLoses as soon as we start learning
		auto data = mLoses;
		auto refnet = fann_copy(mNetwork);
		auto createTrainingData = [data, refnet, this]()
		{
			Benchmark generator;
			auto my_errors = generator.makeTrainingSet( data, refnet );
			if( my_errors)
			{
				std::cout << "own errors: " << data.ballpos.size() << " = " << fann_length_train_data(my_errors) << " data points\n";
				mTrainingData.setDifficultTraining( my_errors, data.ballpos.size() );
			}
			fann_destroy(refnet);
		};
		mLoses.clear();
		
		mBenchFuture = std::async(std::launch::async, createTrainingData);
	}
	
	if(!mLearnFuture.valid())
	{
		auto trainingdata = mTrainingData.getDataForTraining();
		if(trainingdata)
		{
			auto trainingnet = fann_copy(mNetwork);
			std::cout << "learn from data: " << fann_length_train_data(trainingdata) << "\n";
	
			auto learn = [this, trainingnet, trainingdata]()
			{
				fann_train_on_data(trainingnet, trainingdata, mLearnEpochs, 10, mDesiredError);
				// first, compare to last fails
				std::cout << " network improvement: " << int(100 * bench.benchImprovement(trainingnet).score) << "%\n";
				
				// then complete re-evaluation
				BenchResult succ = bench.benchNet( trainingnet );
				mLastSuccessQuota = succ.score;
				std::cout << " benchmark success: " << int(1000 * succ.score) << "Pkt\n";
				
				// explicitly train the benchmark fails
				auto training = bench.makeTrainingSet( succ, trainingnet );
				mTrainingData.setDifficultTraining( training, succ.ballpos.size() );

				return trainingnet;
			};

			mLearnFuture = std::async (std::launch::async, learn);
		}
	}

	// override for serving for now, because bot does not learn that...
	if (!getMatch()->getBallActive() && mSide ==
			// if no player is serving player, assume the left one is
			(getMatch()->getServingPlayer() == NO_PLAYER ? LEFT_PLAYER : getMatch()->getServingPlayer() ))
	{
		return PlayerInputAbs(ref_in.left, ref_in.right, ref_in.up);


	}

	// swap left/right if side is swapped
	if ( mSide == RIGHT_PLAYER )
		std::swap(wantleft, wantright);


	if (mStartTime + 1500 > SDL_GetTicks() && serving)
		return PlayerInputAbs();
	return PlayerInputAbs(wantleft, wantright, mLastJump);
}

