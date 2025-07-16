/*
   Galahad.h is a .h file which includes every other .h file.
   This way, programmers do not have to worry about which .h files
   they need or the specific order these .h's must be listed.
*/

#ifndef __GALAHAD_PX_H
#define __GALAHAD_PX_H

#include "excdef_px.h"
#ifdef __linux__
//#include "exclinuxdef.h"
#include "WinTypes.h"
#include <unistd.h>
#define Sleep(x) (usleep(x*1000))
#endif
#include "proto_px.h" 


#endif

