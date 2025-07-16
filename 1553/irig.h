#ifndef _IRIG_H
#define _IRIG_H

#pragma once


typedef struct t_IrigOnModuleTime {
	unsigned short int days;
	unsigned short int hours;
	unsigned short int minutes;
	unsigned short int seconds;
	unsigned int microsecs;
} t_IrigOnModuleTime;

#endif
