#include "common.h"
#include "profile.h"
#include <time.h>


struct SProfileGlobals
{
	uint 	frameCount;
	double 	start[PROF_COUNT];
	double 	total[PROF_COUNT];
	uint 	hits[PROF_COUNT];
};


SProfileGlobals s_profGlob;


const char *s_profNames[PROF_COUNT] =
{
	"Frame            ", 		// PROF_FRAME
	" Cmd             ",      	// PROF_CMD
	" Keyboard        ", 		// PROF_KEYBOARD
	" VNCWidget       ", 		// PROF_VNC_WIDGET
	" Scene           ",      	// PROF_SCENE
	" Draw            ",      	// PROF_DRAW
	"  Draw Eye       ",      	// PROF_DRAW_EYE
	"   Draw Keyboard ",   		// PROF_DRAW_KEYBOARD
	"   Draw VNCWidget",  		// PROF_DRAW_VNC_WIDGET
	"VNCThread        ", 		// PROF_VNC_THREAD
	" Input           ", 		// PROF_VNC_THREAD_INPUT
	"  Wait           ", 		// PROF_VNC_THREAD_WAIT
	"  Handle         ", 		// PROF_VNC_THREAD_HANDLE
	" Output          ", 		// PROF_VNC_THREAD_OUTPUT
};


static double Prof_MS() 
{
    struct timespec res;
    clock_gettime( CLOCK_REALTIME, &res );
    return 1000.0 * res.tv_sec + (double)res.tv_nsec / 1e6;
}


void Prof_Start( EProfType prof )
{
	s_profGlob.start[prof] = Prof_MS();
	s_profGlob.hits[prof]++;
}


void Prof_Stop( EProfType prof )
{
	s_profGlob.total[prof] += Prof_MS() - s_profGlob.start[prof];
}


static void Prof_Normalize()
{
	uint profIter;

	for ( profIter = 0; profIter < PROF_COUNT; profIter++ )
	{
		s_profGlob.total[profIter] /= s_profGlob.frameCount;
		s_profGlob.hits[profIter] /= s_profGlob.frameCount;
	}
}


static void Prof_Print()
{
	uint profIter;

	for ( profIter = 0; profIter < PROF_COUNT; profIter++ )
	{
		LOG( "%s : [%d] %f", s_profNames[profIter], s_profGlob.hits[profIter], s_profGlob.total[profIter] );
		s_profGlob.total[profIter] = 0.0;
		s_profGlob.hits[profIter] = 0;
	}
}


void Prof_Frame()
{
	s_profGlob.frameCount++;
	if ( s_profGlob.frameCount == 120 )
	{
		Prof_Normalize();
		Prof_Print();
		s_profGlob.frameCount = 0;
	}
}
