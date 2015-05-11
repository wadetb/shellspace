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

var menuStack = [];

var gazeDir = Vec3.create( 0, 0, -1 );

var menuOrigin = Vec3.create( 0, 0, -5 );
var menuDir = Vec3.create( 0, 0, 1 );
var menuUp = Vec3.create( 0, 1, 0 );
var menuRight = Vec3.create( 1, 0, 0 );

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

		var origin = Vec3.create( 0, 0, -5 );
		Vec3.mad( origin, 1 + row * -0.4, menuUp, origin );

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
	// $$$ Testing
	rootMenu.children = [
		{ id: 'scene', caption: 'scene', children: [
			{ id: 'resolution', caption: 'resolution', children: [
				{ id: 'res1024', caption: '1x', command: 'scene resolution 1024' },
				{ id: 'res1024', caption: '1.5x', command: 'scene resolution 1536' },
				{ id: 'res1024', caption: '2x', command: 'scene resolution 2048' },
			] },
		] },
		{ id: 'test3', caption: 'when!', command: 'yo' },
		{ id: 'test4', caption: 'will!', command: 'yo' },
		{ id: 'test5', caption: 'we!', command: 'yo' },
		{ id: 'test6', caption: 'hit!', command: 'yo' }
	];

	menuStack = [ rootMenu ];
	showItems( menuStack[0] );

	// Capture the gaze direction to use as the menu location.
	Vec3.scale( gazeDir, 5.0, menuOrigin );

	Vec3.negate( gazeDir, menuDir );

	Vec3.cross( menuDir, Vec3.xAxis, menuUp );
	Vec3.normalize( menuUp, menuUp );

	Vec3.cross( menuUp, menuDir, menuRight );
	Vec3.normalize( menuRight, menuRight );

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

	gazeDir = Vec3.create( +(args[0]), +(args[1]), +(args[2]) );

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

	log( 'Item ' + m.id + ' activated: ' + m.command );

	postMessage( m.command );

	closeMenu();
}

for ( ;; ) {
	var msg = receivePluginMessage( PLUGIN, 0 );
	var args = decodeMessage( msg );

	// log( 'menu.js: ' + args.join( ' ' ) );

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
	else if ( command == 'contents' ) {
		mergeMenuContents( eval( args[1] ) );
	}
}
