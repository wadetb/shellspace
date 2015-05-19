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

for ( ;; ) {
	var msg = receiveMessage( PLUGIN, 0 );
	var args = decodeMessage( msg );

	log( 'example.js: ' + msg );

	if ( args[0] == PLUGIN )
		args.shift();

	if ( args[0] == 'unload' )
		break;

	log( 'example.js: ' + args );

	var command = args[0];

	if ( command == 'create' ) {
		var id = args[1];
		var type = args[2];

		try { registerWidget( id ); } catch ( e ) {}
		try { registerEntity( id ); } catch ( e ) {}

		setEntityGeometry( id, 'quad' );

		if ( type == 'chrome' ) {
			setEntityTexture( id, TEXTURE0 );
		}
		else if ( type == 'terminal' ) {
			setEntityTexture( id, TEXTURE1 );
		}
		else if ( type == 'hello' ) {
			setEntityTexture( id, TEXTURE2 );
		}

		postMessage( 'shell register example ' + id + ' ' + id + ' placement=fill' );
	} 
	else if ( command == 'destroy' ) {
		unregisterWidget( id );
		unregisterEntity( id );
		postMessage( 'shell unregister ' + id );
	}
}
