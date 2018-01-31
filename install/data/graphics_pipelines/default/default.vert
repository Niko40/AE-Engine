#version 450


// General layout of the vertex shader:
// - Vertex input
// - Uniform buffers input
// - Output to other shader stages
// - Functions
// - Main entry

// Vertex input:
// This portion should never change, we're always expecting this format from the vertex.
layout(location=0) in vec3 vertex_location;
layout(location=1) in vec3 vertex_normal;
layout(location=2) in vec2 vertex_uv;
layout(location=3) in int vertex_material_index;


// Uniform buffers input:
// This portion should never chance, we're always expecting these buffers,
// sets, bindings and content of the uniform buffers.
layout(set=0, binding=0) uniform CameraData
{
	mat4 view_matrix;
	mat4 projection_matrix;
} camera_data;

layout(set=1, binding=0) uniform MeshData
{
	mat4 model_matrix;
} mesh_data;


// Output to other shader stages:
// Change these as needed
layout(location=0) out vec2 fragment_uv;


// Functions:
// All the supporting functions will go here


// Main entry:
// Lastly the main entry to the shader, you can configure the name of the main entry in the xml,
// can be useful for debug variants of the entry function.
void main()
{
	fragment_uv		= vertex_uv;
	gl_Position		= camera_data.projection_matrix * camera_data.view_matrix * mesh_data.model_matrix * vec4( vertex_location, 1.0f );
}
