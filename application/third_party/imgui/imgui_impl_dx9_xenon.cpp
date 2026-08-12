// Backend Xbox 360 (Xenon) pour Dear ImGui v1.72b - reecrit le 2026-08-01
// suite a la confirmation reelle (erreurs de compilation) que le Xenos GPU
// n'a AUCUN pipeline fixe : SetTransform/GetTransform/SetTextureStageState
// n'existent pas du tout sur D3DDevice ("is not a member of 'D3DDevice'"),
// et D3DRS_LIGHTING/D3DRS_SHADEMODE/D3DRS_FOGENABLE n'existent pas non plus
// - coherent avec une architecture "unified shader" ou tout rendu passe par
// des shaders, sans exception (confirme aussi par la documentation
// Microsoft "Unsigned Xbox 360 Developer Best Practices" : "Avoid using the
// fixed-function pipeline. It is not supported on Xbox 360").
//
// Remplace le premier jet (base sur le imgui_impl_dx9.cpp officiel v1.72b,
// FVF + pipeline fixe, qui compile sur PC mais pas sur Xenos) par un chemin
// shader minimal :
//  - Declaration de vertex explicite (D3DVERTEXELEMENT9) au lieu du FVF
//    legacy - le FVF est aussi un concept du pipeline fixe, absent sur
//    Xenos (confirme via frankischilling/snes360-enhanced, homebrew Xbox
//    360 RGH/JTAG reel et buildable, verifie en lisant son vrai code source
//    le 2026-08-01).
//  - D3DUSAGE_CPU_CACHED_MEMORY au lieu de D3DUSAGE_DYNAMIC (absent sur
//    Xenos - memoire unifiee CPU/GPU, la distinction PC n'a pas la meme
//    justification), Lock() sans D3DLOCK_DISCARD (absent aussi - meme
//    pattern que snes360-enhanced : `vertexBuffer->Lock(0, 0, &p, NULL)`).
//
// PIVOT DU 2026-08-01 (6e build reel) : les vertex/pixel shaders NE SONT
// PLUS compiles au runtime via D3DXCompileShader. Cette approche compilait
// bien (elle est meme confirmee utilisee par snes360-enhanced) mais son
// LINK echouait avec 26 symboles non resolus internes a d3dx9.lib lui-meme
// (XShaderPDBBuilder_*, XConvertShaderToMicrocode, XGValidateMicrocode,
// D3DXShader::CompileToMicrocode, etc.) - recherche ciblee (voir
// PROJECT_NOTES.md, section "Sixieme build reel") confirmant qu'aucun
// projet Xbox 360 XDK reel et buildable (RetroArch-360, etc.) ne lie ni
// n'utilise ce chemin : ces symboles appartiennent aux composants internes
// du compilateur/outil de debug de shaders du XDK, pas a une bibliotheque
// destinee a etre linkee dans un binaire homebrew. Le vrai pattern partout
// observe : HLSL compile UNE FOIS, hors ligne (sur PC, au build), et
// microcode Xenon precompile charge directement via CreateVertexShader/
// CreatePixelShader. Source HLSL desormais dans shaders/imgui_vs.hlsl et
// shaders/imgui_ps.hlsl ; microcode precompile attendu dans
// shaders/imgui_vs.h et shaders/imgui_ps.h (voir shaders/README.md pour la
// procedure exacte - depend de l'outil de compilation HLSL fourni par le
// XDK installe, a verifier cote utilisateur).
//
// Point d'incertitude restant : la matrice de projection est envoyee via
// SetVertexShaderConstantF(0, ...), une methode D3D9 standard non-pipeline-
// fixe donc probablement presente sur Xenos, mais pas trouvee confirmee
// noir sur blanc dans le code source reel consulte - a verifier au premier
// rendu (si l'UI apparait deformee/mal positionnee mais PAS noire, c'est le
// premier suspect).
//
// <dinput.h> non inclus (jamais utilise, vestige du fichier officiel dont
// ce backend est adapte - DirectInput est une API PC).

#include "imgui.h"
#include "imgui_impl_dx9_xenon.h"

// <xtl.h> AVANT <d3d9.h> - le SDK Xbox 360 attend que <xtl.h> ait deja
// defini les typedefs Win32-style de base (DWORD, BOOL, WINAPI, VOID...)
// avant que <d3d9.h> (qui inclut lui-meme xbox.h/xconfig.h/xinputdefs.h)
// ne les utilise ; sans lui, ces headers du SDK partent en cascade
// d'erreurs de parsing - erreur reelle obtenue au build du 2026-08-01,
// uniquement dans ce fichier (main.cpp inclut deja <xtl.h> avant <d3d9.h>
// et n'a jamais eu ce probleme).
#include <xtl.h>
#include <d3d9.h>
#include <string.h>

// Microcode Xenon precompile (genere hors ligne a partir de shaders/
// imgui_vs.hlsl et imgui_ps.hlsl - voir shaders/README.md). Chaque header
// doit definir un tableau d'octets nomme g_ImGuiVertexShader /
// g_ImGuiPixelShader.
#include "shaders/imgui_vs.h"
#include "shaders/imgui_ps.h"

// DirectX data
static LPDIRECT3DDEVICE9             g_pd3dDevice = NULL;
static LPDIRECT3DVERTEXBUFFER9       g_pVB = NULL;
static LPDIRECT3DINDEXBUFFER9        g_pIB = NULL;
static LPDIRECT3DTEXTURE9            g_FontTexture = NULL;
static LPDIRECT3DVERTEXDECLARATION9  g_pVertexDecl = NULL;
static LPDIRECT3DVERTEXSHADER9       g_pVertexShader = NULL;
static LPDIRECT3DPIXELSHADER9        g_pPixelShader = NULL;
static int                           g_VertexBufferSize = 5000, g_IndexBufferSize = 10000;

// pos (2 floats) + uv (2 floats) + couleur (D3DCOLOR, 4 octets) = 20 octets,
// correspond exactement a kVertexElements ci-dessous.
struct CUSTOMVERTEX
{
    float    pos[2];
    float    uv[2];
    D3DCOLOR col;
};

static const D3DVERTEXELEMENT9 kVertexElements[] =
{
    { 0, 0,  D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
    { 0, 8,  D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
    { 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
    D3DDECL_END()
};

// La source HLSL vit desormais dans shaders/imgui_vs.hlsl et
// shaders/imgui_ps.hlsl (compilee hors ligne - voir shaders/README.md) ;
// plus de string litterale ici, pour eviter que les deux versions ne
// divergent avec le temps.

static bool ImGui_ImplDX9_CreateShaders()
{
    // g_ImGuiVertexShader / g_ImGuiPixelShader : microcode Xenon precompile,
    // fournis par shaders/imgui_vs.h et shaders/imgui_ps.h (voir includes en
    // tete de fichier). Plus de compilation au runtime (voir commentaire en
    // tete de fichier, "PIVOT DU 2026-08-01").
    HRESULT hr = g_pd3dDevice->CreateVertexShader((const DWORD*)g_ImGuiVertexShader, &g_pVertexShader);
    if (FAILED(hr))
        return false;

    hr = g_pd3dDevice->CreatePixelShader((const DWORD*)g_ImGuiPixelShader, &g_pPixelShader);
    if (FAILED(hr))
        return false;

    return true;
}

static void ImGui_ImplDX9_SetupRenderState(ImDrawData* draw_data)
{
    // Setup viewport
    D3DVIEWPORT9 vp;
    vp.X = vp.Y = 0;
    vp.Width = (DWORD)draw_data->DisplaySize.x;
    vp.Height = (DWORD)draw_data->DisplaySize.y;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    g_pd3dDevice->SetViewport(&vp);

    // Etats de rendu disponibles sur Xenos (verifie par compilation reelle -
    // ceux-la n'ont jamais ete signales comme absents, contrairement a
    // D3DRS_LIGHTING/SHADEMODE/FOGENABLE, retires).
    g_pd3dDevice->SetVertexShader(g_pVertexShader);
    g_pd3dDevice->SetPixelShader(g_pPixelShader);
    g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
    g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false);
    g_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, true);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

    // Matrice de projection orthographique, envoyee au vertex shader (pas
    // de SetTransform - n'existe pas sur Xenos). Valeurs numeriques reprises
    // telles quelles du imgui_impl_dx9.cpp officiel (deja pensees pour la
    // convention "vecteur * matrice", voir kVertexShaderSrc plus haut).
    {
        float L = draw_data->DisplayPos.x + 0.5f;
        float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x + 0.5f;
        float T = draw_data->DisplayPos.y + 0.5f;
        float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y + 0.5f;
        float mat_projection[16] =
        {
            2.0f/(R-L),   0.0f,         0.0f,  0.0f,
            0.0f,         2.0f/(T-B),   0.0f,  0.0f,
            0.0f,         0.0f,         0.5f,  0.0f,
            (L+R)/(L-R),  (T+B)/(B-T),  0.5f,  1.0f
        };
        g_pd3dDevice->SetVertexShaderConstantF(0, mat_projection, 4);
    }
}

// Render function.
void ImGui_ImplDX9_RenderDrawData(ImDrawData* draw_data)
{
    // Avoid rendering when minimized
    if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
        return;

    // Create and grow buffers if needed. D3DUSAGE_CPU_CACHED_MEMORY plutot
    // que D3DUSAGE_DYNAMIC (absent sur Xenos - voir en-tete de fichier).
    if (!g_pVB || g_VertexBufferSize < draw_data->TotalVtxCount)
    {
        if (g_pVB) { g_pVB->Release(); g_pVB = NULL; }
        g_VertexBufferSize = draw_data->TotalVtxCount + 5000;
        if (g_pd3dDevice->CreateVertexBuffer(g_VertexBufferSize * sizeof(CUSTOMVERTEX), D3DUSAGE_CPU_CACHED_MEMORY | D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &g_pVB, NULL) < 0)
            return;
    }
    if (!g_pIB || g_IndexBufferSize < draw_data->TotalIdxCount)
    {
        if (g_pIB) { g_pIB->Release(); g_pIB = NULL; }
        g_IndexBufferSize = draw_data->TotalIdxCount + 10000;
        if (g_pd3dDevice->CreateIndexBuffer(g_IndexBufferSize * sizeof(ImDrawIdx), D3DUSAGE_CPU_CACHED_MEMORY | D3DUSAGE_WRITEONLY, sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 : D3DFMT_INDEX32, D3DPOOL_DEFAULT, &g_pIB, NULL) < 0)
            return;
    }

    // Backup the DX9 state. NOTE : si CreateStateBlock echoue ici, cette
    // fonction s'arrete silencieusement (rien ne se dessine, pas de crash) -
    // a surveiller specifiquement si "ca compile et tourne mais rien ne
    // s'affiche".
    IDirect3DStateBlock9* d3d9_state_block = NULL;
    if (g_pd3dDevice->CreateStateBlock(D3DSBT_ALL, &d3d9_state_block) < 0)
        return;

    // Pas de sauvegarde/restauration de transform (SetTransform/GetTransform
    // n'existent pas sur Xenos) - la matrice de projection est un simple
    // registre de constante shader (c0-c3), deja couvert par
    // CreateStateBlock(D3DSBT_ALL) comme n'importe quel autre etat D3D9.

    // Copy and convert all vertices into a single contiguous buffer, convert
    // colors to DX9 default format. Lock() sans D3DLOCK_DISCARD (absent sur
    // Xenos - voir en-tete de fichier), 0 a la place.
    CUSTOMVERTEX* vtx_dst;
    ImDrawIdx* idx_dst;
    if (g_pVB->Lock(0, (UINT)(draw_data->TotalVtxCount * sizeof(CUSTOMVERTEX)), (void**)&vtx_dst, 0) < 0)
        return;
    if (g_pIB->Lock(0, (UINT)(draw_data->TotalIdxCount * sizeof(ImDrawIdx)), (void**)&idx_dst, 0) < 0)
        return;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_src = cmd_list->VtxBuffer.Data;
        for (int i = 0; i < cmd_list->VtxBuffer.Size; i++)
        {
            vtx_dst->pos[0] = vtx_src->pos.x;
            vtx_dst->pos[1] = vtx_src->pos.y;
            vtx_dst->col = (vtx_src->col & 0xFF00FF00) | ((vtx_src->col & 0xFF0000) >> 16) | ((vtx_src->col & 0xFF) << 16);     // RGBA --> ARGB for DirectX9
            vtx_dst->uv[0] = vtx_src->uv.x;
            vtx_dst->uv[1] = vtx_src->uv.y;
            vtx_dst++;
            vtx_src++;
        }
        memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    g_pVB->Unlock();
    g_pIB->Unlock();
    g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
    g_pd3dDevice->SetIndices(g_pIB);
    // SetVertexDeclaration plutot que SetFVF - le FVF legacy est un concept
    // du pipeline fixe, absent sur Xenos (voir en-tete de fichier).
    g_pd3dDevice->SetVertexDeclaration(g_pVertexDecl);

    // Setup desired DX state
    ImGui_ImplDX9_SetupRenderState(draw_data);

    // Render command lists
    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    ImVec2 clip_off = draw_data->DisplayPos;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != NULL)
            {
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplDX9_SetupRenderState(draw_data);
                else
                    pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                const RECT r = { (LONG)(pcmd->ClipRect.x - clip_off.x), (LONG)(pcmd->ClipRect.y - clip_off.y), (LONG)(pcmd->ClipRect.z - clip_off.x), (LONG)(pcmd->ClipRect.w - clip_off.y) };
                const LPDIRECT3DTEXTURE9 texture = (LPDIRECT3DTEXTURE9)pcmd->TextureId;
                g_pd3dDevice->SetTexture(0, texture);
                g_pd3dDevice->SetScissorRect(&r);
                g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, pcmd->VtxOffset + global_vtx_offset, 0, (UINT)cmd_list->VtxBuffer.Size, pcmd->IdxOffset + global_idx_offset, pcmd->ElemCount/3);
            }
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }

    // Restore the DX9 state
    d3d9_state_block->Apply();
    d3d9_state_block->Release();
}

bool ImGui_ImplDX9_Init(IDirect3DDevice9* device)
{
    ImGuiIO& io = ImGui::GetIO();
    io.BackendRendererName = "imgui_impl_dx9_xenon";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // fidele a l'upstream v1.72b

    g_pd3dDevice = device;
    g_pd3dDevice->AddRef();
    return true;
}

void ImGui_ImplDX9_Shutdown()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

static bool ImGui_ImplDX9_CreateFontsTexture()
{
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels;
    int width, height, bytes_per_pixel;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

    g_FontTexture = NULL;
    // D3DUSAGE_CPU_CACHED_MEMORY plutot que D3DUSAGE_DYNAMIC (voir en-tete
    // de fichier) - cette texture n'est ecrite qu'une fois ici de toute
    // facon (atlas de police, pas mis a jour a chaque frame).
    //
    // D3DFMT_LIN_A8R8G8B8 (format LINEAIRE) plutot que D3DFMT_A8R8G8B8 -
    // fix du 2026-08-01 : les rectangles pleine couleur (barre de titre,
    // onglets) s'affichaient correctement apres le fix /Zpr de la matrice
    // de projection, mais le texte (seul contenu texture) restait
    // invisible. Cause confirmee (documentation XNA/Xbox 360 officielle
    // Microsoft, format D3DFMT_LIN_* confirme comme constante XDK reelle
    // via recherche communautaire sur de vrais formats de texture Xbox 360)
    // : les textures Xbox 360 sont TILED (reagencees en memoire pour le
    // cache GPU) par defaut - un simple LockRect+memcpy ligne par ligne
    // (qui ne tient compte que de Pitch, pas de l'adressage par tuiles)
    // ecrit des donnees correctes en apparence mais physiquement
    // desordonnees au niveau de l'octet des qu'on les relit par
    // tfetch2D/sampling - un atlas de police, majoritairement transparent
    // avec des glyphes epars, se retrouve ainsi invisible presque partout.
    // D3DFMT_LIN_A8R8G8B8 force un agencement lineaire (meme convention
    // que sur PC), evitant ce probleme pour toute texture ecrite par le
    // CPU comme celle-ci.
    if (g_pd3dDevice->CreateTexture(width, height, 1, D3DUSAGE_CPU_CACHED_MEMORY, D3DFMT_LIN_A8R8G8B8, D3DPOOL_DEFAULT, &g_FontTexture, NULL) < 0)
        return false;
    D3DLOCKED_RECT tex_locked_rect;
    if (g_FontTexture->LockRect(0, &tex_locked_rect, NULL, 0) != D3D_OK)
        return false;
    for (int y = 0; y < height; y++)
        memcpy((unsigned char *)tex_locked_rect.pBits + tex_locked_rect.Pitch * y, pixels + (width * bytes_per_pixel) * y, (width * bytes_per_pixel));
    g_FontTexture->UnlockRect(0);

    io.Fonts->TexID = (ImTextureID)g_FontTexture;

    return true;
}

bool ImGui_ImplDX9_CreateDeviceObjects()
{
    if (!g_pd3dDevice)
        return false;
    if (!g_pVertexDecl && FAILED(g_pd3dDevice->CreateVertexDeclaration(kVertexElements, &g_pVertexDecl)))
        return false;
    if (!g_pVertexShader && !ImGui_ImplDX9_CreateShaders())
        return false;
    if (!ImGui_ImplDX9_CreateFontsTexture())
        return false;
    return true;
}

void ImGui_ImplDX9_InvalidateDeviceObjects()
{
    if (!g_pd3dDevice)
        return;
    if (g_pVB) { g_pVB->Release(); g_pVB = NULL; }
    if (g_pIB) { g_pIB->Release(); g_pIB = NULL; }
    if (g_pVertexDecl) { g_pVertexDecl->Release(); g_pVertexDecl = NULL; }
    if (g_pVertexShader) { g_pVertexShader->Release(); g_pVertexShader = NULL; }
    if (g_pPixelShader) { g_pPixelShader->Release(); g_pPixelShader = NULL; }
    if (g_FontTexture) { g_FontTexture->Release(); g_FontTexture = NULL; ImGui::GetIO().Fonts->TexID = NULL; }
}

void ImGui_ImplDX9_NewFrame()
{
    if (!g_FontTexture)
        ImGui_ImplDX9_CreateDeviceObjects();
}
