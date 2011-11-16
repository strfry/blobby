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

#include <deque>
#include <map>
#include <boost/noncopyable.hpp>

struct SDL_mutex;

enum ThreadEvents
{
	TE_MATCH_PAUSE = 1	// boolean: pause/unpause
};


struct ThreadSentEvent
{
	// addressing
	unsigned long sender;
	unsigned long recipent;

	// message id
	unsigned long message;
	
	// additional data
	/// \todo boost.any?
	union
	{
		bool		  boolean;
		unsigned long integer;
		double		  number;
		void*		  pointer;	
	};
};


class ThreadEventManager : public boost::noncopyable
{
	/// \todo SDL_Thread* mit abspeichern und verwalten?
	public:
		ThreadEventManager(unsigned int t);
		// change this to make it only possible to delete TEM when thread is destroyed
		~ThreadEventManager();
		
		bool hasEvents() const;
		ThreadSentEvent popEvent();
		void send(ThreadSentEvent event, unsigned long target) const;
		static void sendEventFromCallingThread(ThreadSentEvent ev, unsigned long target);
		
		static const ThreadEventManager& getCurrentEventManager();
		
	private:
		void pushEvent(ThreadSentEvent ev);
		
		unsigned long mThread;
		std::deque<ThreadSentEvent> mQueue;
		static SDL_mutex* lock;
		static std::map<unsigned long, ThreadEventManager*> event_threads;
};

