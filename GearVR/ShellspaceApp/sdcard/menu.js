//
// Shellspace menu
// Author: Wade Brainerd <wadeb@wadeb.com>
//
include( 'shellspace.js' );
include( 'vector.js' );

PLUGIN   = 'menu'

try { unregisterPlugin( PLUGIN ); } catch (e) {}

registerPlugin( PLUGIN, SxPluginKind_Widget );

var rootMenu = { id: 'root', children: [] };

var gazeDir = Vec3.create( 0, 0, -1 );

var menuOrigin = Vec3.create( 0, 0, -5 );
var menuDir = Vec3.create( 0, 0, 1 );
var menuUp = Vec3.create( 0, 1, 0 );
var menuRight = Vec3.create( 1, 0, 0 );

function renderCaption( texture, text ) {
	var bitmap = new Bitmap();
	bitmap.setInfo( { width: 200, height: 40 } );
	bitmap.allocPixels();

	var paint = new Paint();
	paint.setAntiAlias( true );
	paint.setColor( 0x00000000 );
	paint.setTextSize( 30 );

	var canvas = new Canvas( bitmap );
	canvas.drawColor( 0xffffffff );
	canvas.translate( 5, 35 );
	canvas.drawText( text , 0, 0, paint);

	loadTextureBitmap( texture, bitmap );
}

function buildMenu() {
	var stack = [ rootMenu ];
	var row = 0;

	while ( stack.length ) {
		var m = stack.pop();

		m.row = row;

		if ( m.caption ) {
			if ( m.texture == undefined ) {
				m.texture = m.id + "_tex";
				try { registerTexture( m.texture ); } catch (e) {}

				renderCaption( m.texture, m.caption );
			}

			if ( m.entity == undefined ) {
				m.entity = m.id + "_ent";
				try { registerEntity( m.entity ); } catch (e) {}

				setEntityGeometry( m.entity, "quad" );
				setEntityTexture( m.entity, m.texture );

				var origin = Vec3.create( menuOrigin );
				Vec3.mad( origin, 1 + row * -0.4, menuUp, origin );

				m.origin = [0, 1 + row * -0.4, -5];
				m.scale = [1, 0.2, 1];

				orientEntity( m.entity, { origin: m.origin, scale: m.scale } );
			}
		}

		if ( m.children ) {
			for ( var i = m.children.length - 1; i >= 0; i-- ) {
				stack.push( m.children[i] );
			}
		}

		row++;
	}
}

function clearMenu() {
	// Destroy existing contents.
	var stack = [ rootMenu ];

	while ( stack.length ) {
		var m = stack.pop();

		if ( m.entity ) {
			unregisterEntity( m.entity );
		}

		if ( m.texture ) {
			unregisterTexture( m.texture );
		}

		if ( m.children ) {
			for ( var i = 0; i < m.children.length; i++ ) {
				stack.push( m.children[i] );
			}
		}
	}

	// Reset to empty root.
	rootMenu = { id: 'root', children: [] };
}

function mergeMenuContents( contents ) {
	// $$$ For now this is just a simple append, later I will support
	//     poking new items into existing menus (a tree merge).
	rootMenu.children = rootMenu.children.concat( contents );
}

function getActiveItem() {
	var stack = [ rootMenu ];

	if ( Math.abs( gazeDir.z ) < 0.001 )
		return null;

	while ( stack.length ) {
		var m = stack.pop();

		if ( m.entity ) {
			// $$$ Assumes the menu is always facing down +Z, but in
			//     practice it needs to auto-orient when opened.
			//     To fix gazeDir needs to be projected along the
			//     menu normal.
			var zratio = m.origin[2] / gazeDir[2];
			var px = gazeDir[0] * zratio;
			var py = gazeDir[1] * zratio;

			var dx = Math.abs( px - m.origin[0] );
			var dy = Math.abs( py - m.origin[1] );
			
			if ( dx <= m.scale[0] && dy <= m.scale[1] )
				return m;
		}

		if ( m.children ) {
			for ( var i = 0; i < m.children.length; i++ ) {
				stack.push( m.children[i] );
			}
		}
	}

	return null;
}

function openMenu( args ) {
	// $$$ Testing
	mergeMenuContents( [
		{ id: 'test1', caption: 'hello!', command: 'yo' },
		{ id: 'test2', caption: 'world!', command: 'yo' },
		{ id: 'test3', caption: 'when!', command: 'yo' },
		{ id: 'test4', caption: 'will!', command: 'yo' },
		{ id: 'test5', caption: 'we!', command: 'yo' },
		{ id: 'test6', caption: 'hit!', command: 'yo' }
	] );
	buildMenu();

	// Capture the gaze direction to use as the menu location.
	Vec3.scale( gazeDir, 5.0, menuOrigin );

	Vec3.negate( gazeDir, menuDir );

	Vec3.cross( menuDir, Vec3.xAxis, menuUp );
	Vec3.normalize( menuUp, menuUp );

	Vec3.cross( menuUp, menuDir, menuRight );
	Vec3.normalize( menuRight, menuRight );

	// Notify the shell.
	postMessage( 'shell menu opened' );
}

function closeMenu( args ) {
	// Teardown the entities and data.
	clearMenu();

	// Notify the shell.
	postMessage( 'shell menu closed' );
}

function onTap() {
	var m = getActiveItem();
	if ( !m ) {
		log( "onTap: No active widget" );
		return;
	}
	if ( !m.command ) {
		log( "onTap: Active widget has no command" );
		return;
	}

	postMessage( m.command );
}

clearMenu();

for ( ;; ) {
	var msg = receivePluginMessage( PLUGIN, 0 );
	args = decodeMessage( msg );

	log( 'menu.js: ' + args.join( ' ' ) );

	if ( args[0] == PLUGIN )
		args.shift();

	var command = args.shift();
	if ( command == 'unload' ) {
		closeMenu();
		break;
	}
	
	if ( command == 'open' ) {
		openMenu();
	}
	else if ( command == 'close' ) {
		closeMenu();
	}
	else if ( command == 'gaze' ) {
		gazeDir = Vec3.create( +(args[0]), +(args[1]), +(args[2]) );
	}
	else if ( command == 'tap' ) {
		onTap();
	}
	else if ( command == 'contents' ) {
		mergeMenuContents( eval( args[1] ) );
		// $$$ It would be nice to not destroy everything; it could be done
		//     by only creating entities that are not already created, and 
		//     dividing buildMenu into build and layout steps, so that a
		//     global layoutMenu could be run after new items are merged in.
		clearMenu();
		buildMenu();
	}
}
