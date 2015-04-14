#ifndef __COMMAND_H__
#define __COMMAND_H__

void Cmd_Frame();
void Cmd_Add( const char *cmd );

uint Cmd_Argc();
const char *Cmd_Argv( uint argIndex );

#endif
