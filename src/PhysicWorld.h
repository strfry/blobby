#pragma once

#include "PhysicObject.h"

#include "Global.h"
#include "InputSource.h"
#include <vector>
#include <queue>

const float BLOBBY_SPEED = 4.5; // BLOBBY_SPEED is necessary to determine the size of the input buffer

namespace RakNet
{
	class BitStream;
}

struct PhysicEvent
{
	enum Type {
		PE_NONE,
		PE_BALL_HIT_BLOBBY,
		PE_BALL_HIT_GROUND
	} type;
	
	float time_substep;	// when did it happen
	Vector2 position;	// where did it happen
	
	operator bool() const {
		return type != PE_NONE;
	}
};

class PhysicWorld
{
	public:
		PhysicWorld();
		~PhysicWorld();
	
		void step();
		
		const PhysicObject& getBall() const;
		PhysicObject& getBallReference();
		const PhysicObject& getBlob(PlayerSide player) const;
		PhysicObject& getBlobReference(PlayerSide player);
		
		PhysicEvent getNextEvent();
		
		// ------------------
		//  copied functions
		// ------------------

		float getBlobState(PlayerSide player) const;
		float getBallRotation() const;

		// Blobby animation methods
		void blobbyAnimationStep(PlayerSide player);
		void blobbyStartAnimation(PlayerSide player);

		// This reports the intensity of the collision
		// which was detected and also queried last.
		float lastHitIntensity() const;

		// Set a new state received from server over a RakNet BitStream
		void setState(RakNet::BitStream* stream);

		// Fill a Bitstream with the state
		void getState(RakNet::BitStream* stream) const;

		// Fill a Bitstream with a side reversed state
		void getSwappedState(RakNet::BitStream* stream) const;
		
	private:
		std::vector<PhysicObject> mObjects;
		std::queue<PhysicEvent> mEventQueue;
};

