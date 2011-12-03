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

#include "ThreadSentEvent.h"
#include <cassert>
#include <SDL/SDL_mutex.h>
#include <SDL/SDL_thread.h>
#include "BlobbyThread.h"
#include <iostream>

/// is the order of initialisation determined here?
SDL_mutex* ThreadEventManager::lock = 0;
std::map<const BlobbyThread*, ThreadEventManager*> ThreadEventManager::event_threads;

ThreadEventManager::ThreadEventManager(const BlobbyThread* t) : mThread(t)
{
	if(!lock)
		lock  = SDL_CreateMutex();
	
	SDL_LockMutex(lock);
	
		std::cout << "create " << mThread << "\n";

	assert( event_threads.find(mThread) == event_threads.end() );
	event_threads[mThread] = this;

	SDL_UnlockMutex(lock);
}

ThreadEventManager::~ThreadEventManager()
{
	std::cout << "delete " << mThread << "\n";
	event_threads.erase(event_threads.find(mThread));
}

bool ThreadEventManager::hasEvents() const
{
	/// \todo locks?
	return !mQueue.empty();
}

ThreadSentEvent ThreadEventManager::popEvent()
{
	SDL_LockMutex(lock);
	
	ThreadSentEvent event = mQueue.front();
	mQueue.pop_front();
	
	SDL_UnlockMutex(lock);
	
	return event;
}

void ThreadEventManager::pushEvent(ThreadSentEvent ev)
{
	SDL_LockMutex(lock);
	
	// check that we send to the right thread
	assert(ev.recipent == mThread);
	mQueue.push_back(ev);
	
	SDL_UnlockMutex(lock);
}

void ThreadEventManager::send(ThreadSentEvent event, const ThreadID& target) const
{
	/// \todo we need another way of checking if some thread is currently active!
	// don't send from another thread as this thread
	assert(SDL_ThreadID() == mThread->getID());
	
	SDL_LockMutex(lock);
	
	/// if target is NULL, we want to send to main thread?
	/// \todo wouldn't NULL be good for broadcast?
	/// this will be changed again!
	//if(target == 0)
	//	target = mainThreadEventManager.mThread;
	
	assert(event_threads.find(target.getThread()) != event_threads.end());
	event.recipent = target;
	event.sender = mThread;
	event_threads[target.getThread()]->pushEvent(event);
	
	// now, check if we have to perform a callback
	// still mutex'd, so other thread won't be able to retrieve 
	// this event until the callback was finished

	for( std::map<std::pair<ThreadID, unsigned long>, callbackF>::const_iterator it = callbacks.begin();
			it != callbacks.end(); ++it)
	{
		if(it->first.first == target.getThread() && it->first.second == event.message)
		{
			/// \todo oops, we don't know about the Blobby Thread here :(
			(*it->second)(0, event);
		}		
	}

	SDL_UnlockMutex(lock);
}

void ThreadEventManager::addCallback(callbackF function, unsigned long message_type, const ThreadID& thread)
{
	SDL_LockMutex(lock);
	callbacks[std::make_pair(thread, message_type)] = function;
	SDL_UnlockMutex(lock);
}

void ThreadEventManager::sendEventFromCallingThread(ThreadSentEvent ev, const ThreadID& target)
{
	// let the caller's TEM send the event!
	getCurrentEventManager().send(ev, target);
}


const ThreadEventManager& ThreadEventManager::getCurrentEventManager()
{
	ThreadID caller = SDL_ThreadID();
	/// \todo use auto
	/// \todo we have to find out which blobby thread is associated with caller
	std::map<const BlobbyThread*, ThreadEventManager*>::iterator mgr = event_threads.find(caller.getThread());
	
	// no thread is allowed to have no thread event manager!
	assert( mgr != event_threads.end());
	
	return *mgr->second;
}