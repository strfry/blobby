#ifndef TRAININGDATASET_H_INCLUDED
#define TRAININGDATASET_H_INCLUDED

#include <atomic>
#include <array>
#include <vector>
#include <mutex>

struct fann_train_data;

/// this class is used to accumulate training data
class TrainingDataSet
{
public:
	TrainingDataSet( int reservoir_size, int input_count );
	
	void addDefaultBotDataPoint(const std::vector<float>& input, const std::array<float, 3>&);
	
	void saveData() const;
	void loadData();
	
	/// test whether a new batch of training data is ready
	bool isNewDataReady() const;
	
	int getFailureDataCount() const;
	
	/// gets a pointer to data that can be safely used for training.
	fann_train_data* getDataForTraining();
	
	void setDifficultTraining(fann_train_data* difficultTraining, int records);
private:
	fann_train_data* mCurrentTrainingData = nullptr;
	
	fann_train_data* mCurrentTestData = nullptr; 
	
	fann_train_data* mDifficultTraining = nullptr;
	int mDifficultTrainingSize = 0;
	std::mutex mDTM;
	
	fann_train_data* mCurrentDataCollection = nullptr;
	std::atomic<unsigned int> mCollectionIndex{0};
	std::vector<float> mLastInput; // cache last input, to reduce data spam on serve
};

#endif // TRAININGDATASET_H_INCLUDED
