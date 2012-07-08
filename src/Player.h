/*=============================================================================
Blobby Volley 2
Copyright (C) 2008 Jonathan Sieber (jonathan_sieber@yahoo.de)

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

#include <string>

#include <boost/scoped_ptr.hpp>

#include "Global.h"
#include "BlobbyDebug.h"

class InputSource;

class Player : public ObjectCounter<Player>
{
public:
	Player(PlayerSide side);
	~Player();

	void loadFromConfig(const std::string& prefix, bool initInput = true);

	InputSource* getInputSource() const;
	std::string getName() const;
	Color getColor() const;
	
	void setColor(Color ncol);
	void setName(const std::string& name);
private:

	bool mInitialised;
	const PlayerSide mPlayerSide;
	boost::scoped_ptr<InputSource> mInputSource;
	std::string mName;
	
	Color mStaticColor;
	bool mOscillating;
};

