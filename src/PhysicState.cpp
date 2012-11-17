/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

/* header include */
#include "PhysicState.h"

/* includes */
#include <limits>

#include "raknet/BitStream.h"

#include "GameConstants.h"
#include "GenericIO.h"

USER_SERIALIZER_IMPLEMENTATION_HELPER(PhysicState)
{
	io.number( value.blobPosition[LEFT_PLAYER].x );
	io.number( value.blobPosition[LEFT_PLAYER].y );
	
	io.number( value.blobVelocity[LEFT_PLAYER].x );
	io.number( value.blobVelocity[LEFT_PLAYER].y );
	
	io.number( value.blobPosition[RIGHT_PLAYER].x );
	io.number( value.blobPosition[RIGHT_PLAYER].y );	
	
	io.number( value.blobVelocity[RIGHT_PLAYER].x );
	io.number( value.blobVelocity[RIGHT_PLAYER].y );
	
	io.number( value.ballPosition.x );
	io.number( value.ballPosition.y );	
	
	io.number( value.ballVelocity.x );
	io.number( value.ballVelocity.y );
	
	io.number( value.ballAngularVelocity );
	
	// the template keyword is needed here so the compiler knows generic is
	// a template function and does not complain about <>.
	io.template generic<PlayerInput> ( value.playerInput[LEFT_PLAYER] );
	io.template generic<PlayerInput> ( value.playerInput[RIGHT_PLAYER] );
	
}

void PhysicState::swapSides()
{
	blobPosition[LEFT_PLAYER].x = RIGHT_PLANE - blobPosition[LEFT_PLAYER].x;
	blobPosition[RIGHT_PLAYER].x = RIGHT_PLANE - blobPosition[RIGHT_PLAYER].x;
	std::swap(blobPosition[LEFT_PLAYER], blobPosition[RIGHT_PLAYER]);
	
	ballPosition.x = RIGHT_PLANE - ballPosition.x;
	ballVelocity.x = -ballVelocity.x;
	ballAngularVelocity = -ballAngularVelocity;
	
	std::swap(playerInput[LEFT_PLAYER].left, playerInput[LEFT_PLAYER].right);
	std::swap(playerInput[RIGHT_PLAYER].left, playerInput[RIGHT_PLAYER].right);
	std::swap(playerInput[LEFT_PLAYER], playerInput[RIGHT_PLAYER]);
}

bool PhysicState::operator==(const PhysicState& other) const
{
	return
		blobPosition[LEFT_PLAYER] == other.blobPosition[LEFT_PLAYER] && 
		blobPosition[RIGHT_PLAYER] == other.blobPosition[RIGHT_PLAYER] &&
		blobVelocity[LEFT_PLAYER] == other.blobVelocity[LEFT_PLAYER] && 
		blobVelocity[RIGHT_PLAYER] == other.blobVelocity[RIGHT_PLAYER] &&
		ballPosition == other.ballPosition &&
		ballVelocity == other.ballVelocity &&
		ballAngularVelocity == other.ballAngularVelocity &&
		playerInput[LEFT_PLAYER] == other.playerInput[LEFT_PLAYER] && 
		playerInput[RIGHT_PLAYER] == other.playerInput[RIGHT_PLAYER];
}
