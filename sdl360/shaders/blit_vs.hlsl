/* blit_vs.hlsl: fullscreen-quad passthrough. 360-legal semantics only. */
struct VS_IN  { float4 Pos : POSITION; float2 Uv : TEXCOORD0; };
struct VS_OUT { float4 Pos : POSITION; float2 Uv : TEXCOORD0; };
VS_OUT main( VS_IN In ) {
    VS_OUT Out;
    Out.Pos = In.Pos;
    Out.Uv = In.Uv;
    return Out;
}
