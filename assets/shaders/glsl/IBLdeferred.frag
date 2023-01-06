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

layout(SET(4) binding=10) uniform samplerCube prefilteredEnvTex;
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

	//=============================
	vec3 F0 = mix(vec3(0.04),albedo.rgb, MRA.x);
	vec3 R=reflect(-V,N);

	vec3 E_spec=textureLod(prefilteredEnvTex,R,MRA.y*ROUGHNESS_1_LOD).rgb;
	vec3 E_diff=textureLod(prefilteredEnvTex,N,ROUGHNESS_1_LOD).rgb;

	float NdotV=max(dot(N,V),0);

	//hemispherical directional reflectance
	vec2 envBRDF=textureLod(brdfTex,vec2(NdotV,MRA.y),0).xy;
	vec3 RsF_V=envBRDF.x*F0+envBRDF.y;
	float RsF1_V=envBRDF.x+envBRDF.y;

	vec3 f_spec=F0*envBRDF.x+envBRDF.y;
	vec3 f_diff=albedo.rgb*(1-RsF_V)*(1-MRA.x);

	float RsF1_avg=textureLod(brdfTex,vec2(1,MRA.y),0).z;
	vec3 F_avg=(20.f*F0+1)/21.f;
	vec3 f_ms=F_avg*RsF1_avg*(1-RsF1_V)/(1-F_avg*(1-RsF1_avg));

	vec3 IBL_Lo=E_diff*f_diff+E_spec*(f_spec+f_ms)+emission;
	IBL_Lo*=MRA.z;
	//vec3 IBL_Lo=E_spec;
	//==============================
	//fragOut=vec4(IBL_Lo,albedo.a);
	fragOut=vec4(IBL_Lo,1.);
}