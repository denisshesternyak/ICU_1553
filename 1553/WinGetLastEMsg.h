#pragma once

#include <string>
using namespace std;

#include <iostream>
using std::cerr;
using std::cout;
using std::endl;

#include <sstream>
using std::ostringstream;

// only needed for unicode
#ifdef _WIN32
#include <AtlBase.h>
#endif
//#include <AtlConv.h>

string GetWindowsLastErrorMessage();

//----------------------------------------------------
// WindowsSocketsErrorDescription
//	An interface class that retrives the last error 
//	that occured when using one of the windows scokets 
//	library functions
//
// Note: After calling one of the functions of setLastError
//	 the error description is in the public lastError 
//	 variable
//----------------------------------------------------
class WindowsSocketsErrorDescription
{
public:

	// Holds the last error description
	string lastError;

	void setLastError(const char * errorPrefix);
	void setLastError(const string& errorPrefix);

};// of class WindowsSocketsErrorDescription
