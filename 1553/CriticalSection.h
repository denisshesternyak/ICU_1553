#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#define CRITICAL_SECTION pthread_mutex_t
#define DeleteCriticalSection pthread_mutex_destroy
#define EnterCriticalSection pthread_mutex_lock
#define LeaveCriticalSection pthread_mutex_unlock
#endif
// #include <tchar.h>


///////////////////////////////////
// CRITICAL SECTION SYNCHRONIZATION
///////////////////////////////////

class CCriticalSectionSynchronization
{
public:
	//
	// Ctor & Dtor
	//
	inline CCriticalSectionSynchronization();
	inline ~CCriticalSectionSynchronization();

	//
	// Operators
	//
	inline void Lock();
	inline void Unlock();

private:

	// Data Memeber
	CRITICAL_SECTION crtiticalSection;

	// disallow copying and assignment
// YYB-Check:	CriticalSectionSynchronization( const CriticalSectionSynchronization & );
// YYB-Check:	CriticalSectionSynchronization& operator= ( const CriticalSectionSynchronization & );

}; // of class CriticalSectionSynchronization

inline
CCriticalSectionSynchronization::CCriticalSectionSynchronization()
{
#ifdef _WIN32
	InitializeCriticalSection( &crtiticalSection);
#else
	pthread_mutex_init (&crtiticalSection, NULL);
#endif
}

inline
CCriticalSectionSynchronization::~CCriticalSectionSynchronization()
{
	DeleteCriticalSection( &crtiticalSection);
}

inline
void
CCriticalSectionSynchronization::Lock()
{
	EnterCriticalSection( &crtiticalSection);
}

inline
void
CCriticalSectionSynchronization::Unlock()
{
	LeaveCriticalSection( &crtiticalSection);
}

/* Skip errors for now

//////////////////////////
// WINDOWS ERROR MESSAGES
//////////////////////////

class WindowsErrorMessages  // as i got it, the class was named WindowsErrorMessages()
{
public:
	static string getString(DWORD erroCode);

};


string 
WindowsErrorMessages::getString(DWORD systemErrorCode)
{

	// get the system error message     
	LPTSTR s;

	if(::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		systemErrorCode,
		0,
		(LPTSTR)&s,
		0,
		NULL) == 0)	{
		
		// the FormatMessage function itself failed
		ostringstream errorMessageString;
		errorMessageString << "Unknown Windows system error code [" << systemErrorCode << "]";
		
		return( errorMessageString.str() );
		
	}
	else{
		// FormatMessage function succeeded
		
		// so, clear from the string CTRF 
		LPTSTR p = _tcschr(s, _T('\r'));
		if(p != NULL){
			*p = _T('\0');
		}		
		
		// save the formated error string
		string errorString = s;
		
		// clearing the memory allocated by FormatMessage function
		::LocalFree(s);
		
		return( errorString );
		
	}// of else FormatMessage succeeded

} // of function getString

*/
