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

#include "ThreadID.h"
#include "BlobbyThread.h"

ThreadID::GTI ThreadID::t_all;
ThreadID::GTI ThreadID::t_none;
ThreadID::GTI ThreadID::t_current;


ThreadID::ThreadID( const GTI& )
{
	
}
ThreadID::ThreadID(const BlobbyThread* t) : mThread(t)
{
	
}
ThreadID::ThreadID(unsigned int id) : mThread( BlobbyThread::getThread(id)) 
{
	
}
	
const BlobbyThread* ThreadID::getThread() const
{
	return mThread;
}

unsigned int ThreadID::getThreadID() const
{
	return mThread->getID();
}

bool ThreadID::operator==(const ThreadID& other) const
{
	/// \todo if we create generic ThreadIDs, we must define how they are compared.
	///		do we compare for exact equality or is == more a "addresses also" relation?
	mThread == other.mThread;
}

bool ThreadID::operator<(const ThreadID& other) const
{
	return mThread < other.mThread;
}
