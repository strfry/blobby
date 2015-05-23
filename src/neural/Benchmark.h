#ifndef BENCHMARK_H_INCLUDED
#define BENCHMARK_H_INCLUDED

#include "PhysicWorld.h"
#include "InputCoder.h"
#include <random>
#include <vector>

struct fann;
struct fann_train_data;

struct BenchResult
{
	float score;
	std::vector<Vector2> ballpos;
	std::vector<Vector2> ballvel;
	std::vector<Vector2> blobpos;
	std::vector<int> mWeights;
	
	void addFail(Vector2 bp, Vector2 bv, Vector2 blob);
	void clear();
};

// this class is used to test how good the bot operates
class Benchmark
{
public:
	BenchResult benchNet(fann* net, int tests = 1000);
	
	// tests all configurations that failed in the last benchmark to get the improvement
	BenchResult benchImprovement( fann* net );
	
	/// creates a set of training data from the failed benchmark results
	fann_train_data* makeTrainingSet( BenchResult fails, fann* network );
	
private:
	// bench helpers
	void setupTest();
	// if result != null, records behaviour
	bool runTest( std::vector<PlayerInput>* result = nullptr, int* first_hit = nullptr );
	
	// training set helpers
	fann_train_data* simulateNewTry( const std::vector<PlayerInput>& input, const std::vector<bool>& change_pos );
	
	PhysicWorld mVirtualReality;
	
	Vector2 mInitialBallPosition;
	Vector2 mInitialBallVelocity;
	Vector2 mInitialBlobPosition;
	
	fann* mTestedNetwork;
	InputCoder mCoder;
	
	std::default_random_engine mRandom;
	std::uniform_real_distribution<double> mDistribution{0.0, 1.0};
	
	BenchResult mLastBenchmark;
};

#endif // BENCHMARK_H_INCLUDED
