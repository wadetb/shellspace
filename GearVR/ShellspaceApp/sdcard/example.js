//
// Shellspace Example script
//

// All IDs in Shellspace are global, so entity and widget names need to be
//  tagged with the plugin so as not to conflict.
PLUGIN = 'shellspace_example'
WIDGET = PLUGIN + '_w0'
ENTITY = PLUGIN + '_e0'

registerPlugin( PLUGIN, 0 );
registerEntity( ENTITY );

setEntityGeometry( ENTITY, "quad" ); // An internal quad.
setEntityTexture( ENTITY, "white" );
orientEntity( ENTITY, { origin: [ 0, 0, -10], angles: [0, 0, 0], scale: [1, 1, 1] } );

for ( ;; ) {
	var msg = receivePluginMessage( PLUGIN, 0 );

	args = msg.split( ' ' );

	// $$$ Need to strip quotes.
	if ( args[0] == ('\"'+PLUGIN+'\"') )
		args.shift();

	if ( args[0] == '\"unload\"' )
		break;
}

unregisterEntity( ENTITY );
unregisterPlugin( PLUGIN );
