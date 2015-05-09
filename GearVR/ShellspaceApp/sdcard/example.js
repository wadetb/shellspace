//
// Shellspace Example script
//
include( '/storage/extSdCard/Oculus/Shellspace/shellspace.js' );

// All IDs in Shellspace are global, so entity and widget names need to be
//  tagged with the plugin name so as not to conflict.
PLUGIN   = 'shellspace_example'
WIDGET   = PLUGIN + '_widget'
ENTITY0  = PLUGIN + '_ent0'
ENTITY1  = PLUGIN + '_ent1'
ENTITY2  = PLUGIN + '_ent2'
TEXTURE0 = PLUGIN + '_tex0'
TEXTURE1 = PLUGIN + '_tex1'
TEXTURE2 = PLUGIN + '_tex2'

// Clean up from any prior plugin loads as Shellspace doesn't automatically do it
//  when the plugin unloads (yet).
try { unregisterTexture( TEXTURE0 ); } catch (e) {}
try { unregisterTexture( TEXTURE1 ); } catch (e) {}
try { unregisterTexture( TEXTURE2 ); } catch (e) {}
try { unregisterEntity( ENTITY0 ); } catch (e) {}
try { unregisterEntity( ENTITY1 ); } catch (e) {}
try { unregisterEntity( ENTITY2 ); } catch (e) {}
try { unregisterWidget( WIDGET ); } catch (e) {}
try { unregisterPlugin( PLUGIN ); } catch (e) {}

// Asset files- asset() returns an ArrayBuffer.
ASSETS = {
	'safari':   asset( '/storage/extSdCard/Oculus/Shellspace/safari.jpg' ),
	'terminal': asset( '/storage/extSdCard/Oculus/Shellspace/terminal.jpg' )
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

// Register and position the entities.
registerEntity( ENTITY0 );
setEntityGeometry( ENTITY0, "quad" ); // A builtin unit quad
setEntityTexture( ENTITY0, TEXTURE0 );
orientEntity( ENTITY0, { origin: [5, 0, -5], angles: [0, 0, 0], scale: [1.6, 1, 1] } );

registerEntity( ENTITY1 );
setEntityGeometry( ENTITY1, "quad" );
setEntityTexture( ENTITY1, TEXTURE1 );
orientEntity( ENTITY1, { origin: [-5, 0, -5], angles: [0, 0, 0], scale: [1.6, 1, 1] } );

registerEntity( ENTITY2 );
setEntityGeometry( ENTITY2, "quad" );
setEntityTexture( ENTITY2, TEXTURE2 );
orientEntity( ENTITY2, { origin: [0, 0, -5], angles: [0, 0, 0], scale: [1.6, 1, 1] } );

// Submit our entities to the shell.
postMessage( "shell register " + WIDGET + " " + ENTITY0 );
postMessage( "shell register " + WIDGET + " " + ENTITY1 );
postMessage( "shell register " + WIDGET + " " + ENTITY2 );

for ( ;; ) {
	var msg = receivePluginMessage( PLUGIN, 0 );

	args = msg.split( ' ' );

	// $$$ Need to strip quotes or just move messages to JSON.
	if ( args[0] == ('\"'+PLUGIN+'\"') )
		args.shift();

	if ( args[0] == '\"unload\"' )
		break;
}
