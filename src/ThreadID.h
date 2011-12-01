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

class BlobbyThread;

/*! \class ThreadID
	\brief identifying a thread
	\details This class can identify a thread, either with ID or with BlobbyThread pointer.
			Additionally, more generic IDs are possible (such as all, none, current etc)
	\todo check on construction time if thread really exists. turn this class into a kind of 
			safe pointer to prevent access on deleted threads.
*/
class ThreadID
{
	struct GTI {};
	public:
		static GTI t_all;
		static GTI t_none;
		static GTI t_current;
	
	/// \todo implement this!
	explicit ThreadID( const GTI& = t_none );
	/// construct from BlobbyThread
	ThreadID(const BlobbyThread* t);
	
	/// construct from ID
	ThreadID(unsigned int);
	
	// get ID representations
	
	const BlobbyThread* getThread() const;
	unsigned int getThreadID() const;
	
	bool operator==(const ThreadID& other) const;
	/// \todo this operation does not make sense, but it is needed to 
	///			use this as map keys!
	bool operator<(const ThreadID& other) const;
	
	private:
		const BlobbyThread* mThread;
};

