/*
    Shellspace - One tiny step towards the VR Desktop Operating System
    Copyright (C) 2015  Wade Brainerd

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
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
