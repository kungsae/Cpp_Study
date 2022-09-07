//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
//***************************************************************************************

// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#define MaxLights 16

struct Light
{
    float3 Strength;     // 빛의 색상
    float FalloffStart; // point/spot light only
    float3 Direction;   // directional/spot light only
    float FalloffEnd;   // point/spot light only
    float3 Position;    // point light only
    float SpotPower;    // spot light only
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    // 광택도는 거칠기의 반대이다: 광택도 : 1 - 거칠기
    float Shininess;
};

// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
// 프레넬 방정식의 슐릭 근사를 구한다.
// 법선이 n인 표면에서 프레넬 효과에 의해 반사되는 빛의 비율을 빛 벡터 L과 표면 법선 n 사이의 각도에 근거해서 근사한다.
// R0 = ((n-1)/(n+1))^2, 여기서 n은 굴절 지수
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}

// 눈에 도달한 반사광의 양을 계산한다. 
// 즉 분산 반사와 반영 반사의 합을 구한다.
float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    // 거칠기에서 얻은 광택도로부터 m을 유도한다.
    const float m = mat.Shininess * 256.0f;
    float3 halfVec = normalize(toEye + lightVec);

    float roughnessFactor = (m + 8.0f) / 8.0f * pow(max(dot(halfVec, normal), 0.0f), m) ;
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);

    float3 specAlbedo = fresnelFactor * roughnessFactor;

    // 반영 반사율 공식이 [0, 1] 구간 바깥의 값을 낼 수도 있지만
    // 우리는 LDR 렌더링을 구현하므로 반사율을 1미만으로 낮춘다.
    // LDR : low dynamic range (저동적범위) [ 0, 1 ] 구간에 있다고 기대한다.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

//---------------------------------------------------------------------------------------
// Evaluates the lighting equation for directional lights.
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // 빛 벡터는 광선들이 나아가는 방향의 반대 방향을 가리킨다.
    float3 lightVec = -L.Direction;

    // 람베르트 코사인 법칙에 따라 빛의 세기를 줄인다.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

// Constant data that varies per frame.

cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

cbuffer cbMaterial : register(b1)
{
	float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float  gRoughness;
	float4x4 gMatTransform;
};

// Constant data that varies per material.
cbuffer cbPass : register(b2)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;

    float4 gAmbientLight;

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
};
 
struct VertexIn
{
	float3 PosL    : POSITION;
    float3 NormalL : NORMAL;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;
	
    // Transform to world space.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

    // Transform to homogeneous clip space.
    vout.PosH = mul(posW, gViewProj);

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    pin.NormalW = normalize(pin.NormalW);

    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    float4 ambient = gAmbientLight * gDiffuseAlbedo;

    const float shiniess = 1.0f - gRoughness;
    Material mat = { gDiffuseAlbedo ,gFresnelR0 ,shiniess };

    float shadowFactor = 1.0f;
    float3 result = 0.0f;
    int i = 0;

    for (i = 0; i < 3; i++)
    {
        float3 lightVec = gLights[i].Position - pin.PosW;

        float d = length(lightVec);

        lightVec = normalize(lightVec);

        float ndotL = max(dot(lightVec, pin.NormalW), 0.0f);
        float3 lightStrength = gLights[i].Strength * ndotL;

        float att = saturate((gLights[i].FalloffEnd - d) / (gLights[i].FalloffEnd - gLights[i].FalloffStart));

        lightStrength *= att;

        const float m = mat.Shininess * 256.0f;
        float3 halfVec = normalize(toEyeW + lightVec);

        float roughnessFactor = (m + 8.0f) / 8.0f * pow(max(dot(halfVec, pin.NormalW), 0.8f), m);
        
        float cosinAngle = saturate(dot(pin.NormalW, lightVec));
        float f0 = 1.0f - cosinAngle;
        float3 fresnelFactor = mat.FresnelR0 + (1.0f - mat.FresnelR0) * (f0 * f0 * f0 * f0 * f0);

        float3 specAlbedo = roughnessFactor * fresnelFactor;

        specAlbedo = specAlbedo / (specAlbedo + 1.0f);

        result += shadowFactor * lightStrength * (mat.DiffuseAlbedo.rgb + specAlbedo);  

    }


    float4 directLight = float4(result, 0.0f);

    //float vdotn = max(dot(toEyeW, pin.NormalW), 0);
    //if (vdotn < 0.3f)
    //    directLight *= 0.0f;

    //float diffuse = saturate(directLight);
    //directLight = ceil(diffuse * 5) / 5.0f;

    float4 litColor = ambient+ directLight;

    litColor.a = gDiffuseAlbedo.a;


    return litColor;
}


