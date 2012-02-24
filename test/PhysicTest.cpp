#define BOOST_TEST_MODULE Physic
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <iomanip>
#include <limits>
#include "PhysicObject.h"
#include "PhysicWorld.h"

std::ostream& operator<<(std::ostream& stream, const Vector2& vector)
{
	stream << "(" << vector.x << ", " << vector.y << ")";
	return stream;
}

std::ostream& operator<<(std::ostream& stream, const PhysicObject::MotionState& ms)
{
	stream << ms.object->getDebugName() << " [" << ms.pos << ", " << ms.vel << "]";
	return stream;
}


Vector2 getRandVec() 
{
	return Vector2(rand() % 101 - 50, rand() % 101 - 50);
}


// ****************************************************************************
// ***			V e c t o r 2 D  -  T e s t									***
// ****************************************************************************

BOOST_AUTO_TEST_SUITE( vector )

BOOST_AUTO_TEST_CASE( constructor )
{
	// zero
	Vector2 vec;
	BOOST_CHECK_EQUAL(vec.x, 0);
	BOOST_CHECK_EQUAL(vec.y, 0);
	BOOST_CHECK_EQUAL(vec.val[0], 0);
	BOOST_CHECK_EQUAL(vec.val[1], 0);
	
	// components
	int x = rand() % 101 - 50;
	int y = rand() % 101 - 50;
	Vector2 vec2 = Vector2(x, y);
	
	BOOST_CHECK_EQUAL(vec2.x, x);
	BOOST_CHECK_EQUAL(vec2.y, y);
	BOOST_CHECK_EQUAL(vec2.val[0], x);
	BOOST_CHECK_EQUAL(vec2.val[1], y);
	
	// difference
	Vector2 vec3 = Vector2(vec2, vec);
	BOOST_CHECK_EQUAL(vec3.x, -x);
	BOOST_CHECK_EQUAL(vec3.y, -y);
	
	BOOST_CHECK_EQUAL(Vector2(vec2, vec2), Vector2(0,0));
}

BOOST_AUTO_TEST_CASE( clear )
{
	Vector2 vec = getRandVec();
	vec.clear();
	BOOST_CHECK_EQUAL(vec, Vector2());
	
}

BOOST_AUTO_TEST_CASE( simple_reflection )
{
	Vector2 vec = getRandVec();
	
	BOOST_CHECK_EQUAL(vec, vec.reflectedX().reflectedX());
	BOOST_CHECK_EQUAL(vec, vec.reflectedY().reflectedY());
	
	BOOST_CHECK_EQUAL(-vec.x, vec.reflectedX().x);
	BOOST_CHECK_EQUAL(vec.y, vec.reflectedX().y);
	
	BOOST_CHECK_EQUAL(vec.x, vec.reflectedY().x);
	BOOST_CHECK_EQUAL(-vec.y, vec.reflectedY().y);
}

BOOST_AUTO_TEST_CASE( length_scaling )
{
	Vector2 zero = Vector2();
	Vector2 vec = getRandVec();
	
	BOOST_CHECK_EQUAL( zero, zero.scaled( 4.5f ) );
	BOOST_CHECK_EQUAL( zero, zero.scaledX( 5.5f ) );
	BOOST_CHECK_EQUAL( zero, zero.scaledX( -4.5f ) );
	BOOST_CHECK_EQUAL( zero, vec.scaled( 0 ) );
	
	float sc = (rand() % 101 - 50) / 11.f;
	
	BOOST_CHECK_EQUAL( vec.x * sc, vec.scaledX(sc).x );
	BOOST_CHECK_EQUAL( vec.y * sc, vec.scaledY(sc).y );
	
	BOOST_CHECK_EQUAL( vec.scaledX(sc).scaledY(sc), vec.scaled(sc) );
	
	float dif = std::abs(vec.scaled(sc).length() - vec.length() * sc);
	BOOST_CHECK( dif < std::numeric_limits<float>::epsilon() * vec.length() * sc  );
	
	BOOST_CHECK_EQUAL( zero.length(), 0.f  );
}

BOOST_AUTO_TEST_CASE( vector_decomposition )
{
	Vector2 v1 = Vector2(1,0);
	Vector2 v2 = Vector2(0,1);
	std::cout << v1.decompose(v2) << "\n";
	BOOST_CHECK_EQUAL(v1, v1.decompose(v2).recompose(v2));
	
	Vector2 v3 = Vector2(10,10);
	Vector2 v4 = Vector2(1,1);
	std::cout << v3.decompose(v4) << "\n";
	BOOST_CHECK_EQUAL(v3, v3.decompose(v4).recompose(v4));
	
	Vector2 vec = getRandVec();
	Vector2 vec2 = getRandVec();
	
	BOOST_CHECK_EQUAL(vec, vec.decompose(vec2).recompose(vec2));
}


BOOST_AUTO_TEST_SUITE_END()





// ****************************************************************************
// ***		P h y s i c  -  O b j e c t   T e s t							***
// ****************************************************************************



BOOST_AUTO_TEST_SUITE( physic_object )

/// this check tests whether the constructor sets all values
/// as desired
BOOST_AUTO_TEST_CASE( constructor )
{
	// default c'tor does not make any promises
	
	// pos/vel c'tor
	Vector2 position = getRandVec();
	Vector2 velocity = getRandVec();
	PhysicObject po = PhysicObject(position, velocity);
	
	BOOST_CHECK_EQUAL(po.getPosition(), position);
	BOOST_CHECK_EQUAL(po.getVelocity(), velocity);
	BOOST_CHECK_EQUAL(po.getAcceleration(), Vector2(0,0));
	BOOST_CHECK_EQUAL(po.getDebugName(), "");
	BOOST_CHECK_EQUAL(po.getWorld(), (void*)0);
}

/// this test checks that setters and getters work as expected, i.e., that getters report
/// the value last set by a setter
BOOST_AUTO_TEST_CASE( get_set_round_trip )
{
	Vector2 position = getRandVec();
	Vector2 velocity = getRandVec();
	PhysicObject po = PhysicObject(Vector2(0,0), Vector2(0,0));
	
	BOOST_CHECK_EQUAL(po.getPosition(), Vector2(0,0));
	BOOST_CHECK_EQUAL(po.getVelocity(), Vector2(0,0));
	
	po.setPosition(position);
	BOOST_CHECK_EQUAL(po.getPosition(), position);
	
	po.setVelocity(velocity);
	BOOST_CHECK_EQUAL(po.getVelocity(), velocity);
	
	po.setDebugName("name_test");
	BOOST_CHECK_EQUAL(po.getDebugName(), "name_test");
	
	PhysicWorld w;
	po.setWorld(&w);
	BOOST_CHECK_EQUAL(po.getWorld(), &w);
}

/// this test checks that position does not change when velocity and acceleration are 0
BOOST_AUTO_TEST_CASE( no_motion )
{
	Vector2 position = getRandVec();
	PhysicObject po = PhysicObject(position, Vector2(0,0));
	
	for(int i=0; i < 10; ++i)
	{
		po.step();
	}
	
	BOOST_CHECK_EQUAL(po.getPosition(), position);
	BOOST_CHECK_EQUAL(po.getVelocity(), Vector2(0,0));
}

/// this test checks that velocity does not change when acceleration is 0
BOOST_AUTO_TEST_CASE( linear_motion )
{
	Vector2 position = getRandVec();
	Vector2 velocity = getRandVec();
	PhysicObject po = PhysicObject(position, velocity);
	
	for(int i=0; i < 10; ++i)
	{
		po.step();
	}
	
	BOOST_CHECK_EQUAL(po.getPosition(), position + velocity * 10);
	BOOST_CHECK_EQUAL(po.getVelocity(), velocity);
}

/// this test checks that velocity does not change when acceleration is 0
BOOST_AUTO_TEST_CASE( accelerated_motion )
{
	Vector2 position = getRandVec();
	Vector2 velocity = getRandVec();
	Vector2 acceleration = getRandVec();
	PhysicObject po = PhysicObject(position, velocity);
	po.setAcceleration(acceleration);
	
	for(int i=0; i < 10; ++i)
	{
		po.step();
	}
	
	BOOST_CHECK_EQUAL(po.getPosition(), position + velocity * 10 + acceleration/2 * 10*10);
	BOOST_CHECK_EQUAL(po.getVelocity(), velocity + acceleration * 10);
}


// Motion State
/// tests that a motion state contains the right values
BOOST_AUTO_TEST_CASE( motion_state )
{
	Vector2 position = getRandVec();
	Vector2 velocity = getRandVec();
	Vector2 acceleration = getRandVec();
	PhysicObject po = PhysicObject(position, velocity);
	po.setAcceleration(acceleration);
	
	for(int i=0; i < 10; ++i)
	{
		po.step();
	}
	
	BOOST_CHECK_EQUAL(po.getMotionState().pos, po.getPosition());
	BOOST_CHECK_EQUAL(po.getMotionState().vel, po.getVelocity());
}


/// tests that the motion state prediction is equivalent to calling step if 
/// no interactions with other objects happen
BOOST_AUTO_TEST_CASE( motion_state_prediction )
{
	Vector2 position = getRandVec();
	Vector2 velocity = getRandVec();
	Vector2 acceleration = getRandVec();
	PhysicObject po = PhysicObject(position, velocity);
	po.setAcceleration(acceleration);
	
	PhysicObject::MotionState future = po.getPredictedMotionState();
	po.step();
	
	BOOST_CHECK_EQUAL(po.getMotionState(), future);
}

// substeps
/// 
BOOST_AUTO_TEST_CASE( substep_linear_motion )
{
	Vector2 position = getRandVec() / 11.f + getRandVec();
	Vector2 velocity = getRandVec() / 11.f;
	PhysicObject po = PhysicObject(position, velocity);
	PhysicObject ref = PhysicObject(position, velocity);
	
	for(int i=0; i < 75*10; ++i)
	{
		po.step(0.5f);
		po.step(0.5f);
		ref.step();
	}
	
	std::cout <<  std::setprecision (50) << (po.getPosition() - ref.getPosition()) << "\n";;
	
	// currently, this test will always fail. we have to add a desired precision
	BOOST_CHECK_EQUAL(po.getMotionState(), ref.getMotionState());

	po = PhysicObject(position, velocity);
	ref = PhysicObject(position, velocity);
	
	for(int i=0; i < 75*10; ++i)
	{
		for(int j=0; j < 16; ++j)
		{
			po.step(1.f/16);
		}
		ref.step();
	}
	
	std::cout <<  std::setprecision (50) << (po.getPosition() - ref.getPosition()) << "\n";;
	
	// currently, this test will always fail. we have to add a desired precision
	BOOST_CHECK_EQUAL(po.getMotionState(), ref.getMotionState());
}
BOOST_AUTO_TEST_SUITE_END()
