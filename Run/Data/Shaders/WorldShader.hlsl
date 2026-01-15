cbuffer CameraConstants : register(b2)
{
    float4x4 WorldToCameraTransform;
    float4x4 CameraToRenderTransform;
    float4x4 RenderToClipTransform;
}

cbuffer ModelConstants : register(b3)
{
    float4x4 ModelToWorldTransform;
    float4 ModelColor;
};

cbuffer WorldConstants : register(b5)
{
    float3 CameraWorldPosition;
    float Padding1;
    
    float4 IndoorLightColor;
    float4 OutdoorLightColor;
    float4 SkyColor;
    
    float FogNearDistance;
    float FogFarDistance;
    float FogMaxAlpha;
    float Padding3;
};

Texture2D diffuseTexture : register(t0);
SamplerState diffuseSampler : register(s0);

struct vs_input_t
{
    float3 localPosition : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

struct v2p_t
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0;
    float3 worldPosition : TEXCOORD1;
};

v2p_t VertexMain(vs_input_t input)
{
    v2p_t output;
    
    float4 localPos = float4(input.localPosition, 1.0f);
    float4 worldPos = mul(ModelToWorldTransform, localPos);
    
    float4 cameraPos = mul(WorldToCameraTransform, worldPos);
    float4 renderPos = mul(CameraToRenderTransform, cameraPos);
    float4 clipPos = mul(RenderToClipTransform, renderPos);
    
    output.position = clipPos;
    output.worldPosition = worldPos.xyz;
    output.color = input.color;
    output.uv = input.uv;
    
    return output;
}

float3 DiminishingAdd(float3 a, float3 b)
{
    // DiminishingAdd(a, b) = 1 - (1-a)(1-b)
    return 1.0f - (1.0f - a) * (1.0f - b);
}

float4 PixelMain(v2p_t input) : SV_TARGET
{
    float4 texColor = diffuseTexture.Sample(diffuseSampler, input.uv);
    
    if (texColor.a < 0.1f)
        discard;
    
    float outdoorInfluence = input.color.r;
    float indoorInfluence = input.color.g;
    float directionShade = input.color.b;
    
    float3 litColor;
    
    if (directionShade < 0.2f && outdoorInfluence > 0.3f)
    {
        // Tint模式：这是红色tint
        litColor = texColor.rgb * input.color.rgb;
    }
    else
    {
        float3 outdoorContribution = outdoorInfluence * OutdoorLightColor.rgb;
        float3 indoorContribution = indoorInfluence * IndoorLightColor.rgb;
        
        float3 combinedLight = DiminishingAdd(outdoorContribution, indoorContribution);
        
        litColor = texColor.rgb * combinedLight * directionShade;
    }
    
    litColor *= ModelColor.rgb;
    
    float distanceToCamera = length(input.worldPosition - CameraWorldPosition);
    
    float fogFraction = (distanceToCamera - FogNearDistance) / (FogFarDistance - FogNearDistance);
    fogFraction = saturate(fogFraction);
    
    float fogAlpha = fogFraction * FogMaxAlpha;
    
    float3 finalColor = lerp(litColor, SkyColor.rgb, fogAlpha);
    
    return float4(finalColor, texColor.a * ModelColor.a);
}