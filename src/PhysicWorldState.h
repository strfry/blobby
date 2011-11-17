#pragma once 

class PhysicWorld;

class PhysicWorldState
{
	friend class PhysicWorld;
	
	public:
	
		Vector2 getBlobPosition(PlayerSide player) const;
		// no way to get blob velocity
		float getBlobState(PlayerSide player) const;
		
		Vector2 getBallPosition() const;
		Vector2 getBallVelocity() const;		
		float getBallRotation() const;
		float getBallSpeed() const;
	
	protected:
		Vector2 mBlobPosition[MAX_PLAYERS];
		Vector2 mBlobVelocity[MAX_PLAYERS];
		
		Vector2 mBallVelocity;
		Vector2 mBallPosition;

		float mBallRotation;
		float mBallAngularVelocity;
		float mBlobState[MAX_PLAYERS];
		float mCurrentBlobbyAnimationSpeed[MAX_PLAYERS];
};

