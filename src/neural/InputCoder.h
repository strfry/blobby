#ifndef INPUTCODER_H_INCLUDED
#define INPUTCODER_H_INCLUDED

#include <vector>

class PhysicState;

// a simple interface for encoding blobby situations into NN input
class InputCoder
{
public:
	InputCoder();
	
	std::vector<float> encode(const PhysicState& state);
	
	int getChannels() const; // gets the number of input channels
private:
	
	static float parabel_time( float grav, float vel, float y0, float target);
	
	int mBallXResolution;
	int mBallYResolution;
	int mBallXVelResolution;
	int mBallYVelResolution;
	int mInputSize;
	
	// flags 
	bool f_ground_pos = true;
	bool f_relative_pos = true;
};

#endif // INPUTCODER_H_INCLUDED
