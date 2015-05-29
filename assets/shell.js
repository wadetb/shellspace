//
// Shellspace shell
// Author: Wade Brainerd <wadeb@wadeb.com>
//
include( 'shellspace.js' );
include( 'vector.js' );
include( 'sprintf.js' );
include( 'gl-matrix.js' );

PLUGIN   = 'shell'

try { unregisterPlugin( PLUGIN ); } catch (e) {}

registerPlugin( PLUGIN, SxPluginKind_Shell );

var rootCell = null;
var rootDepth = 40.0;

var menuOpened = false;
var activeCell = null;
var squareCell = null;

var gazeDir = Vec3.create( 0, 0, -1 );

function radToDeg( v ) {
	return v * 180.0 / Math.PI;
}

function degToRad( v ) {
	return v * Math.PI / 180.0;
}

function dump_r( cell, depth ) {
	log( 'kind:' + cell.kind + ' lat:' + cell.lat + ' lon:' + cell.lon + ' latArc:' + cell.latArc + ' lonArc:' + cell.lonArc );

	if ( cell.kind == 'root' ) {
		for ( var i = 0; y < cell.children.length; i++ ) {
			var s = '';
			for ( var i = 0; i < depth; i++ )
				s += ' ';
			s += ' i:' + i;
			log( s );

			dump_r( cell.children[i], depth + 1 );
		}
	} else if ( cell.kind == 'widget' ) {
		log( 'widget:' + cell.widget + ' entity:' + cell.entity );
	}
}

function dump() {
	return dump_r( rootCell, 1 );
}

function orientEntityToCell( cell, entity, properties ) {
	properties = properties || cell.properties;

	if ( properties.placement == 'center' ) {
		var origin = vec3.fromValues( 0, 0, -rootDepth );
		vec3.transformMat4( origin, origin, cell.xform );

		// log( 'orientEntityToCell: center' );
		// log( 'origin: ' + vec3.str( origin ) );

		orientEntity( entity, {
			origin: [ origin[0], origin[1], origin[2] ],
			angles: [ cell.lat, cell.lon, 0 ],
			scale:  [ 1, 1, 1 ]
		} );
	} else if ( properties.placement == 'fill' ) {
		var origin = vec3.fromValues( 0, 0, -rootDepth );
		vec3.transformMat4( origin, origin, cell.xform );

		var uSize = rootDepth * Math.sin( degToRad( cell.lonArc * 0.5 ) );
		var vSize = rootDepth * Math.sin( degToRad( cell.latArc * 0.5 ) );

		// log( 'orientEntityToCell: fill' );
		// log( 'origin: ' + vec3.str( origin ) );
		// log( 'uSize: ' + uSize );
		// log( 'vSize: ' + vSize );

		try {
			orientEntity( entity, {
				origin: [ origin[0], origin[1], origin[2] ],
				angles: [ cell.lat, cell.lon, 0 ],
				scale:  [ uSize, vSize, 1.0 ] 
			});
		} catch ( e ) {
		}
	}
}

function orientSquareToCell( cell, entity ) {
	orientEntityToCell( cell, entity, { placement: 'fill' } );
}

function layoutWidget( cell ) {
	orientEntityToCell( cell, cell.entity );

	postMessage( sprintf( '%s %s arc %s %s %s', cell.plugin, cell.widget, cell.latArc, cell.lonArc, rootDepth ) );
}

function layoutWidgets_r( cell ) {
	if ( cell.kind == 'root' ) {
		for ( var i = 0; i < cell.children.length; i++ ) {
			layoutWidgets_r( cell.children[i] );
		}
	} else if ( cell.kind == 'widget' ) {
		layoutWidget( cell );
	}
}

function layoutWidgets() {
	layoutWidgets_r( rootCell );
}

function layoutCells_r( cell, xform ) {
	// $$$ should there be a translation?

	cell.xform = mat4.create();
	mat4.rotateY( cell.xform, cell.xform, degToRad( cell.lon ) );
	mat4.rotateX( cell.xform, cell.xform, degToRad( cell.lat ) );

	cell.invXform = mat4.create();
	mat4.transpose( cell.invXform, cell.xform );

	// log( 'layoutCells_r: ' );
	// log( 'xform: ' + mat4.str( cell.xform ) );
	// log( 'invXform: ' + mat4.str( cell.invXform ) );

	if ( cell.kind == 'root' ) {
		for ( var i = 0; i < cell.children.length; i++ ) {
			layoutCells_r( cell.children[i], cell.xform );
		}
	}
}

function layoutCells() {
	var rootXform = mat4.create();

	layoutCells_r( rootCell, rootXform );
}

function rayCast_r( cell, dir ) {
	var localDir = vec3.create();
	vec3.transformMat4( localDir, vec3.fromValues( dir[0], dir[1], dir[2] ), cell.invXform );

	// log( 'rayCast_r: ' );
	// log( 'xform: ' + mat4.str( cell.xform ) );
	// log( 'invXform: ' + mat4.str( cell.invXform ) );
	// log( 'localDir: ' + vec3.str( localDir ) );

	if ( cell.kind == 'root' ) {
		for ( var i = 0; i < cell.children.length; i++ ) {
			var result = rayCast_r( cell.children[i], localDir );
			if ( result )
				return result;
		}
	}
	else {
		var targetLat = radToDeg( Math.atan2( localDir[1], -localDir[2] ) );
		var targetLon = radToDeg( Math.atan2( localDir[0], -localDir[2] ) );

		if ( Math.abs( targetLat ) <= cell.latArc * 0.5 &&
			 Math.abs( targetLon ) <= cell.lonArc * 0.5 )
			return cell;
	}

	return null;
}

function rayCast( dir ) {
	return rayCast_r( rootCell, dir );
}

function findCellByWidget_r( cell, wid ) {
	if ( cell.kind == 'root' ) {
		for ( var i = 0; i < cell.children.length; i++ ) {
			var result = findCellByWidget_r( cell.children[i], wid );
			if ( result )
				return result;
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

function getGlobePosition( out, u, v, latArc, lonArc, depth )
{
	var lon = degToRad( (u - 0.5) * lonArc ) - Math.PI/2;

	out[0] = depth * Math.cos( lon );
	out[1] = depth * (v - 0.5) * Math.sin( degToRad( latArc ) );
	out[2] = depth * Math.sin( lon ) + depth;
}

GLOBE_HORIZONTAL = 64.0
GLOBE_VERTICAL	 = 32.0

function makeGlobeRect( id, latArc, lonArc, depth ) {
	var vertexCount = ( GLOBE_HORIZONTAL + 1 ) * ( GLOBE_VERTICAL + 1 );
	var indexCount = GLOBE_HORIZONTAL * GLOBE_VERTICAL * 6;

	var positions = new Float32Array( vertexCount * 3 );
	var position = vec3.create();

	var index = 0;

	for ( var y = 0; y <= GLOBE_VERTICAL; y++ )
	{
		var v = y / GLOBE_VERTICAL;

		for ( var x = 0; x <= GLOBE_HORIZONTAL; x++ )
		{
			var u = x / GLOBE_HORIZONTAL;

			getGlobePosition( position, u, v, latArc, lonArc, depth );
			
			positions[index * 3 + 0] = position[0];
			positions[index * 3 + 1] = position[1];
			positions[index * 3 + 2] = position[2];

			index++;
		}
	}

	var texCoords = new Float32Array( vertexCount * 2 );
	var colors = new Uint32Array( vertexCount );

	index = 0;

	for ( var y = 0; y <= GLOBE_VERTICAL; y++ )
	{
		var v = y / GLOBE_VERTICAL;

		for ( var x = 0; x <= GLOBE_HORIZONTAL; x++ )
		{
			var u = x / GLOBE_HORIZONTAL;

			texCoords[index * 2 + 0] = u;
			texCoords[index * 2 + 1] = 1.0 - v;

			colors[index] = 0xffffffff;

			index++;
		}
	}

	var indices = new Uint16Array( indexCount );

	index = 0;

	for ( var x = 0; x < GLOBE_HORIZONTAL; x++ )
	{
		for ( var y = 0; y < GLOBE_VERTICAL; y++ )
		{
			indices[index + 0] = y * (GLOBE_HORIZONTAL + 1) + x;
			indices[index + 1] = y * (GLOBE_HORIZONTAL + 1) + x + 1;
			indices[index + 2] = (y + 1) * (GLOBE_HORIZONTAL + 1) + x;
			indices[index + 3] = (y + 1) * (GLOBE_HORIZONTAL + 1) + x;
			indices[index + 4] = y * (GLOBE_HORIZONTAL + 1) + x + 1;
			indices[index + 5] = (y + 1) * (GLOBE_HORIZONTAL + 1) + x + 1;
			index += 6;
		}
	}

	sizeGeometry( id, vertexCount, indexCount );

	updateGeometryPositionRange( id, 0, vertexCount, positions );
	updateGeometryTexCoordRange( id, 0, vertexCount, texCoords );
	updateGeometryColorRange( id, 0, vertexCount, colors );

	updateGeometryIndexRange( id, 0, indexCount, indices );

	presentGeometry( id );
}

function makeCmd( args ) {
	if ( args[1] == 'rect' ) {
		var id = args[2];
		var latArc = +(args[3]);
		var lonArc = +(args[4]);
		var depth = +(args[5]);

		makeGlobeRect( id, latArc, lonArc, depth );
	}
}

function registerCmd( args ) {
	var pid = args[1];
	var wid = args[2];
	var eid = args[3];

	if ( findCellByWidget( wid ) ) {
		log( 'Widget ' + wid + ' already registered' );
		return;
	}

	var cell = activeCell;
	if ( !cell ) {
		log( 'No active cell to place ' + wid + ' in' );
		return;
	}

	if ( cell.kind != 'empty' ) {
		log( 'Active cell is already full; can\'t place ' + wid );
		return;
	}

	log( 'registerCmd: wid:' + wid + ' eid:' + eid );

	cell.kind = 'widget';
	cell.plugin = pid;
	cell.widget = wid;
	cell.entity = eid;

	cell.properties = {
		placement: 'center',
		gazeMouse: 'false',
	}

	args.shift(2);
	for ( var i = 0; i < args.length; i++ ) {
		if ( args[i].indexOf( '=' ) !== -1 ) {
			kv = args[i].split( '=' );
			cell.properties[kv[0]] = kv[1];
		}
	}

	// parentEntity( eid, 'root' );

	layoutWidgets();

	activeCell = null;
	refreshActiveCell();
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

	activeCell = null;
	refreshActiveCell();
}

var gazeMouse = {
	x: 0,
	y: 0,
	buttons: 0
};

function gazeMouseDir() {
	var cell = activeCell;
	assert( cell );

	var localGazeDir = vec3.create();
	vec3.transformMat4( localGazeDir, vec3.fromValues( gazeDir[0], gazeDir[1], gazeDir[2] ), cell.invXform );

	var gazeXz = vec3.clone( localGazeDir );
	gazeXz[1] = 0;
	vec3.normalize( gazeXz, gazeXz );

	var gazeYz = vec3.clone( localGazeDir );
	gazeYz[0] = 0;
	vec3.normalize( gazeYz, gazeYz );

	var xzAngle = radToDeg( Math.acos( Math.abs( gazeXz[2] ) ) );
	var yzAngle = radToDeg( Math.acos( Math.abs( gazeYz[2] ) ) );

	var x = xzAngle / (0.5*cell.lonArc);
	var y = yzAngle / (0.5*cell.latArc);

	if ( localGazeDir[0] < 0 )
		x = -x;

	if ( localGazeDir[1] < 0 )
		y = -y;

	gazeMouse.x = x;
	gazeMouse.y = y;

	// $$$ This sends -1 to 1 space, which is fine, but it's unclear what a real mouse would send via the same message.
	//     Thinking that a real mouse will maybe just send deltas (with + or - notation to indicate relativity).
	postMessage( sprintf( '%s %s mouse %f %f %d', cell.plugin, cell.widget, gazeMouse.x, gazeMouse.y, gazeMouse.buttons ) );
}

function gazeMouseTouch( args ) {
	var cell = activeCell;
	assert( cell );

	gazeMouse.buttons = +(args[1]);

	postMessage( sprintf( '%s %s mouse %f %f %d', cell.plugin, cell.widget, gazeMouse.x, gazeMouse.y, gazeMouse.buttons ) );
}

function postToActiveWidget( args ) {
	var cell = activeCell;

	if ( !cell || cell.kind == 'empty' )
		return;

	if ( activeCell.properties.gazeMouse ) {
		if ( args[0] == 'gaze' ) {
			gazeMouseDir();
			return;
		}
		else if ( args[0] == 'touch' ) {
			gazeMouseTouch( args );
			return;
		}
	}

	assert( cell.kind == 'widget' );
	postMessage( sprintf( '%s %s ', cell.plugin, cell.widget ) + args.join( ' ' ) );
}

function postToMenu( args ) {
	if ( args[0] != 'menu' )
		args.unshift( 'menu' );

	postMessage( args.join( ' ' ) );
}

function refreshActiveCell() {
	assert( Vec3.lengthSqr( gazeDir ) > 0.0 );
	cell = rayCast( gazeDir );

	if ( cell && cell != squareCell ) {
		squareCell = cell;
		orientSquareToCell( squareCell, 'square' );
	}

	if ( cell != activeCell ) {
		activeCell = cell;
		if ( cell && cell.kind == 'widget' )
			postMessage( 'menu activate ' + cell.plugin + ' ' + cell.widget );
		else if ( cell && cell.kind == 'empty' )
			postMessage( 'menu activate empty' );
		else
			postMessage( 'menu activate none' );
	}
}

function gazeCmd( args ) {
	if ( menuOpened ) {
		postToMenu( args );
	} else {
		gazeDir = Vec3.create( +(args[1]), +(args[2]), +(args[3]) );

		refreshActiveCell();

		postToActiveWidget( args );
	}
}

function setPropertyCmd( args ) {
	var wid = args[1];
	var property = args[2];
	var value = args[3];

	var cell = findCellByWidget( wid );

	if ( !cell ) {
		log( 'Unable to find a cell containing widget ' + wid );
		dump();
		return;
	}

	cell.property[property] = value;
}

makeSquare();

try { unregisterEntity( 'root' ); } catch (e) {}
registerEntity( 'root' );

rootCell = {
	lat: 0, lon: 0, latArc: 0, lonArc: 0,
	kind: 'root',
	children: include( 'default.layout' )
}

layoutCells();

activeCell = null;
refreshActiveCell();

// $$$ This is a hack to ensure the shell plugin finishes loading before user.cfg is executed.
//     We need a way for the command system to delay until prior actions have completed before continuing
//     to process messages, but this is quite difficult given the decentralized queues.
postMessage( 'exec user.cfg' );

for ( ;; ) {
	var msg = receiveMessage( PLUGIN, 0 );

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

		case 'setproperty':
			setPropertyCmd( args );
			break;

		case 'key':
		case 'touch':
		case 'swipe':
		case 'tap':
			if ( menuOpened )
				postToMenu( args );
			else
				postToActiveWidget( args );
			break;

		case 'make': 
			makeCmd( args );
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
