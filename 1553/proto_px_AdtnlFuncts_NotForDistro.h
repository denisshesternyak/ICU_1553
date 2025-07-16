// These functions are for Excalibur development and/or testing,
// and are not to be included in the Software Tools distribution.

#ifndef __Px_PROTO_NODISTRO_H
#define __Px_PROTO_NODISTRO_H


/* function prototypes */
#ifdef __cplusplus
extern "C" {
#endif

/* GGET.C */
int borland_dll Get_UseUMMRIfAvailable_Px(int handlepx, int * pUseUMMRFlag);

/* GSET.C */
int borland_dll Set_UseUMMRIfAvailable_Px(int handlepx, int useUMMRFlag);

#ifdef __cplusplus
}
#endif

#endif
