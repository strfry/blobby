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
#include "DataCollector.h"

#include <future>
#include <mutex>
#include <thread>

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

		PlayerSide mSide;

		// error data
		std::default_random_engine mRandom;
		
		DuelMatchState mOldState;
		int mLastHitCount = 0;
		int mLastOppHitCount = 0;
		int mLastScore = 0;
		int mLastOppScore = 0;
		int timer = 0;
		PlayerSide mLastSide = LEFT_PLAYER;
		ScriptedInputSource reference;
		
		InputCoder mCoder;
		fann* mNetwork;
		
		DataCollector mCollector;
		CollectorInterface mOwnCollector;
		CollectorInterface mOppCollector;
		fann_train_data* mCurrentTraining = nullptr;
		
		void exercise();
		
		std::atomic<bool> mRunThread{true}; // declared before thread, so it is initialized before
		std::thread mExerciseThread;
		std::mutex mNetworkMutex;
};

