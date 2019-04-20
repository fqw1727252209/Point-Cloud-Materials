// VS 输出 <-> GS 输入
struct GSInput
{
    float4 position : SV_POSITION;
    float4 worldposition: POSITION;
};

// PS 输入 <-> GS 输出
struct GSOutput
{
    float4 position : SV_POSITION;
    float4 worldposition: POSITION;
    float3 worldnormal: NORMAL;
};


// 入口
[maxvertexcount(3)]
void main(triangle GSInput input[3], inout TriangleStream<GSOutput> OutputStream)
{
    GSOutput output;
    // 向量A
    float3 faceEdgeA = input[1].position.xyz - input[0].position.xyz;
    // 向量B
    float3 faceEdgeB = input[2].position.xyz - input[0].position.xyz;
    // 计算法线 
    output.worldnormal = normalize(cross(faceEdgeA, faceEdgeB));
    for (int i = 0; i < 3; i++){
        output.position = input[i].position;
        output.worldposition = input[i].worldposition;
        OutputStream.Append(output);
    }
    // 结束三角面
    //OutputStream.RestartStrip();
}