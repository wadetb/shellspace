#include "common.h"
#include "shellplugin.h"
#include "keyboard.h"
#include "message.h"


#define ROOT_DEPTH 			40.0f
#define ROOT_LAT_ARC 		120
#define ROOT_LON_ARC 		360
#define ROOT_GRID_WIDTH 	5
#define ROOT_GRID_HEIGHT 	3

#define ROOT_GUTTER 		5


enum ECellKind
{
	CELL_EMPTY,
	CELL_GRID,
	CELL_WIDGET
};


struct SCell
{
	ECellKind 		kind;
	float 			lat;
	float 			lon;
	float 			latArc;
	float 			lonArc;
	union 
	{
		struct
		{
			uint 			width;
			uint 			height;
			SCell 			*children;
		} grid;
		struct
		{
			SxWidgetHandle 	wid;
			SxEntityHandle 	eid;
		} widget;
	};
};


struct SShellGlobals
{
	SCell 			root;
	SxOrientation	rootOrient;

	SxVector3 		gazeDir;

	pthread_t 		pluginThread;

	SxWidgetHandle 	active;
};


SShellGlobals s_shell;


void Shell_InitCell( SCell *cell )
{
	memset( cell, 0, sizeof( SCell ) );
}


void Shell_ClearCell( SCell *cell )
{
	uint childIter;

	assert( cell );

	if ( cell->kind == CELL_GRID )
	{
		assert( cell->grid.children );
		assert( cell->grid.width );
		assert( cell->grid.height );

		for ( childIter = 0; childIter < cell->grid.width * cell->grid.height; childIter++ )
			Shell_ClearCell( &cell->grid.children[childIter] );

		free( cell->grid.children );
	}
	else if ( cell->kind == CELL_WIDGET )
	{
		assert( cell->widget.wid );
		assert( cell->widget.eid );

		free( (char *)cell->widget.wid );
		free( (char *)cell->widget.eid );
	}

	memset( cell, 0, sizeof( SCell ) );
}


void Shell_MakeGrid( SCell *cell, uint width, uint height )
{
	Shell_ClearCell( cell );

	memset( cell, 0, sizeof( SCell ) );

	cell->kind = CELL_GRID;

	cell->grid.width = width;
	cell->grid.height = height;

	cell->grid.children = (SCell *)malloc( width * height * sizeof( SCell ) );
	memset( cell->grid.children, 0, width * height * sizeof( SCell ) );
}


void Shell_CopyGrid( SCell *from, SCell *to )
{
	assert( from );
	assert( to );

	Shell_ClearCell( to );
}


SCell *Shell_GetGridChild( SCell *cell, uint x, uint y )
{
	int 	index;

	assert( cell );
	assert( cell->kind == CELL_GRID );
	assert( cell->grid.children );
	assert( x < cell->grid.width );
	assert( y < cell->grid.height );

	index = y * cell->grid.width + x;

	return &cell->grid.children[index];
}


void Shell_SetGridChild( SCell *cell, uint x, uint y, SCell *child )
{
	int 	index;
	SCell 	*oldChild;

	assert( cell );
	assert( cell->kind == CELL_GRID );
	assert( cell->grid.children );
	assert( x < cell->grid.width );
	assert( y < cell->grid.height );

	index = y * cell->grid.width + x;

	oldChild = &cell->grid.children[index];
	Shell_ClearCell( oldChild );

	cell->grid.children[index] = *child;
}


void Shell_ResizeGrid( SCell *cell, uint width, uint height )
{
	SCell	newCell;
	uint 	x;
	uint 	y;
	SCell 	*child;

	assert( cell );
	assert( cell->kind == CELL_GRID );

	Shell_InitCell( &newCell );
	Shell_MakeGrid( &newCell, width, height );

	for ( y = 0; y < height; y++ )
	{
		if ( y >= cell->grid.height )
			break;

		for ( x = 0; x < width; x++ )
		{
			if ( x >= cell->grid.width )
				break;

			child = Shell_GetGridChild( cell, x, y );

			Shell_SetGridChild( &newCell, x, y, child );
		}
	}
}


void Shell_GetCellPoint( float lat, float lon, float depth, SxVector3 *out )
{
	float cu;
	float cv;
	float su;
	float sv;

	S_SinCos( S_degToRad( lon ), &su, &cu );
	S_SinCos( S_degToRad( lat ), &sv, &cv );

	Vec3Set( out, depth * su, depth * sv, -depth * cu * cv );
}


void Shell_OrientEntityToCell( SCell *cell, SxEntityHandle eid )
{
	SxOrientation	orient;
	SxTrajectory 	tr;

	IdentityOrientation( &orient );

	Shell_GetCellPoint( cell->lat, cell->lon, ROOT_DEPTH, &orient.origin );

	// $$$ Grrrh, these are reversed.
	orient.angles.pitch = -cell->lon;
	orient.angles.yaw = cell->lat;

	tr.kind = SxTrajectoryKind_Instant;

	g_pluginInterface.orientEntity( eid, &orient, &tr );
}


void Shell_OrientSquareToCell( SCell *cell, const char *eid )
{
	SxOrientation	orient;
	float 			lat0;
	float 			lat1;
	float 			lon0;
	float 			lon1;
	SxVector3 		p0;
	SxVector3 		p1;
	SxVector3 		p2;
	float 			uSize;
	float 			vSize;
	SxTrajectory 	tr;

	IdentityOrientation( &orient );

	lat0 = cell->lat - cell->latArc * 0.5f;
	lat1 = cell->lat + cell->latArc * 0.5f;

	lon0 = cell->lon - cell->lonArc * 0.5f;
	lon1 = cell->lon + cell->lonArc * 0.5f;

	Shell_GetCellPoint( lat0, lon0, ROOT_DEPTH, &p0 );
	Shell_GetCellPoint( lat0, lon1, ROOT_DEPTH, &p1 );
	Shell_GetCellPoint( lat1, lon0, ROOT_DEPTH, &p2 );

	Vec3Avg( p1, p2, &orient.origin );

	// Assumes square is modeled in -1..1 coords.
	uSize = Vec3Distance( p0, p1 ) * 0.5f;
	vSize = Vec3Distance( p0, p2 ) * 0.5f;

	Vec3Set( &orient.scale, uSize, vSize, 1.0f );

	// $$$ Grrrh, these are reversed.
	orient.angles.pitch = -cell->lon;
	orient.angles.yaw = cell->lat;

	tr.kind = SxTrajectoryKind_Instant;

	g_pluginInterface.orientEntity( eid, &orient, &tr );
}


void Shell_LayoutWidget( SCell *cell )
{
	char 	msgBuf[MSG_LIMIT];

	assert( cell->kind = CELL_WIDGET );
	assert( cell->widget.eid );

	Shell_OrientEntityToCell( cell, cell->widget.eid );

	snprintf( msgBuf, MSG_LIMIT, "arc %f %f %f", cell->latArc, cell->lonArc, ROOT_DEPTH );

	g_pluginInterface.sendMessage( cell->widget.wid, msgBuf );
}


void Shell_LayoutWidgets_r( SCell *cell )
{
	uint 			x;
	uint 			y;
	SCell 			*child;

	if ( cell->kind == CELL_GRID )
	{
		assert( cell->grid.width );
		assert( cell->grid.height );
		assert( cell->grid.children );

		for ( y = 0; y < cell->grid.height; y++ )
		{
			for ( x = 0; x < cell->grid.width; x++ )
			{
				child = Shell_GetGridChild( cell, x, y );

				Shell_LayoutWidgets_r( child );
			}
		}
	}
	else if ( cell->kind == CELL_WIDGET )
	{
		Shell_LayoutWidget( cell );
	}
}


void Shell_LayoutWidgets()
{
	Shell_LayoutWidgets_r( &s_shell.root );
}


void Shell_LayoutCells_r( SCell *cell, float lat, float lon, float latArc, float lonArc )
{
	uint 			x;
	uint 			y;
	float 			childLatArc;
	float 			childLonArc;
	float 			childLat;
	float 			childLon;
	SCell 			*child;

	cell->lat = lat;
	cell->lon = lon;
	cell->latArc = latArc - ROOT_GUTTER;
	cell->lonArc = lonArc - ROOT_GUTTER;

	if ( cell->kind == CELL_GRID )
	{
		assert( cell->grid.width );
		assert( cell->grid.height );
		assert( cell->grid.children );

		childLonArc = lonArc / (float)cell->grid.width;
		childLatArc = latArc / (float)cell->grid.height;

		childLat = lat - (latArc * 0.5f) + (childLatArc * 0.5f);

		for ( y = 0; y < cell->grid.height; y++ )
		{
			childLon = lon - (lonArc * 0.5f) + (childLonArc * 0.5f);

			for ( x = 0; x < cell->grid.width; x++ )
			{
				child = Shell_GetGridChild( cell, x, y );

				Shell_LayoutCells_r( child, childLat, childLon, childLatArc, childLonArc );

				childLon += childLonArc;
			}

			childLat += childLatArc;
		}
	}
}


void Shell_LayoutCells()
{
	Shell_LayoutCells_r( &s_shell.root, 0, 0, ROOT_LAT_ARC, ROOT_LON_ARC );
}


SCell *Shell_Raycast_r( SCell *cell, float targetLat, float targetLon )
{
	uint 			x;
	uint 			y;
	SCell 			*child;

	if ( cell->kind == CELL_GRID )
	{
		assert( cell->grid.width );
		assert( cell->grid.height );
		assert( cell->grid.children );

		for ( y = 0; y < cell->grid.height; y++ )
		{
			for ( x = 0; x < cell->grid.width; x++ )
			{
				child = Shell_GetGridChild( cell, x, y );

				if ( fabsf( targetLat - child->lat ) <= child->latArc * 0.5f &&
				     fabsf( targetLon - child->lon ) <= child->lonArc * 0.5f )
				{
					return Shell_Raycast_r( child, targetLat, targetLon );
				}
			}
		}

		return NULL;
	}

	return cell;
}


SCell *Shell_Raycast( float targetLat, float targetLon )
{
	return Shell_Raycast_r( &s_shell.root, targetLat, targetLon );
}


SCell *Shell_GetActiveCell()
{
	float targetLat;
	float targetLon;

	assert( Vec3LengthSqr( s_shell.gazeDir ) > 0.0f );

	Shell_LayoutCells();

	targetLat = S_radToDeg( atan2f( s_shell.gazeDir.y, -s_shell.gazeDir.z ) );
	targetLon = S_radToDeg( atan2f( s_shell.gazeDir.x, -s_shell.gazeDir.z ) );

	return Shell_Raycast( targetLat, targetLon );
}


SCell *Shell_FindCellByEntity_r( SCell *cell, SxEntityHandle eid )
{
	uint 			childCount;
	uint 			childIter;
	SCell 			*child;
	SCell 			*result;

	if ( cell->kind == CELL_GRID )
	{
		assert( cell->grid.children );

		childCount = cell->grid.width * cell->grid.height;

		for ( childIter = 0; childIter < childCount; childIter++ )
		{
			child = &cell->grid.children[childIter];

			result = Shell_FindCellByEntity_r( child, eid );
			if ( result )
				return result;
		}
	}
	else if ( cell->kind == CELL_WIDGET )
	{
		if ( S_strcmp( cell->widget.eid, eid ) == 0 )
			return cell;
	}

	return NULL;
}


SCell *Shell_FindCellByEntity( SxEntityHandle eid )
{
	return Shell_FindCellByEntity_r( &s_shell.root, eid );
}


void Shell_CreateRoot()
{
	// SxTrajectory 	tr;

	IdentityOrientation( &s_shell.rootOrient );

	// Standard 3 row layout.
	Shell_MakeGrid( &s_shell.root, ROOT_GRID_WIDTH, ROOT_GRID_HEIGHT );

	// Divide the top middle cell.
	Shell_MakeGrid( Shell_GetGridChild( &s_shell.root, 2, 2 ), 4, 2 );

	Shell_LayoutCells();

	// tr.kind = SxTrajectoryKind_Instant;

	// g_pluginInterface.registerEntity( "root" );
	// g_pluginInterface.orientEntity( "root", &s_shell.rootOrient, &tr );
}


void Shell_CreateSquare()
{
	// 0                     1
	//  4                   5
	//  6                   7
	// 2                     3
	ushort indices[] = 
	{ 
		0, 1, 4, 4, 1, 5, 
		6, 7, 2, 2, 7, 3,
		0, 4, 2, 2, 4, 6,
		5, 1, 7, 7, 1, 3 
	};

	float positions[] = 
	{ 
		-1.0f, -1.0f, 0.0f, 
		 1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f, 
		 1.0f,  1.0f, 0.0f, 
		-0.9f, -0.9f, 0.0f, 
		 0.9f, -0.9f, 0.0f,
		-0.9f,  0.9f, 0.0f, 
		 0.9f,  0.9f, 0.0f, 
	};

	float texCoords[] = 
	{ 
		-1.0f, -1.0f, 
		 1.0f, -1.0f,
		-1.0f,  1.0f, 
		 1.0f,  1.0f, 
		-0.9f, -0.9f, 
		 0.9f, -0.9f,
		-0.9f,  0.9f, 
		 0.9f,  0.9f, 
	};

	byte colors[] = 
	{
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
	};

	g_pluginInterface.registerGeometry( "square" );
	g_pluginInterface.sizeGeometry( "square", 8, 24 );
	g_pluginInterface.updateGeometryIndexRange( "square", 0, 24, indices );
	g_pluginInterface.updateGeometryPositionRange( "square", 0, 8, (SxVector3 *)positions );
	g_pluginInterface.updateGeometryTexCoordRange( "square", 0, 8, (SxVector2 *)texCoords );
	g_pluginInterface.updateGeometryColorRange( "square", 0, 8, (SxColor *)colors );
	g_pluginInterface.presentGeometry( "square" );

	g_pluginInterface.registerEntity( "square" );
	g_pluginInterface.setEntityGeometry( "square", "square" );
	g_pluginInterface.setEntityTexture( "square", "white" );
}


void Shell_RegisterCmd( const SMsg *msg, void *context )
{
	SxWidgetHandle 	wid;
	SxEntityHandle 	eid;
	SCell 			*cell;

	wid = Msg_Argv( msg, 1 );
	eid = Msg_Argv( msg, 2 );

	if ( Shell_FindCellByEntity( eid ) )
	{
		LOG( "Shell_RegisterCmd: Entity %s is already registered.", eid );
		return;
	}

	g_pluginInterface.parentEntity( eid, "root" );

	cell = Shell_GetActiveCell();
	if ( !cell )
	{
		LOG( "Shell_RegisterCmd: No active cell to place %s in.", eid );
		return;
	}

	if ( cell->kind != CELL_EMPTY )
	{
		LOG( "Shell_RegisterCmd: Active cell is already full, can't place %s.", eid );
		return;
	}

	cell->kind = CELL_WIDGET;

	assert( cell->widget.wid == NULL );
	assert( cell->widget.eid == NULL );

	cell->widget.wid = strdup( wid );
	cell->widget.eid = strdup( eid );

	LOG( "Register %s with gazeDir %f %f %f", eid, s_shell.gazeDir.x, s_shell.gazeDir.y, s_shell.gazeDir.z );

	Shell_LayoutWidgets();
}


void Shell_UnregisterCmd( const SMsg *msg, void *context )
{
	SxEntityHandle 	eid;
	SCell 			*cell;

	eid = Msg_Argv( msg, 1 );

	g_pluginInterface.parentEntity( eid, "" );

	cell = Shell_FindCellByEntity( eid );
	if ( !cell )
	{
		LOG( "Shell_RegisterCmd: Unable to find a cell containing %s.", eid );
		return;
	}

	assert( cell->widget.wid );
	assert( cell->widget.eid );

	free( (char *)cell->widget.wid );
	free( (char *)cell->widget.eid );

	cell->widget.wid = NULL;
	cell->widget.eid = NULL;

	cell->kind = CELL_EMPTY;
}


void Shell_MoveCmd( const SMsg *msg, void *context )
{
}


void Shell_KeyCmd( const SMsg *msg, void *context )
{
	// uint 	code;
	// uint 	down;
	SCell 	*cell;
	char 	msgBuf[MSG_LIMIT];

	// $$$ should key event include modifier state?  probably....

	// code = atoi( Msg_Argv( msg, 1 ) );
	// down = atoi( Msg_Argv( msg, 1 ) );

	cell = Shell_GetActiveCell();
	if ( !cell )
		return;

	if ( cell->kind == CELL_EMPTY )
	{
		// Keyboard_Show( "launcher.vrkey" );
		// $$$ orient launcher to cell?
		return;
	}
	else
	{
		assert( cell->kind == CELL_WIDGET );
		
		Msg_Format( msg, msgBuf, MSG_LIMIT );

		LOG( "%s <- %s", cell->widget.wid, msgBuf );

		g_pluginInterface.sendMessage( cell->widget.wid, msgBuf );
	}
}


void Shell_GazeCmd( const SMsg *msg, void *context )
{
	SCell *activeCell;

	s_shell.gazeDir.x = atof( Msg_Argv( msg, 1 ) );
	s_shell.gazeDir.y = atof( Msg_Argv( msg, 2 ) );
	s_shell.gazeDir.z = atof( Msg_Argv( msg, 3 ) );

	activeCell = Shell_GetActiveCell();
	if ( activeCell )
		Shell_OrientSquareToCell( activeCell, "square" );
}


SMsgCmd s_shellCmds[] =
{
	{ "register", 		Shell_RegisterCmd, 			"register <wid> <eid>" },
	{ "unregister", 	Shell_UnregisterCmd, 		"unregister <wid>" },
	{ "move", 			Shell_MoveCmd, 				"move <wid> <cell>" },
	{ "key", 			Shell_KeyCmd, 				"key <code> <down>" },
	{ "gaze", 			Shell_GazeCmd, 				"gaze <x> <y> <z>" },
	{ NULL, NULL, NULL }
};


static void *Shell_PluginThread( void *context )
{
	char 	msgBuf[MSG_LIMIT];
	SMsg 	msg;

	pthread_setname_np( pthread_self(), "Shell" );

	g_pluginInterface.registerPlugin( "shell", SxPluginKind_Shell );

	for ( ;; )
	{
		g_pluginInterface.receivePluginMessage( "shell", SX_WAIT_INFINITE, msgBuf, MSG_LIMIT );

		Msg_ParseString( &msg, msgBuf );
		if ( Msg_Empty( &msg ) )
			continue;

		if ( Msg_IsArgv( &msg, 0, "shell" ) )
			Msg_Shift( &msg, 1 );

		if ( Msg_IsArgv( &msg, 0, "unload" ) )
			break;

		MsgCmd_Dispatch( &msg, s_shellCmds, NULL );
	}

	g_pluginInterface.unregisterPlugin( "shell" );

	return 0;
}


void Shell_InitPlugin()
{
	int err;

	Shell_CreateRoot();
	Shell_CreateSquare();

	Vec3Set( &s_shell.gazeDir, 0.0f, 0.0f, -1.0f );

	err = pthread_create( &s_shell.pluginThread, NULL, Shell_PluginThread, NULL );
	if ( err != 0 )
		FAIL( "Shell_InitPlugin: pthread_create returned %i", err );
}
