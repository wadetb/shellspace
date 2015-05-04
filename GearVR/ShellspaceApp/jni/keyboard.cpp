/*
    Shellspace - One tiny step towards the VR Desktop Operating System
    Copyright (C) 2015  Wade Brainerd

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "common.h"
#include "keyboard.h"
#include "command.h"
#include <App.h>
#include <BitmapFont.h>
#include <Input.h>
#include <OvrApp.h>
#include <PackageFiles.h>
#include "gason/gason.h"


#define MAX_KEYBOARDS 			32
#define MAX_KEYBOARD_KEYS 		128

#define INVALID_KEY				-1

#define COS_MIN_PICK_ANGLE 		0.9961946980917455f // 5 degrees


struct SKey
{
	float	x;
	float	y;
	char	*label;
	char	*code;
};


struct SKeyboard
{
	char 	*fileName;
	int 	keyCount;
	SKey 	*keys;
};


struct SKeyboardGlobals
{
	bool 				visible;

	Vector3f			pos;
	Vector3f			up;
	Vector3f			right;
	Vector3f			normal;
	float				scale;
	float 				depth;

	int 				key;

	uint 				keyboardCount;
	SKeyboard			keyboards[MAX_KEYBOARDS];
	int 				activeIndex;
};


static SKeyboardGlobals s_keyGlob;


static const char *s_builtinKeyboards[] =
{
	"assets/intro.vrkey",
	"assets/connect.vrkey",
	"assets/alphanum.vrkey",
	// "assets/view.vrkey",
	NULL
};


void Keyboard_Init()
{
	s_keyGlob.scale = 0.05f;
	s_keyGlob.depth = 20.0f;

	s_keyGlob.activeIndex = 0;
	s_keyGlob.key = INVALID_KEY;

	s_keyGlob.pos = Vector3f( 0.0f, 0.0f, -1.0f );
	s_keyGlob.pos.Normalize();
	s_keyGlob.pos *= s_keyGlob.depth;

	s_keyGlob.up = Vector3f( 0.0f, 1.0f, 0.0f );
	s_keyGlob.right = Vector3f( 1.0f, 0.0f, 0.0f );
	s_keyGlob.normal = s_keyGlob.right.Cross( s_keyGlob.up );

	Keyboard_LoadKeyboards();

	s_keyGlob.visible = true;
}


void Keyboard_Show( const char *name )
{
	uint 		keyboardIter;
	SKeyboard 	*keyboard;

	Keyboard_LoadKeyboards();

	for ( keyboardIter = 0; keyboardIter < s_keyGlob.keyboardCount; keyboardIter++ ) 
	{
		keyboard = &s_keyGlob.keyboards[keyboardIter];

		if ( strcmp( keyboard->fileName, name ) == 0 )
		{
			s_keyGlob.activeIndex = keyboardIter;
			break;
		}
	}

	s_keyGlob.visible = true;
}


void Keyboard_Orient( const SxVector3& eyeDir, float depth )
{
	s_keyGlob.depth = depth;

	s_keyGlob.pos = Vector3f( eyeDir.x, eyeDir.y, eyeDir.z );
	s_keyGlob.pos.Normalize();
	s_keyGlob.pos *= s_keyGlob.depth;

	s_keyGlob.normal = s_keyGlob.pos;
	s_keyGlob.normal.Normalize();
	s_keyGlob.normal = -s_keyGlob.normal;

	s_keyGlob.up = Vector3f( 0.0f, 1.0f, 0.0f );
	s_keyGlob.right = s_keyGlob.up.Cross( s_keyGlob.normal );
	s_keyGlob.right.Normalize();
	s_keyGlob.up = s_keyGlob.normal.Cross( s_keyGlob.right );
	s_keyGlob.up.Normalize();
	s_keyGlob.right = -s_keyGlob.right; // $$$
}


void Crash()
{
	// *(int*)0 = 0;
	assert( false );
	// g_app->app->GetVrJni()->ThrowNew(g_app->app->GetVrJni()->FindClass("java/lang/Exception"), "HIHIHI" );
}


void Keyboard_Toggle()
{
	s_keyGlob.key = INVALID_KEY;

	// COFFEE_TRY_JNI(g_jni, Crash());
	// Crash();

	if ( s_keyGlob.visible )
	{
		s_keyGlob.visible = false;
	}
	else
	{
		Keyboard_LoadKeyboards();

		s_keyGlob.visible = true;
	}
}


static SKeyboard *Keyboard_GetActive()
{
	assert( (int)s_keyGlob.activeIndex < (int)s_keyGlob.keyboardCount );
	return &s_keyGlob.keyboards[s_keyGlob.activeIndex];
}


static void Keyboard_PickKey( const Vector3f &eyePos, const Vector3f &eyeDir )
{
	SKeyboard 	*keyboard;
	int 		closestKey;
	float 		closestDot;
	int 		keyIter;
	SKey 		*key;
	Vector3f 	keyPos;
	Vector3f 	keyDir;
	float 		keyDot;

	keyboard = Keyboard_GetActive();

	closestKey = INVALID_KEY;
	closestDot = COS_MIN_PICK_ANGLE;

	for ( keyIter = 0; keyIter < keyboard->keyCount; keyIter++ )
	{
		key = &keyboard->keys[keyIter];

		if ( !key->code )
			continue;

		keyPos = s_keyGlob.pos + 
			s_keyGlob.right * (float)key->x * s_keyGlob.scale - 
			s_keyGlob.up    * (float)key->y * s_keyGlob.scale;

		keyDir = keyPos - eyePos;
		keyDir.Normalize();

		keyDot = keyDir.Dot( eyeDir );
		if ( keyDot > closestDot )
		{
			closestKey = keyIter;
			closestDot = keyDot;
		}
	}

	s_keyGlob.key = closestKey;
}


static void Keyboard_Previous()
{
	s_keyGlob.activeIndex = (s_keyGlob.activeIndex + s_keyGlob.keyboardCount - 1) % s_keyGlob.keyboardCount;
	s_keyGlob.key = INVALID_KEY;
	LOG( "Selected keyboard: %s", s_keyGlob.keyboards[s_keyGlob.activeIndex].fileName );
}


static void Keyboard_Next()
{
	s_keyGlob.activeIndex = (s_keyGlob.activeIndex + 1) % s_keyGlob.keyboardCount;	
	s_keyGlob.key = INVALID_KEY;
	LOG( "Selected keyboard: %s", s_keyGlob.keyboards[s_keyGlob.activeIndex].fileName );
}


static sbool Keyboard_IsPicked()
{
	return s_keyGlob.key != INVALID_KEY;
}


static void Keyboard_UpdatePickedKey( uint buttonState )
{
	bool 		isTap;
	SKeyboard 	*keyboard;
	SKey 		*key;
	char 		code[1024];

	isTap = (buttonState & BUTTON_TOUCH_SINGLE) ? true : false;

	keyboard = Keyboard_GetActive();
	key = &keyboard->keys[s_keyGlob.key];

	if ( !key->code )
		return;

	if ( isTap )
	{
		snprintf( code, sizeof( code ), "%s", key->code );

		if ( code[0] == '+' )
		{
			Cmd_Add( code );

			code[0] = '-';
			Cmd_Add( code );
		}
		else
		{
			if ( isTap )
				Cmd_Add( code );
		}
	}
}


void Keyboard_Frame( uint buttonState, const Vector3f &eyePos, const Vector3f &eyeDir )
{
	if ( !s_keyGlob.visible )
		return;

	if ( !s_keyGlob.keyboardCount )
		return;

	Prof_Start( PROF_KEYBOARD );

	if ( buttonState & BUTTON_SWIPE_FORWARD )
		Keyboard_Next();

	if ( buttonState & BUTTON_SWIPE_BACK )
		Keyboard_Previous();

	Keyboard_PickKey( eyePos, eyeDir );

	if ( Keyboard_IsPicked() )
		Keyboard_UpdatePickedKey( buttonState );

	Prof_Stop( PROF_KEYBOARD );
}


void Keyboard_Draw()
{
	BitmapFont  		*font;
	BitmapFontSurface 	*fontSurface;
	fontParms_t 		fontParms;
	SKeyboard 			*keyboard;
	SKey 				*key;
	uint 				keyIter;
	Vector3f 			keyPos;
	Vector4f 			color;
	const char 			*label;

	if ( !s_keyGlob.visible )
		return;

	Prof_Start( PROF_DRAW_KEYBOARD );

	font = &g_app->app->GetDefaultFont();
	fontSurface = &g_app->app->GetWorldFontSurface();

	fontParms.CenterVert = true;
	fontParms.CenterHoriz = true;
    fontParms.TrackRoll = false;

	keyboard = Keyboard_GetActive();

	for ( keyIter = 0; keyIter < (uint)keyboard->keyCount; keyIter++ )
	{
		key = &keyboard->keys[keyIter];

		keyPos = s_keyGlob.pos + 
			s_keyGlob.right * (float)key->x * s_keyGlob.scale - 
			s_keyGlob.up    * (float)key->y * s_keyGlob.scale;

		if ( keyIter == (uint)s_keyGlob.key )
			color = Vector4f( 1.0f, 0.0f, 0.0f, 1.0f );
		else
			color = Vector4f( 1.0f, 1.0f, 1.0f, 1.0f );

		if ( key->label )
			label = key->label;
		else
			label = key->code;

		fontSurface->DrawText3D( *font, fontParms, keyPos, s_keyGlob.normal, s_keyGlob.up, 10.0f, color, label );
	}

	Prof_Stop( PROF_DRAW_KEYBOARD );
}


static sbool Keyboard_ParseKeyboard( char *buffer, const char *fileName, SKeyboard *keyboard )
{
	JsonAllocator	allocator;
	JsonValue 		keyboardJson;
	char 			*endptr;
	int 			status;
	SKey 			*key;
	JsonNode 		*xJson;
	JsonNode 		*yJson;
	JsonNode 		*labelJson;
	JsonNode 		*codeJson;
	uint 			keyCount;
	SKey 			keys[MAX_KEYBOARD_KEYS];

	status = jsonParse( buffer, &endptr, &keyboardJson, allocator );
	if ( status != JSON_OK ) 
	{
	    LOG( "%s at %zd\n", jsonStrError( status ), endptr - buffer );
	    return sfalse;
	}

	if ( keyboardJson.getTag() != JSON_OBJECT )
	{
		LOG( "Expected JSON object." );
		return sfalse;
	}

	keyCount = 0;

	for ( auto keysJson : keyboardJson )
	{
		if ( keysJson->value.getTag() != JSON_ARRAY || strcmp( keysJson->key, "keys" ) )
		{
			LOG( "Expected keys array." );
			continue;
		}

		for ( auto keyJson : keysJson->value )
		{
			key = &keys[keyCount];

			if ( keyJson->value.getTag() != JSON_OBJECT )
			{
				LOG( "Expected key object." );
				continue;
			}

			xJson = getFirstChildByKey( keyJson->value, "x" );
			if ( !xJson || xJson->value.getTag() != JSON_NUMBER )
			{
				LOG( "Missing x field" );
				continue;
			}
			key->x = xJson->value.toNumber();

			yJson = getFirstChildByKey( keyJson->value, "y" );
			if ( !yJson || yJson->value.getTag() != JSON_NUMBER )
			{
				LOG( "Missing y field" );
				continue;
			}
			key->y = yJson->value.toNumber();

			labelJson = getFirstChildByKey( keyJson->value, "label" );
			if ( labelJson && labelJson->value.getTag() == JSON_STRING )
				key->label = strdup( labelJson->value.toString() );
			else
				key->label = NULL;

			codeJson = getFirstChildByKey( keyJson->value, "code" );
			if ( codeJson && codeJson->value.getTag() == JSON_STRING )
				key->code = strdup( codeJson->value.toString() );
			else
				key->code = NULL;

			if ( !key->code && !key->label )
			{
				LOG( "Key is has neither a code nor a label field." );
				continue;
			}

			keyCount++;
		}
	}

	if ( !keyCount )
	{
		LOG( "Keyboard contains no keys." );
		return sfalse;
	}

	keyboard->fileName = strdup( fileName );
	
	keyboard->keys = (SKey *)malloc( sizeof( SKey ) * keyCount );
	assert( keyboard->keys );

	memcpy( keyboard->keys, keys, sizeof( SKey ) * keyCount );

	keyboard->keyCount = keyCount;

	return strue;
}


static sbool Keyboard_LoadKeyboardFromFileSystem( const char *fileName, SKeyboard *keyboard )
{
	FILE 	*in;
	size_t 	length;
	char 	*buffer;
	size_t 	read;
	sbool 	result;

	LOG( "Loading %s from the filesystem", fileName );

	in = fopen( fileName, "r" );
	if ( !in )
	{
		LOG( "Failed to load JSON file: %s", fileName );
		return sfalse;
	}

	fseek( in, 0, SEEK_END );
	length = ftell( in );
	fseek( in, 0, SEEK_SET );

	buffer = (char *)malloc( length + 1 );
	if ( !buffer )
	{
		LOG( "Failed to allocate %d bytes of memory for the JSON file: %s", length + 1, fileName );
		fclose( in );
		return sfalse;
	}

	read = fread( buffer, 1, length, in );
	if ( read != length )
	{
		LOG( "Failed to read %d bytes from the JSON file: %s", length, fileName );
		fclose( in );
		return sfalse;
	}
	buffer[length] = 0;

	fclose( in );

	result = Keyboard_ParseKeyboard( buffer, fileName, keyboard );

	free( buffer );

	return result;
}


static sbool Keyboard_LoadKeyboardFromPackage( const char *fileName, SKeyboard *keyboard )
{
	int 	length;
	void 	*buffer;
	bool 	result;

	LOG( "Loading %s from package", fileName );

	ovr_ReadFileFromApplicationPackage( fileName, length, buffer );

	if ( !buffer )
	{
		LOG( "Failed to load JSON file: %s", fileName );
		return sfalse;
	}
	
	result = Keyboard_ParseKeyboard( (char *)buffer, fileName, keyboard );

	free( buffer );

	return result;
}


void Keyboard_FreeKeyboards()
{
	uint 		keyboardIter;
	SKeyboard 	*keyboard;
	uint 		keyIter;
	SKey 		*key;

	for ( keyboardIter = 0; keyboardIter < s_keyGlob.keyboardCount; keyboardIter++ )
	{
		keyboard = &s_keyGlob.keyboards[keyboardIter];

		for ( keyIter = 0; keyIter < (uint)keyboard->keyCount; keyIter++ )
		{
			key = &keyboard->keys[keyIter];

			if ( key->label )
				free( key->label );

			if ( key->code )
				free( key->code );
		}

		free( keyboard->fileName );
		free( keyboard->keys );
	}

	s_keyGlob.keyboardCount = 0;
}


#define KEYBOARD_FILESYS_PATH "Oculus/Shellspace/"


void Keyboard_LoadKeyboards()
{
	char 					*oldFileName;
	const char				**builtinName;
	SKeyboard 				*keyboard;
	Array< String > 		searchPaths;
	StringHash< String >	uniqueFileList;
	uint 					keyboardIter;

	if ( (int)s_keyGlob.activeIndex >= 0 && (int)s_keyGlob.activeIndex < (int)s_keyGlob.keyboardCount )
		oldFileName = strdup( s_keyGlob.keyboards[s_keyGlob.activeIndex].fileName );
	else
		oldFileName = NULL;

	Keyboard_FreeKeyboards();

	for ( builtinName = s_builtinKeyboards; *builtinName; builtinName++ )
	{
		if ( s_keyGlob.keyboardCount == MAX_KEYBOARDS )
			break;

		keyboard = &s_keyGlob.keyboards[s_keyGlob.keyboardCount];
		if ( Keyboard_LoadKeyboardFromPackage( *builtinName, keyboard ) )
			s_keyGlob.keyboardCount++;
	}

	g_app->app->GetStoragePaths().PushBackSearchPathIfValid( EST_SECONDARY_EXTERNAL_STORAGE, EFT_ROOT, "", searchPaths );
	g_app->app->GetStoragePaths().PushBackSearchPathIfValid( EST_PRIMARY_EXTERNAL_STORAGE, EFT_ROOT, "", searchPaths );

	uniqueFileList = RelativeDirectoryFileList( searchPaths, KEYBOARD_FILESYS_PATH );

	for ( auto iter = uniqueFileList.Begin(); iter != uniqueFileList.End(); ++iter )
	{
		for ( auto searchPathIter = searchPaths.Begin();
			  searchPathIter != searchPaths.End();
			  ++searchPathIter )
		{
			if ( s_keyGlob.keyboardCount == MAX_KEYBOARDS )
				break;

			keyboard = &s_keyGlob.keyboards[s_keyGlob.keyboardCount];
			if ( Keyboard_LoadKeyboardFromFileSystem( *searchPathIter + iter->First, keyboard ) )
			{
				s_keyGlob.keyboardCount++;
				break;
			}
		}
	}
	
	s_keyGlob.activeIndex = 0;

	if ( oldFileName )
	{
		for ( keyboardIter = 0; keyboardIter < s_keyGlob.keyboardCount; keyboardIter++ ) 
		{
			keyboard = &s_keyGlob.keyboards[keyboardIter];

			if ( strcmp( keyboard->fileName, oldFileName ) == 0 )
			{
				s_keyGlob.activeIndex = keyboardIter;
				break;
			}
		}

		free( oldFileName );
	}
}


bool Keyboard_IsVisible()
{
	return s_keyGlob.visible;
}

