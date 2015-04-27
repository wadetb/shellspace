#ifndef __REGISTRY_H__
#define __REGISTRY_H__

enum ERegistry
{
	PLUGIN_REGISTRY,
	WIDGET_REGISTRY,
	GEOMETRY_REGISTRY,
	TEXTURE_REGISTRY,
	ENTITY_REGISTRY,
	REGISTRY_COUNT
};

#define BUFFER_COUNT 		3
#define ALL_BUFFERS_MASK	0x7

struct SPlugin
{
	SRefLink		poolLink;

	char			*id;

	SxPluginKind	kind;

	SRefLink		widgetList;
};

struct SWidget
{
	SRefLink		poolLink;

	char			*id;

	SRefLink		geometryList;
	SRefLink		textureList;
	SRefLink		entityList;
	
	SRefLink		pluginLink;
};

struct SGeometry
{
	SRefLink		poolLink;
	
	char			*id;
	
	SRefLink		widgetLink;

	uint 			vertexCount;
	uint 			indexCount;

	GLuint 			vertexArrayObjects[BUFFER_COUNT];
	GLuint 			vertexBuffers[BUFFER_COUNT];
	GLuint 			indexBuffers[BUFFER_COUNT];

	uint 			vertexCounts[BUFFER_COUNT];
	uint 			indexCounts[BUFFER_COUNT];

	byte 			updateIndex;
	byte 			drawIndex;

	uint 			presentFrame;
};

struct STexture
{
	SRefLink		poolLink;
	
	char			*id;
	
	SRefLink		widgetLink;

	SxTextureFormat	format;
	ushort 			width;
	ushort 			height;

	GLuint 			texId[BUFFER_COUNT];
	ushort 			texWidth[BUFFER_COUNT];
	ushort			texHeight[BUFFER_COUNT];

	byte 			updateIndex;
	byte 			drawIndex;

	uint 			presentFrame;
};

struct SEntity
{
	SRefLink		poolLink;
	SRefLink		activeLink;
	
	char			*id;
	
	SRefLink		widgetLink;
	
	SRef 			geometryRef;
	SRef 			textureRef;
	
	SxOrientation	orientation;
	float 			visibility;

	SRef 			parentRef;
	SRefLink		parentLink;
	SRef 			firstChild;
};

void Registry_Init();
void Registry_Shutdown();

SRef Registry_Register( ERegistry reg, const char *id );
void Registry_Unregister( ERegistry reg, SRef ref );
uint Registry_GetCount( ERegistry reg );

#define Registry_RefForIndex( index ) (SRef)( 1 + (index) )

SRef Registry_GetPluginRef( const char *id );
SRef Registry_GetWidgetRef( const char *id );
SRef Registry_GetGeometryRef( const char *id );
SRef Registry_GetTextureRef( const char *id );
SRef Registry_GetEntityRef( const char *id );

SPlugin *Registry_GetPlugin( SRef ref );
SWidget *Registry_GetWidget( SRef ref );
SGeometry *Registry_GetGeometry( SRef ref );
STexture *Registry_GetTexture( SRef ref );
SEntity *Registry_GetEntity( SRef ref );

SRef Registry_GetEntityRefByPointer( SEntity *entity );

#endif
