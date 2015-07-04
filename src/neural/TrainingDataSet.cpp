#include "TrainingDataSet.h"
#include <iterator>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <cassert>

extern "C"
{
	#include "fann.h"
}

TrainingDataSet::TrainingDataSet( int reservoir_size, int input_count ) : 
	mCurrentDataCollection( fann_create_train(reservoir_size, input_count, 3) ),
	mLastInput( input_count ), mDataHistogram(mCoder.getMaxStateIdx(), 0)
{
	
}

void TrainingDataSet::saveData() const
{
	fann_save_train(mCurrentTrainingData, "training.dat");
}

void TrainingDataSet::loadData()
{
	assert( mCurrentTrainingData == nullptr );
	mCurrentTrainingData = fann_read_train_from_file("training.dat");
}

void TrainingDataSet::addDefaultBotDataPoint(const std::vector<float>& input, const std::array<float, 3>& desired_out)
{
	// if we have filled the reservoir, we cannot do anything here. 
	if(isNewDataReady() )
		return;
	
	// check that we got a new sample
	if( std::equal(input.begin(), input.end(), mLastInput.begin()) )
		return;
	
	mLastInput = input;
	std::copy( input.begin(), input.end(), mCurrentDataCollection->input[mCollectionIndex] );
	std::copy( desired_out.begin(), desired_out.end(), mCurrentDataCollection->output[mCollectionIndex] );
	++mCollectionIndex;
}

void TrainingDataSet::setDifficultTraining(fann_train_data* dt, int recs)
{
	std::unique_lock<std::mutex>(mDTM);
	if(mDifficultTraining == nullptr )
	{
		mDifficultTraining = dt;
		mDifficultTrainingSize = recs;
	} else 
	{
		// merge
		auto old = mDifficultTraining;
		mDifficultTraining = fann_merge_train_data(mDifficultTraining, dt);
		fann_destroy_train( old );
		fann_destroy_train( dt );
		mDifficultTrainingSize += recs;
	}
}

bool TrainingDataSet::isNewDataReady() const
{
	return mCollectionIndex == fann_length_train_data(mCurrentDataCollection);
}

fann_train_data* TrainingDataSet::getDataForTraining()
{
	/// \todo add a test data set!
	
	// if new data ready, just use old one
	if( !isNewDataReady() )
		return mCurrentTrainingData;
	
	// otherwise
	// do we call this the first time? if yes, we just return the collected data
	if(mCurrentTrainingData == nullptr)
	{
		// create new training data
		mCurrentTrainingData = fann_create_train(fann_length_train_data(mCurrentDataCollection), 
												fann_num_input_train_data(mCurrentDataCollection),  
												fann_num_output_train_data(mCurrentDataCollection));
		std::swap(mCurrentTrainingData, mCurrentDataCollection);
		mCollectionIndex = 0;
		return mCurrentTrainingData;
	} // otherwise
	else 
	{		
		mDataHistogram.assign(mCoder.getMaxStateIdx(), 0);
		int copy_to = 0;
		for( int i = 0; i < fann_length_train_data(mCurrentTrainingData); ++i)
		{
			int idx = mCoder.getStateIndex( mCurrentTrainingData->input[i]);
			
			// copy
			for( int j = 0; j < mCurrentTrainingData->num_input; ++j)
				mCurrentTrainingData->input[copy_to][j] = mCurrentTrainingData->input[i][j];
			for( int j = 0; j < mCurrentTrainingData->num_output; ++j)
				mCurrentTrainingData->output[copy_to][j] = mCurrentTrainingData->output[i][j];
			
			// count in  histogram
			++mDataHistogram[idx];
			std::cout << idx << "\n";
			if( rand() % mDataHistogram[idx] < 5 )
			{
				++copy_to;
			}
		}
		
		std::cout << "shrink dataset: " << fann_length_train_data(mCurrentTrainingData) << " -> "  << copy_to << "\n";
		
		// shrink amount of data in training net to get space for new stuff
		int amount = mCurrentTrainingData->num_data;
		mCurrentTrainingData->num_data = copy_to;
		auto merged = fann_merge_train_data( mCurrentTrainingData, mCurrentDataCollection );
		// restore amount to ensure proper deletion? just to be safe
		mCurrentTrainingData->num_data = amount;
		std::swap(mCurrentTrainingData, merged);
		fann_destroy_train(merged);
		
		if( mDifficultTraining )
		{
			std::unique_lock<std::mutex>(mDTM);
			auto merged = fann_merge_train_data( mCurrentTrainingData, mDifficultTraining );
			std::swap(mCurrentTrainingData, merged);
			fann_destroy_train(merged);
			fann_destroy_train(mDifficultTraining);
			mDifficultTraining = nullptr;
			mDifficultTrainingSize = 0;
		}
		
		// now shuffle the data
		fann_shuffle_train_data(mCurrentTrainingData);
		
		// allow new collections
		mCollectionIndex = 0;
		return mCurrentTrainingData;
	}
}

int TrainingDataSet::getFailureDataCount() const
{
	if(mDifficultTraining == nullptr )
	{
		return 0;
	} else 
	{
		std::unique_lock<std::mutex>(mDTM);
		return mDifficultTrainingSize;
	}
}
