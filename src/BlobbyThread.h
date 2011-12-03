#ifndef BLOBBYTHREAD_H_INCLUDED
#define BLOBBYTHREAD_H_INCLUDED

#include "SpeedController.h"
#include <map>
#include <iostream>

typedef int(*thread_func)(void*);

struct SDL_Thread;
struct SDL_mutex;
struct SDL_cond;
class SpeedController;
class ThreadEventManager;
class BlobbyThread;
struct ThreadSentEvent;

template<class T>
struct ThreadRunParams
{
	T* in;
	BlobbyThread* thread;	
};

/*! 	\class BlobbyThread
		\brief represents a thread
		\detailed
			This class manages an SDL_Thread together with communication with other threads,
				regulating speed, passing parameters to threads, initialisation and shutdown.
*/
class BlobbyThread
{
	private:
		template<class T>
		struct TS
		{
			static int GenericThreadFunction(void* data);
		};
		
		template<class T>
		struct ThreadParameters
		{
			public:
			typedef int (*init_func)(ThreadRunParams<T>);
			typedef int (*main_func)(ThreadRunParams<T>);
			
			T* in;
			int speed;	// ?
			BlobbyThread* thread;
			init_func init;
			main_func loop;
			SDL_cond* init_cond;
		};
	
	public:
		template<class T>
		BlobbyThread(	typename BlobbyThread::ThreadParameters<T>::init_func init, 
						typename BlobbyThread::ThreadParameters<T>::main_func mfunc, 
						T* data, int speed):
						// NULL all pointers for easier debugging/safety issues
						mThread(0), mLock(0), scon(0), event_mgr(0), started(false)
						
		{
			ThreadParameters<T> params;
			params.thread = this;
			params.init = init;
			params.loop = mfunc;
			params.speed = speed;
			params.in = data;
			
			createBlobbyThread(&BlobbyThread::TS<T>::GenericThreadFunction, &params);
		}
		
		ThreadEventManager& getEventManager();
		unsigned int getID() const;
		
		void sendEvent(ThreadSentEvent ev, const BlobbyThread* target);
		
		static const BlobbyThread* getThread(unsigned int);
		static const BlobbyThread* getMainThread() { return blobbyMainThread; };
		static bool initThreading();
	protected:
		void lock() const;
		void unlock() const;
		
		~BlobbyThread();
	
	private:
	
		// this creation function is needed only once
		// for creating a blobby thread without starting
		// a new thread. we need this to create a thread 
		// manager for the main thread.
		BlobbyThread(unsigned int ID);
	
		void createBlobbyThread(thread_func tf, void* data);
		void initThread(int speed);
		void signalInitFinished(SDL_cond* cond);
		
		SDL_Thread* mThread;
		mutable SDL_mutex*  mLock;
		SpeedController* scon;
		ThreadEventManager* event_mgr;
		
		bool started;
		
		static SDL_mutex* globalThreadManagementLock;
+		static BlobbyThread* blobbyMainThread;
		
		static std::map<unsigned int, BlobbyThread*> ID_thread_map;
};

template<class T>
int BlobbyThread::TS<T>::GenericThreadFunction(void* data)
{
	// creation of thread
	/// make a copy of the data passed!
	/// we copy speed, thread, and the two functions.
	/// da data \p in points to is not! copied!
	
	// this action is critical. we must prevent to return to the main thread here 
	// to ensure data is not deleted before we made our copy.
	ThreadParameters<T> params = *reinterpret_cast<ThreadParameters<T>* >(data);
	
	// basic thread initialization: create SpeedController and ThreadEventManager
	params.thread->initThread(params.speed);
	
	// only execute if there is an init function
	if(params.init)
	{
		int stat = (*params.init)( {params.in, params.thread} );
		
		// failure on init
		if(stat != 0)
			return stat;
	}
	
	// from here, it is no more safe to use params.init_cond;
	params.thread->signalInitFinished(params.init_cond);
	params.init_cond = 0;
	
	// now the main thread can leave the BlobbyThread::BlobbyThread()
	
	// now the thread main loop
	if(params.loop)
	{
		while(true)
		{
			// do thread loop
			// lock thread so while we are in thread main, thread cannot be
			// changed externally
			params.thread->lock();
			(*params.loop)( {params.in, params.thread} );
			params.thread->unlock();
			
			// update speedcontroller
			params.thread->scon->wait();
		}
	}
};


#endif // BLOBBYTHREAD_H_INCLUDED
