#version 460
layout(location=0) out vec4 fragOut;

layout(location = 0) in VS_OUT { 
	vec4 color;
	vec2 texcoord;
	vec3 position;
	vec3 normal;
}fs_in;

void main()
{
	fragOut= fs_in.color;
	//fragOut=vec4(1);
}