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

#include "ThreadID.h"

struct SDL_mutex;

enum ThreadEvents
{
	TE_MATCH_PAUSE = 1,	// boolean: pause/unpause
	TE_GAME_EVENT  = 2	// long game events
};


class BlobbyThread;


struct ThreadSentEvent
{
	// addressing
	ThreadID sender;		// auto
	ThreadID recipent;		// auto

	// message id
	unsigned long message;		// required
	
	// additional data
	/// \todo boost.any?
	union						// optional
	{
		bool		  boolean;
		unsigned long integer;
		double		  number;
		void*		  pointer;	
	};
};

/// \todo we should use the global thread management lock here instead of a new one.
class ThreadEventManager : public boost::noncopyable
{
	public:
		ThreadEventManager(const BlobbyThread* t);
		// change this to make it only possible to delete TEM when thread is destroyed
		~ThreadEventManager();
		
		bool hasEvents() const;
		ThreadSentEvent popEvent();
		void send(ThreadSentEvent event, const ThreadID& target) const;
		
		// 2 methods for callbacks: 
		//	direct -> other thread "injects" function into this 
		//		thread; function is executed whenever other thread receives a message of a
		//  	certain id
		//	defered: technically the same as above, this function only gathers all relevant 
		//			information and writes it to a public location of the other thread
		//			TODO: how do we prevent subsequent calls not to overwrite old date 
		//					until it was processed
		//			TODO: how could we use signals/conditions? they seem to be what we need, at least
		//					in some regards.
		///	\todo make first parameter a reference
		typedef int(*callbackF)(BlobbyThread*, const ThreadSentEvent& event);
		void addCallback(callbackF function, unsigned long message_type, const ThreadID& target);
		
		static void sendEventFromCallingThread(ThreadSentEvent ev, const ThreadID& target);
		
		static const ThreadEventManager& getCurrentEventManager();
		
	private:
		void pushEvent(ThreadSentEvent ev);
		
		const BlobbyThread* mThread;
		std::deque<ThreadSentEvent> mQueue;
		
		// here we safe our callbacks
		// maybe a hash_map would be better
		// pair: thread - message; function
		std::map<std::pair<ThreadID, unsigned long>, callbackF > callbacks;
		static SDL_mutex* lock;
		static std::map<const BlobbyThread*, ThreadEventManager*> event_threads;
};
