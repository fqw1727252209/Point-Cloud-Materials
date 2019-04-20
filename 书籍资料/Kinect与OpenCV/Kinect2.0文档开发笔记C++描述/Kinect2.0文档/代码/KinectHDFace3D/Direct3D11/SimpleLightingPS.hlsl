// C Buffer 0 : 储存平行光信息
cbuffer LightMaterialBuffer : register(b0) {
    // 平行光方向
    float3 lightDirection   : packoffset(c0.x);
    // 高光指数
    float shininess : packoffset(c0.w);
    // 光源位置
    float4 lightPosition: packoffset(c1);
    // 光源颜色
    float4 lightColor: packoffset(c2);
    // 光源的环境光反射系数
    float4 globalAmbient: packoffset(c3);
    // 摄像机的位置
    float4 cameraPosition: packoffset(c4);
    // 材质的自发光
    float4 Ke: packoffset(c5);
    // 材质的环境光系数
    float4 Ka: packoffset(c6);
    // 材质的漫反射系数
    float4 Kd: packoffset(c7);
    // 材质的高光系数
    float4 Ks: packoffset(c8);
}

// PS 输入 <-> GS 输出
struct PSInput
{
    float4 position : SV_POSITION;
    float4 worldposition: POSITION;
    float3 worldnormal: NORMAL;
};

// Shader 入口
float4 main(PSInput input) : SV_TARGET{

    float3 P = input.worldposition.xyz;

    float3 N = normalize(input.worldnormal);

    // 自发光颜色
    float4 emissive = Ke;

    // 环境光
    float4 ambient = Ka * globalAmbient;

    // 计算漫反射光
    // 用LightDirection就是纯平行光
    // 光源位置减顶点位置，是不考虑衰减的点光源
    float3 L = normalize(lightPosition.xyz - P);
    float diffuseLight = max(dot(N, L), 0);
    float4 diffuse = Kd * lightColor * diffuseLight;;
    // 计算高光
    float3 V = normalize(cameraPosition.xyz - P);
    float3 H = normalize(L + V);
    float specularLight = pow(max(dot(N, H), 0), shininess);

    if (diffuseLight <= 0){
        specularLight = 0;
    }
    float4 specular = Ks * lightColor * specularLight;
    // 合成
    float4 finalcolor = emissive + ambient + diffuse + specular;

    return finalcolor;
}
