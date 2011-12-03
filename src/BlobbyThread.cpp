#include "BlobbyThread.h"
#include "SpeedController.h"
#include "ThreadSentEvent.h"
#include "SDL/SDL_thread.h"
#include <cassert>
#include <iostream>

SDL_mutex* 		BlobbyThread::globalThreadManagementLock 	= 0;
BlobbyThread* 	BlobbyThread::blobbyMainThread 				= 0;
std::map<unsigned int, BlobbyThread*> BlobbyThread::ID_thread_map;


bool BlobbyThread::initThreading()
{
	// create mutex for general threading management operations
	globalThreadManagementLock = SDL_CreateMutex();

	// create standard thread
	// we don't need to keep this pointer as it is also in 
	// the static variable blobbyMainThread
	BlobbyThread* main = new BlobbyThread(SDL_ThreadID());
}

BlobbyThread::BlobbyThread(unsigned int ID) : mThread(0), scon(0), started(true)
{
	// check that we don't have a main thread!
	assert(blobbyMainThread == 0);
	/// \todo with our current architecture, it is not safe
	///		to call getID for the main thread as mThread is 0!
	// create our mutex
	mLock = SDL_CreateMutex();
	
	event_mgr = new ThreadEventManager(this);
	
	// register this thread as main thread
	blobbyMainThread = this;
	
	// register as ormale thread too
	ID_thread_map[SDL_ThreadID()] = this;
}

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
	
	// now the thread is fully usable, we can register it
	ID_thread_map[SDL_GetThreadID(mThread)] = this;
	
	// rnow the thread is fully usable, we can register it
	ID_thread_map[SDL_GetThreadID(mThread)] = this;
	
	std::cout << "THREAD CREATED\n";
}

void BlobbyThread::initThread(int speed)
{
	std::cout << "create speed controller";
	// create SpeedController
	scon = new SpeedController(speed, SDL_ThreadID());
	// create ThreadEventManager
	event_mgr = new ThreadEventManager(this);
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

void BlobbyThread::sendEvent(ThreadSentEvent ev, const ThreadID& target)
{
	event_mgr->send(ev, target);
}

const BlobbyThread* BlobbyThread::getThread(unsigned int id)
{
	/// \todo add checks!
		std::cout << ID_thread_map.begin()->first << " " << id  << "\n";
	assert(ID_thread_map.find(id) != ID_thread_map.end() );
	return ID_thread_map[id];
}
