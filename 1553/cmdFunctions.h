#include <stdio.h>
#ifndef _WIN32
#include <stdarg.h>
#endif // _WIN32
//#include <stdarg.h>
//#include <atlstr.h>
//#include <cstring.h>
//#include <cstdarg>    // va_start, va_end, std::va_list
//#include <cstddef>    // std::size_t
////#include <stdexcept>  // std::runtime_error
//#include <vector>     // std::vector

void ReadString(char* value)
{		
	if (isCmd)
	{
		unsigned char rqfilename[150];
#ifdef _WIN32
		strcpy_s(rqfilename, 150, value);
#else
		strncpy(rqfilename, value,  150);
#endif // _WIN32
		printf(" %s", rqfilename);
		printf("\n");		
	}
	else
	{
		scanf(" %s",value);
	}
}

void ReadChar(char* value)
{		
	if (isCmd)
	{
		printf(" %c", *value);
		printf("\n");		
	}
	else
	{
		scanf(" %c",value);
	}
}
void ReadInt(int* value)
{
	if (isCmd)
	{
		printf("%d", *value);
		printf("\n");	
	}
	else
	{
		scanf("%d",value);
	}
}
void ReadUInt(unsigned int* value)
{
	if (isCmd)
	{
		printf("%u", *value);
		printf("\n");	
	}
	else
	{
		scanf("%u",value);
	}
}
void ReadULong(unsigned long* value)
{
	if (isCmd)
	{
		printf("%lu", *value);
		printf("\n");	
	}
	else
	{
		scanf("%lu",value);
	}
}
void ReadXULong(unsigned long* value)
{
	if (isCmd)
	{
		printf("%lx", *value);
		printf("\n");	
	}
	else
	{
		scanf("%lx",value);
	}
}
void ReadFloat(float* value)
{
	if (isCmd)
	{
		printf("%f", *value);
		printf("\n");	
	}
	else
	{
		scanf("%f",value);
	}
}
int ReadKey()
{	
	if (isCmd)
	{
		printf("\n");
		return 0;
	}
	else
	{	
#ifdef _WIN32
		return _getch();
#else
		return getchar();
#endif
	}
}




int monToInt(char mon[3])
{
	switch(mon[0])
	{
	case 'J':
	case 'j':
		switch(mon[2])
		{
		case 'N':
		case 'n':
			switch(mon[1])
			{
			case 'A':
			case 'a':
				return 1;					
			case 'U':
			case 'u':
				return 6;					
			}
			break;
		case 'L':
		case 'l':
			return 7;				
		}			

	case 'F':
	case 'f':
		return 2;			

	case 'M':
	case 'm':
		switch(mon[2])
		{
		case 'R':
		case 'r':
			return 3;					
		case 'Y':
		case 'y':
			return 5;					
		}
		break;

	case 'A':
	case 'a':
		switch(mon[1])
		{
		case 'P':
		case 'p':
			return 4;
		case 'U':
		case 'u':
			return 8;
		}
		break;

	case 'S':
	case 's':
		return 9;

	case 'O':
	case 'o':
		return 10;

	case 'N':
	case 'n':
		return 11;

	case 'D':
	case 'd':
		return 12;
	}

	return 0;
}

#include <time.h>
int isBigger(const char *date1, const char *date2)
{
	// This function is used to compare two IrigB timetags.
	// We want to know if the first argument is later/bigger than the second argument.
	// NOTE that this function returns 1 for OK, and 0 for error.
	
	int mon1, mon2;
	int day1, day2;
	int hour1, hour2;
	int minute1, minute2;
	int seconds1, seconds2;
	int microseconds1, microseconds2;

	int index1 = 0;
	int index2 = 0;

	char monArr1[4], monArr2[4];
	char dayArr1[3], dayArr2[3];
	char hourArr1[3], hourArr2[3];
	char minuteArr1[3], minuteArr2[3];
	char secondsArr1[3], secondsArr2[3];
	char microsecondsArr1[7], microsecondsArr2[7];
	memset(monArr1, '\0', 4);
	memset(monArr2, '\0', 4);
	memset(dayArr1, '\0', 3);
	memset(dayArr2, '\0', 3);
	memset(hourArr1, '\0', 3);
	memset(hourArr2, '\0', 3);
	memset(minuteArr1, '\0', 3);
	memset(minuteArr2, '\0', 3);
	memset(secondsArr1, '\0', 3);
	memset(secondsArr2, '\0', 3);
	memset(microsecondsArr1, '\0', 6);
	memset(microsecondsArr2, '\0', 6);

	//Parse///////////////////////////////////////////////////////////

	// --------------------------------------------------------
	// new Irig time (date1)
	dayArr1[0] = date1[index1];
	++index1;
	if (date1[index1] != ' ')
	{
		dayArr1[1] = date1[index1];	
		++index1;
	}

	++index1; // for space

	monArr1[0] = date1[index1];
	++index1;
	monArr1[1] = date1[index1];
	++index1;
	monArr1[2] = date1[index1];
	++index1;

	++index1; // for space

	hourArr1[0] = date1[index1];
	++index1;
	if (date1[index1] != ':')
	{
		hourArr1[1] = date1[index1];
		++index1;
	}

	++index1; // for :

	minuteArr1[0] = date1[index1];
	++index1;
	minuteArr1[1] = date1[index1];
	++index1;

	++index1; // for :

	secondsArr1[0] = date1[index1];
	++index1;
	secondsArr1[1] = date1[index1];
	++index1;

	++index1; // for .

	microsecondsArr1[0] = date1[index1];
	++index1;
	microsecondsArr1[1] = date1[index1];
	++index1;
	microsecondsArr1[2] = date1[index1];
	++index1;
	microsecondsArr1[3] = date1[index1];
	++index1;
	microsecondsArr1[4] = date1[index1];
	++index1;
	microsecondsArr1[5] = date1[index1];
	++index1;

	// --------------------------------------------------------

	// previous Irig time (date2)
	dayArr2[0] = date2[index2];
	++index2;
	if (date2[index2] != ' ')
	{
		dayArr2[1] = date2[index2];	
		++index2;
	}

	++index2; // for space

	monArr2[0] = date2[index2];
	++index2;
	monArr2[1] = date2[index2];
	++index2;
	monArr2[2] = date2[index2];
	++index2;

	++index2; // for space

	hourArr2[0] = date2[index2];
	++index2;
	if (date2[index2] != ':')
	{
		hourArr2[1] = date2[index2];
		++index2;
	}

	++index2; // for :

	minuteArr2[0] = date2[index2];
	++index2;
	minuteArr2[1] = date2[index2];
	++index2;

	++index2; // for :

	secondsArr2[0] = date2[index2];
	++index2;
	secondsArr2[1] = date2[index2];
	++index2;

	++index2; // for .

	microsecondsArr2[0] = date2[index2];
	++index2;
	microsecondsArr2[1] = date2[index2];
	++index2;
	microsecondsArr2[2] = date2[index2];
	++index2;
	microsecondsArr2[3] = date2[index2];
	++index2;
	microsecondsArr2[4] = date2[index2];
	++index2;
	microsecondsArr2[5] = date2[index2];
	++index2;


	//////////////////////////////////////////////////////////////////

	// check that month number is not decreasing
	mon1 = monToInt(monArr1);
	mon2 = monToInt(monArr2);

	if (mon1 < mon2)
	{
		return 0;
	}
	else if (mon1 == mon2)
	{
		// same month, continue checking the next time component
		// check that day number is not decreasing
		day1 = atoi(dayArr1);	
		day2 = atoi(dayArr2);	

		if (day1 < day2)
		{
			return 0;
		}
		else if (day1 == day2)
		{
			// same day, continue checking the next time component
			// check that hour number is not decreasing
			hour1 = atoi(hourArr1);	
			hour2 = atoi(hourArr2);	

			if (hour1 < hour2)
			{
				return 0;
			}
			else if (hour1 == hour2)
			{
				// same hour, continue checking the next time component
				// check that minute number is not decreasing
				minute1 = atoi(minuteArr1);	
				minute2 = atoi(minuteArr2);	

				if (minute1 < minute2)
				{
					return 0;
				}
				else if (minute1 == minute2)
				{
					// same minute, continue checking the next time component
					// check that seconds number is not decreasing
					seconds1 = atoi(secondsArr1);	
					seconds2 = atoi(secondsArr2);	

					if (seconds1 < seconds2)
					{
						return 0;
					}
					else if (seconds1 == seconds2)
					{
						// same second, continue checking the next time component
						// check that microseconds number is not decreasing
						microseconds1 = atoi(microsecondsArr1);	
						microseconds2 = atoi(microsecondsArr2);	

						if (microseconds1 < microseconds2)
						{
							return 0;
						}
					}
				}
			}
		}
	}
	// else -- some component, somewhere higher in the timetag, has increased, Irig timetag increased; return 1
	
	// if we got here, then the new Irig time is later (bigger) than the previous Irig time, success
	return 1;
}



# define MAX_BUFFER 256
#ifdef _WIN32
void Print(_Inout_ FILE * _File, _In_z_ _Printf_format_string_ const char * _Format, ...)
#else
void Print(FILE * _File, const char * _Format, ...)
#endif // _WIN32
{
	char buffer[MAX_BUFFER];
	va_list args;
	va_start (args, _Format);
	vsnprintf (buffer, MAX_BUFFER, _Format, args);	

	if (isCmd)	
		printf(buffer);	
	else
		fprintf(_File, buffer);

	va_end (args);
}
