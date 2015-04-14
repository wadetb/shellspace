#define GLSL_NOISE \
	"uniform vec2 noiseParams;\n" \
	"" \
	"float NoiseTemporalDither1D( float scale, float t )\n" \
	"{\n" \
	"	return -scale + 2.0 * scale * t;\n" \
	"}\n" \
	"" \
	"float NoiseSpatialDither1D( vec2 position, float scale )\n" \
	"{\n" \
	"	float positionModX = float( int( position.x ) & 1 );\n" \
	"	float positionModY = float( int( position.y ) & 1 );\n" \
	"	vec2 positionMod = vec2( positionModX, positionModY );\n" \
	"	return ( -scale + 2.0 * scale * positionMod.x ) * ( -1.0 + 2.0 * positionMod.y );\n" \
	"}\n" \
	"" \
	"vec2 NoiseSpatioTemporalDither2D( vec2 position, vec2 scale, float t )\n" \
	"{\n" \
	"	float noise1 = NoiseSpatialDither1D( position, 1.0 );\n" \
	"	float noise2 = NoiseTemporalDither1D( 1.0, t );\n" \
	"	return scale * vec2( noise1 * noise2, noise1 );\n" \
	"}\n" \
	""
