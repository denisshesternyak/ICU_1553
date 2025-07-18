#ifndef _PX_INCL_H
#define _PX_INCL_H

/*
This file is include file for the PX dll code, not for applications.
In application programs use proto_px instead of including each header file separately.
*/

#ifndef __EXC_DEF_H
#include "excdef_px.h"
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <stdlib.h>
#define Sleep(x) (usleep(x*1000))
#endif

#ifdef VXWORKSP
#include "excwindef.h"
#endif

#ifndef VMEVXI
#ifndef PXVISA
#ifndef VXWORKSP
#include "deviceio.h"
#endif
#endif
#endif

#ifdef VXWORKSP
#include "exc4000vxp.h"
#include <sysLib.h>
#endif

#include "irig.h"

#include "proto_px.h" /*  BCRUN.H MONSEQ.H AND RTRUN.H INCLUDED IN PROTO.H */

#ifndef MMSI
#ifndef THREAD_SAFE_PX     /* the non _px drivers are not thread safe! */
#include "proto_px_back.h" /* prototype for non _px drivers, for backward compatibility */
#endif
#endif

#include "instance_px.h"
#include "monlkup.h"


/* Definitions for old compilers */
#ifndef ULONG_PTR
#if defined(_WIN64)
    typedef unsigned __int64 ULONG_PTR, *PULONG_PTR;
#else
    typedef unsigned long ULONG_PTR, *PULONG_PTR;
#endif
typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
#endif

#endif
