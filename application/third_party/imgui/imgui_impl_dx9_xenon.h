#pragma once
// Backend Xbox 360 (Xenon) adapte du imgui_impl_dx9 officiel **v1.72b**
// (ocornut/imgui, MIT) - PAS v1.90.9. Voir imgui_impl_dx9_xenon.cpp et
// ARCHITECTURE.md ("Rendu : Dear ImGui") pour pourquoi le coeur ImGui a ete
// retrograde de v1.90.9 a v1.72b le 2026-08-01 : le toolset XDK Xbox 360 ne
// supporte quasiment aucune feature C++11 (confirme par plusieurs vraies
// erreurs de build), et v1.90.9 declare son API publique avec des enums
// avant-declares a type sous-jacent explicite (`enum ImGuiDir : int;`) et
// `enum class`, deux constructions C++11 que ce compilateur ne parse pas du
// tout - pas patchable ligne a ligne, ca casse toute l'API publique. v1.72b
// (2019) n'utilise aucune des deux ; verifie en compilant reellement son
// coeur avec g++ -std=c++03 (aucune erreur) avant de l'integrer ici.
#include "imgui.h"
#ifndef IMGUI_DISABLE

struct IDirect3DDevice9;

bool ImGui_ImplDX9_Init(IDirect3DDevice9* device);
void ImGui_ImplDX9_Shutdown();
void ImGui_ImplDX9_NewFrame();
void ImGui_ImplDX9_RenderDrawData(ImDrawData* draw_data);

bool ImGui_ImplDX9_CreateDeviceObjects();
void ImGui_ImplDX9_InvalidateDeviceObjects();

#endif // #ifndef IMGUI_DISABLE
