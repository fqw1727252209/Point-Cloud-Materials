// C Buffer 0 : 储存变换矩阵
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};


// VS 输入 <-> 顶点输入
struct VSInput
{
    // 位置 自动对齐到16字节
    float4 position : POSITION;
};

// VS 输出 <-> PS 输入
struct VSOutput
{
    float4 position : SV_POSITION;
    float4 raw_position: POSITION;
};

// Shader 入口
VSOutput main(VSInput input)
{
    VSOutput output;
    float4 worldPosition;
    // 改变顶点为四个分量齐次坐标.
    input.position.w = 1.0f;

    // 计算转换
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);


    // 原位置
    output.raw_position = input.position;

    return output;
}

