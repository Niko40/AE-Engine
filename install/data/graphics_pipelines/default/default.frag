#version 450

layout(location=0) in vec3 Fragment_Color;
layout(location=1) in vec2 Fragment_UV;

layout(set=2, binding=0) uniform sampler2D tex;

layout(location=0) out vec4 FinalColor;

void main()
{
	FinalColor = vec4( Fragment_Color, 1.0f );
	FinalColor = texture( tex, Fragment_UV );
}
