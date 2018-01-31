#version 450


// vertex input
layout(location=0) in vec3 vertex_location;
layout(location=1) in vec3 vertex_normal;
layout(location=2) in vec2 vertex_uv;
layout(location=3) in int vertex_material_index;


// output to other shader stages
layout(location=0) out vec2 fragment_uv;


void main()
{
	fragment_uv		= vertex_uv;
	gl_Position		= vec4( vertex_location, 1.0f );
}
