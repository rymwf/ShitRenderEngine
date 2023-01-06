#version 460

#ifdef VULKAN
#define SET(x) set=x,
#define SAMPLE_INPUT subpassInput 
#define SAMPLE_INPUT_MS subpassInputMS 
#define SAMPLE_FETCH(a,b,c) subpassLoad(a)
#define INPUT_ATTACHMENT_INDEX(x) input_attachment_index = x,
#define PUSH_CONSTANT push_constant
#else
#define SET(x)
#define SAMPLE_INPUT sampler2D 
#define SAMPLE_INPUT_MS sampler2DMS
#define SAMPLE_FETCH(a,b,c) texelFetch(a,b,c)
#define INPUT_ATTACHMENT_INDEX(x)
#define PUSH_CONSTANT binding=0
#endif

layout (SET(1) INPUT_ATTACHMENT_INDEX(0) binding = 0) uniform SAMPLE_INPUT inputPos;
layout (SET(1) INPUT_ATTACHMENT_INDEX(1) binding = 1) uniform SAMPLE_INPUT inputAlbedo;
layout (SET(1) INPUT_ATTACHMENT_INDEX(2) binding = 2) uniform SAMPLE_INPUT inputN;
layout (SET(1) INPUT_ATTACHMENT_INDEX(3) binding = 3) uniform SAMPLE_INPUT inputMRA;
layout (SET(1) INPUT_ATTACHMENT_INDEX(4) binding = 4) uniform SAMPLE_INPUT inputEmission;

#define PI 3.141592653
#define ROUGHNESS_1_LOD 7
#define EPSILON 1e-6

layout(location = 0)out vec4 fragOut;

layout(SET(0) binding=0) buffer Camera{
	mat4 V;
	mat4 P;
	vec3 eyePos;
	int cascadeNum;
	float cascadeSplit[];
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
	mat4 P[6];
	mat4 V[6];
}light;
layout(SET(3)binding=8)uniform sampler2DArrayShadow shadowMap2D;
layout(SET(3)binding=9)uniform samplerCubeShadow shadowMapCube;

layout(SET(4) binding=11) uniform sampler2D brdfTex;

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
//float textureProj(vec4 shadowCoord,vec2 offset,uint cascadeIndex)
//{
	//if(shadowCoord.z>0.&&shadowCoord.z<1.&&shadowCoord.w>0)
	//{
		//float dist=texture(shadowMap2D,vec3(shadowCoord.st+offset,cascadeIndex)).r;
		//return float(dist>=shadowCoord.z);
	//}
	//return 1.;
//}
float filterPCF(vec3 p,vec3 N,int cascadeIndex)
{
	const float normalBiasFactor=0;
	vec3 shadowPos=p;//+N*normalBiasFactor;
	vec4 shadowCoord=light.P[cascadeIndex]*light.V[cascadeIndex]*vec4(shadowPos,1);
	shadowCoord.xyz/=shadowCoord.w;
	shadowCoord.xy=shadowCoord.xy*.5+.5;
	return texture(shadowMap2D,vec4(shadowCoord.xy,cascadeIndex,shadowCoord.z)).r;
}
//float filterHard(vec3 p,vec3 N,int cascadeIndex)
//{
//	const float normalBiasFactor=0;
//	vec3 shadowPos=p;//+N*normalBiasFactor;
//	vec4 shadowCoord=light.P[cascadeIndex]*light.V[cascadeIndex]*vec4(shadowPos,1);
//	shadowCoord.xyz/=shadowCoord.w;
//	shadowCoord.xy=shadowCoord.xy*.5+.5;
//	return textureProj(shadowCoord,vec2(0),cascadeIndex);
//}
//float occDepthAvg(vec3 shadowCoord,vec2 ds,uint cascadeIndex)
//{
//	float dist=0.;
//	mat2 rotM=rotate(rand(shadowCoord.xy)*2*PI);
//	int count=0;
//	int range=1;
//	for(int x=-range;x<=range;x++){
//		for(int y=-range;y<=range;y++){
//			float a=texture(shadowMap2D,vec3(shadowCoord.st+rotM*ds*vec2(x,y),cascadeIndex)).r;
//			if(a<shadowCoord.z)
//			{
//				dist+=a;
//				++count;
//			}
//		}
//	}
//	return count==0?0:dist/count;
//}
//float filterVSM(vec3 p,vec3 N,uint cascadeIndex)
//{
//	const float normalBiasFactor=0.05;
//	const float shadowFilterRadius=10;
//
//	vec3 shadowPos=p+N*normalBiasFactor;
//	vec4 shadowCoord=light.P[cascadeIndex]*light.V[cascadeIndex]*vec4(shadowPos,1);
//	shadowCoord.xyz/=shadowCoord.w;
//	shadowCoord.xy=shadowCoord.xy*.5+.5;
//	
//	ivec2 texDim=textureSize(shadowMap2D,0).xy;
//	vec2 ds=1./vec2(texDim);
//	
//	//blocker search
//	float scale=shadowCoord.z*shadowFilterRadius;
//	float d_occ=occDepthAvg(shadowCoord.xyz,ds*scale,cascadeIndex);
//	if(d_occ==0)
//	return 1.;
//	
//	float radius=(shadowCoord.z-d_occ)/shadowCoord.z*shadowFilterRadius;
//	//====================
//	float shadowFactor=0.;
//	
//	mat2 rotM=rotate(rand(shadowCoord.xy)*2*PI);
//	float dist0=texture(shadowMap2D,vec3(shadowCoord.xy,cascadeIndex)).r;
//	float M1=0.,M2=0;
//	
//	int count=0;
//	int range=2;
//	for(int x=-range;x<=range;x++){
//		for(int y=-range;y<=range;y++){
//			float a=texture(shadowMap2D,vec3(shadowCoord.xy+rotM*radius*ds*vec2(x,y),cascadeIndex)).r;
//			M1+=a;
//			M2+=a*a;
//			++count;
//		}
//	}
//	M1/=count;
//	M2/=count;
//	
//	if(M1>shadowCoord.z)
//	return 1.;
//	
//	float var=abs(M2-M1*M1);
//	float b=shadowCoord.z-M1;
//	float pmax=var/(var+b*b);
//	pmax=smoothstep(.2,1,pmax);
//	return pmax;
//}
//==========================
void main()
{
	ivec2 uv=ivec2(gl_FragCoord.xy);
	vec3 col=vec3(0);

	int sampleIndex=0;

	vec4 albedo=SAMPLE_FETCH(inputAlbedo,uv,sampleIndex);
	if(albedo.a<EPSILON)
	{
		fragOut=vec4(0);
		return;
	}

	vec3 pos=SAMPLE_FETCH(inputPos,uv,sampleIndex).rgb;
	vec3 N=SAMPLE_FETCH(inputN,uv,sampleIndex).rgb;
	vec3 MRA=SAMPLE_FETCH(inputMRA,uv,sampleIndex).rgb;
	vec3 emission=SAMPLE_FETCH(inputEmission,uv,sampleIndex).rgb;

	vec3 V=normalize(camera.eyePos-pos);
	vec3 lightPos=light.transformMatrix[3].xyz;
	vec3 L=vec3(0,1,0);
	switch(light.lightType)
	{
		case 0://direcitional
		{
			vec3 L_dir=mat3(light.transformMatrix)*vec3(0,0,-1);
			L=normalize(-L_dir);
		}
		break;
		case 1://point
		break;
		case 2://sphere
		break;
		case 3://spot
		break;
		case 4://tube
		break;
	}
	//vec3 l=lightPos-pos;
	//=============================
	//calc shadow 
	float depth=-(camera.V*vec4(pos,1)).z;
	int cascadeIndex=0;
	for(int i=0;i<camera.cascadeNum-1;++i)
	{
		if(depth>camera.cascadeSplit[i])
		cascadeIndex=i+1;
		else
		break;
	}
	//float shadowFactor=filterVSM(pos,N,cascadeIndex);
	//float shadowFactor=filterHard(pos,N,cascadeIndex);
	float shadowFactor=filterPCF(pos,N,cascadeIndex);

	const vec3 tempCols[]={
		{1.f,0,0},
		{0.,1.f,0},
		{0.,0,1.f},
		{1.f,0,1.f},
		{1.f,1.f,0},
	};
	vec3 tintColor=tempCols[cascadeIndex];
	//fragOut=vec4(shadowFactor*tintColor,1);
	//fragOut=vec4(tintColor,1);
	//fragOut=vec4(1,0,0,1);
	//return;

	//=============================
	vec3 F0 = mix(vec3(0.04),albedo.rgb, MRA.x);
	vec3 H=normalize(L+V);
	vec3 R=reflect(-V,N);

	float NdotH=dot(N,H);
	float HdotL=dot(H,L);
	float HdotV=max(dot(H,V),0);
	float NdotL=max(dot(N,L),0);
	float NdotV=dot(N,V);
	float LdotR=dot(L,R);

	vec3 F=fresnelSchlick(HdotL,F0);
	vec3 F_avg=(20*F0+1)/21.;
	vec3 F_V=fresnelSchlick(NdotV,F0);

	float alpha=MRA.y*MRA.y;

	vec2 envBRDF_V=textureLod(brdfTex,vec2(NdotV,MRA.y),0).xy;
	vec2 envBRDF_L=textureLod(brdfTex,vec2(NdotL,MRA.y),0).xy;

	vec3 RsF_L=envBRDF_L.x*F0+envBRDF_L.y;
	vec3 RsF_V=envBRDF_V.x*F0+envBRDF_V.y;
	vec3 RsF_avg=vec3(
		textureLod(brdfTex,vec2(F.r,MRA.y),0).z,
		textureLod(brdfTex,vec2(F.g,MRA.y),0).z,
		textureLod(brdfTex,vec2(F.b,MRA.y),0).z);

	float RsF1_L=envBRDF_L.x+envBRDF_L.y;
	float RsF1_V=envBRDF_V.x+envBRDF_V.y;
	float RsF1_avg=textureLod(brdfTex,vec2(1,MRA.y),0).z;

	//AshikhminShirley
	vec3 f_diff=albedo.rgb*(1-RsF_L)*(1-RsF_V)/(PI*(1-RsF_avg)+0.1);
	vec3 f_spec=F*NDF_GGX(NdotH,alpha)*G_GGX(NdotL,NdotV,alpha);

	float fms=(1-RsF1_L)*(1-RsF1_V)/(PI*(1-RsF1_avg)+0.1);
	vec3 k=F_avg*RsF1_avg/(1-F_avg*(1-RsF1_avg));
	vec3 f_ms=k*fms;

	vec3 irradiance=light.radiance*light.color*max(dot(N,L),0)*PI;

	vec3 Lo=irradiance*(f_diff*(1-MRA.x)+f_spec+f_ms)*shadowFactor;//*tintColor;

	//Lo*=tintColor;
	//vec3 IBL_Lo=E_spec;
	//==============================
	//fragOut=vec4(Lo,albedo.a);
	fragOut=vec4(Lo,1.);
}