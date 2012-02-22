#pragma once

#include "Vector.h"
#include "PhysicCollisionShape.h"
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

class PhysicWorld;

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
		
		// shape
		void addCollisionShape( boost::shared_ptr<ICollisionShape> newshape );
		void clearCollisionShapes();
		int getCollisionShapeCount() const;
		boost::weak_ptr<const ICollisionShape> getCollisionShape(int i) const; 
		
		AABBox getBoundingBox() const;
		
		void step(float time = 1.f);
		
		void setDebugName(const std::string& name);
		const std::string& getDebugName() const;
		void setWorld(PhysicWorld* world);
		PhysicWorld* getWorld() const;
		
		struct MotionState
		{
			Vector2 pos;
			Vector2 vel;
			const PhysicObject* object;
		};
		
		MotionState getMotionState() const;
		MotionState getPredictedMotionState(float time = 1) const;
	private:
		// motion state
		Vector2 mPosition;
		Vector2 mVelocity;
		Vector2 mAcceleration;
		
		// object geometry/properties
		std::vector<boost::shared_ptr<ICollisionShape>> mCollisionShapes;
		AABBox mBoundingBox;
		
		std::string mDebugName;
		
		// world this object is in
		PhysicWorld* mConnectedWorld;
};
