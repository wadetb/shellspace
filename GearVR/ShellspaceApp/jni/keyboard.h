#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

void Keyboard_Init();
void Keyboard_LoadKeyboards();
void Keyboard_Frame( uint buttonState, const Vector3f &eyePos, const Vector3f &eyeDir );
void Keyboard_Draw();
void Keyboard_Toggle();
bool Keyboard_IsVisible();

#endif
