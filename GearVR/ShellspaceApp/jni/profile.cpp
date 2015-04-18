#include "common.h"
#include "profile.h"
#include <time.h>


#define PROF_FRAME_COUNT		120
#define PROF_PROBE_HIT_COUNT	(PROF_FRAME_COUNT * 10)


struct SProfileGlobals
{
	uint 	frameCount;
	
	double 	start[PROF_COUNT];
	double 	min[PROF_COUNT];
	double 	max[PROF_COUNT];
	double 	avgPerCall[PROF_COUNT];
	double 	avgPerFrame[PROF_COUNT];
	double 	total[PROF_COUNT];
	uint 	hits[PROF_COUNT];

	uint 	probe;
	uint 	probeHitCount;
	uint 	probeHitFrame[PROF_PROBE_HIT_COUNT];
	double 	probeHitTime[PROF_PROBE_HIT_COUNT];
};


SProfileGlobals s_profGlob;


const char *s_profNames[PROF_COUNT] =
{
	"Frame", 					// PROF_FRAME
	" Cmd",      				// PROF_CMD
	" Keyboard", 				// PROF_KEYBOARD
	" VNCWidget", 				// PROF_VNC_WIDGET
	"  InQueue",                // PROF_VNC_WIDGET_INQUEUE
	"   Advance",               // PROF_VNC_WIDGET_ADVANCE
	"   Update",                // PROF_VNC_WIDGET_UPDATE
	"  UpdateTextureRect",      // PROF_VNC_WIDGET_UPDATE_TEXTURE_RECT
	" Scene",      				// PROF_SCENE
	" Draw",      				// PROF_DRAW
	"  Draw Eye",      			// PROF_DRAW_EYE
	"   Draw Keyboard ",   		// PROF_DRAW_KEYBOARD
	"   Draw VNCWidget",  		// PROF_DRAW_VNC_WIDGET
	"VNCThread", 				// PROF_VNC_THREAD
	" Input", 					// PROF_VNC_THREAD_INPUT
	"  Wait", 					// PROF_VNC_THREAD_WAIT
	"  Handle", 				// PROF_VNC_THREAD_HANDLE
	"   Resize", 				// PROF_VNC_THREAD_HANDLE_RESIZE
	"   Update", 				// PROF_VNC_THREAD_HANDLE_UPDATE
	"   Cursor Shape", 			// PROF_VNC_THREAD_HANDLE_CURSOR_SHAPE
	"   Cursor Pos", 			// PROF_VNC_THREAD_HANDLE_CURSOR_POS
	"  Lock InQueue", 			// PROF_VNC_THREAD_LOCK_IN_QUEUE
	" Output", 					// PROF_VNC_THREAD_OUTPUT
};


double Prof_MS() 
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
	double ms = Prof_MS() - s_profGlob.start[prof];

	s_profGlob.total[prof] += ms;
	if ( ms > s_profGlob.max[prof] )
		s_profGlob.max[prof] = ms;
	if ( ms < s_profGlob.min[prof] )
		s_profGlob.min[prof] = ms;

	if ( prof == s_profGlob.probe && s_profGlob.probeHitCount < PROF_PROBE_HIT_COUNT )
	{
		s_profGlob.probeHitFrame[s_profGlob.probeHitCount] = s_profGlob.frameCount;
		s_profGlob.probeHitTime[s_profGlob.probeHitCount] = ms;
		s_profGlob.probeHitCount++;
	}
}


static void Prof_Normalize()
{
	uint profIter;

	for ( profIter = 0; profIter < PROF_COUNT; profIter++ )
	{
		if ( s_profGlob.hits[profIter] )
			s_profGlob.avgPerCall[profIter] = s_profGlob.total[profIter] / s_profGlob.hits[profIter];
		else
			s_profGlob.avgPerCall[profIter] = 0.0;

		s_profGlob.avgPerFrame[profIter] = s_profGlob.total[profIter] / s_profGlob.frameCount;

		if ( s_profGlob.min[profIter] == FLT_MAX )
			s_profGlob.min[profIter] = 0.0f;
	}
}


static const char *s_dashes = "-----------------------------------------------------------";


static void Prof_Print()
{
	uint 	profIter;
	uint 	hitIter;
	uint 	hitFrame;
	double	frameAvg;
	double	frameTotal;
	double	frameMin;
	double	frameMax;
	uint 	frameCount;

	LOG( "%-30s   %4s %8s %8s %8s %8s %8s", "name", "hits", "avg/c", "avg/f", "total", "min", "max" );
	LOG( "%.30s   %.4s %.8s %.8s %.8s %.8s %.8s", s_dashes, s_dashes, s_dashes, s_dashes, s_dashes, s_dashes, s_dashes );

	for ( profIter = 0; profIter < PROF_COUNT; profIter++ )
	{
		LOG( "%-30s : %4d %8.2f %8.2f %8.2f %8.2f %8.2f", 
			s_profNames[profIter], 
			s_profGlob.hits[profIter], 
			s_profGlob.avgPerCall[profIter],
			s_profGlob.avgPerFrame[profIter],
			s_profGlob.total[profIter],
			s_profGlob.min[profIter],
			s_profGlob.max[profIter] );

		s_profGlob.total[profIter] = 0.0;
		s_profGlob.hits[profIter] = 0;
		s_profGlob.max[profIter] = 0;
		s_profGlob.min[profIter] = FLT_MAX;
	}

	LOG( "" );

	if ( s_profGlob.probe < PROF_COUNT )
	{
		LOG( "Probe \"%s\":", s_profNames[s_profGlob.probe] );

		hitFrame = UINT_MAX;

		for ( hitIter = 0; hitIter < s_profGlob.probeHitCount; hitIter++ )
		{
			if ( s_profGlob.probeHitFrame[hitIter] != hitFrame )
			{
				if ( hitIter != 0 )
					LOG( "Min: %f Max: %f Total: %f Avg: %f Count: %d", frameMin, frameMax, frameTotal, frameTotal / frameCount, frameCount );

				hitFrame = s_profGlob.probeHitFrame[hitIter];
				LOG( "Frame %d: ", hitFrame );

				frameMin = FLT_MAX;
				frameMax = 0.0;
				frameTotal = 0.0;
				frameCount = 0;
			}

			LOG( "%d : %f", hitIter, s_profGlob.probeHitTime[hitIter] );

			if ( s_profGlob.probeHitTime[hitIter] < frameMin )
				frameMin = s_profGlob.probeHitTime[hitIter];
			if ( s_profGlob.probeHitTime[hitIter] > frameMax )
				frameMax = s_profGlob.probeHitTime[hitIter];
			frameTotal += s_profGlob.probeHitTime[hitIter];
			frameCount++;
		}

		s_profGlob.probeHitCount = 0;

		LOG( "" );
	}
}


void Prof_Frame()
{
	s_profGlob.probe = PROF_COUNT;
	// s_profGlob.probe = PROF_VNC_WIDGET_UPDATE;

	s_profGlob.frameCount++;
	if ( s_profGlob.frameCount == PROF_FRAME_COUNT )
	{
		Prof_Normalize();
		Prof_Print();
		s_profGlob.frameCount = 0;
	}
}
