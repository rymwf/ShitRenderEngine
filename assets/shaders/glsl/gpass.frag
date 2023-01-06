#version 460

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif

#define PI 3.141592653
#define EPSILON 1e-6

struct VS_OUT { 
	vec4 color;
	vec2 texcoord;
	vec3 position;
	vec3 normal;
};

layout(location = 0) in VS_OUT fs_in;

layout(location=0) out vec3 outPos;
layout(location=1) out vec4 outAlbedo;
layout(location=2) out vec3 outN;
layout(location=3) out vec3 outMetallicRoughnessAO;
layout(location=4) out vec3 outEmission;

layout(SET(5) binding=1)uniform Material
{
	int baseColorTextureIndex;		   // srgb
	int metallicRoughnessTextureIndex; // g roughness, b metallic, [r occlussion]
	int normalTextureIndex;			   // rgb
	int occlusionTextureIndex;		   // r occlussion

	//base color
	vec4 baseColorFactor;

	//emissive
	vec3 emissiveFactor;
	int emissiveTextureIndex; // srgb

	//
	int alphaMode; // 0 opaque, 1 cutoff, 2 blend
	float alphaCutoff;
	float normalTextureScale;
	float occlusionTextureStrength;

	vec2 uvOffset;
	vec2 uvScale;

	//sheen
	vec3 sheenColorFactor; //linear
	float sheenRoughnessFactor;

	int sheenColorTextureIndex;
	int sheenRoughnessTextureIndex;

	float metallicFactor;
	float roughnessFactor;

	//clear coat
	float clearcoatFactor;
	float clearcoatRoughnessFactor;
	int clearcoatTextureIndex;
	int clearcoatRoughnessTextureIndex;

	int clearcoatNormalTextureIndex;
}material;

layout(SET(5) binding=0) uniform sampler2D textures[8];

mat3 surfaceTBN(vec3 n)
{
	vec3 UpVec=abs(n.z)<.999?vec3(0,0,1):vec3(1,0,0);
	//vec3 UpVec = abs(N.y) < 0.999 ? vec3(0,1,0):vec3(1,0,0);
	//vec3 UpVec=vec3(0,1,0);
	vec3 t=normalize(cross(UpVec,n));
	vec3 b=cross(n,t);
	return mat3(t,b,n);
}
mat4 surfaceTBN(vec3 n,vec3 o)
{
	vec3 UpVec=abs(n.z)<.999?vec3(0,0,1):vec3(1,0,0);
	//vec3 UpVec = abs(N.y) < 0.999 ? vec3(0,1,0):vec3(1,0,0);
	//vec3 UpVec=vec3(0,1,0);
	vec3 t=normalize(cross(UpVec,n));
	vec3 b=cross(n,t);
	return mat4(t,0,b,0,n,0,o,1);
}

void main()
{
	vec3 N=fs_in.normal;
	mat3 TBN=surfaceTBN(N);

	float metallic=material.metallicFactor;
	float roughness=material.roughnessFactor;
	vec3 emissiveColor=material.emissiveFactor;
	vec4 albedo=material.baseColorFactor;
	float ao=material.occlusionTextureStrength;

	//===========================
	if(material.normalTextureIndex>=0)
	{
	 	N+=TBN*normalize(texture(textures[material.normalTextureIndex],fs_in.texcoord).xyz*2-1);
		N=normalize(N);
	}
	if(material.baseColorTextureIndex>=0)
		albedo*=texture(textures[material.baseColorTextureIndex],fs_in.texcoord);

	if(material.metallicRoughnessTextureIndex>=0)
	{
		vec4 a=texture(textures[material.metallicRoughnessTextureIndex],fs_in.texcoord);
		metallic*=a.b;
		roughness*=a.g;
		if(material.occlusionTextureIndex==material.metallicRoughnessTextureIndex)
		{
			ao*=a.r;
		}
	}
	if( material.occlusionTextureIndex>0&& material.occlusionTextureIndex!=material.metallicRoughnessTextureIndex)
	{
		ao*=texture(textures[material.occlusionTextureIndex],fs_in.texcoord).r;
	}

	if(material.emissiveTextureIndex>=0)
		emissiveColor=texture(textures[material.emissiveTextureIndex],fs_in.texcoord).rgb;

	if(albedo.a<0.5)
		discard;

	//=======================
	outPos=fs_in.position;
	outAlbedo=albedo;
	outN=N;
	outMetallicRoughnessAO=vec3(metallic,roughness,ao);
	outEmission=emissiveColor;
}