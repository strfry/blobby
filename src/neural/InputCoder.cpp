#include "InputCoder.h"
#include "PhysicState.h"
#include "GameConstants.h"
#include <cassert>
#include <iostream>

InputCoder::InputCoder() : mBallXResolution(16), mBallYResolution(20), mBallXVelResolution(16), mBallYVelResolution(16), mBlobXResolution(8), mBlobYResolution(8)
{
	mInputSize = mBallXResolution + mBallYResolution + mBallXVelResolution + 
					mBallYVelResolution + mBlobXResolution + mBlobYResolution + 6;
}

int InputCoder::getChannels() const
{
	return mInputSize;
}

float register_overlap( float min, float max, int step, int count, float position )
{
	float p = float(step) / count + 0.5;
	float reference = (max - min) * p + min;
	float range = 2 * (max - min) / count;
	float dist = std::abs( position - reference ) / range;
	return std::max(0.f, 1.f - dist);
}

float code(std::vector<float>& target, int& iter, float min, float max, float count, float pos)
{
	if( min > pos || pos > max)
	{
		std::cout << "coding problem: " << min << " < " << pos << " < " << max << " not fulfilled\n"; 
	}
	for( int i = 0; i < count; ++i)
	{
		target.at(iter) = register_overlap(min, max, i, count, pos);
		++iter;
	}
}

std::vector<float> InputCoder::encode(const PhysicState& world)
{
	std::vector<float> coded( mInputSize );
	int idx = 0;
	// give bot ball positions by x/y neurons
	code(coded, idx, 0, NET_POSITION_X, mBlobXResolution,  world.blobPosition[LEFT_PLAYER].x);
	code(coded, idx, 50, GROUND_PLANE_HEIGHT, mBlobYResolution,  world.blobPosition[LEFT_PLAYER].y);
	code(coded, idx, LEFT_PLANE, RIGHT_PLANE, mBallXResolution, world.ballPosition.x);
	code(coded, idx, -300, GROUND_PLANE_HEIGHT_MAX, mBallYResolution, world.ballPosition.y);
	code(coded, idx, -BALL_COLLISION_VELOCITY - 2, BALL_COLLISION_VELOCITY + 2, mBallXVelResolution, world.ballVelocity.x);
	code(coded, idx, -BALL_COLLISION_VELOCITY - 1, BALL_COLLISION_VELOCITY + 20, mBallYVelResolution, world.ballVelocity.y);
	
	// helper var: position where ball will hit the ground: crude estimate
	if( f_ground_pos )
	{
		float time_to_ground = parabel_time(BALL_GRAVITATION, world.ballVelocity.y, world.ballPosition.y, GROUND_PLANE_HEIGHT);
		coded[++idx] = (world.ballPosition.x + world.ballVelocity.x * time_to_ground) / 800;
	}
	
	// relative positions in better resolution
	if( f_relative_pos )
	{
		coded[++idx] = (world.ballPosition.x - world.blobPosition[LEFT_PLAYER].x) / 2 / (BALL_RADIUS + BLOBBY_LOWER_RADIUS);
	}
	return coded;
}

void acc( const float coded[], int idx, int& cumm_count, int max )
{
	idx *= (max / 2);
	for( int p = 0; p < cumm_count + max; ++p )
		if( coded[p + cumm_count] > 0.5 )
		{
			cumm_count += (max / 2);
			idx += p;
			return;	
		}
}

unsigned int InputCoder::getStateIndex( const float coded[] ) const
{
	int cmt = 0;
	int idx = 0;
	acc( coded, idx, cmt, mBlobXResolution);
	acc( coded, idx, cmt, mBlobYResolution);
	acc( coded, idx, cmt, mBallXResolution);
	acc( coded, idx, cmt, mBallYResolution);
	acc( coded, idx, cmt, mBallXVelResolution);
	acc( coded, idx, cmt, mBallYVelResolution);
	return idx;
}

unsigned int InputCoder::getMaxStateIdx() const
{
	return (mBallXResolution/2) * (mBallYResolution/2) * (mBlobXResolution/2) * (mBlobYResolution/2) * (mBallXVelResolution/2) * (mBallYVelResolution/2);
}

float InputCoder::parabel_time( float grav, float vel, float y0, float target)
{
	// 0 = g/2 t² + v t + y0 - d
	// t²+ 2v/g + (y0 - d) / g
	// =>
	float sq = vel*vel + 2*grav*(target - y0);

	float p = 2*vel / grav;
	float q = 2*(y0 - target) / grav;
	
	// return bullshit when position is unreachable
	if( p*p/4-q < 0)
		return 0;
	
	return -p/2 + std::sqrt(p*p/4-q);
}
