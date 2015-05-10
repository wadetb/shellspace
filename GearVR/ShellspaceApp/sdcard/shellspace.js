SxPluginKind_Widget = 0
SxPluginKind_Shell = 1
SxPluginKind_Input = 2

function decodeMessage( msg ) {
	var args = msg.match( /("[^"]*")|([^\s]+)/g );

	for (var i = 0; i < args.length; i++) {
		var arg = args[i];
		if ( arg.charAt(0) == '"' ) {
			args[i] = arg.substr(1, arg.length - 2);
		}
	}
	
	return args;
}
