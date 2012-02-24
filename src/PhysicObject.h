#pragma once

#include "Vector.h"
#include "PhysicCollisionShape.h"
#include "PhysicConstraint.h"
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
		/// sets rotation
		void setRotation(float r);
		/// sets angular velocity
		void setAngularVelocity(float a);
		
		/// gets current position
		const Vector2& getPosition() const;
		/// gets current velocity
		const Vector2& getVelocity() const;
		/// gets current acceleration
		const Vector2& getAcceleration() const;
		/// gets current rotation
		float getRotation() const;
		/// gets angular velocity
		float getAngularVelocity() const;
		
		// -----------------------------
		
		// shape
		void addCollisionShape( boost::shared_ptr<ICollisionShape> newshape );
		void clearCollisionShapes();
		int getCollisionShapeCount() const;
		boost::weak_ptr<const ICollisionShape> getCollisionShape(int i) const; 
		
		AABBox getBoundingBox() const;
		
		
		// constraint
		void addConstraint( boost::shared_ptr<IPhysicConstraint> cst );
		void clearConstraints();
		int getConstraintCount() const;
		boost::weak_ptr<const IPhysicConstraint> getConstraint(int i) const; 
		
		
		void addForce(const Vector2& force);
		void clearForces();
		
		void step(float time = 1.f);
		
		/// assigns a debug name to this object
		void setDebugName(const std::string& name);
		/// gets the debug name of this object
		const std::string& getDebugName() const;
		
		void setCollisionType(unsigned int ct);
		unsigned int getCollisionType() const;
		
		/// sets the world this object is in
		/// \todo do we really want the world to change for any given
		/// object. i guess not. So we have to do sth. so that world can 
		///	be set only once.
		void setWorld(const PhysicWorld* world);
		
		/// gets the world associated with this physic object
		const PhysicWorld* getWorld() const;
		
		struct MotionState
		{
			Vector2 pos;
			Vector2 vel;
			float rot;
			float rev;
			const PhysicObject* object;
			
			bool operator==(const MotionState& other) const 
			{
				return pos == other.pos && vel== other.vel;// && object == other.object;
			}
		};
		
		/// returns a motion state for the object.
		/// contains the current position and velocity
		MotionState getMotionState() const;
		
		/// returns a prediction of the motion state after given time
		/// fulfills: getPredictedMotionState(t) = (this->step(t), this->getMotionState());
		MotionState getPredictedMotionState(float time = 1) const;
		
	private:
		// motion state
		Vector2 mPosition;
		Vector2 mVelocity;
		Vector2 mAcceleration;
		float mRotation;
		float mAngularVelocity;
		
		// object geometry/properties
		unsigned int mCollisionType;
		std::vector<boost::shared_ptr<ICollisionShape>> mCollisionShapes;
		std::vector<boost::shared_ptr<IPhysicConstraint>> mConstraints;
		AABBox mBoundingBox;
		
		std::string mDebugName;
		
		// world this object is in
		const PhysicWorld* mConnectedWorld;
};
