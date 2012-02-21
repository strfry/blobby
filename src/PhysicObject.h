#pragma once

#include "Vector.h"
#include <string>
#include <vector>

class PhysicWall;

class PhysicObject
{
	public:
		/// \brief default constructor
		/// \details leaves all member variables in default
		///			initialised state.
		/// \post mWalls.size() = 0 (that's a guarantee vector already makes)
		PhysicObject() = default;
		
		/// \brief position/velocity constructor
		/// \details initialises position and velocity
		/// \post getAcceleration() = (0,0)
		PhysicObject(const Vector2& p, const Vector2& v);
	
		// ----------------------------
		// motion state getters/setters
		
		/// sets position
		void setPosition(const Vector2& np);
		/// sets velocity
		void setVelocity(const Vector2& nv);
		/// sets acceleration
		void setAcceleration( const Vector2& na);
		
		/// gets current position
		const Vector2& getPosition() const;
		/// gets current velocity
		const Vector2& getVelocity() const;
		/// gets current acceleration
		const Vector2& getAcceleration() const;
		
		// -----------------------------
		
		/// \depreciated
		Vector2& getPosition();
		Vector2& getVelocity();
		
		void step();
		
		void setDebugName(const std::string& name);
		const std::string& getDebugName() const;
		void setRadius(float rad);
		float getRadius() const;
		
		void addWall(PhysicWall* pw);
		
	private:
		Vector2 mPosition;
		Vector2 mVelocity;
		Vector2 mAcceleration;
		
		float mRadius;
		
		std::vector<PhysicWall*> mWalls;
		
		std::string mDebugName;
};
