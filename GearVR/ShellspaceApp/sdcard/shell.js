//
// Shellspace shell
// Author: Wade Brainerd <wadeb@wadeb.com>
//
include( 'shellspace.js' );
include( 'vector.js' );

PLUGIN   = 'shell'

try { unregisterPlugin( PLUGIN ); } catch (e) {}

registerPlugin( PLUGIN, SxPluginKind_Shell );

rootCell = { kind: 'empty' };
rootDepth = 40.0;
rootGutter = 5.0;

menuOpened = false;
activeCell = null;

gazeDir = Vec3.create( 0, 0, -1 );

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

	orientEntity( entity, {
		origin: [ origin[0], origin[1], origin[2] ],
		angles: [ -cell.lon, cell.lat, 0 ],
		scale:  [ 1, 1, 1 ]
	} );
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

	log( 'sendMessage ' + cell.widget + ' ' + cell.entity );

	var message = 'lat ' + cell.latArc + ' ' + cell.lonArc + ' ' + rootDepth;

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

function layoutCells_r( cell, lat, lon, latArc, lonArc )
{
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
	log( 'layoutCells' );
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

function findCellByEntity_r( cell, eid ) {
	if ( cell.kind == 'grid' ) {
		for ( var y = 0; y < cell.height; y++ ) {
			for ( var x = 0; x < cell.width; x++ ) {
				var child = getChild( cell, x, y );
				var result = findCellByEntity_r( child, eid );
				if ( result )
					return result;
			}
		}
	} else if ( cell.kind == 'widget' ) {
		if ( cell.entity == eid )
			return cell;
	}

	return null;
}

function findCellByEntity() {
	return findCellByEntity_r( rootCell );
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
		-0.9, -0.9, 0.0, 
		 0.9, -0.9, 0.0,
		-0.9,  0.9, 0.0, 
		 0.9,  0.9, 0.0, 
	] );

	var texCoords = new Float32Array( [
		-1.0, -1.0, 
		 1.0, -1.0,
		-1.0,  1.0, 
		 1.0,  1.0, 
		-0.9, -0.9, 
		 0.9, -0.9,
		-0.9,  0.9, 
		 0.9,  0.9, 
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

	if ( findCellByEntity( eid ) ) {
		log( 'Entity ' + eid + ' already registered' );
		return;
	}

	var cell = getActiveCell();
	if ( !cell ) {
		log( 'No active cell to place ' + eid + ' in' );
		return;
	}

	if ( cell.kind != 'empty' ) {
		log( 'Active cell is already full; can\'t place ' + eid );
		return;
	}	

	parentEntity( eid, 'root' );

	cell.kind = 'widget';
	cell.widget = wid;
	cell.entity = eid;

	layoutWidgets();
}

function unregisterCmd( args ) {
	var eid = args[1];

	var cell = findCellByEntity( eid );
	if ( !cell ) {
		log( 'Unable to find a cell containing entity ' + eid );
		return;
	} 

	parentEntity( eid, '' );

	cell.kind = 'empty';
}

function postToActiveWidget( args ) {
	var cell = getActiveCell();

	if ( !cell || cell.kind == 'empty' )
		return;

	assert( cell.kind == 'widget' );
	try { sendMessage( cell.widget, args.join( ' ' ) );	} catch ( e ) {}
}

function postToMenu( args ) {
	if ( args[0] != 'menu' )
		args.unshift( 'menu' );

	postMessage( args.join( ' ' ) );
}

function gazeCmd( args ) {
	if ( menuOpened ) {
		postToMenu( args );
	} else {
		gazeDir = Vec3.create( +(args[1]), +(args[2]), +(args[3]) );

		cell = getActiveCell();
		if ( cell )
			orientSquareToCell( cell, 'square' );

		postToActiveWidget( args );
	}
}

makeSquare();

try { unregisterEntity( 'root' ); } catch (e) {}
registerEntity( 'root' );

makeGrid( rootCell, 5, 3 );
makeGrid( getChild( rootCell, 2, 2 ), 4, 2 );

for ( ;; ) {
	var msg = receivePluginMessage( PLUGIN, 0 );
	var args = decodeMessage( msg );

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
