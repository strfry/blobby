/**
 * @file NeuralBot.h
 * @brief Contains a class which allows using lua scripted bots
 */

#pragma once

#include <string>
#include <random>

#include "Global.h"
#include "InputSource.h"
#include "Vector.h"
#include "DuelMatchState.h"
#include "ScriptedInputSource.h"
#include "InputCoder.h"
#include "TrainingDataSet.h"
#include "Benchmark.h"

#include <future>
#include <mutex>

/// \class NeuralBot
/// \brief Bot controller
/// \details NeuralBot provides an implementation of InputSource, which uses
/// Lua scripts to get its input. The given script is automatically initialised
/// and provided with an interface to the game.

/// The API documentation can now be found in doc/ScriptAPI.txt

// The time the bot waits after game start

struct lua_State;
class DuelMatch;

class fann;
class fann_train_data;


class NeuralBot : public InputSource
{
	public:
		/// The constructor automatically loads and initializes the script
		/// with the given filename. The side parameter tells the script
		/// which side is it on.
		NeuralBot(PlayerSide side, unsigned int difficulty);
		~NeuralBot();

		virtual PlayerInputAbs getNextInput();

	private:

		unsigned int mStartTime;

		// ki strength values
		int mDifficulty;

		float mLastBallSpeed;

		PlayerSide mSide;

		// error data
		std::normal_distribution<double> mPositionErrorDistribution;
		std::default_random_engine mRandom;
		
		std::vector<std::vector<double>> mSituationCache;
		DuelMatchState mOldState;
		int mLastHitCount = 0;
		int mLastScore = 0;
		BenchResult mLoses;

		int mLearningPhase = 0;

		ScriptedInputSource reference;

		fann* mNetwork;

		std::future<fann*> mLearnFuture;
		std::future<void>  mBenchFuture;

		int mLearnEpochs = 100;
		int mDataSetSize = 10000;
		float mDesiredError = 0.01;
		
		// feedback loop
		bool mLastJump;
		
		InputCoder mCoder;
		
		TrainingDataSet mTrainingData;
		
		Benchmark bench;
		
		float mLastSuccessQuota = 0.0;
};

