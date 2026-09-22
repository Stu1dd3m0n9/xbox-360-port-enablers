/* blit_ps.hlsl: sample framebuffer texture. No discard (360 fxc rejects). */
sampler2D fb : register(s0);
float4 main( float2 Uv : TEXCOORD0 ) : COLOR { return tex2D( fb, Uv ); }
