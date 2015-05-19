#version 300 es

uniform sampler2D Texture0;

in  highp   vec2 oTexCoord;
in  lowp    vec4 oColor;
out mediump vec4 fragColor;

void main()
{
	fragColor = oColor * texture( Texture0, oTexCoord );
}
