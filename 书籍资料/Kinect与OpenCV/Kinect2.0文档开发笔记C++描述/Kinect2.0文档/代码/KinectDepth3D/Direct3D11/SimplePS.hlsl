

// PS 输入 <-> VS 输出
struct PSInput
{
    float4 position : SV_POSITION;
    float4 raw_position: POSITION;
};

// 光谱颜色
float4 spectral_color(float l){
    float t;
    float4 color = float4(0, 0, 0, 1);
    // R
    if ((l >= 400.0) && (l < 410.0)) { t = (l - 400.0) / (410.0 - 400.0); color.x = +(0.33*t) - (0.20*t*t); }
    else if ((l >= 410.0) && (l < 475.0)) { t = (l - 410.0) / (475.0 - 410.0); color.x = 0.14 - (0.13*t*t); }
    else if ((l >= 545.0) && (l < 595.0)) { t = (l - 545.0) / (595.0 - 545.0); color.x = +(1.98*t) - (t*t); }
    else if ((l >= 595.0) && (l < 650.0)) { t = (l - 595.0) / (650.0 - 595.0); color.x = 0.98 + (0.06*t) - (0.40*t*t); }
    else if ((l >= 650.0) && (l < 700.0)) { t = (l - 650.0) / (700.0 - 650.0); color.x = 0.65 - (0.84*t) + (0.20*t*t); }
    // G
    if ((l >= 415.0) && (l < 475.0)) { t = (l - 415.0) / (475.0 - 415.0); color.y = +(0.80*t*t); }
    else if ((l >= 475.0) && (l < 590.0)) { t = (l - 475.0) / (590.0 - 475.0); color.y = 0.8 + (0.76*t) - (0.80*t*t); }
    else if ((l >= 585.0) && (l < 639.0)) { t = (l - 585.0) / (639.0 - 585.0); color.y = 0.84 - (0.84*t); }
    // B
    if ((l >= 400.0) && (l < 475.0)) { t = (l - 400.0) / (475.0 - 400.0); color.y = +(2.20*t) - (1.50*t*t); }
    else if ((l >= 475.0) && (l < 560.0)) { t = (l - 475.0) / (560.0 - 475.0); color.y = 0.7 - (t)+(0.30*t*t); }
    return color;
}

// Shader 入口
float4 main(PSInput input) : SV_TARGET{
    float4 judgment = float4(0.4, 4.5, 400, 700);
    if ((input.raw_position.z >= judgment.x) && (input.raw_position.z <= judgment.y)){
        // 将 0.4~4.5映射到 400~700
        // 73.1707 = (judgment.w-judgment.z)*(judgment.y-judgment.x)
        return spectral_color(judgment.w - (input.raw_position.z - judgment.x) * 73.1707f) * 2.f;
    }
    return float4(0, 0, 0, 1);
}

/*
void spectral_color(double &r,double &g,double &b,double l) // RGB <0,1> <- lambda l <400,700> [nm]
    {
    double t;  r=0.0; g=0.0; b=0.0;
         if ((l>=400.0)&&(l<410.0)) { t=(l-400.0)/(410.0-400.0); r=    +(0.33*t)-(0.20*t*t); }
    else if ((l>=410.0)&&(l<475.0)) { t=(l-410.0)/(475.0-410.0); r=0.14         -(0.13*t*t); }
    else if ((l>=545.0)&&(l<595.0)) { t=(l-545.0)/(595.0-545.0); r=    +(1.98*t)-(     t*t); }
    else if ((l>=595.0)&&(l<650.0)) { t=(l-595.0)/(650.0-595.0); r=0.98+(0.06*t)-(0.40*t*t); }
    else if ((l>=650.0)&&(l<700.0)) { t=(l-650.0)/(700.0-650.0); r=0.65-(0.84*t)+(0.20*t*t); }
         if ((l>=415.0)&&(l<475.0)) { t=(l-415.0)/(475.0-415.0); g=             +(0.80*t*t); }
    else if ((l>=475.0)&&(l<590.0)) { t=(l-475.0)/(590.0-475.0); g=0.8 +(0.76*t)-(0.80*t*t); }
    else if ((l>=585.0)&&(l<639.0)) { t=(l-585.0)/(639.0-585.0); g=0.84-(0.84*t)           ; }
         if ((l>=400.0)&&(l<475.0)) { t=(l-400.0)/(475.0-400.0); b=    +(2.20*t)-(1.50*t*t); }
    else if ((l>=475.0)&&(l<560.0)) { t=(l-475.0)/(560.0-475.0); b=0.7 -(     t)+(0.30*t*t); }
    }
//---------------------------------------------------------------------------
*/