#ifndef __VNCWIDGET_H__
#define __VNCWIDGET_H__

#define INVALID_KEY_CODE 		(-1)

struct SVNCWidget;

SVNCWidget *VNC_CreateWidget();
void VNC_DestroyWidget( SVNCWidget *vnc );

void VNC_UpdateWidget( SVNCWidget *vnc );
void VNC_DrawWidget( SVNCWidget *vnc, const Matrix4f &view );

void VNC_Connect( SVNCWidget *vnc, const char *server, const char *password );
void VNC_Disconnect( SVNCWidget *vnc );

sbool VNC_IsConnected( SVNCWidget *vnc );

uint VNC_KeyCodeForAndroidCode( uint androidCode );

void VNC_KeyboardEvent( SVNCWidget *vnc, uint code, sbool down );
void VNC_MouseEvent( SVNCWidget *vnc, int x, int y, int buttons );

void VNC_UpdateHeadMouse( SVNCWidget *vnc, Vector3f eyePos, Vector3f eyeDir, sbool touch );

int VNC_GetTexID( SVNCWidget *vnc );

int VNC_GetWidth( SVNCWidget *vnc );
int VNC_GetHeight( SVNCWidget *vnc );
int VNC_GetTexWidth( SVNCWidget *vnc );
int VNC_GetTexHeight( SVNCWidget *vnc );

sbool VNC_Command( SVNCWidget *vnc );

#endif
