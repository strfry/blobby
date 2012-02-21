#define BOOST_TEST_MODULE Physic
#include <boost/test/unit_test.hpp>

#include <iostream>
#include "PhysicObject.h"

std::ostream& operator<<(std::ostream& stream, const Vector2& vector)
{
	stream << "(" << vector.x << ", " << vector.y << ")";
	return stream;
}

Vector2 getRandVec() 
{
	return Vector2(rand() % 101 - 50, rand() % 101 - 50);
}


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

BOOST_AUTO_TEST_SUITE_END()


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


BOOST_AUTO_TEST_SUITE_END()
