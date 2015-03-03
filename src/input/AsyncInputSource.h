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

#pragma once

#include "InputSource.h"
#include <mutex>

/*! \class AsyncInputSource
	\brief Async updated input source.
	\details the setInput and getNextInput functions of this InputSource
			are mutex protected, so the input can safely be updated from a
			thread other than the match thread!
*/
class AsyncInputSource : public InputSource
{
	public:
		AsyncInputSource();
		~AsyncInputSource();

		// override setInput to be mutex protected.
		virtual void setInput(PlayerInputAbs ip) override;

		virtual PlayerInputAbs getNextInput() override;

	private:
		std::mutex mUpdateMutex;

		// data of other thread that is copied to actual input
		// synchronized
		PlayerInputAbs mNextInput;
};

