// Vertex shader du backend ImGui Xbox 360 (imgui_impl_dx9_xenon.cpp).
// Source HLSL identique a celle qui etait auparavant compilee AU RUNTIME via
// D3DXCompileShader - abandonne le 2026-08-01 apres confirmation reelle
// (erreurs de link LNK2001 sur XShaderPDBBuilder_*/XConvertShaderToMicrocode/
// etc., internes a d3dx9.lib) que le compilateur de shaders runtime n'est
// pas une bibliotheque destinee a etre linkee dans un binaire homebrew
// (aucun projet Xbox 360 reel et buildable trouve ne l'utilise - voir
// PROJECT_NOTES.md, section "Sixieme build reel"). Ce fichier doit desormais
// etre compile UNE FOIS (build-time, sur PC, via l'outil de compilation
// HLSL du XDK) en microcode Xenon, dont le resultat est charge tel quel au
// runtime par CreateVertexShader() - voir shaders/README.md pour les
// instructions concretes.
//
// c0-c3 : matrice de projection orthographique, fournie par le C++ via
// SetVertexShaderConstantF(0, ...) (pas de SetTransform - n'existe pas sur
// Xenos, pipeline fixe absent).
float4x4 ProjectionMatrix : register(c0);

struct VS_INPUT
{
    float2 pos : POSITION;
    float2 uv  : TEXCOORD0;
    float4 col : COLOR0;
};

struct PS_INPUT
{
    float4 pos : POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = mul(float4(input.pos.xy, 0.0, 1.0), ProjectionMatrix);
    output.col = input.col;
    output.uv = input.uv;
    return output;
}
