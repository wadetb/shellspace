//
// Shellspace Example script
//
include( '/storage/extSdCard/Oculus/Shellspace/shellspace.js' );

// All IDs in Shellspace are global, so entity and widget names need to be
//  tagged with the plugin so as not to conflict.
PLUGIN  = 'shellspace_example'
WIDGET  = PLUGIN + '_widget'
ENTITY  = PLUGIN + '_ent'
TEXTURE = PLUGIN + '_tex'

// Clean up from any prior plugin loads.
try { unregisterTexture( TEXTURE ); } catch (e) {}
try { unregisterEntity( ENTITY ); } catch (e) {}
try { unregisterWidget( WIDGET ); } catch (e) {}
try { unregisterPlugin( PLUGIN ); } catch (e) {}

ASSETS = {
	'safari':   asset( '/storage/extSdCard/Oculus/Shellspace/safari.jpg' ),
	'terminal': asset( '/storage/extSdCard/Oculus/Shellspace/terminal.jpg' )
};

registerPlugin( PLUGIN, SxPluginKind_Widget );
registerEntity( ENTITY );

registerTexture( TEXTURE );
loadTextureJpeg( TEXTURE, ASSETS['terminal'] );

setEntityGeometry( ENTITY, "quad" ); // A builtin unit quad
setEntityTexture( ENTITY, TEXTURE );
orientEntity( ENTITY, { origin: [ 0, 0, -5], angles: [0, 0, 0], scale: [1.6, 1, 1] } );

// Submit our entity to the shell.
postMessage( "shell register " + WIDGET + " " + ENTITY );

for ( ;; ) {
	var msg = receivePluginMessage( PLUGIN, 0 );

	args = msg.split( ' ' );

	// $$$ Need to strip quotes or just move messages to JSON.
	if ( args[0] == ('\"'+PLUGIN+'\"') )
		args.shift();

	if ( args[0] == '\"unload\"' )
		break;
}

unregisterEntity( ENTITY );
unregisterPlugin( PLUGIN );
