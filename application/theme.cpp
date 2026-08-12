#include "theme.h"
#include "font_data_inter.h"

// Conversion hex -> ImVec4, alpha 1.0 par defaut - juste pour ecrire la
// palette de ETAT_DU_PROJET.md telle quelle (#0A0A0C etc) au lieu de la
// retranscrire a la main en flottants 0..1, source d'erreurs faciles a ne
// pas remarquer visuellement (une difference de 1/255 sur un fond quasi
// noir ne saute pas aux yeux, mieux vaut ecrire exactement le code hex
// donne par HB).
static ImVec4 HexColor(unsigned int hex, float alpha = 1.0f) {
    float r = ((hex >> 16) & 0xFF) / 255.0f;
    float g = ((hex >> 8) & 0xFF) / 255.0f;
    float b = (hex & 0xFF) / 255.0f;
    return ImVec4(r, g, b, alpha);
}

const ImVec4 kColorKeyboardAccent = ImVec4(0x8Bu / 255.0f, 0xD4u / 255.0f, 0x1Au / 255.0f, 1.0f); // #8BD41A
const ImVec4 kColorMouseAccent    = ImVec4(0x2Eu / 255.0f, 0x9Bu / 255.0f, 0xF0u / 255.0f, 1.0f); // #2E9BF0

void ApplyX360RemapTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Palette ETAT_DU_PROJET.md section 3 :
    //   Fond              #0A0A0C
    //   Cartes/panneaux   #131417
    //   Texte principal   blanc casse
    //   Texte secondaire  #8A94A6
    const ImVec4 bg            = HexColor(0x0A0A0C);
    const ImVec4 panel         = HexColor(0x131417);
    const ImVec4 panelHovered  = HexColor(0x1B1D22);
    const ImVec4 panelActive   = HexColor(0x22252B);
    const ImVec4 border        = HexColor(0x2A2D33);
    const ImVec4 textMain      = HexColor(0xF2F1ED);
    const ImVec4 textSecondary = HexColor(0x8A94A6);

    colors[ImGuiCol_Text]           = textMain;
    colors[ImGuiCol_TextDisabled]   = textSecondary;
    colors[ImGuiCol_WindowBg]       = bg;
    colors[ImGuiCol_ChildBg]        = panel;
    colors[ImGuiCol_PopupBg]        = panel;
    colors[ImGuiCol_Border]         = border;
    // Ombres portees : pas de support simple en ImGui 1.72b (voir
    // ETAT_DU_PROJET.md, contraintes techniques) - BorderShadow existe mais
    // ne dessine qu'une bordure supplementaire decalee, pas un vrai flou.
    // Desactive plutot que d'approximer mal ; a simuler plus tard si besoin
    // via un ImDrawList dedie (rectangle semi-transparent legerement decale
    // derriere chaque carte), pas via ce canal.
    colors[ImGuiCol_BorderShadow]   = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    colors[ImGuiCol_FrameBg]        = panel;
    colors[ImGuiCol_FrameBgHovered] = panelHovered;
    colors[ImGuiCol_FrameBgActive]  = panelActive;

    colors[ImGuiCol_TitleBg]          = bg;
    colors[ImGuiCol_TitleBgActive]    = bg;
    colors[ImGuiCol_TitleBgCollapsed] = bg;
    colors[ImGuiCol_MenuBarBg]        = panel;

    colors[ImGuiCol_ScrollbarBg]          = bg;
    colors[ImGuiCol_ScrollbarGrab]        = panelHovered;
    colors[ImGuiCol_ScrollbarGrabHovered] = panelActive;
    colors[ImGuiCol_ScrollbarGrabActive]  = kColorMouseAccent;

    colors[ImGuiCol_CheckMark]       = kColorMouseAccent;
    colors[ImGuiCol_SliderGrab]      = kColorMouseAccent;
    colors[ImGuiCol_SliderGrabActive] = kColorKeyboardAccent;

    colors[ImGuiCol_Button]        = panel;
    colors[ImGuiCol_ButtonHovered] = panelHovered;
    colors[ImGuiCol_ButtonActive]  = panelActive;

    colors[ImGuiCol_Header]        = panelHovered;
    colors[ImGuiCol_HeaderHovered] = panelActive;
    colors[ImGuiCol_HeaderActive]  = kColorMouseAccent;

    colors[ImGuiCol_Separator]        = border;
    colors[ImGuiCol_SeparatorHovered] = kColorMouseAccent;
    colors[ImGuiCol_SeparatorActive]  = kColorMouseAccent;

    colors[ImGuiCol_ResizeGrip]        = border;
    colors[ImGuiCol_ResizeGripHovered] = kColorMouseAccent;
    colors[ImGuiCol_ResizeGripActive]  = kColorMouseAccent;

    colors[ImGuiCol_Tab]                = panel;
    colors[ImGuiCol_TabHovered]         = panelHovered;
    colors[ImGuiCol_TabActive]          = panelActive;
    colors[ImGuiCol_TabUnfocused]       = panel;
    colors[ImGuiCol_TabUnfocusedActive] = panelHovered;

    colors[ImGuiCol_PlotLines]            = kColorMouseAccent;
    colors[ImGuiCol_PlotLinesHovered]     = kColorKeyboardAccent;
    colors[ImGuiCol_PlotHistogram]        = kColorMouseAccent;
    colors[ImGuiCol_PlotHistogramHovered] = kColorKeyboardAccent;

    colors[ImGuiCol_TextSelectedBg] = ImVec4(kColorMouseAccent.x, kColorMouseAccent.y, kColorMouseAccent.z, 0.35f);
    colors[ImGuiCol_DragDropTarget] = kColorKeyboardAccent;

    // Focus manette franc et visible (ETAT_DU_PROJET.md, contraintes
    // techniques : "pas de survol souris garanti") - c'est CE liseret qui
    // indique a l'utilisateur quel element la manette va activer, doit donc
    // etre net et contraste, pas une simple teinte discrete.
    colors[ImGuiCol_NavHighlight]          = kColorMouseAccent;
    colors[ImGuiCol_NavWindowingHighlight] = kColorMouseAccent;
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(bg.x, bg.y, bg.z, 0.7f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(bg.x, bg.y, bg.z, 0.7f);

    // Cartes aux coins arrondis avec bordure subtile (maquette 2, voir
    // ETAT_DU_PROJET.md) - arrondi modere, pas de style "bulle" exagere.
    style.WindowRounding   = 8.0f;
    style.ChildRounding    = 8.0f;
    style.FrameRounding    = 6.0f;
    style.PopupRounding    = 8.0f;
    style.TabRounding      = 6.0f;
    style.GrabRounding     = 6.0f;
    style.ScrollbarRounding = 8.0f;

    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize  = 1.0f;
    style.FrameBorderSize  = 1.0f;
    style.PopupBorderSize  = 1.0f;

    // Respiration genereuse - lecture a 2-3m (ETAT_DU_PROJET.md), un
    // agencement dense comme sur un ecran de PC serait illisible depuis un
    // canape. Valeurs de depart, a affiner une fois la mise en page de la
    // maquette 2 en place.
    style.WindowPadding = ImVec2(20.0f, 20.0f);
    style.FramePadding  = ImVec2(12.0f, 8.0f);
    style.ItemSpacing    = ImVec2(12.0f, 12.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 8.0f);
    style.IndentSpacing  = 24.0f;
}

void LoadX360RemapFont(float pixelSize) {
    ImGuiIO& io = ImGui::GetIO();

    // Plages de glyphes : Latin de base (0x0020-0x00FF, couvre deja les
    // accents francais les plus courants via Latin-1 Supplement : é è ê à
    // ù ç ...) + Latin Extended-A (0x0100-0x017F, couvre les caracteres
    // portugais restants : ã õ, et quelques francais rares comme œ selon
    // la police). Tableau statique : ImGui garde un pointeur vers ces
    // donnees, elles doivent survivre au-dela de cet appel (voir
    // ImFontAtlas::AddFontFromFileTTF, parametre glyph_ranges).
    static const ImWchar s_glyphRanges[] = {
        0x0020, 0x00FF, // Latin de base + Latin-1 Supplement
        0x0100, 0x017F, // Latin Extended-A
        0,
    };

    // Chargement DEPUIS LA MEMOIRE (2026-08-08, revu suite a une remarque de
    // HB) - plus un chargement depuis un fichier sur HDD:\... comme la
    // premiere version de cette fonction. Un fichier externe aurait oblige
    // CHAQUE utilisateur de X360RemapStudio.xex a se procurer la police et
    // la deposer manuellement au bon endroit ; le tableau d'octets
    // kInterFont24ptRegularData (font_data_inter.h/.cpp, genere depuis le
    // .ttf fourni par HB) est compile directement dans le binaire, donc
    // present pour tout le monde sans aucune manipulation - meme principe
    // que le microcode des shaders ImGui deja embarque dans ce projet.
    //
    // FontDataOwnedByAtlas = false est OBLIGATOIRE ici : par defaut, ImGui
    // prend possession du buffer passe a AddFontFromMemoryTTF et appelle
    // free() dessus a la destruction de l'atlas. kInterFont24ptRegularData
    // est un tableau `const` statique, pas une allocation sur le tas -
    // laisser ImGui tenter de le liberer corromprait la memoire. Voir le
    // commentaire d'AddFontFromMemoryTTF dans imgui.h.
    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;

    ImFont* font = io.Fonts->AddFontFromMemoryTTF(
        (void*)kInterFont24ptRegularData, kInterFont24ptRegularSize,
        pixelSize, &fontConfig, s_glyphRanges);

    // Repli de securite si jamais le chargement echoue quand meme (donnees
    // corrompues, atlas hors memoire...) - meme raisonnement que pour
    // l'ancienne version basee fichier : ne JAMAIS laisser l'atlas de
    // police vide, ca plante le rendu ImGui des la premiere frame.
    if (!font) {
        io.Fonts->AddFontDefault();
    }
}
