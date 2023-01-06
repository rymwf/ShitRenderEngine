#version 460

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif

#define PI 3.141592653
#define ROUGHNESS_1_LOD 7

struct VS_OUT { 
	vec4 color;
	vec2 texcoord;
	vec3 position;
	vec3 normal;
};

layout(location = 0) in VS_OUT fs_in;

layout(location = 0)out vec4 fragOut;

layout(SET(0) binding=0) buffer Camera{
	mat4 V;
	mat4 P;
	vec3 eyePos;
	int cascadeNum;
	float cascadeSplit[];
}camera;

layout(SET(4) binding=10) uniform samplerCube prefilteredEnvTex;
layout(SET(4) binding=11) uniform sampler2D brdfTex;

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

float rand(vec2 co){
	return fract(sin(dot(co,vec2(12.9898,78.233)))*43758.5453);
}

mat2 rotate(float angle)
{
	float sinAngle=sin(angle);
	float cosAngle=cos(angle);
	return mat2(cosAngle,sinAngle,-sinAngle,cosAngle);
}
vec3 ACESFilm(vec3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((x*(a*x+b))/(x*(c*x+d)+e),0,1);
}

vec3 fresnelSchlick(float NdotL, vec3 F0)
{
    //F0 is the specular reflectance at normal incience
    //return F0 + (1.0 - F0) * pow(1.0 - clamp(NdotL,0,1), 5.0);
   return F0 + (1.0 - F0) * exp2((-5.55473 * NdotL- 6.98316) * NdotL);
}
float NDF_GGX(float NdotH, float alphag)
{
    float alphag2= alphag*alphag;
    float NdotH2 = NdotH*NdotH;
	float c=step(0.,NdotH);

    float nom   = alphag2;
    float denom = (NdotH2 * (alphag2- 1.0) + 1.0);

    //return alphag2;
    return max(alphag2,0.00001)*c/ (PI * denom * denom);
}
float G_GGX(float NdotL,float NdotV,float alpha)
{
	NdotL=abs(NdotL);
	NdotV=abs(NdotV);
	return 0.5*mix(2*NdotL*NdotV,NdotL+NdotV,alpha);///(4*NdotV*NdotL+0.00001);
}
vec3 diffShirley(vec3 F0,vec3 albedo,float NdotL,float NdotV)
{
	return 21/(20*PI)*(1-F0)*albedo*(1-pow(1-NdotL,5))*(1-pow(1-NdotV,5));
}

//==========================

void main()
{
	vec3 V= normalize(camera.eyePos-fs_in.position);
	vec3 R;
	vec3 L=vec3(1,0,0);
	vec3 N=fs_in.normal;
	mat3 TBN=surfaceTBN(N);

	float metallic=material.metallicFactor;
	float roughness=material.roughnessFactor;
	vec3 emissiveColor=material.emissiveFactor;
	vec4 albedo=material.baseColorFactor;
	float ao=material.occlusionTextureStrength;

	//===========================
	//if(light.lightType==1)
	//{
	//	L=mat3(light.transformMatrix)*vec3(0,0,1);
	//}
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
	if(material.occlusionTextureIndex!=material.metallicRoughnessTextureIndex)
	{
		ao*=texture(textures[material.occlusionTextureIndex],fs_in.texcoord).r;
	}

	if(material.emissiveTextureIndex>=0)
		emissiveColor=texture(textures[material.emissiveTextureIndex],fs_in.texcoord).rgb;
	
	//=============================
	vec3 F0 = mix(vec3(0.04),albedo.rgb, metallic);
	R=reflect(-V,N);

	vec3 E_spec=textureLod(prefilteredEnvTex,R,roughness*ROUGHNESS_1_LOD).rgb;
	vec3 E_diff=textureLod(prefilteredEnvTex,N,ROUGHNESS_1_LOD).rgb;

	float NdotV=max(dot(N,V),0);

	//hemispherical directional reflectance
	vec2 envBRDF=textureLod(brdfTex,vec2(NdotV,roughness),0).xy;
	vec3 RsF_V=envBRDF.x*F0+envBRDF.y;
	float RsF1_V=envBRDF.x+envBRDF.y;

	vec3 f_spec=F0*envBRDF.x+envBRDF.y;
	vec3 f_diff=albedo.rgb*(1-RsF_V)*(1-metallic);

	float RsF1_avg=textureLod(brdfTex,vec2(1,roughness),0).z;
	vec3 F_avg=(20.f*F0+1)/21.f;
	vec3 f_ms=F_avg*RsF1_avg*(1-RsF1_V)/(1-F_avg*(1-RsF1_avg));

	vec3 IBL_Lo=E_diff*f_diff+E_spec*(f_spec+f_ms)+emissiveColor;
	//vec3 IBL_Lo=E_spec;
	//==============================
	fragOut=vec4(IBL_Lo,1);
}