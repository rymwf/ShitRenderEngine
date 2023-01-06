#version 460

#ifdef VULKAN
#define SET(x) set=x,
#else
#define SET(x)
#endif

#define PI 3.141592653

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
	int cascadeNum; float cascadeSplit[];
}camera;

layout(SET(3) binding=2)buffer Light
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

	vec3 vertices[4];
}light;
layout(SET(3)binding=8)uniform sampler2DArray shadowMap2D;
layout(SET(3)binding=9)uniform samplerCube shadowMapCube;

//layout(SET(4) binding=10) uniform samplerCube prefilteredEnvTex;
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
	//vec3 L_dir=mat3(light.transformMatrix)*vec3(0,0,-1);
	vec3 N=fs_in.normal;
	vec3 H;
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

	//=======================

	R=reflect(-V,N);
	H=normalize(L+V);

	float NdotH=dot(N,H);
	float HdotL=dot(H,L);
	float HdotV=max(dot(H,V),0);
	float NdotL=max(dot(N,L),0);
	float NdotV=dot(N,V);
	float LdotR=dot(L,R);

	vec3 F0 = mix(vec3(0.04),albedo.rgb, metallic);
	vec3 F=fresnelSchlick(HdotL,F0);
	vec3 F_avg=(20*F0+1)/21.;
	vec3 F_V=fresnelSchlick(NdotV,F0);

	float alpha=roughness*roughness;

	vec2 envBRDF_V=textureLod(brdfTex,vec2(NdotV,roughness),0).xy;
	vec2 envBRDF_L=textureLod(brdfTex,vec2(NdotL,roughness),0).xy;

	vec3 RsF_L=envBRDF_L.x*F0+envBRDF_L.y;
	vec3 RsF_V=envBRDF_V.x*F0+envBRDF_V.y;
	vec3 RsF_avg=vec3(
		textureLod(brdfTex,vec2(F.r,roughness),0).z,
		textureLod(brdfTex,vec2(F.g,roughness),0).z,
		textureLod(brdfTex,vec2(F.b,roughness),0).z);

	float RsF1_L=envBRDF_L.x+envBRDF_L.y;
	float RsF1_V=envBRDF_V.x+envBRDF_V.y;
	float RsF1_avg=textureLod(brdfTex,vec2(1,roughness),0).z;

	//AshikhminShirley
	vec3 f_diff=albedo.rgb*(1-RsF_L)*(1-RsF_V)/(PI*(1-RsF_avg)+0.1);
	vec3 f_spec=F*NDF_GGX(NdotH,alpha)*G_GGX(NdotL,NdotV,alpha);

	float fms=(1-RsF1_L)*(1-RsF1_V)/(PI*(1-RsF1_avg)+0.1);
	vec3 k=F_avg*RsF1_avg/(1-F_avg*(1-RsF1_avg));
	vec3 f_ms=k*fms;

	vec3 irradiance=light.radiance*light.color*max(dot(N,L),0)*PI;

	vec3 Lo=irradiance*(f_diff*(1-metallic)+f_spec+f_ms);

	//vec3 Lo_c=irradiance*f_c_spec+ambientIrradiance*f_c_spec_ambient;
	//Lo*=1-uboMaterial.clearcoat_thickness*Fc_V;
	//Lo+=uboMaterial.clearcoat_thickness*Lo_c;

	//=======================

	fragOut= vec4(Lo,1);
}