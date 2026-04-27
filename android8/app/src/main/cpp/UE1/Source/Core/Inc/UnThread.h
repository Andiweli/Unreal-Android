/*=============================================================================
	UnThread.h: Threading API.
	Copyright 2024 fgsfds.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Threads.
-----------------------------------------------------------------------------*/

typedef void* UTHREAD;
typedef void* UMUTEX;

#ifdef PLATFORM_WIN32
typedef DWORD THREAD_RET;
typedef THREAD_RET ( __stdcall *THREAD_FUNC )( void* Arg );
#else
typedef void* THREAD_RET;
typedef THREAD_RET ( *THREAD_FUNC )( void* Arg );
#endif

// Thread operations.
CORE_API UTHREAD appThreadSpawn( THREAD_FUNC Func, void* Arg, const char* Name, UBOOL bDetach, DWORD* OutThreadId );
CORE_API THREAD_RET appThreadJoin( UTHREAD Thread );

// Recursive mutex operations.
CORE_API UMUTEX appMutexCreate( const char* Name );
CORE_API UBOOL appMutexLock( UMUTEX Mutex );
CORE_API UBOOL appMutexUnlock( UMUTEX Mutex );
CORE_API void appMutexFree( UMUTEX Mutex );

// Mutex object.
class CORE_API FMutex
{
public:
	FMutex( const char* InName ) : Name( InName )
	{
		Handle = appMutexCreate( InName );
		check(Handle);
	}

	~FMutex()
	{
		appMutexFree( Handle );
		Handle = nullptr;
	}

	void Lock() { check( appMutexLock( Handle ) ); }
	void Unlock() { check( appMutexUnlock( Handle ) ); }

private:
	UMUTEX Handle;
	const char* Name;
};

// Scoped lock, using FMutex.
class CORE_API FScopedLock
{
public:
	FScopedLock( FMutex& InMutex ) : Mutex( InMutex )
	{
		Mutex.Lock();
	}
	~FScopedLock()
	{
		Mutex.Unlock();
	}
private:
	FMutex& Mutex;
};
