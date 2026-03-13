float4 BasicPS(float4 pos : SV_POSITION) : SV_TARGET
{
    float t = pos.x / 1260.0f;
    float3 colorLeft = float3(1.0f, 0.0f, 0.0f); // 赤
    float3 colorRight = float3(0.0f, 0.0f, 1.0f); // 青
    
    float3 finalColor = lerp(colorLeft, colorRight, t);

    return float4(finalColor, 1.0f);
}