#version 460

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif

layout(triangles) in;
layout(triangle_strip,max_vertices=30) out;

layout(SET(0) binding=0) buffer SSBOCamera
{
	mat4 V;
	mat4 P;
	vec3 eyePos;
	int cascadeNum;
	float cascadeSplit[];
}camera;

layout(SET(3) binding=2) buffer SSBOLight
{
	mat4 transformMatrix;
	vec3 color;
	int lightType; // 1 directional, 2 sphere, 3 spot, 4 tube, disable when <0
	vec3 radius;
	float rmax;

	float radiance;
	float cosThetaU;
	float cosThetaP;
	int vertexNum;

	vec4 vertices[4];
	mat4 P[6];
	mat4 V[6];
}light;

layout(location = 0) in vec2 fs_texcoord[];

layout (location=0) out vec2 gs_texcoord;

void main()
{
	int count=1;
	if(light.lightType==0)
		count=camera.cascadeNum;
	else if(light.lightType==1)
		count=6;

	for(int i=0;i<count;++i)
	{
		gl_Layer=i;
		for(int j=0;j<3;++j)
		{
			gl_Position=light.P[i]*light.V[i]*gl_in[j].gl_Position;
			gs_texcoord=fs_texcoord[i];
			EmitVertex();
		}
		EndPrimitive();
	}
}