#pragma once

// Police "Inter", coupe optique 24pt, style Regular - SIL Open Font
// License 1.1 (texte complet : https://openfontlicense.org). Fournie par
// HB le 2026-08-08.
//
// Le tableau d'octets dans font_data_inter.cpp est GENERE AUTOMATIQUEMENT a
// partir du fichier source conserve dans assets/fonts/Inter_24pt-Regular.ttf
// - ne pas editer le .cpp a la main, regenerer depuis le .ttf si la police
// change (voir le commentaire en tete de font_data_inter.cpp).
//
// Pourquoi un tableau d'octets compile dans le binaire plutot qu'un fichier
// charge depuis HDD:\... (comme la premiere version de theme.cpp) : signale
// par HB (2026-08-08) - charger depuis HDD: obligerait CHAQUE utilisateur de
// X360RemapStudio.xex a se procurer la police et la deposer manuellement au
// bon endroit avant que l'interface ne s'affiche correctement. Incorporee
// dans le binaire, la police est presente pour tout le monde des le
// telechargement, sans manipulation - meme principe deja utilise dans ce
// projet pour le microcode des shaders ImGui (voir
// third_party/imgui/shaders/imgui_vs.h/imgui_ps.h).
extern const unsigned char kInterFont24ptRegularData[];
extern const int kInterFont24ptRegularSize;
