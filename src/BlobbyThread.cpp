#include "BlobbyThread.h"
#include "SpeedController.h"
#include "ThreadSentEvent.h"
#include "SDL/SDL_thread.h"
#include <cassert>
#include <iostream>

BlobbyThread::~BlobbyThread()
{
	// free the lock
	SDL_DestroyMutex(mLock);

	delete scon;
	delete event_mgr;
	
	/// \todo allow graceful exit! 
	SDL_KillThread(mThread);
}

void BlobbyThread::createBlobbyThread(thread_func thread, void* data)
{
	started = false;
	
	// condition for supervising initialization of thread
	SDL_cond* init  = SDL_CreateCond();
	reinterpret_cast<ThreadParameters<void>*>(data)->init_cond = init;
	
	// create our mutex
	mLock = SDL_CreateMutex();
	
	// the thread function creates the speed and event manager
	mThread = SDL_CreateThread(thread, data);
	
	/// \todo we don't mutex the started variable but we have to lock mLock as CondWait requires a
	/// locked mutex
	SDL_LockMutex(mLock);
	
	while(!started)
	{
		if(SDL_CondWait(init, mLock))
		{
			/// \todo error!
		}
	}
	
	// we unlock the mutex
	SDL_UnlockMutex(mLock);
	
	// after this condition, scon and EventManager have to be initialised
	
	std::cout << "THREAD CREATED\n";
}

void BlobbyThread::initThread(int speed)
{
	std::cout << "create speed controller";
	// create SpeedController
	scon = new SpeedController(speed, SDL_ThreadID());
	// create ThreadEventManager
	event_mgr = new ThreadEventManager(SDL_GetThreadID(mThread));
}

void BlobbyThread::signalInitFinished(SDL_cond* cond)
{
	started = true;
	SDL_CondSignal(cond);
}

ThreadEventManager& BlobbyThread::getEventManager()
{
	assert(event_mgr);
	return *event_mgr;
}

unsigned int BlobbyThread::getID() const
{
	return SDL_GetThreadID(mThread);
}

void BlobbyThread::lock() const
{
	SDL_LockMutex(mLock);	
}

void BlobbyThread::unlock() const
{
	SDL_UnlockMutex(mLock);
}

void BlobbyThread::sendEvent(ThreadSentEvent ev, unsigned long target)
{
	event_mgr->send(ev, target);
}
