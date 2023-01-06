#version 460
//#extension all: warn

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inTexCoord0;
layout(location = 4) in vec4 inColor0;
//#ifdef VULKAN
//layout(location = 5) in uvec4 inJoints0;
//#else
layout(location = 5) in vec4 inJoints0;
//#endif
layout(location = 6) in vec4 inWeights0;
//layout(location = 11) in vec4 inInstanceColorFactor;
//layout(location = 12) in mat4 inInstanceMatrix;

layout(location = 0) out vec2 vs_texcoord;

//set 0 and 1 is frame dependent set
//set 1 may update every frame

layout(SET(1) binding=0) uniform UBONode{
	mat4 M;
};
layout(std430,SET(2) binding=1) buffer UBOSkin
{
	int hasSkin;
	mat4 jointMatrices[];
};

void main() 
{
	vec4 vertexPos;
	if(hasSkin!=0)
	{
		mat4 skinMatrix=
			inWeights0[0]*jointMatrices[uint(inJoints0[0])]+ 
			inWeights0[1]*jointMatrices[uint(inJoints0[1])]+ 
			inWeights0[2]*jointMatrices[uint(inJoints0[2])]+ 
			inWeights0[3]*jointMatrices[uint(inJoints0[3])];
		vertexPos=skinMatrix*vec4(inPos, 1);
	}
	else
	{
		vertexPos=M*vec4(inPos, 1);
	}

	gl_Position = vertexPos;
	vs_texcoord = inTexCoord0;
}