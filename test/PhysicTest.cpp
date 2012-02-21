#define BOOST_TEST_MODULE Physic
#include <boost/test/unit_test.hpp>

#include <iostream>
#include "PhysicObject.h"
#include "PhysicWorld.h"
#include "PhysicWall.h"

std::ostream& operator<<(std::ostream& stream, const Vector2& vector)
{
	stream << "(" << vector.x << ", " << vector.y << ")";
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
	
	float radius = rand() % 101 - 50;
	po.setRadius(radius);
	BOOST_CHECK_EQUAL(po.getRadius(), radius);
	
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


BOOST_AUTO_TEST_SUITE_END()





// ****************************************************************************
// ***		P h y s i c  -  W a l l   T e s t								***
// ****************************************************************************



BOOST_AUTO_TEST_SUITE( physic_wall )

/// this check tests whether the constructor sets all values
/// as desired
BOOST_AUTO_TEST_CASE( constructor )
{
	/// \todo we don't have any functions that can do constructor testing
}

/// this test tests a simple wall collision
BOOST_AUTO_TEST_CASE( simple_collision )
{
	PhysicWall wall = PhysicWall(PhysicWall::HORIZONTAL, 500);
	PhysicObject ob = PhysicObject(Vector2(0,0), Vector2(0, 30));
	
	ob.setRadius(20);
	ob.addWall(&wall);
	
	for(int i = 0; i < 50; ++i )
	{
		ob.step();
		if(ob.getPosition().y > 480)
		{
			std::cout << "position " << i << " " << ob.getPosition().y << std::endl;
		}
		BOOST_CHECK(ob.getPosition().y <= 480);
		
	}
}


BOOST_AUTO_TEST_SUITE_END()
