//
// Shellspace shell
// Author: Wade Brainerd <wadeb@wadeb.com>
//
include( 'shellspace.js' );
include( 'vector.js' );

PLUGIN   = 'shell'

try { unregisterPlugin( PLUGIN ); } catch (e) {}

registerPlugin( PLUGIN, SxPluginKind_Shell );

var rootCell = { kind: 'empty' };
var rootDepth = 40.0;
var rootGutter = 5.0;

var menuOpened = false;
var activeId = 'none';
var squareCell = null;

var gazeDir = Vec3.create( 0, 0, -1 );

function radToDeg( v ) {
	return v * 180.0 / Math.PI;
}

function degToRad( v ) {
	return v * Math.PI / 180.0;
}

function getChild( cell, x, y ) {
	assert( cell.kind == 'grid' );
	return cell.children[x + y*cell.width];
}

function dump_r( cell, depth ) {
	log( 'kind:' + cell.kind + ' lat:' + cell.lat + ' lon:' + cell.lon + ' latArc:' + cell.latArc + ' lonArc:' + cell.lonArc );

	if ( cell.kind == 'grid' ) {
		for ( var y = 0; y < cell.height; y++ ) {
			for ( var x = 0; x < cell.width; x++ ) {
				var s = '';
				for ( var i = 0; i < depth; i++ )
					s += ' ';
				s += ' x:' + x + ' y:' + y;
				log( s );

				var child = getChild( cell, x, y );
				dump_r( child, depth + 1 );
			}
		}
	} else if ( cell.kind == 'widget' ) {
		log( 'widget:' + cell.widget + ' entity:' + cell.entity );
	}
}

function dump() {
	return dump_r( rootCell, 1 );
}

function makeGrid( cell, width, height ) {
	cell.kind = 'grid';
	cell.width = width;
	cell.height = height;
	cell.children = []

	for ( var i = 0; i < width * height; i++ )
		cell.children.push( { kind: 'empty' } );

	return cell;
}

function getCellPoint( lat, lon, depth ) {
	var cu = Math.cos( degToRad( lon ) );
	var su = Math.sin( degToRad( lon ) );
	var cv = Math.cos( degToRad( lat ) );
	var sv = Math.sin( degToRad( lat ) );

	return Vec3.create( depth * su, depth * sv, -depth * cu * cv );
}

function orientEntityToCell( cell, entity ) {
	var origin = getCellPoint( cell.lat, cell.lon, rootDepth );

	try {
		orientEntity( entity, {
			origin: [ origin[0], origin[1], origin[2] ],
			angles: [ cell.lat, -cell.lon, 0 ],
			scale:  [ 1, 1, 1 ]
		} );
	} catch ( e ) {
	}
}

function orientSquareToCell( cell, entity ) {
	var lat0 = cell.lat - cell.latArc * 0.5;
	var lat1 = cell.lat + cell.latArc * 0.5;

	var lon0 = cell.lon - cell.lonArc * 0.5;
	var lon1 = cell.lon + cell.lonArc * 0.5;

	var p0 = getCellPoint( lat0, lon0, rootDepth );
	var p1 = getCellPoint( lat0, lon1, rootDepth );
	var p2 = getCellPoint( lat1, lon0, rootDepth );

	var origin = Vec3.create( 0, 0, 0 );
	Vec3.avg( p1, p2, origin );

	var uSize = Vec3.distance( p0, p1 ) * 0.5;
	var vSize = Vec3.distance( p0, p2 ) * 0.5;

	orientEntity( entity, {
		origin: [ origin[0], origin[1], origin[2] ],
		angles: [ cell.lat, -cell.lon, 0 ],
		scale:  [ uSize, vSize, 1.0 ] 
	});
}

function layoutWidget( cell ) {
	orientEntityToCell( cell, cell.entity );

	// log( 'sendMessage ' + cell.widget + ' ' + cell.entity );

	var message = 'arc ' + cell.latArc + ' ' + cell.lonArc + ' ' + rootDepth;

	log( cell.widget + ' ' + message );

	try { sendMessage( cell.widget, message ); } catch ( e ) {}
}

function layoutWidgets_r( cell ) {
	if ( cell.kind == 'grid' ) {
		for ( var y = 0; y < cell.height; y++ ) {
			for ( var x = 0; x < cell.width; x++ ) {
				var child = getChild( cell, x, y );
				layoutWidgets_r( child );
			}
		}
	} else if ( cell.kind == 'widget' ) {
		layoutWidget( cell );
	}
}

function layoutWidgets() {
	layoutWidgets_r( rootCell );
}

function layoutCells_r( cell, lat, lon, latArc, lonArc ) {
	cell.lat = lat;
	cell.lon = lon;
	cell.latArc = latArc - rootGutter;
	cell.lonArc = lonArc - rootGutter;

	if ( cell.kind == 'grid' ) {
		var childLonArc = lonArc / cell.width;
		var childLatArc = latArc / cell.height;

		var childLat = lat - (latArc * 0.5) + (childLatArc * 0.5);

		for ( var y = 0; y < cell.height; y++ ) {
			var childLon = lon - (lonArc * 0.5) + (childLonArc * 0.5);

			for ( var x = 0; x < cell.width; x++ ) {
				var child = getChild( cell, x, y );

				layoutCells_r( child, childLat, childLon, childLatArc, childLonArc );

				childLon += childLonArc;
			}

			childLat += childLatArc;
		}
	}
}

function layoutCells() {
	layoutCells_r( rootCell, 0, 0, 120, 360 );
}

function rayCast_r( cell, targetLat, targetLon ) {
	if ( cell.kind == 'grid' ) {
		for ( var y = 0; y < cell.height; y++ ) {
			for ( var x = 0; x < cell.width; x++ ) {
				var child = getChild( cell, x, y );

				// log( 'rayCast child ' + cell.kind + ' x' + x + ' y' + y );
				// log( 'lat ' + child.lat + ' lon' + child.lon );
				// log( 'latArc ' + child.latArc + ' lonArc' + child.lonArc );

				if ( Math.abs( child.lat - targetLat ) <= child.latArc * 0.5 &&
					 Math.abs( child.lon - targetLon ) <= child.lonArc * 0.5 )
					return rayCast_r( child, targetLat, targetLon );
			}
		}

		return null;
	}

	return cell;
}

function rayCast( targetLat, targetLon ) {
	return rayCast_r( rootCell, targetLat, targetLon );
}

function getActiveCell() {
	assert( Vec3.lengthSqr( gazeDir ) > 0.0 );

	layoutCells();

	var targetLat = radToDeg( Math.atan2( gazeDir[1], -gazeDir[2] ) );
	var targetLon = radToDeg( Math.atan2( gazeDir[0], -gazeDir[2] ) );

	// log( "getActiveCell targetLat:" + targetLat + " targetLon:" + targetLon );

	return rayCast( targetLat, targetLon );
}

function findCellByWidget_r( cell, wid ) {
	if ( cell.kind == 'grid' ) {
		for ( var y = 0; y < cell.height; y++ ) {
			for ( var x = 0; x < cell.width; x++ ) {
				var child = getChild( cell, x, y );

				var result = findCellByWidget_r( child, wid );
				if ( result )
					return result;
			}
		}
	} else if ( cell.kind == 'widget' ) {
		if ( cell.widget == wid ) {
			return cell;
		}
	}

	return null;
}

function findCellByWidget( wid ) {
	return findCellByWidget_r( rootCell, wid );
}

function makeSquare() {
	var indices = new Uint16Array( [
		0, 1, 4, 4, 1, 5, 
		6, 7, 2, 2, 7, 3,
		0, 4, 2, 2, 4, 6,
		5, 1, 7, 7, 1, 3 
	] );

	var positions = new Float32Array( [
		-1.0, -1.0, 0.0, 
		 1.0, -1.0, 0.0,
		-1.0,  1.0, 0.0, 
		 1.0,  1.0, 0.0, 
		-0.975, -0.975, 0.0, 
		 0.975, -0.975, 0.0,
		-0.975,  0.975, 0.0, 
		 0.975,  0.975, 0.0, 
	] );

	var texCoords = new Float32Array( [
		0, 0, 
		0, 0,
		0, 0, 
		0, 0, 
		0, 0, 
		0, 0,
		0, 0, 
		0, 0, 
	] );

	var colors = new Uint8Array( [
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
		255, 255, 255, 255, 
	] );

	try { unregisterGeometry( 'square' ); } catch (e) {}
	try { unregisterEntity( 'square' ); } catch (e) {}

	registerGeometry( 'square' );
	sizeGeometry( 'square', 8, 24 );
	updateGeometryIndexRange( 'square', 0, 24, indices );
	updateGeometryPositionRange( 'square', 0, 8, positions );
	updateGeometryTexCoordRange( 'square', 0, 8, texCoords );
	updateGeometryColorRange( 'square', 0, 8, colors );
	presentGeometry( 'square' );

	registerEntity( 'square' );
	setEntityGeometry( 'square', 'square' );
	setEntityTexture( 'square', 'white' );
}

function registerCmd( args ) {
	var wid = args[1];
	var eid = args[2];

	if ( findCellByWidget( wid ) ) {
		log( 'Widget ' + wid + ' already registered' );
		return;
	}

	var cell = getActiveCell();
	if ( !cell ) {
		log( 'No active cell to place ' + wid + ' in' );
		return;
	}

	if ( cell.kind != 'empty' ) {
		log( 'Active cell is already full; can\'t place ' + wid );
		return;
	}

	// parentEntity( eid, 'root' );

	log( 'registerCmd: wid:' + wid + ' eid:' + eid );

	cell.kind = 'widget';
	cell.widget = wid;
	cell.entity = eid;

	layoutWidgets();

	refreshActiveCell();

	// $$$ VNC hack to alter defaults after the widget spawns.
	if ( wid.match( /vnc/ ) ) {
		sendMessage( wid, wid + ' zpush 8' );
	}
}

function unregisterCmd( args ) {
	var wid = args[1];

	log( 'unregisterCmd: wid:' + wid );

	// $$$ VNC hack to deal w/the bug where the VNC thread exit
	// is never detected by VNC_Destroy, so the VNC plugin thread freezes.
	// Instead, just unregister the entity and pretend it's working.
	if ( wid.match( /vnc/ ) ) {
		log( 'VNC shutdown hack on ' + wid );
		try { unregisterWidget( wid ); } catch ( e ) {}
		try { unregisterEntity( wid ); } catch ( e ) {}
		try { unregisterTexture( wid ); } catch ( e ) {}
		try { unregisterGeometry( wid ); } catch ( e ) {}
	}

	var cell = findCellByWidget( wid );
	if ( !cell ) {
		log( 'Unable to find a cell containing widget ' + wid );
		dump();
		return;
	}

	// parentEntity( eid, '' );

	cell.kind = 'empty';

	refreshActiveCell();
}

function postToActiveWidget( args ) {
	var cell = getActiveCell();

	if ( !cell || cell.kind == 'empty' )
		return;

	// log( 'postToActiveWidget: ' + cell.widget + ' < ' + args.join( ' ' ) );

	assert( cell.kind == 'widget' );
	try { sendMessage( cell.widget, args.join( ' ' ) );	} catch ( e ) {}
}

function postToMenu( args ) {
	if ( args[0] != 'menu' )
		args.unshift( 'menu' );

	postMessage( args.join( ' ' ) );
}

function refreshActiveCell() {
	cell = getActiveCell();

	if ( cell && cell != squareCell ) {
		squareCell = cell;
		orientSquareToCell( squareCell, 'square' );
	}

	// $$$ To make this more useful to the menu, it might be desireable to register the
	//     plugin name in addition to the widget name?  For now the menu just has
	//     hardcoded entity string matching.  Alternately we could enforce that 
	//     entity ID include widget and plugin ID as prefix tokens.
	var newActiveId;
	if ( cell && cell.kind == 'empty' ) 
		newActiveId = 'empty';
	else if ( cell && cell.kind == 'widget' ) 
		newActiveId = cell.widget;
	else
		newActiveId = 'none';

	if ( newActiveId != activeId ) {
		activeId = newActiveId;
		postMessage( 'menu activate ' + activeId );
	}
}

function gazeCmd( args ) {
	if ( menuOpened ) {
		postToMenu( args );
	} else {
		gazeDir = Vec3.create( +(args[1]), +(args[2]), +(args[3]) );

		postToActiveWidget( args );

		refreshActiveCell();
	}
}

makeSquare();

try { unregisterEntity( 'root' ); } catch (e) {}
registerEntity( 'root' );

makeGrid( rootCell, 5, 3 );
makeGrid( getChild( rootCell, 2, 2 ), 4, 2 );

layoutCells();
// dump();

refreshActiveCell();

for ( ;; ) {
	var msg = receivePluginMessage( PLUGIN, 0 );

	if ( !msg ) // $$$ Not sure why this is happening.
		break;

	var args = decodeMessage( msg );

	if ( !msg.match( /gaze/ ) ) {
		log( 'shell.js: ' + args.join( ' ' ) );
	}

	if ( args[0] == PLUGIN )
		args.shift();

	var command = args[0];
	if ( command == 'unload' ) {
		break;
	}
	
	switch ( command ) {
		case 'register':
			registerCmd( args );
			break;

		case 'unregister':
			unregisterCmd( args );
			break;

		case 'gaze':
			gazeCmd( args );
			break;

		case 'key':
		case 'swipe':
		case 'tap':
			if ( menuOpened )
				postToMenu( args );
			else
				postToActiveWidget( args );
			break;

		case 'menu': 
			switch( args[1] ) {
				case 'open':
					if ( menuOpened ) {
						postMessage( 'menu close' );
					} else {
						menuOpened = true;

						postToMenu( args );
						postToActiveWidget( args );
					}
					break;

				case 'closed':
					menuOpened = false;
					break;
			}
			break;
	}
}
