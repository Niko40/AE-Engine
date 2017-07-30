#version 450


// vertex input
layout(location=0) in vec3 vertex_location;
layout(location=1) in vec3 vertex_normal;
layout(location=2) in vec2 vertex_uv;
layout(location=3) in int vertex_material_index;


// uniform buffers input
layout(set=0, binding=0) uniform CameraData
{
	mat4 view_matrix;
	mat4 projection_matrix;
} camera_data;

layout(set=1, binding=0) uniform MeshData
{
	mat4 model_matrix;
} mesh_data;


// output to other shader stages
layout(location=0) out vec2 fragment_uv;


// main entry function to shader
void main()
{
	fragment_uv		= vertex_uv;
	gl_Position		= camera_data.projection_matrix * camera_data.view_matrix * mesh_data.model_matrix * vec4( vertex_location, 1.0f );
}
