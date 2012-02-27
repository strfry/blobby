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
struct AABBox
{
	AABBox();
	AABBox(Vector2 uL, Vector2 lR);
	AABBox(Vector2 center, float half_width, float half_height);
	
	void clear();
	
	void merge(const AABBox& other);
	bool intersects(const AABBox& other) const;
	bool isPointInside(Vector2 point) const;
	
	AABBox& operator+=(Vector2 vec);
	AABBox operator+(Vector2 vec) const;
	
	Vector2 upperLeft;
	Vector2 lowerRight;
};

