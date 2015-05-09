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
#include "v8plugin.h"
#include "command.h"
#include "file.h"
#include "message.h"
#include <include/v8.h>
#include <include/libplatform/libplatform.h>


using namespace v8;


struct SV8Instance
{
	pthread_t 	thread;
	char 		*fileName;
	char 		*source;
	Isolate 	*isolate;
};


struct SV8Globals
{
	pthread_t 			pluginThread;
	Platform 			*platform;
	SxPluginInterface	*sx;
};


SV8Globals s_v8;


// http://www.borisvanschooten.nl/blog/2014/06/23/typed-arrays-on-embedded-v8-2014-edition/
class MallocArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
  public:
    virtual void* Allocate(size_t length) {
        return calloc(length,1);
    }
    virtual void* AllocateUninitialized(size_t length) {
        return malloc(length);
    }
    // XXX we assume length is not needed
    virtual void Free(void*data, size_t length) {
        free(data);
    }
};

MallocArrayBufferAllocator s_allocator;


const char* V8_StringArg( const String::Utf8Value& value )
{
	return *value ? *value : "<not a string>";
}

#define ToCString V8_StringArg


int V8_IntArg( const Handle<Value>& value )
{
	return value->Int32Value();
}


float V8_FloatArg( const Handle<Value>& value )
{
	return value->ToNumber()->Value();
}


void V8_Throw( Isolate *isolate, const char *format, ... )
{
    va_list args;
    char 	buffer[256];

    va_start( args, format );
    vsnprintf( buffer, sizeof( buffer ), format, args );
    va_end( args );

    isolate->ThrowException( String::NewFromUtf8( isolate, buffer ) );
}


void V8_CheckResult( Isolate *isolate, SxResult result )
{
	const char 	*error;

	if ( result != SX_OK )
	{
		switch ( result )
		{
		case SX_NOT_IMPLEMENTED: 		
			error = "Requestion action is not yet implemented"; 
			break;
		case SX_INVALID_HANDLE:
			error = "Handle is invalid or unregistered";
			break;
		case SX_ALREADY_REGISTERED:
			error = "Handle was already registered";
			break;
		case SX_OUT_OF_RANGE:
			error = "Argument out of range";
			break;
		default:
			error = "<unknown error>";
			break;
		}

		V8_Throw( isolate, error );
	}
}


void V8_RegisterPluginCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->registerPlugin( 
			V8_StringArg( String::Utf8Value( args[0] ) ), 
			(SxPluginKind)V8_IntArg( args[1] ) ) );
}


void V8_UnregisterPluginCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->unregisterPlugin( 
			V8_StringArg( String::Utf8Value( args[0] ) ) ) );
}


void V8_ReceivePluginMessageCallback( const FunctionCallbackInfo<Value>& args )
{
	char 	msgBuf[MSG_LIMIT];

	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->receivePluginMessage( 
			V8_StringArg( arg0 ),
			V8_IntArg( args[1] ),
			msgBuf, MSG_LIMIT ) );

	args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), msgBuf ) );
}


void V8_RegisterWidgetCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->registerWidget( 
			V8_StringArg( arg0 ) ) );
}


void V8_UnregisterWidgetCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->unregisterWidget( 
			V8_StringArg( arg0 ) ) );
}


void V8_PostMessageCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->postMessage( 
			V8_StringArg( arg0 ) ) );
}


void V8_SendMessageCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );
	String::Utf8Value arg1( args[1] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->sendMessage( 
			V8_StringArg( arg0 ),
			V8_StringArg( arg1 ) ) );
}


void V8_ReceiveWidgetMessageCallback( const FunctionCallbackInfo<Value>& args )
{
	char 	msgBuf[MSG_LIMIT];

	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->receiveWidgetMessage( 
			V8_StringArg( arg0 ),
			V8_IntArg( args[1] ),
			msgBuf, MSG_LIMIT ) );

	args.GetReturnValue().Set( String::NewFromUtf8( args.GetIsolate(), msgBuf ) );
}


void V8_RegisterGeometryCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->registerGeometry( 
			V8_StringArg( arg0 ) ) );
}


void V8_UnregisterGeometryCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->unregisterGeometry( 
			V8_StringArg( arg0 ) ) );
}


void V8_SizeGeometryCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->sizeGeometry( 
			V8_StringArg( arg0 ),
			V8_IntArg( args[1] ),
			V8_IntArg( args[2] ) ) );
}


void V8_UpdateGeometryIndexRangeCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

    Local<ArrayBuffer> arg3 = Local<ArrayBuffer>::Cast( args[3] );
    ArrayBuffer::Contents buf = arg3->GetContents();

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->updateGeometryIndexRange( 
			V8_StringArg( arg0 ),
			V8_IntArg( args[1] ),
			V8_IntArg( args[2] ),
			(ushort *)buf.Data() ) );
}


void V8_UpdateGeometryPositionRangeCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

    Local<ArrayBuffer> arg3 = Local<ArrayBuffer>::Cast( args[3] );
    ArrayBuffer::Contents buf = arg3->GetContents();

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->updateGeometryPositionRange( 
			V8_StringArg( arg0 ),
			V8_IntArg( args[1] ),
			V8_IntArg( args[2] ),
			(SxVector3 *)buf.Data() ) );
}


void V8_UpdateGeometryTexCoordRangeCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

    Local<ArrayBuffer> arg3 = Local<ArrayBuffer>::Cast( args[3] );
    ArrayBuffer::Contents buf = arg3->GetContents();

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->updateGeometryTexCoordRange( 
			V8_StringArg( arg0 ),
			V8_IntArg( args[1] ),
			V8_IntArg( args[2] ),
			(SxVector2 *)buf.Data() ) );
}


void V8_UpdateGeometryColorRangeCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

    Local<ArrayBuffer> arg3 = Local<ArrayBuffer>::Cast( args[3] );
    ArrayBuffer::Contents buf = arg3->GetContents();

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->updateGeometryColorRange( 
			V8_StringArg( arg0 ),
			V8_IntArg( args[1] ),
			V8_IntArg( args[2] ),
			(SxColor *)buf.Data() ) );
}


void V8_PresentGeometryCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->presentGeometry( 
			V8_StringArg( arg0 ) ) );
}


void V8_RegisterTextureCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->registerTexture( 
			V8_StringArg( arg0 ) ) );
}


void V8_UnregisterTextureCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->unregisterTexture( 
			V8_StringArg( arg0 ) ) );
}


void V8_FormatTextureCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->formatTexture( 
			V8_StringArg( arg0 ),
			(SxTextureFormat)V8_IntArg( args[1] ) ) );
}


void V8_SizeTextureCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->sizeTexture( 
			V8_StringArg( arg0 ),
			V8_IntArg( args[1] ),
			V8_IntArg( args[2] ) ) );
}


void V8_UpdateTextureRectCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

    Local<ArrayBuffer> arg6 = Local<ArrayBuffer>::Cast( args[6] );
    ArrayBuffer::Contents buf = arg6->GetContents();

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->updateTextureRect( 
			V8_StringArg( arg0 ),
			V8_IntArg( args[1] ),
			V8_IntArg( args[2] ),
			V8_IntArg( args[3] ),
			V8_IntArg( args[4] ),
			V8_IntArg( args[5] ),
			buf.Data() ) );
}


void V8_LoadTextureJpegCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

    Local<ArrayBuffer> arg1 = Local<ArrayBuffer>::Cast( args[1] );
    ArrayBuffer::Contents buf = arg1->GetContents();

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->loadTextureJpeg( 
			V8_StringArg( arg0 ),
			buf.Data(), 
			buf.ByteLength() ) );
}


void V8_PresentTextureCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->presentTexture( 
			V8_StringArg( arg0 ) ) );
}


void V8_RegisterEntityCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->registerEntity( 
			V8_StringArg( arg0 ) ) );
}


void V8_UnregisterEntityCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->unregisterEntity( 
			V8_StringArg( arg0 ) ) );
}


void V8_SetEntityGeometryCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );
	String::Utf8Value arg1( args[1] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->setEntityGeometry( 
			V8_StringArg( arg0 ),
			V8_StringArg( arg1 ) ) );
}


void V8_SetEntityTextureCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );
	String::Utf8Value arg1( args[1] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->setEntityTexture( 
			V8_StringArg( arg0 ),
			V8_StringArg( arg1 ) ) );
}


void V8_GetVector3( Isolate *isolate, Handle<Value> &object, SxVector3 *result )
{
 	Handle<Array> array = Handle<Array>::Cast( object );

 	Local<Number> xNumber = array->Get( Integer::New( isolate, 0 ) )->ToNumber();
 	result->x = xNumber->Value();

 	Local<Number> yNumber = array->Get( Integer::New( isolate, 1 ) )->ToNumber();
 	result->y = yNumber->Value();

 	Local<Number> zNumber = array->Get( Integer::New( isolate, 2 ) )->ToNumber();
 	result->z = zNumber->Value();
}


void V8_GetOrientation( Isolate *isolate, const Handle<Value>& object, SxOrientation *orient )
{
	IdentityOrientation( orient );

	Handle<Array> array = Handle<Array>::Cast( object );

	Handle<Value> origin = array->Get( String::NewFromUtf8( isolate, "origin" ) );
	V8_GetVector3( isolate, origin, &orient->origin );
}


void V8_OrientEntityCallback( const FunctionCallbackInfo<Value>& args )
{
	SxOrientation 	orient;
	SxTrajectory 	tr;

	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	V8_GetOrientation( args.GetIsolate(), args[1], &orient );

	tr.kind = SxTrajectoryKind_Instant;

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->orientEntity( 
			V8_StringArg( arg0 ),
			&orient,
			&tr ) );
}


void V8_SetEntityVisibilityCallback( const FunctionCallbackInfo<Value>& args )
{
	SxTrajectory 	tr;

	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );

	tr.kind = SxTrajectoryKind_Instant;

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->setEntityVisibility( 
			V8_StringArg( arg0 ),
			V8_FloatArg( args[1] ),
			&tr ) );
}


void V8_ParentEntityCallback( const FunctionCallbackInfo<Value>& args )
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value arg0( args[0] );
	String::Utf8Value arg1( args[1] );

	V8_CheckResult( args.GetIsolate(), 
		s_v8.sx->parentEntity( 
			V8_StringArg( arg0 ),
			V8_StringArg( arg1 ) ) );
}


void IncludeCallback( const FunctionCallbackInfo<Value>& args ) 
{
	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value fileNameStr( args[0] );

    char *text = (char *)File_Read( *fileNameStr, NULL );
    if ( !text )
	{
		V8_Throw( args.GetIsolate(), "Unable to open %s", *fileNameStr );
	    return;
	}

    Handle<String> source = String::NewFromUtf8( args.GetIsolate(), text );
    free( text );

    Handle<Script> script = Script::Compile( source );

    Handle<Value> result = script->Run();

 	args.GetReturnValue().Set( result );
}


void AssetCallback( const FunctionCallbackInfo<Value>& args ) 
{
	uint 	dataSize;

	HandleScope handleScope( args.GetIsolate() );

	String::Utf8Value fileNameStr( args[0] );

    char *data = (char *)File_Read( *fileNameStr, &dataSize );
    if ( !data )
	{
		V8_Throw( args.GetIsolate(), "Unable to open %s", *fileNameStr );
	    return;
	}

    Handle<ArrayBuffer> buf = ArrayBuffer::New( args.GetIsolate(), 
    	data, dataSize, 
    	ArrayBufferCreationMode::kInternalized );

 	args.GetReturnValue().Set( buf );
}


Handle<Context> V8_CreateShellContext( Isolate* isolate )
{
	Handle<ObjectTemplate> global = ObjectTemplate::New( isolate );

	global->Set( String::NewFromUtf8( isolate, "include" ), 
		         FunctionTemplate::New( isolate, IncludeCallback ) );

	global->Set( String::NewFromUtf8( isolate, "asset" ), 
		         FunctionTemplate::New( isolate, AssetCallback ) );


	global->Set( String::NewFromUtf8( isolate, "registerPlugin" ), 
		         FunctionTemplate::New( isolate, V8_RegisterPluginCallback ) );

	global->Set( String::NewFromUtf8( isolate, "unregisterPlugin" ), 
		         FunctionTemplate::New( isolate, V8_UnregisterPluginCallback ) );

	global->Set( String::NewFromUtf8( isolate, "receivePluginMessage" ), 
		         FunctionTemplate::New( isolate, V8_ReceivePluginMessageCallback ) );

	global->Set( String::NewFromUtf8( isolate, "registerWidget" ), 
		         FunctionTemplate::New( isolate, V8_RegisterWidgetCallback ) );

	global->Set( String::NewFromUtf8( isolate, "unregisterWidget" ), 
		         FunctionTemplate::New( isolate, V8_UnregisterWidgetCallback ) );

	global->Set( String::NewFromUtf8( isolate, "postMessage" ), 
		         FunctionTemplate::New( isolate, V8_PostMessageCallback ) );

	global->Set( String::NewFromUtf8( isolate, "sendMessage" ), 
		         FunctionTemplate::New( isolate, V8_SendMessageCallback ) );

	global->Set( String::NewFromUtf8( isolate, "receiveWidgetMessage" ), 
		         FunctionTemplate::New( isolate, V8_ReceiveWidgetMessageCallback ) );


	global->Set( String::NewFromUtf8( isolate, "registerGeometry" ), 
		         FunctionTemplate::New( isolate, V8_RegisterGeometryCallback ) );

	global->Set( String::NewFromUtf8( isolate, "unregisterGeometry" ), 
		         FunctionTemplate::New( isolate, V8_UnregisterGeometryCallback ) );

	global->Set( String::NewFromUtf8( isolate, "sizeGeometry" ), 
		         FunctionTemplate::New( isolate, V8_SizeGeometryCallback ) );

	global->Set( String::NewFromUtf8( isolate, "updateGeometryIndexRange" ), 
		         FunctionTemplate::New( isolate, V8_UpdateGeometryIndexRangeCallback ) );

	global->Set( String::NewFromUtf8( isolate, "updateGeometryPositionRange" ), 
		         FunctionTemplate::New( isolate, V8_UpdateGeometryPositionRangeCallback ) );

	global->Set( String::NewFromUtf8( isolate, "updateGeometryTexCoordRange" ), 
		         FunctionTemplate::New( isolate, V8_UpdateGeometryTexCoordRangeCallback ) );

	global->Set( String::NewFromUtf8( isolate, "updateGeometryColorRange" ), 
		         FunctionTemplate::New( isolate, V8_UpdateGeometryColorRangeCallback ) );

	global->Set( String::NewFromUtf8( isolate, "presentGeometry" ), 
		         FunctionTemplate::New( isolate, V8_PresentGeometryCallback ) );


	global->Set( String::NewFromUtf8( isolate, "registerTexture" ), 
		         FunctionTemplate::New( isolate, V8_RegisterTextureCallback ) );

	global->Set( String::NewFromUtf8( isolate, "unregisterTexture" ), 
		         FunctionTemplate::New( isolate, V8_UnregisterTextureCallback ) );

	global->Set( String::NewFromUtf8( isolate, "formatTexture" ), 
		         FunctionTemplate::New( isolate, V8_FormatTextureCallback ) );

	global->Set( String::NewFromUtf8( isolate, "sizeTexture" ), 
		         FunctionTemplate::New( isolate, V8_SizeTextureCallback ) );

	global->Set( String::NewFromUtf8( isolate, "updateTextureRect" ), 
		         FunctionTemplate::New( isolate, V8_UpdateTextureRectCallback ) );

	global->Set( String::NewFromUtf8( isolate, "loadTextureJpeg" ), 
		         FunctionTemplate::New( isolate, V8_LoadTextureJpegCallback ) );

	global->Set( String::NewFromUtf8( isolate, "presentTexture" ), 
		         FunctionTemplate::New( isolate, V8_PresentTextureCallback ) );


	global->Set( String::NewFromUtf8( isolate, "registerEntity" ), 
		         FunctionTemplate::New( isolate, V8_RegisterEntityCallback ) );

	global->Set( String::NewFromUtf8( isolate, "unregisterEntity" ), 
		         FunctionTemplate::New( isolate, V8_UnregisterEntityCallback ) );

	global->Set( String::NewFromUtf8( isolate, "setEntityGeometry" ), 
		         FunctionTemplate::New( isolate, V8_SetEntityGeometryCallback ) );

	global->Set( String::NewFromUtf8( isolate, "setEntityTexture" ), 
		         FunctionTemplate::New( isolate, V8_SetEntityTextureCallback ) );

	global->Set( String::NewFromUtf8( isolate, "orientEntity" ), 
		         FunctionTemplate::New( isolate, V8_OrientEntityCallback ) );

	global->Set( String::NewFromUtf8( isolate, "setEntityVisibility" ), 
		         FunctionTemplate::New( isolate, V8_SetEntityVisibilityCallback ) );

	global->Set( String::NewFromUtf8( isolate, "parentEntity" ), 
		         FunctionTemplate::New( isolate, V8_ParentEntityCallback ) );


	return Context::New( isolate, NULL, global );
}


void V8_ReportException( Isolate* isolate, TryCatch* tryCatch ) 
{
	char 		underLine[1024];

    HandleScope handleScope( isolate );

    String::Utf8Value exception( tryCatch->Exception() );
    const char* exceptionString = V8_StringArg( exception );

    Handle<Message> message = tryCatch->Message();
    if (message.IsEmpty()) 
    {
        // V8 didn't provide any extra information about this error; just
        // print the exception.
        LOG( exceptionString );
        return;
    }

    // Print (filename):(line number): (message).
    String::Utf8Value filename( message->GetScriptResourceName() );
    const char* filenameString = V8_StringArg(filename);

    int lineNumber = message->GetLineNumber();

    LOG( "%s:%i: %s", filenameString, lineNumber, exceptionString );

    // Print line of source code.
    String::Utf8Value sourceLine( message->GetSourceLine() );
    const char* sourceLineString = V8_StringArg( sourceLine );
    LOG( sourceLineString );

    // Print wavy underline (GetUnderline is deprecated).
    uint start = message->GetStartColumn();
    uint end = message->GetEndColumn();

    uint i;
    for ( i = 0; i < end; i++ )
    {
    	if ( i >= sizeof( underLine ) - 1 )
    		break;
    	underLine[i] = i < start ? ' ' : '^';
    }
    underLine[i] = '\0';
    LOG( underLine );

    String::Utf8Value stackTrace( tryCatch->StackTrace() );
    if ( stackTrace.length() > 0 ) 
    {
        const char* stackTraceString = V8_StringArg( stackTrace );
        LOG( stackTraceString );
    }
}


void *V8_InstanceThread( void *threadContext )
{
	SV8Instance	*v8;

	v8 = (SV8Instance *)threadContext;
	assert( v8 );

	LOG( "V8 instance %s (%p) starting up...", v8->fileName, threadContext );

	v8->isolate = Isolate::New();

	{
		Isolate::Scope isolateScope( v8->isolate );
		HandleScope handleScope( v8->isolate );

		Local<Context> context = V8_CreateShellContext( v8->isolate );

		{
			Context::Scope contextScope( context );

			TryCatch tryCatch( v8->isolate );

		    Local<String> fileName = String::NewFromUtf8( v8->isolate, v8->fileName );
			Local<String> source = String::NewFromUtf8( v8->isolate, v8->source );
			
			Local<Script> script = Script::Compile( source, fileName );

		    if ( script.IsEmpty() )
			{
				V8_ReportException( v8->isolate, &tryCatch );
			}
			else
			{
				script->Run();

				if ( tryCatch.HasCaught() )
				{
					V8_ReportException( v8->isolate, &tryCatch );
				}
			}
		}
	}

	LOG( "V8 instance %s (%p) shutting down...", v8->fileName, threadContext );

	v8->isolate->Dispose();

	free( v8->fileName );
	free( v8->source );

	memset( v8, 0, sizeof( SV8Instance ) );
	free( v8 );

	return NULL;
}


void V8_LoadCmd( const SMsg *msg, void *context )
{
	const char 		*fileName;
	char 			*source;
	SV8Instance 	*v8;
	int 			err;

	fileName = Msg_Argv( msg, 1 );

	source = (char *)File_Read( fileName, NULL );
	if ( !source )
		return;

	v8 = (SV8Instance *)malloc( sizeof( SV8Instance ) );
	assert( v8 );

	memset( v8, 0, sizeof( SV8Instance ) );

	v8->fileName = strdup( fileName );
	v8->source = strdup( source );

	err = pthread_create( &v8->thread, NULL, V8_InstanceThread, v8 );
	if ( err != 0 )
		FAIL( "V8_LoadCmd: pthread_create returned %i", err );
}


SMsgCmd s_v8Cmds[] =
{
	{ "load", 			V8_LoadCmd, 			 	"load <jsfile>" },
	{ NULL, NULL, NULL }
};


void *V8_PluginThread( void *context )
{
	char 	msgBuf[MSG_LIMIT];
	SMsg 	msg;

	pthread_setname_np( pthread_self(), "V8" );

    V8::SetArrayBufferAllocator( &s_allocator );

	s_v8.platform = platform::CreateDefaultPlatform();
	V8::InitializePlatform( s_v8.platform );

	V8::Initialize();

	g_pluginInterface.registerPlugin( "v8", SxPluginKind_Widget );

	for ( ;; )
	{
		g_pluginInterface.receivePluginMessage( "v8", SX_WAIT_INFINITE, msgBuf, MSG_LIMIT );

		Msg_ParseString( &msg, msgBuf );
		if ( Msg_Empty( &msg ) )
			continue;

		if ( Msg_IsArgv( &msg, 0, "v8" ) )
			Msg_Shift( &msg, 1 );

		if ( Msg_IsArgv( &msg, 0, "unload" ) )
			break;

		MsgCmd_Dispatch( &msg, s_v8Cmds, NULL );
	}

	g_pluginInterface.unregisterPlugin( "v8" );

	V8::Dispose();

	V8::ShutdownPlatform();
	delete s_v8.platform;
	s_v8.platform = NULL;

	return NULL;
}


void V8_InitPlugin()
{
	int err;

	s_v8.sx = &g_pluginInterface;

	err = pthread_create( &s_v8.pluginThread, NULL, V8_PluginThread, NULL );
	if ( err != 0 )
		FAIL( "V8_InitPlugin: pthread_create returned %i", err );
}

