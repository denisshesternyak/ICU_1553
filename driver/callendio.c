#include <stdio.h>
#include "excsysio.h"
#include <fcntl.h>
int main(int argc, char** argv){
	int iRet, iHandle, dev = 0;
	char szFullDevName[100];
	sprintf(szFullDevName, "/dev/excalbr%d", dev);
	iHandle = open (szFullDevName, O_RDWR);
	if(iHandle < 0){
		printf("Could not open %s\n", szFullDevName);
		return -1;
	}
	iRet = ioctl(iHandle, EXC_IOCTL_CALL_ENDIO);
	if(iRet < 0)
		printf("EXC_IOCTL_CALL_ENDIO ioctl failed\n");
	return 0;
}
