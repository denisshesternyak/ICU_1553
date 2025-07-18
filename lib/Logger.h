#pragma once

#include <string>
#include <queue>
#include <fstream>

class CCriticalSection;

using namespace std;

class CLogger {

	public:
		CLogger();
		~CLogger();
#ifdef UNIT_TESTS
		static void unitTests();
#endif //UNIT_TESTS
		bool GetNextLogEntry(string &logEntry);
		void AddToLog(const char * szFormat, ...);
		void vAddToLog (const char * szFormat, va_list pData);
		void vAddToLogWithPrefix(const char * szFormat, char * szPrefix, va_list pData);
		void AddToLogWithPrefix(const char * szFormat, char * szPrefix, ...);
		void Clear();

		void Flush();

		void TurnOnLogToFile(string filename);
		void TurnOffLogToFile();

		void TurnOnInternalLog();
		void TurnOffInternalLog();

	private:
		queue<string> mLog;

		bool mIsReporting;

		CCriticalSection * mpCriticalSection;
		fstream mFile;

		bool mDoInternalLogging;
		bool mDoLogToFile;

};
