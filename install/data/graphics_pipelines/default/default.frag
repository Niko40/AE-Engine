#version 450


// uniform buffers and images input
layout(set=2, binding=0) uniform PipelineData
{
	mat4 todo;
} pipeline_data;

layout(set=3, binding=0) uniform sampler2D diffuse;


// input from other shader stages
layout(location=0) in vec2 fragment_uv;


// output of final colors
layout(location=0) out vec4 final_color;


// main entry function to shader
void main()
{
	final_color = texture( diffuse, fragment_uv );
}
