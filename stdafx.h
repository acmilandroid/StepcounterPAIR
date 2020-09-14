// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// memory leak finder
#define _CRTDBG_MAP_ALLOC

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>

// memory leak finder
#include <crtdbg.h>

#include <malloc.h>
#include <memory.h>
#include <string>
#include <CommDlg.h>
#include <tchar.h>
#include <stdio.h>

const int MAX_PATH_LEN = 512;
const int NUM_SENSORS = 3;
const int NUM_AXES = 3;
const int SAMPLE_RATE = 15;
// set max data = sample rate(hz) * 60 (seconds in min) * 60 (minutes in hour) * num_max_hours
const int MAX_DATA = SAMPLE_RATE*60*60*2;
const int MAX_STEPS = 5000;
const long MAX_RANGE = 100000;			/* maximum data range */
const long MIN_RANGE = -100000;
const int SMALL_LEAP = 1;				/* small forward: 1/15 second */
const int BIG_LEAP = 15;				/* large forward: 1 second */
const int TIMER_SECOND = 1;				/* ID of timer used to notify app every 1 second for playback */
// TODO: reference additional headers your program requires here
