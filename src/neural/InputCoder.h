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
	
	// calculates a numeric index based on the state. similar states have the same index.
	unsigned int getStateIndex( const float coded[] ) const;
	unsigned int getMaxStateIdx() const;
	
	int getChannels() const; // gets the number of input channels
private:
	
	static float parabel_time( float grav, float vel, float y0, float target);
	
	int mBallXResolution;
	int mBallYResolution;
	int mBallXVelResolution;
	int mBallYVelResolution;
	int mBlobXResolution;
	int mBlobYResolution;
	int mInputSize;
	
	// flags 
	bool f_ground_pos = true;
	bool f_relative_pos = true;
};

#endif // INPUTCODER_H_INCLUDED
