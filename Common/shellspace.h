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
#ifndef __SHELLSPACE_H__
#define __SHELLSPACE_H__

//
// Result codes
//
enum SxResult
{
    SX_OK                    = 0,
    SX_NOT_IMPLEMENTED,                     // Requestion action is not yet implemented
    SX_INVALID_HANDLE,                      // Handle was invalid or already unregistered
    SX_ALREADY_REGISTERED,                  // Handle was already registered
    SX_OUT_OF_RANGE,                        // Argument was out of range
};


//
// Handles
//
// Handles are plugin-defined unique IDs for identifying the subject of API
//  calls.  Plugins can use any string so long as it is unique.
//
// NB: The method for ensuring uniqueness is that for now, each plugin will
//  just use a plugin-specific prefix.
//
// Handle values are data-type specific, meaning that is valid to use "me" as
//  a plugin, widget, texture, geometry and entity handle!
//
// Note that handles are global. Until security measures are added (if they 
//  ever are), plugins are free to interact with each others' objects.
// 

typedef const char *SxPluginHandle;
typedef const char *SxWidgetHandle;
typedef const char *SxTextureHandle;
typedef const char *SxGeometryHandle;
typedef const char *SxEntityHandle;

//
// Plugins
//
// Plugins are the means by which the Shellspace core is extended.
//  
// Different categories of plugins exist including:
//
// - Widget plugins for spawning widgets.
// - Shell plugins for arranging widgets in the VR environment and for routing
//    messages.
// - Input plugins to supply data from input devices.
//

enum SxPluginKind
{
    SxPluginKind_Widget,
    SxPluginKind_Shell,
    SxPluginKind_Input,
    SxPluginKind_Count
};

typedef SxResult (*SxRegisterPlugin)( SxPluginHandle wd, SxPluginKind kind );
typedef SxResult (*SxUnregisterPlugin)( SxPluginHandle wd );

//
// Widgets
// 
// A widget is a functional 3D element in the VR world, similar in scope to 
//  an app running on a phone or a program running on a 2D desktop.
//
// A widget can be confined to a spatial area by the shell, can surround the 
//  view with content, or can be a mixture of the two.
//

typedef SxResult (*SxRegisterWidget)( SxWidgetHandle wd );
typedef SxResult (*SxUnregisterWidget)( SxWidgetHandle wd );

//
// Messages
//
// Messages are the transport through which low bandwidth, domain specific 
//  communication takes place.  
//
// Messages are simple text string in command line argument format, e.g.
//  "spawn vnc 10.90.240.11 --format 565"
// Arguments may be quoted, with any internal quotes escaped as \".
//
// Widgets can register for preferential receipt of certain messages via
//  listeners.  Otherwise it's up to the shell and other widgets to pass
//  messages along.  
//
// For example the shell registers for "key" and "mouse" messages, keeps 
//  track of the active widget, and forwards those messages that it doesn't
//  consume itself to the active widget.
//

typedef SxResult (*SxBroadcastMessage)( const char *message );
typedef SxResult (*SxSendMessage)( SxWidgetHandle wd, const char *message );
typedef SxResult (*SxReceiveMessages)( SxWidgetHandle wd, const char *result, unsigned int resultLen );

typedef SxResult (*SxRegisterMessageListeners)( SxWidgetHandle wd, const char *messages );
typedef SxResult (*SxUnregisterMessageListeners)( SxWidgetHandle wd, const char *messages );

//
// Entities
//
// Widgets present data to the user through 3D objects called entities.  
//
// An entity consists of geometry (3D vertices and triangle indices) and 
//  a 2D texture map, placed at a location in the world.
//
// For simplicity, an entity has only a single geometry object and a single
//  (optional) albedo texture map.
//

// 
// Common types
//

struct SxColor
{
    byte r;
    byte g;
    byte b;
    byte a;
};

struct SxVector2
{
    float x;
    float y;
};

struct SxVector3
{
    float x;
    float y;
    float z;
};

//
// Textures
//

enum SxTextureFormat
{
    SxTextureFormat_R8G8B8A8,
    SxTextureFormat_R8G8B8A8_SRGB,
    SxTextureFormat_Count
};

//
// Orientation
//
// An entity's placement in the world is controlled by an orientation
//  object which includes the entity's parent (for base coordinate system),
//  position, rotation and scale.
// Angles are given in degrees.
//

struct SxAngles
{
    float           yaw;
    float           pitch;
    float           roll;
};


struct SxOrientation
{
    SxAngles        angles;
    SxVector3       origin;
    SxVector3       scale;
};

//
// Trajectories
// 
// A trajectory defines the way an entity animates from one state to another.
// 

enum SxTrajectoryKind
{
    SxTrajectoryKind_Instant,
    SxTrajectoryKind_Linear,
    SxTrajectoryKind_SmoothStep,
    SxTrajectoryKind_Count
};

struct SxTrajectory
{
    SxTrajectoryKind    kind;
    float               duration;
};

//
// Geometry
//

//
// sxRegisterGeometry
// sxUnregisterGeometry
//
// Register/unregister a geometry handle.  
// When initially registered, the geometry has no contents and will not draw
//  anything.
// 
typedef SxResult (*SxRegisterGeometry)( SxGeometryHandle geo );
typedef SxResult (*SxUnregisterGeometry)( SxGeometryHandle geo );

//
// sxSizeGeometry
// 
// Sets the number of vertices and indices stored in the geometry handle.
// Invalidates existing contents, if any.
//
typedef SxResult (*SxSizeGeometry)( SxGeometryHandle geo, unsigned int vertexCount, unsigned int indexCount );

//
// sxUpdateGeometryIndexRange
//
// Updates a region of the geometry's index buffer with new index data.
// The input data is copied and may be discarded after the function returns.
// 
typedef SxResult (*SxUpdateGeometryIndexRange)( SxGeometryHandle geo, unsigned int firstIndex, unsigned int indexCount, const ushort *indices );

//
// sxUpdateGeometryIndexRange
//
// Updates a range of 3D position values in the geometry's vertex buffer.
// The input data is copied and may be discarded after the function returns.
// 
typedef SxResult (*SxUpdateGeometryPositionRange)( SxGeometryHandle geo, unsigned int firstVertex, unsigned int vertexCount, const SxVector3 *positions );

//
// sxUpdateGeometryTexCoordRange
//
// Updates a range of 2D texture coordinate values in the geometry's vertex 
//  buffer.
// The input data is copied and may be discarded after the function returns.
// 
typedef SxResult (*SxUpdateGeometryTexCoordRange)( SxGeometryHandle geo, unsigned int firstVertex, unsigned int vertexCount, const SxVector2 *texCoords );

//
// sxUpdateGeometryColorRange
//
// Updates a range of color values in the geometry's vertex buffer.
// The input data is copied and may be discarded after the function returns.
// 
typedef SxResult (*SxUpdateGeometryColorRange)( SxGeometryHandle geo, unsigned int firstVertex, unsigned int vertexCount, const SxColor *colors );

//
// sxPresentGeometry
//
// Makes the geometry visible, after all prior updates have completed.
// 
typedef SxResult (*SxPresentGeometry)( SxGeometryHandle geo );

//
// Textures
//

//
// sxRegisterTexture
// sxUnregisterTexture
//
// Register/unregister a texture handle.  
// When initially registered, the texture has no contents and will draw as 
//  solid white.
// 
typedef SxResult (*SxRegisterTexture)( SxTextureHandle tx );
typedef SxResult (*SxUnregisterTexture)( SxTextureHandle tx );

//
// sxFormatTexture
//
// Sets a texture's pixel data format.  This will be the pixel format passed
//  to sxUpdateTextureRect.
// Invalidates existing contents, if any.
// 
typedef SxResult (*SxFormatTexture)( SxTextureHandle tex, SxTextureFormat format );

//
// sxSizeTexture
//
// Sets a texture's dimensions.  
// Invalidates existing contents, if any.
// 
typedef SxResult (*SxSizeTexture)( SxTextureHandle tex, unsigned int width, unsigned int height );

//
// sxClearTexture
//
// Clears a texture to a solid color.
//
// The given color value always has 8 bit per component, regardless of the
//  texture format.
// 
typedef SxResult (*SxClearTexture)( SxTextureHandle tex, SxColor color );

//
// sxUpdateTextureRect
//
// Updates a rectangular region of the texture with new pixel data.
// The input data is copied and may be discarded after the function returns.
// 
typedef SxResult (*SxUpdateTextureRect)( SxTextureHandle tx, unsigned int x, unsigned int y, unsigned int width, unsigned int height, unsigned int pitch, const void *data );

//
// sxRenderTextureSvg
//
// Renders 2D vector graphics into the texture from XML SVG commands passed via
//  a string, interpreted by the Skia library.
// The dimensions of the area rendered are derived from the SVG file.
// 
typedef SxResult (*SxRenderTextureSvg)( SxTextureHandle tx, unsigned int x, unsigned int y, const char *svg );

//
// sxRenderTextureJpeg
//
// Renders JPEG image data into the texture.
// The dimensions of the area rendered are derived from the JPEG header.
// 
typedef SxResult (*SxRenderTextureJpeg)( SxTextureHandle tx, unsigned int x, unsigned int y, const void *jpeg );

//
// sxPresentTexture
//
// Makes the texture visible, after all prior updates have completed.
// 
typedef SxResult (*SxPresentTexture)( SxGeometryHandle geo );

//
// Entities
//

//
// sxRegisterEntity
// sxUnregisterEntity
//
// Register/unregister an entity handle.  
// When initially registered, the entity has no geometry and will not draw.
// 
typedef SxResult (*SxRegisterEntity)( SxEntityHandle ent );
typedef SxResult (*SxUnregisterEntity)( SxEntityHandle ent );

//
// sxSetEntityGeometry
//
// Attaches geometry to an entity.
// Without geometry, the entity will not draw.
// 
typedef SxResult (*SxSetEntityGeometry)( SxEntityHandle ent, SxGeometryHandle geo );

//
// sxSetEntityTexture
//
// Attaches a texture to an entity.
// A texture is optional, without one the geometry will render as if it had a solid white texture attached.
// 
typedef SxResult (*SxSetEntityTexture)( SxEntityHandle ent, SxTextureHandle tx );

//
// sxOrientEntity
//
// Sets the orientation of an entity, with optional animation.
// 
typedef SxResult (*SxOrientEntity)( SxEntityHandle ent, const SxOrientation *o, const SxTrajectory *tr );

//
// sxSetEntityVisibility
//
// Sets the visibility (alpha) of an entity, with optional animation.
// 
typedef SxResult (*SxSetEntityVisibility)( SxEntityHandle ent, float visibility, const SxTrajectory *tr );

//
// sxParentEntity
//
// Sets the parent of an entity.  
// An entity's orientation is relative to the orientation of its parent.
// 
typedef SxResult (*SxParentEntity)( SxEntityHandle ent, SxEntityHandle parent );

// 
// Plugin interface
//

#define SX_PLUGIN_INTERFACE_VERSION     1

struct SxPluginInterface
{
    unsigned int                        version;
    SxRegisterPlugin                    registerPlugin;
    SxUnregisterPlugin                  unregisterPlugin;
    SxRegisterWidget                    registerWidget;
    SxUnregisterWidget                  unregisterWidget;
    SxBroadcastMessage                  broadcastMessage;
    SxSendMessage                       sendMessage;
    SxReceiveMessages                   receiveMessages;
    SxRegisterMessageListeners          registerMessageListeners;
    SxUnregisterMessageListeners        unregisterMessageListeners;
    SxRegisterGeometry                  registerGeometry;
    SxUnregisterGeometry                unregisterGeometry;
    SxSizeGeometry                      sizeGeometry;
    SxUpdateGeometryIndexRange          updateGeometryIndexRange;
    SxUpdateGeometryPositionRange       updateGeometryPositionRange;
    SxUpdateGeometryTexCoordRange       updateGeometryTexCoordRange;
    SxUpdateGeometryColorRange          updateGeometryColorRange;
    SxPresentGeometry                   presentGeometry;
    SxRegisterTexture                   registerTexture;
    SxUnregisterTexture                 unregisterTexture;
    SxFormatTexture                     formatTexture;
    SxSizeTexture                       sizeTexture;
    SxClearTexture                      clearTexture;
    SxUpdateTextureRect                 updateTextureRect;
    SxRenderTextureSvg                  renderTextureSvg;
    SxRenderTextureJpeg                 renderTextureJpeg;
    SxPresentTexture                    presentTexture;
    SxRegisterEntity                    registerEntity;
    SxUnregisterEntity                  unregisterEntity;
    SxSetEntityGeometry                 setEntityGeometry;
    SxSetEntityTexture                  setEntityTexture;
    SxOrientEntity                      orientEntity;
    SxSetEntityVisibility               setEntityVisibility;
    SxParentEntity                      parentEntity;
};

extern SxPluginInterface g_pluginInterface;

#endif
