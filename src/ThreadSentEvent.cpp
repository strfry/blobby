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
#include <iostream>

SDL_mutex* ThreadEventManager::lock = 0;
std::map<unsigned long, ThreadEventManager*> ThreadEventManager::event_threads;

ThreadEventManager::ThreadEventManager(unsigned int t) : mThread(t)
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

void ThreadEventManager::send(ThreadSentEvent event, unsigned long target) const
{
	// don't send from another thread as this thread
	assert(SDL_ThreadID() == mThread);
	
	SDL_LockMutex(lock);
	
	assert(event_threads.find(target) != event_threads.end());
	event.recipent = target;
	event.sender = SDL_ThreadID();
	event_threads[target]->pushEvent(event);

	SDL_UnlockMutex(lock);
}

void ThreadEventManager::sendEventFromCallingThread(ThreadSentEvent ev, unsigned long target)
{
	// let the caller's TEM send the event!
	getCurrentEventManager().send(ev, target);
}


const ThreadEventManager& ThreadEventManager::getCurrentEventManager()
{
	unsigned long caller = SDL_ThreadID();
	/// \todo use auto
	std::map<unsigned long, ThreadEventManager*>::iterator mgr = event_threads.find(caller);
	
	// no thread is allowed to have no thread event manager!
	assert( mgr != event_threads.end());
	
	return *mgr->second;
}
