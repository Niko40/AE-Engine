#version 450

layout(location=0) in vec3 Vertex_Location;
layout(location=1) in vec3 Vertex_Color;
layout(location=2) in vec2 Vertex_UV;

layout(set=0, binding=0) uniform ShaderData_Camera
{
	mat4 View_Matrix;
	mat4 Projection_Matrix;
} shader_data_camera;

layout(set=1, binding=0) uniform ShaderData_Object
{
	mat4 Model_Matrix;
} shader_data_object;

layout(location=0) out vec3 Fragment_Color;
layout(location=1) out vec2 Fragment_UV;

void main()
{
	Fragment_Color 	= Vertex_Color;
	Fragment_UV		= Vertex_UV;
	gl_Position		= shader_data_camera.Projection_Matrix * shader_data_camera.View_Matrix * shader_data_object.Model_Matrix * vec4( Vertex_Location, 1.0f );
}
