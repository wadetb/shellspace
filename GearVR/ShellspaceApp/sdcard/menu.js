//
// Shellspace menu
// Author: Wade Brainerd <wadeb@wadeb.com>
//
include( '/storage/extSdCard/Oculus/Shellspace/shellspace.js' );

PLUGIN   = 'menu'

try { unregisterPlugin( PLUGIN ); } catch (e) {}

registerPlugin( PLUGIN, SxPluginKind_Widget );

for ( ;; ) {
	var msg = receivePluginMessage( PLUGIN, 0 );

	args = msg.split( ' ' );

	// $$$ Need to strip quotes or just move messages to JSON.
	if ( args[0] == ('\"'+PLUGIN+'\"') )
		args.shift();

	if ( args[0] == '\"unload\"' )
		break;
}

