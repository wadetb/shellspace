//
// Shellspace Example script
//
include( 'shellspace.js' );
include( 'vector.js' );

// All IDs in Shellspace are global, so entity and widget names need to be unique.
PLUGIN   = 'example'
TEXTURE0 = PLUGIN + '_tex0'
TEXTURE1 = PLUGIN + '_tex1'
TEXTURE2 = PLUGIN + '_tex2'

// Clean up from any prior plugin loads as Shellspace doesn't automatically do it
//  when the plugin unloads (yet).
try { unregisterPlugin( PLUGIN ); } catch ( e ) {}

try { unregisterTexture( TEXTURE0 ); } catch ( e ) {}
try { unregisterTexture( TEXTURE1 ); } catch ( e ) {}
try { unregisterTexture( TEXTURE2 ); } catch ( e ) {}

try { unregisterWidget( 'chrome0' ); } catch ( e ) {}
try { unregisterWidget( 'terminal0' ); } catch ( e ) {}
try { unregisterWidget( 'hello0' ); } catch ( e ) {}

try { unregisterEntity( 'chrome0' ); } catch ( e ) {}
try { unregisterEntity( 'terminal0' ); } catch ( e ) {}
try { unregisterEntity( 'hello0' ); } catch ( e ) {}

// Asset files- asset() returns an ArrayBuffer.
ASSETS = {
	'safari':   asset( 'safari.jpg' ),
	'terminal': asset( 'terminal.jpg' )
};

// Register the plugin (will be done automatically later)
registerPlugin( PLUGIN, SxPluginKind_Widget );

// Safari screenshot
registerTexture( TEXTURE0 );
loadTextureJpeg( TEXTURE0, ASSETS['safari'] );

// Terminal screenshot
registerTexture( TEXTURE1 );
loadTextureJpeg( TEXTURE1, ASSETS['terminal'] );

// Rendered SVG contents
registerTexture( TEXTURE2 );

var paint = new Paint();
var bitmap = new Bitmap();

bitmap.setInfo( { width: 400, height: 300 } );
bitmap.allocPixels();

paint.setAntiAlias( true );
paint.setColor( 0x00000000 );
paint.setTextSize( 30 );

var canvas = new Canvas( bitmap );
canvas.drawColor( 0xffffffff );
canvas.save();
canvas.translate(50, 100);
canvas.drawText("Hello, world!", 0, 0, paint);
canvas.restore();

loadTextureBitmap( TEXTURE2, bitmap );

var rootDepth = 40;

function degToRad( v ) {
	return v * Math.PI / 180.0;
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

	log( "origin x:" + origin[0] + " y:" + origin[1] + " z:" + origin[2] );

	orientEntity( entity, {
		origin: [ origin[0], origin[1], origin[2] ],
		angles: [ cell.lat, -cell.lon, 0 ],
		scale:  [ 22, 12, 1 ]
	} );
}

var leftCell = { kind: 'empty', lat: 0, lon:-72, latArc: 35, lonArc: 67 };
var rightCell = { kind: 'empty', lat: 0, lon:72, latArc: 35, lonArc: 67 };

for ( ;; ) {
	var msg = receivePluginMessage( PLUGIN, 0 );
	var args = decodeMessage( msg );

	log( 'example.js: ' + msg );

	if ( args[0] == PLUGIN )
		args.shift();

	if ( args[0] == 'unload' )
		break;

	log( 'example.js: ' + args );

	var command = args[0];
	var type = args[1];

	if ( command == 'spawn' ) {
		if ( type == 'chrome' ) {
			try { registerWidget( 'chrome0' ); } catch ( e ) {}
			try { registerEntity( 'chrome0' ); } catch ( e ) {}
			setEntityGeometry( 'chrome0', "quad" );
			setEntityTexture( 'chrome0', TEXTURE0 );
			orientEntityToCell( leftCell, 'chrome0' );
			// postMessage( 'shell register chrome0 chrome0' );
		}
		else if ( type == 'terminal' ) {
			try { registerWidget( 'terminal0' ); } catch ( e ) {}
			try { registerEntity( 'terminal0' ); } catch ( e ) {}
			setEntityGeometry( 'terminal0', "quad" );
			setEntityTexture( 'terminal0', TEXTURE1 );
			orientEntityToCell( rightCell, 'terminal0' );
			// postMessage( 'shell register chrome0 chrome0' );
		}
		else if ( type == 'hello' ) {
			try { registerWidget( 'hello0' ); } catch ( e ) {}
			try { registerEntity( 'hello0' ); } catch ( e ) {}
			setEntityGeometry( 'hello0', "quad" );
			setEntityTexture( 'hello0', TEXTURE2 );
			orientEntity( 'hello0', {
				origin: [ 0, 0, -40 ],
				angles: [ 0, 0, 0 ],
				scale: [ 24, 12, 1 ]
			} );
			// postMessage( "shell register " + WIDGET + " " + 'hello0' );
		}
	} else if ( command == 'destroy' ) {
		if ( type == 'chrome' ) {
			unregisterWidget( 'chrome0' );
			unregisterEntity( 'chrome0' );
			// postMessage( "shell unregister " + WIDGET + " " + 'chrome0' );
		}
		else if ( type == 'terminal' ) {
			unregisterWidget( 'terminal0' );
			unregisterEntity( 'terminal0' );
			// postMessage( "shell unregister " + WIDGET + " " + 'terminal0' );
		}
		else if ( type == 'hello' ) {
			unregisterWidget( 'hello0' );
			unregisterEntity( 'hello0' );
			// postMessage( "shell unregister " + WIDGET + " " + 'hello0' );
		}
	}
}
