#ifndef __FILE_H__
#define __FILE_H__

void File_Init();
void File_Shutdown();

void File_AddPath( const char *path );

byte *File_Read( const char *fileName, uint *bytesRead );

sbool File_Command();

#endif
