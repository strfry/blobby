#pragma once

#include "Vector.h"

class PhysicObject
{
	public:
		void setPosition(const Vector2& np);
		void setVelocity(const Vector2& nv);
	
		const Vector2& getPosition() const;
		const Vector2& getVelocity() const;
		
		Vector2& getPosition();
		Vector2& getVelocity();
		
	private:
		Vector2 mPosition;
		Vector2 mVelocity;
		Vector2 mAcceleration;
};
