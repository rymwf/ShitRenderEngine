#version 460
#extension GL_EXT_scalar_block_layout :enable

#define PI 3.141592653

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif
layout(early_fragment_tests)in;

layout(location=0) out vec4 outColor;

layout(location =0) in vec3 fs_position;

layout(SET(4) binding=10) uniform samplerCube prefilteredEnvTex;

void main()
{
	vec3 uv= normalize(fs_position);
	outColor=vec4(textureLod(prefilteredEnvTex,uv,0).rgb,1);
}