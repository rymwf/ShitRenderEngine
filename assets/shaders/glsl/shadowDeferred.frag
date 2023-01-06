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

#define PI 3.141592653
#define EPSILON 1e-6

layout(location = 0)out float fragOut;

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
layout(SET(3)binding=8)uniform sampler2DArray shadowMap2D;
layout(SET(3)binding=9)uniform samplerCube shadowMapCube;

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
float rand(vec2 co){
	return fract(sin(dot(co,vec2(12.9898,78.233)))*43758.5453);
}

mat2 rotate(float angle)
{
	float sinAngle=sin(angle);
	float cosAngle=cos(angle);
	return mat2(cosAngle,sinAngle,-sinAngle,cosAngle);
}
float textureProj(vec4 shadowCoord,vec2 offset,uint cascadeIndex)
{
	if(shadowCoord.z>0.&&shadowCoord.z<1.&&shadowCoord.w>0)
	{
		float dist=texture(shadowMap2D,vec3(shadowCoord.st+offset,cascadeIndex)).r;
		return float(dist>=shadowCoord.z);
	}
	return 1.;
}
float filterHard(vec3 p,vec3 N,int cascadeIndex)
{
	const float normalBiasFactor=0;
	vec3 shadowPos=p;//+N*normalBiasFactor;
	vec4 shadowCoord=light.P[cascadeIndex]*light.V[cascadeIndex]*vec4(shadowPos,1);
	shadowCoord.xyz/=shadowCoord.w;
	shadowCoord.xy=shadowCoord.xy*.5+.5;
	return textureProj(shadowCoord,vec2(0),cascadeIndex);
}
float occDepthAvg(vec3 shadowCoord,vec2 ds,uint cascadeIndex)
{
	float dist=0.;
	mat2 rotM=rotate(rand(shadowCoord.xy)*2*PI);
	int count=0;
	int range=1;
	for(int x=-range;x<=range;x++){
		for(int y=-range;y<=range;y++){
			float a=texture(shadowMap2D,vec3(shadowCoord.st+rotM*ds*vec2(x,y),cascadeIndex)).r;
			if(a<shadowCoord.z)
			{
				dist+=a;
				++count;
			}
		}
	}
	return count==0?0:dist/count;
}
float filterVSM(vec3 p,vec3 N,uint cascadeIndex)
{
	const float normalBiasFactor=0.05;
	const float shadowFilterRadius=10;

	vec3 shadowPos=p+N*normalBiasFactor;
	vec4 shadowCoord=light.P[cascadeIndex]*light.V[cascadeIndex]*vec4(shadowPos,1);
	shadowCoord.xyz/=shadowCoord.w;
	shadowCoord.xy=shadowCoord.xy*.5+.5;
	
	ivec2 texDim=textureSize(shadowMap2D,0).xy;
	vec2 ds=1./vec2(texDim);
	
	//blocker search
	float scale=shadowCoord.z*shadowFilterRadius;
	float d_occ=occDepthAvg(shadowCoord.xyz,ds*scale,cascadeIndex);
	if(d_occ==0)
	return 1.;
	
	float radius=(shadowCoord.z-d_occ)/shadowCoord.z*shadowFilterRadius;
	//====================
	float shadowFactor=0.;
	
	mat2 rotM=rotate(rand(shadowCoord.xy)*2*PI);
	float dist0=texture(shadowMap2D,vec3(shadowCoord.xy,cascadeIndex)).r;
	float M1=0.,M2=0;
	
	int count=0;
	int range=2;
	for(int x=-range;x<=range;x++){
		for(int y=-range;y<=range;y++){
			float a=texture(shadowMap2D,vec3(shadowCoord.xy+rotM*radius*ds*vec2(x,y),cascadeIndex)).r;
			M1+=a;
			M2+=a*a;
			++count;
		}
	}
	M1/=count;
	M2/=count;
	
	if(M1>shadowCoord.z)
	return 1.;
	
	float var=abs(M2-M1*M1);
	float b=shadowCoord.z-M1;
	float pmax=var/(var+b*b);
	pmax=smoothstep(.2,1,pmax);
	return pmax;
}

//==========================
void main()
{
	ivec2 uv=ivec2(gl_FragCoord.xy);
	vec3 col=vec3(0);

	int sampleIndex=0;

	vec4 albedo=SAMPLE_FETCH(inputAlbedo,uv,sampleIndex);
	if(albedo.a<EPSILON)
	{
		fragOut=1.;
		return;
	}

	vec3 pos=SAMPLE_FETCH(inputPos,uv,sampleIndex).rgb;
	vec3 N=SAMPLE_FETCH(inputN,uv,sampleIndex).rgb;

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
	float shadowFactor=filterVSM(pos,N,cascadeIndex);

	//float shadowFactor=filterHard(pos,N,cascadeIndex);

	const vec3 tempCols[]={
		{1.f,0,0},
		{0.,1.f,0},
		{0.,0,1.f},
		{1.f,0,1.f},
		{1.f,1.f,0},
	};
	vec3 tintColor=tempCols[cascadeIndex];

	fragOut=shadowFactor;
}