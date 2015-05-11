#version 300 es

uniform mediump mat4 Mvpm;

in vec4 Position;
in vec4 VertexColor;
in vec2 TexCoord;

uniform mediump vec4 UniformColor;
out  lowp vec4 oColor;
out highp vec2 oTexCoord;

void main()
{
	gl_Position = Mvpm * Position;
	oTexCoord = TexCoord * UniformColor.xy;
	oColor = VertexColor;
}
