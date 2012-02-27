/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)

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

#pragma once

#include "Vector.h"

/// \todo policy for upperLeft > lowerRight
/*! \brief Axis Aligned Bounding Box
	\details This class represents an axis aligned bounding box
			(rectangle perpendicular to the screen axis). 
*/
struct AABBox
{
	/// \brief default ctor
	/// \details initialises both corners to (0,0), so 
	AABBox();
	
	/// \brief corners ctor
	/// \details initisalises upperLeft and lowerRight corner with
	///		the respective values.
	AABBox(Vector2 uL, Vector2 lR);
	
	/// \brief center-size ctor
	/// \details the new rect has the coordinates (center - half_width, center - half_height) ... 
	///			(center + half_with, center + half_height)
	AABBox(Vector2 center, float half_width, float half_height);
	
	/// resets to (0,0)...(0,0)
	void clear();
	
	// info
	Vector2 getCenter() const;
	
	void merge(const AABBox& other);
	bool intersects(const AABBox& other) const;
	bool isPointInside(Vector2 point) const;
	
	/// moves the box by vec
	AABBox& operator+=(Vector2 vec);
	/// gets the box moved by vec
	AABBox operator+(Vector2 vec) const;
	
	Vector2 upperLeft;
	Vector2 lowerRight;
};

