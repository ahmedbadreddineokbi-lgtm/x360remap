// Pixel shader du backend ImGui Xbox 360 (imgui_impl_dx9_xenon.cpp).
// Voir imgui_vs.hlsl pour le contexte complet (pivot du 2026-08-01, abandon
// de D3DXCompileShader au runtime). s0 : texture liee via SetTexture(0,...)
// (police ImGui ou toute texture de widget) - remplace le combinateur de
// texture fixe (SetTextureStageState), absent sur Xenos.
sampler2D texture0 : register(s0);

struct PS_INPUT
{
    float4 pos : POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

float4 main(PS_INPUT input) : COLOR
{
    return input.col * tex2D(texture0, input.uv);
}
