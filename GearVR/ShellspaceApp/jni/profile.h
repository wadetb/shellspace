#ifndef __PROFILE_H__
#define __PROFILE_H__

enum EProfType
{
	PROF_FRAME,
	PROF_CMD,
	PROF_KEYBOARD,
	PROF_VNC_WIDGET,
	PROF_VNC_WIDGET_INQUEUE,
	PROF_VNC_WIDGET_ADVANCE,
	PROF_VNC_WIDGET_RESIZE,
	PROF_VNC_WIDGET_UPDATE,
	PROF_VNC_WIDGET_FINISHED_UPDATES,
	PROF_SCENE,
	PROF_DRAW,
	PROF_DRAW_EYE,
	PROF_DRAW_KEYBOARD,
	PROF_DRAW_VNC_WIDGET,
	PROF_VNC_THREAD,
	PROF_VNC_THREAD_INPUT,
	PROF_VNC_THREAD_WAIT,
	PROF_VNC_THREAD_HANDLE,
	PROF_VNC_THREAD_HANDLE_RESIZE,
	PROF_VNC_THREAD_HANDLE_UPDATE,
	PROF_VNC_THREAD_HANDLE_FINISHED_UPDATES,
	PROF_VNC_THREAD_HANDLE_CURSOR_SHAPE,
	PROF_VNC_THREAD_HANDLE_CURSOR_POS,
	PROF_VNC_THREAD_HANDLE_CURSOR_SAVE,
	PROF_VNC_THREAD_HANDLE_CURSOR_RESTORE,
	PROF_VNC_THREAD_LOCK_IN_QUEUE,
	PROF_VNC_THREAD_UPDATE_TEXTURE_RECT,
	PROF_VNC_THREAD_OUTPUT,
	PROF_COUNT
};

double Prof_MS();

void Prof_Start( EProfType prof );
void Prof_Stop( EProfType prof );
void Prof_Frame();

#endif
