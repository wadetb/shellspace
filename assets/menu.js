//
// Shellspace menu
// Author: Wade Brainerd <wadeb@wadeb.com>
//
include( 'shellspace.js' );
include( 'gl-matrix.js' );

PLUGIN   = 'menu'

try { unregisterPlugin( PLUGIN ); } catch (e) {}

registerPlugin( PLUGIN, SxPluginKind_Widget );

var rootMenu = { id: 'root', children: [] };

var activeKind = 'none';
var activeId;

var nextId = 0;
var nextMenuId = 0;

var menuStack = [];

var gazeDir = vec3.create( 0, 0, -1 );

var menuOrigin = vec3.create( 0, 0, -5 );
var menuDir = vec3.create( 0, 0, 1 );
var menuUp = vec3.create( 0, 1, 0 );
var menuRight = vec3.create( 1, 0, 0 );

function renderCaption( texture, text, hilite ) {
	var bitmap = new Bitmap();
	bitmap.setInfo( { width: 200, height: 40 } );
	bitmap.allocPixels();

	var canvas = new Canvas( bitmap );

	canvas.drawColor( hilite ? 0xff8080ff : 0xffffffff );

	var paint = new Paint();
	paint.setAntiAlias( true );
	paint.setColor( 0xffffffff );
	paint.setTextSize( 30 );

	canvas.translate( 5, 35 );
	canvas.drawText( text , 0, 0, paint);

	loadTextureBitmap( texture, bitmap );
}

function mergeMenuContents( contents ) {
	// $$$ For now this is just a simple append, later I will support
	//     poking new items into existing menus (a tree merge).
	rootMenu.children = rootMenu.children.concat( contents );
}

function showItem( m ) {
	if ( !m.id ) {
		m.id = 'menu' + nextMenuId;
		nextMenuId++;
	}

	m.texture = m.id + "_tex";
	try { registerTexture( m.texture ); } catch (e) {}

	renderCaption( m.texture, m.caption, false );

	m.hiliteTexture = m.id + "_hltex";
	try { registerTexture( m.hiliteTexture ); } catch (e) {}

	renderCaption( m.hiliteTexture, m.caption, true );

	m.entity = m.id + "_ent";
	try { registerEntity( m.entity ); } catch (e) {}

	setEntityGeometry( m.entity, "quad" );
	setEntityTexture( m.entity, m.texture );
}

function hideItem( m ) {
	unregisterEntity( m.entity );
	unregisterTexture( m.texture );
	unregisterTexture( m.hiliteTexture );
}

function showItems( menu ) {
	var row = 0;

	for ( var i = 0; i < menu.children.length; i++ ) {
		var m = menu.children[i];

		showItem( m );

		var origin = vec3.clone( menuOrigin );
		vec3.scaleAndAdd( origin, origin, menuUp, 1 + row * -0.4 );

		m.origin = [0, 1 + row * -0.4, -5];
		m.scale = [1, 0.19, 1];

		orientEntity( m.entity, { origin: m.origin, scale: m.scale } );

		row++;
	}
}

function hideItems( menu ) {
	for ( var i = 0; i < menu.children.length; i++ ) {
		var m = menu.children[i];

		hideItem( m );
	}
}

function hitItem( m ) {
	if ( Math.abs( gazeDir[2] ) < 0.001 )
		return false;

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
		return true;

	return false;
}

function hitMenu( menu ) {
	for ( var i = 0; i < menu.children.length; i++ ) {
		var m = menu.children[i];

		if ( hitItem( m ) )
			return m;
	}

	return null;
}

function openMenu( args ) {
	children = []

	if ( activeKind == 'empty' ) {
		var menu = include( 'start.menu' );
		if ( menu )
			children = children.concat( menu );
	}
	else if ( activeKind != 'none' ) {
		var menu = include( activeKind + '.menu' );
		if ( menu )
			children = children.concat( menu );
	}

	children = children.concat(
		{ caption: 'reset', command: 'exec reset.cfg' }
	);

	rootMenu.children = children;

	menuStack = [ rootMenu ];
	showItems( menuStack[0] );

	// Capture the gaze direction to use as the menu location.
	vec3.scale( menuOrigin, gazeDir, 5.0 );
	vec3.negate( menuDir, gazeDir );

	vec3.cross( menuRight, menuDir, vec3.fromValues( 0, 1, 0 ) );
	vec3.normalize( menuRight, menuRight );

	vec3.cross( menuUp, menuDir, menuRight );
	vec3.normalize( menuUp, menuUp );

	// $$$ Angles to orient the entity.

	// Hilite any initially active item.
	var active = hitMenu( menuStack[0] );
	if ( active && active.entity ) {
		setEntityTexture( active.entity, active.hiliteTexture );
	}

	// Notify the shell.
	postMessage( 'shell menu opened' );
}

function closeMenu( args ) {
	if ( menuStack.length ) {
		hideItems( menuStack[0] );

		// Reset to empty root.
		rootMenu = { id: 'root', children: [] };
		menuStack = [];
	}

	// Notify the shell.
	postMessage( 'shell menu closed' );
}

function onGaze( args ) {
	if ( !menuStack.length )
		return;

	var oldActive = hitMenu( menuStack[0] );

	gazeDir = vec3.create( +(args[0]), +(args[1]), +(args[2]) );

	var active = hitMenu( menuStack[0] );

	if ( oldActive != active ) {
		if ( oldActive && oldActive.entity ) {
			setEntityTexture( oldActive.entity, oldActive.texture );
		}

		if ( active && active.entity ) {
			setEntityTexture( active.entity, active.hiliteTexture );
		}
	}
}

function onTap() {
	if ( !menuStack.length )
		return;

	var m = hitMenu( menuStack[0] );
	if ( !m ) {
		log( "onTap: No active item" );
		return;
	}

	// Tap ancestor of active menu.
	for ( var i = 1; i < menuStack.length; i++ ) {
		if ( m == menuStack[i] ) {
			hideItems( menuStack[0] );
			while ( m != menuStack[0] )
				menuStack.shift();
			showItems( menuStack[0] );
			return;
		}
	}
	
	// Tap a menu with children.
	// $$$ Careful if we allow direct tapping descendents, might
	//     need to push an intermediate menu.
	if ( m.children ) {
		hideItems( menuStack[0] );
		menuStack.unshift( m );
		showItems( menuStack[0] );
		return;
	}

	if ( !m.command ) {
		log( "onTap: Active item has no command" );
		return;
	}

	var command = m.command;

	if ( command.match( /\$newId/ ) ) {
		command = command.replace( /\$newId/g, '' + nextId );
		nextId++;
	}

	if ( command.match( /\$activeId/ ) ) {
		if ( activeId ) {
			command = command.replace( /\$activeId/g, '' + activeId );
		}
		else {
			log( 'Command requires an active widget but there is none: ' + command );
			return;
		}
	}

	log( 'Item ' + m.id + ' activated: ' + command );

	postMessage( command );

	if ( m.closeMenu !== false ) {
		closeMenu();
	}
}

for ( ;; ) {
	var msg = receiveMessage( PLUGIN, 0 );
	var args = decodeMessage( msg );

	if ( !msg.match( /gaze/ ) ) {
		log( 'menu.js: ' + args.join( ' ' ) );
	}

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
		onGaze( args );
	}
	else if ( command == 'tap' ) {
		onTap();
	}
	else if ( command == 'activate' ) {
		activeKind = args[0];
		activeId = args[1];
	}
	else if ( command == 'contents' ) {
		mergeMenuContents( eval( args[1] ) );
	}
}
