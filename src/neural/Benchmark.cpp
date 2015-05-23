#include "Benchmark.h"
#include "MatchEvents.h"
#include "GameConstants.h"
#include "fann.h"
#include <iostream>

BenchResult Benchmark::benchNet(fann* net, int tests )
{
	BenchResult results;
	mTestedNetwork = net;
	
	int successes = 0;
	
	for( int c = 0; c < tests; ++c )
	{
		setupTest();
		/// \todo gather fail situations
		if(runTest())
		{
			successes++;
		} else 
		{
			results.addFail(mInitialBallPosition, mInitialBallVelocity, mInitialBlobPosition);
		}
	}
	
	results.score = float(successes) / tests;
	
	mLastBenchmark = results;
	
	return results;
}

BenchResult Benchmark::benchImprovement( fann* net )
{
	BenchResult results;
	if(mLastBenchmark.ballpos.size() == 0)
	{
		std::cout << "NO LAST BENCH FOUND\n";
		results.score = 0;
		return results;
	}
	
	mTestedNetwork = net;
	
	int successes = 0;
	
	for( int i = 0; i < mLastBenchmark.ballpos.size(); ++i)
	{
		mInitialBallPosition = mLastBenchmark.ballpos[i];
		mInitialBallVelocity = mLastBenchmark.ballvel[i];
		mInitialBlobPosition = Vector2(mLastBenchmark.blobpos[i].x, GROUND_PLANE_HEIGHT);
		
		/// \todo gather fail situations
		if(runTest())
		{
			successes++;
		} else 
		{
			results.addFail(mInitialBallPosition, mInitialBallVelocity, mInitialBlobPosition);
		}
	}
	results.score = float(successes) / mLastBenchmark.ballpos.size();
	
	return results;
}

void Benchmark::setupTest()
{
	mInitialBlobPosition.x = mDistribution(mRandom) * (NET_POSITION_X-NET_RADIUS-BLOBBY_LOWER_RADIUS);
	mInitialBlobPosition.y = GROUND_PLANE_HEIGHT;
	int mballstartx =  mDistribution(mRandom) * (NET_POSITION_X - BALL_RADIUS) + NET_POSITION_X;
	int mballstarty = 100 + mDistribution(mRandom) * (GROUND_PLANE_HEIGHT-100);
	
	// push ball out of net
	if(mballstarty > NET_SPHERE_POSITION &&	mballstartx - NET_POSITION_X < BALL_RADIUS + NET_RADIUS)
	{
		mballstartx = NET_POSITION_X + BALL_RADIUS + NET_RADIUS;
	}
	
	mInitialBallPosition = Vector2(mballstartx, mballstarty);
	
	for(int i = 0; i < 100; ++i)
	{
		float ballang = mDistribution(mRandom) * 2 * 3.14159265;
		Vector2 ballvel{ std::sin(ballang) * BALL_COLLISION_VELOCITY , std::cos(ballang) * BALL_COLLISION_VELOCITY };
		// pass the net?
		float distance = 0;
		if( ballvel.x < 0 )
			distance = (mballstartx - NET_POSITION_X);
		else
			distance = -((RIGHT_PLANE - BALL_RADIUS - mballstartx) + (RIGHT_PLANE - NET_POSITION_X));

		float ttn = distance / ballvel.x;
		float yatn = mballstarty + ballvel.x * ttn + BALL_GRAVITATION/2 * ttn * ttn;
		if( yatn < NET_SPHERE_POSITION )
		{
			mInitialBallVelocity = ballvel;
			break;
		}
	}
}

bool Benchmark::runTest( std::vector<PlayerInput>* res_actions, int* first_hit )
{
	mVirtualReality.setBallPosition( mInitialBallPosition );
	mVirtualReality.setBallVelocity( mInitialBallVelocity );
	
	PhysicState state = mVirtualReality.getState();
	state.blobPosition[LEFT_PLAYER] = mInitialBlobPosition;
	mVirtualReality.setState( state );
	
	int success = 0;
	
	if( first_hit)
		*first_hit = -1;
	
	auto event_fn = [&success, &first_hit, &res_actions](MatchEvent event) mutable
	{
		int hitcount = 0;
		if( event.event == MatchEvent::BALL_HIT_GROUND )
		{
			if( event.side == LEFT_PLAYER )
				success = -1;
			else
				success = 1;
		}
		
		if( event.event == MatchEvent::BALL_HIT_BLOB )
		{
			if( event.side == LEFT_PLAYER )
			{
				hitcount++;
				if( first_hit && hitcount == 1)
				{
					*first_hit = (int)res_actions->size();
				}
			}
			else
				success = 1;
		}
		if( hitcount > 3)
			success = -1;
	};
	
	// now start the simulation
	mVirtualReality.setEventCallback( event_fn );
	
	int steps = 0;
	
	while( success == 0 && steps < 1000 )
	{
		steps++;
		// predict fann actions
		auto state = mCoder.encode( mVirtualReality.getState() );
		auto result = fann_run(mTestedNetwork, &state[0]);
		PlayerInput action(result[0] > 0, result[1] > 0, result[2] > 0 );
		
		mVirtualReality.step( action, PlayerInput(), true, true );
		
		if( res_actions )
			res_actions->push_back(action);
	}
	
	return success == 1;
}

// -----------------------------------------------------------------------------------------------------------------------
// training data generation

fann_train_data* Benchmark::makeTrainingSet( BenchResult fails, fann* network )
{
	mTestedNetwork = network;
	
	std::vector<PlayerInput> actions;
	actions.reserve(25);
	int first_hit = -1;
	int solved = 0;
	
	int misses = 0;
	int bad_hits = 0;
	
	fann_train_data* new_training = nullptr;
	
	for( int i = 0; i < fails.ballpos.size(); ++i)
	{
		for( int repeat = 0; repeat < std::sqrt(fails.mWeights[i]+1); ++repeat)
		{
		mInitialBallPosition = fails.ballpos[i];
		mInitialBallVelocity = fails.ballvel[i];
		mInitialBlobPosition = fails.blobpos[i];
		
		// now how do we solve this dilemma?
		
		// play again and record output
		actions.clear();
		runTest(&actions, &first_hit);
		
		// read the end state of the VR
		PhysicState state = mVirtualReality.getState();
		float ball_ground_hit = state.ballPosition.x;
		float blob_end_pos = state.blobPosition[LEFT_PLAYER].x;
		float delta_pos = (ball_ground_hit - blob_end_pos) / BLOBBY_SPEED;
		std::vector<bool> changes(actions.size(), false);
		// did we just miss the ball entirely -> count wrong steps and correct
		if( first_hit == -1)
		{
			misses++;
			for( int i = actions.size() - 1; i >= 0; --i)
			{
				// disable wrong actions, enable right ones
				if(actions[i].left && delta_pos > 1 )
				{
					actions[i].left = false;
					changes[i] = true;
					delta_pos -= 1;
				}
				if(!actions[i].right && delta_pos > 1 )
				{
					actions[i].right = true;
					changes[i] = true;
					delta_pos -= 1;
				}
				
				if(actions[i].right && delta_pos < -1 )
				{
					changes[i] = true;
					actions[i].right = false;
					delta_pos += 1;
				}
				
				if(!actions[i].left && delta_pos < -1 )
				{
					changes[i] = true;
					actions[i].left = true;
					delta_pos += 1;
				}
			}
		} else 
		{
			bad_hits++;
			int changecount = 0;
			for( int i = first_hit; i >= 0 && changecount < BALL_RADIUS / BLOBBY_SPEED; --i)
			{
				if( rand() % 2 == 0)
				{
					changes[i] = true;
					changecount++;
					if( actions[i].left && !actions[i].right ) {
						actions[i].left = false; actions[i].right = true; 
					} else if( !actions[i].left && actions[i].right ) {
						actions[i].left = true; actions[i].right = false;
					} else if( (actions[i].left && actions[i].right) || (!actions[i].left && !actions[i].right))
					{
						bool l = rand() % 2 == 0;
						actions[i].left = l; actions[i].right = !l;
					}
				}
			}
		}
		
		// now we should get the ball almost certainly
		fann_train_data* learned = simulateNewTry(actions, changes);
		if(learned)
		{
			if(new_training)
			{
				auto nt = fann_merge_train_data(new_training, learned);
				fann_destroy_train( learned );
				fann_destroy_train( new_training );
				new_training = nt;
			} else 
			{
				new_training = learned;
			}
			
			solved++;
		}
		}
	}
	
	if( new_training )
		std::cout << "Benchmark sim was able to solve " << solved << " / " << fails.ballpos.size() << " problems [ " << misses << ", " << bad_hits<<  " ]\n";
	
	return new_training;
}


fann_train_data* Benchmark::simulateNewTry( const std::vector<PlayerInput>& input, const std::vector<bool>& change_pos )
{
	mVirtualReality.setBallPosition( mInitialBallPosition );
	mVirtualReality.setBallVelocity( mInitialBallVelocity );
	
	PhysicState state = mVirtualReality.getState();
	state.blobPosition[LEFT_PLAYER] = mInitialBlobPosition;
	mVirtualReality.setState( state );
	
	int success = 0;
	
	auto event_fn = [&success](MatchEvent event) mutable
	{
		int hitcount = 0;
		if( event.event == MatchEvent::BALL_HIT_GROUND )
		{
			if( event.side == LEFT_PLAYER )
				success = -1;
			else
				success = 1;
		if( event.event == MatchEvent::BALL_HIT_BLOB )
		{
			if( event.side == LEFT_PLAYER )
				hitcount++;
			else
				success = 1;
		}
		if( hitcount > 3)
			success = -1;
		}
	};
	
	// now start the simulation
	mVirtualReality.setEventCallback( event_fn );
	
	// collect play data
	int total_changes = std::accumulate( change_pos.begin(), change_pos.end(), int(0), [](int a , bool b ) -> int { return a + (b ? 1 : 0); });
	if( total_changes == 0)
		return nullptr;
	
	fann_train_data* training = fann_create_train(total_changes, mCoder.getChannels(), 3);
	
	int t = 0;
	int count = 0;
	while( success == 0 )
	{
		// predict fann actions
		auto state = mCoder.encode( mVirtualReality.getState() );
		auto result = fann_run(mTestedNetwork, &state[0]);
		PlayerInput netaction = PlayerInput(result[0] > 0, result[1] > 0, result[2] > 0 );
		if( t < input.size() )
		{
			PlayerInput betteraction = input[t];
			
			std::array<float, 3> out{ betteraction.left ? 1 : -1, betteraction.right ? 1 : -1, betteraction.up ? 1 : -1 };
			
			// learn new action
			if(change_pos[t])
			{
				std::copy( state.begin(), state.end(), training->input[count] );
				std::copy( out.begin(),   out.end(), training->output[count] );
				count++;
			}
			
			
			netaction = betteraction;
			++t;
		} 
	
		mVirtualReality.step( netaction, PlayerInput(), true, true );
	}
	
	if( success == -1 )
	{
		fann_destroy_train( training );
		return nullptr;
	}
	
	return training;
}

void BenchResult::addFail(Vector2 bp, Vector2 bv, Vector2 blob)
{
	for( int i = 0; i < ballpos.size(); ++i )
	{
		if( (bp - ballpos[i]).lengthSQ() > 1 )
			continue;
		if( (bv - ballvel[i]).lengthSQ() > 0.01 )
			continue;
		if( (blob - blobpos[i]).lengthSQ() > 1 )
			continue;
		
		mWeights[i] += 1;
		// do not insert
		return;
	}
	
	ballpos.push_back( bp );
	ballvel.push_back( bv );
	blobpos.push_back( blob );
	mWeights.push_back( 1 );
}

void BenchResult::clear()
{
	ballpos.clear( );
	ballvel.clear( );
	blobpos.clear( );
}
