// application.xex - homebrew graphique de configuration (wizard clavier,
// reglages souris, profils a venir). Voir ARCHITECTURE.md pour la decision
// XUI -> Dear ImGui du 2026-08-01.
//
// STATUT : PREMIER JET NON COMPILE, comme le reste des fichiers touchant a
// D3D9/XUI/XInput dans ce dossier - je ne peux ni compiler ni executer de
// code Xbox depuis mon bac a sable. Voir BUILD_APPLICATION.md pour la marche
// a suivre en cas d'erreur de build (attendu, pas une surprise).
//
// Dependances a ajouter au projet avant de compiler (voir BUILD_APPLICATION.md) :
//  - Coeur Dear ImGui (third_party/imgui/, voir VENDORING.md - fichiers non
//    fournis ici, a copier depuis la release officielle v1.90.9)
//  - XexUtils (github.com/ClementDreptin/XexUtils, MIT) comme reference de
//    projet VS, pour Xam::Notify (notifications natives) et Input (detection
//    d'appui bouton) plutot que du code XNotifyUI/XInput fait main.

#include <xtl.h>
#include <d3d9.h>

#include "third_party/imgui/imgui.h"
#include "third_party/imgui/imgui_impl_dx9_xenon.h"

#include "wizard_session.h"
#include "calibration_session.h" // assistant de calibration souris (2026-08-03)
#include "input_state_reader.h"
#include "config_writer.h"
#include "i18n.h"
#include "theme.h"
#include "donation_links.h" // liens de soutien au projet, onglet "Soutenir le projet" (2026-08-11)

// XexUtils - vendorise dans hiddriver360-master/XexUtils/ (voir
// BUILD_APPLICATION.md et ARCHITECTURE.md pour comment il est integre).
// API reelle confirmee en lisant XexUtils/src/Xam.h et Input.h directement
// (2026-08-01) - remplace les noms devines dans une premiere version de ce
// fichier (Xam::Notify(wstring), XInputGetState brut) :
//  - XexUtils::Xam::XNotify(const std::string&, XNOTIFYQUEUEUI_TYPE) : notif
//    systeme, prend une string etroite (pas wstring) et convertit en interne.
//  - XexUtils::Input::GetInput(uint32_t userIndex = 0) -> Gamepad* : jamais
//    nullptr (retourne toujours un pointeur vers un slot statique interne,
//    meme si la manette est deconnectee), avec wButtons/PressedButtons deja
//    debounces et ThumbLeftX/Y deja normalises en [-1,+1] avec deadzone
//    appliquee - plus besoin de le refaire a la main.
//
// Chemin relatif explicite (pas juste "Input.h"/"Xam.h") et PAS de
// XexUtils/src dans AdditionalIncludeDirectories - voir application.vcxproj
// pour pourquoi (Math.h y portait le meme nom que le math.h standard et
// shadowait tous les #include <math.h>/<cmath> du projet, insensibilite a
// la casse de Windows oblige - cause reelle de la cascade d'erreurs
// "acosf n'est pas membre de global namespace" du premier build). Un chemin
// relatif explicite n'a pas ce probleme : seul le dossier du fichier
// includant (ici XexUtils/src lui-meme, pour les #include internes de
// Input.h/Xam.h) est cherche, jamais tout le projet.
//
// <cstdint>/<string>/<vector> ajoutes explicitement avant : Input.h/Xam.h
// utilisent uint32_t/std::string/std::vector sans les inclure eux-memes (ils
// sont ecrits pour etre inclus apres XexUtils/src/pch.h, qui les fournit -
// voir Input.cpp/Xam.cpp qui font "#include \"pch.h\"" en premiere ligne).
// Erreur reelle obtenue au premier build : "'string': is not a member of
// 'std'" sur Xam.h.
#include <cstdint>
#include <string>
#include <vector>
#include <fstream>
#include <cstdarg>
#include <cstdlib> // strtoul - saisie du titleId hexadecimal, onglet Profils (Jalon 7)
#include <cstdio>  // std::remove - bascule du mode compatibilite, onglet Calibration
#include <string.h>
#include "../XexUtils/src/Input.h"
#include "../XexUtils/src/Xam.h"
#include "../XexUtils/src/Filesystem.h"

// --- Log persistant cote appli --------------------------------------------
// Demande utilisateur du 2026-08-02 : "un systeme plus robuste de
// verification et de log... pour eviter l'aller-venu entre pc et console".
// Meme esprit que FileLog() dans hiddriver/main.cpp (deja existant, cote
// plugin) mais ici cote application.xex - un seul fichier a recuperer via
// "rgh fs cat" apres coup regroupe l'historique des sauvegardes tentees
// (wizard, reglages souris, boutons souris, defaut), avec succes/echec
// explicite plutot qu'une notification XNotify ephemere qu'on peut manquer.
// Troncature au premier appel de chaque lancement de application.xex
// (2026-08-03, meme demande HB que pour X360Remap_plugin.log : le fichier
// s'accumulait sur plusieurs lancements/redemarrages, melangeant plusieurs
// sessions de test dans le meme rgh fs cat). L'appli est mono-thread au
// sens ou AppLog n'est appele que depuis la boucle ImGui principale, pas de
// synchronisation requise.
static bool g_appLogTruncatedThisRun = false;
static void AppLog(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    _vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    buf[sizeof(buf) - 1] = '\0';
    va_end(args);

    std::ios_base::openmode mode = std::ios::binary;
    mode |= g_appLogTruncatedThisRun ? std::ios::app : std::ios::trunc;

    std::ofstream f("HDD:\\X360RemapStudio\\X360Remap_studio.log", mode);
    if (f.is_open()) {
        f << "[" << GetTickCount() << "ms] " << buf << "\r\n";
        g_appLogTruncatedThisRun = true;
    }
}

// --- Etat de l'application -----------------------------------------------
enum AppTab {
    TAB_WIZARD = 0,
    TAB_MOUSE_SETTINGS,
    TAB_PROFILES,
    TAB_CALIBRATION,
    // Ajoutes 2026-08-11 (deux pages supplementaires demandees par HB)
    TAB_HOWTO,
    TAB_SUPPORT,
};

static AppTab g_activeTab = TAB_WIZARD;
static WizardSession* g_wizard = nullptr;
static CalibrationSession* g_calib = nullptr;

// Snapshot lu une seule fois par frame (dans main(), voir la boucle plus
// bas) et partage par DrawWizardTab()/UpdateMouseCursor() - evite 2 lectures
// disque separees du meme fichier par frame comme le faisait la toute
// premiere version. Ajoute le 2026-08-01 en meme temps que le diagnostic
// visuel ci-dessous (voir DrawDebugOverlay), suite a un retour utilisateur
// ou ni le clavier ni le curseur souris ne fonctionnaient - ce global rend
// aussi possible d'afficher exactement ce que le jeu voit, plutot que de
// deviner cote code.
static InputStateSnapshot g_lastSnapshot;

// VID/PID en dur pour ce premier jet - un vrai ecran de selection de
// peripherique (liste de ce qui est present dans X360Remap.json) viendra une
// fois cet ecran unique confirme buildable et navigable sur console.
static const uint16_t kKeyboardVid = 1121;
static const uint16_t kKeyboardPid = 16193;
static const uint16_t kMouseVid = 1121;
static const uint16_t kMousePid = 20021;

// Jalon 8 (application/ROADMAP_APPLICATION.md) - peripherique EFFECTIVEMENT
// utilise par tous les appels config_writer.h de ce fichier ; les constantes
// ci-dessus ne restent qu'une valeur de repli (premier lancement, aucun
// device encore dans X360Remap.json - voir ListKnownDevices). Initialisees
// au repli par defaut, changees uniquement par DrawDeviceSelector plus bas
// (voir aussi son commentaire pour le detail des heuristiques clavier/souris).
static uint16_t g_selectedKeyboardVid = kKeyboardVid;
static uint16_t g_selectedKeyboardPid = kKeyboardPid;
static uint16_t g_selectedMouseVid = kMouseVid;
static uint16_t g_selectedMousePid = kMousePid;

// Reglages souris en cours d'edition - charges une fois au demarrage de
// l'onglet, ecrits sur disque uniquement quand l'utilisateur confirme
// (bouton "Enregistrer"), pas a chaque frame.
static int g_mouseSensitivity = 10000;
static int g_mouseDeadzone = 0;
static bool g_mouseInvertY = false;
static bool g_mouseSettingsLoaded = false;

// Mapping des 3 clics souris (gauche/droit/milieu, idx 0/1/2 - convention
// HID boot-mouse) vers un bouton Xbox. Demande utilisateur du 2026-08-02
// ("pourquoi y'a pas un mapping pour les buttons souris"). Liste
// volontairement restreinte aux cibles les plus utiles plutot que les 17
// champs de kButtonFieldNames en entier (mapping.cpp) - "Par defaut" (index
// 0) signifie "ne pas ecrire d'entree pour cet index", ce qui laisse
// hiddriver.xex appliquer son fallback code en dur (gauche->r2, droit->l2,
// milieu->r3, voir HidFillMouseState dans hiddriver/main.cpp).
// ATTENTION (limite connue, acceptee pour ce round) : "Enregistrer boutons
// souris" remplace TOUJOURS entierement les entrees idx 0/1/2 par ce que
// montrent ces 3 combos - une entree existante avec un champ hors de cette
// liste (improbable en pratique pour un clic souris) serait affichee comme
// "Par defaut" et donc perdue si on clique Enregistrer sans y prendre garde.
struct MouseButtonTarget { StrId labelId; const char* fieldName; };
static const MouseButtonTarget kMouseButtonTargets[] = {
    { STR_SETTINGS_TARGET_DEFAULT, nullptr },
    { STR_SETTINGS_TARGET_A, "a_button" },
    { STR_SETTINGS_TARGET_B, "b_button" },
    { STR_SETTINGS_TARGET_X, "x_button" },
    { STR_SETTINGS_TARGET_Y, "y_button" },
    { STR_SETTINGS_TARGET_LB, "l1" },
    { STR_SETTINGS_TARGET_RB, "r1" },
    { STR_SETTINGS_TARGET_LT, "l2" },
    { STR_SETTINGS_TARGET_RT, "r2" },
    { STR_SETTINGS_TARGET_L3, "l3" },
    { STR_SETTINGS_TARGET_R3, "r3" },
    { STR_SETTINGS_TARGET_START, "start" },
    { STR_SETTINGS_TARGET_BACK, "back" },
    // D-Pad ajoute le 2026-08-03 (retour HB : "les choix de touche pour la
    // molette avant arriere sont limites"). Utile en particulier pour la
    // molette, dont le comportement PAR DEFAUT est justement D-Pad Haut/Bas
    // pour la navigation Aurora : sans ces entrees, il etait impossible de
    // reassigner explicitement la molette sur D-Pad Gauche/Droite, ni de
    // remettre Haut/Bas apres avoir essaye autre chose (le seul retour en
    // arriere possible etait "Par defaut"). Necessite le correctif du meme
    // jour dans ApplyButtonFieldToXInput (hiddriver/main.cpp), qui ignorait
    // silencieusement toute cible D-Pad.
    { STR_SETTINGS_TARGET_DPAD_UP, "dpad_up" },
    { STR_SETTINGS_TARGET_DPAD_DOWN, "dpad_down" },
    { STR_SETTINGS_TARGET_DPAD_LEFT, "dpad_left" },
    { STR_SETTINGS_TARGET_DPAD_RIGHT, "dpad_right" },
};
static const int kMouseButtonTargetCount = sizeof(kMouseButtonTargets) / sizeof(kMouseButtonTargets[0]);

static int g_mouseBtnLeftSel = 0;
static int g_mouseBtnRightSel = 0;
static int g_mouseBtnMiddleSel = 0;

// Molette avant/arriere (idx 3/4, convention reservee - voir
// FindWheelOverride dans hiddriver/main.cpp). Par defaut ("Par defaut" =
// index 0, comme les clics ci-dessus) elle reste cablee sur D-Pad Haut/Bas
// pour la navigation Aurora - demande utilisateur du 2026-08-02 de la rendre
// configurable, au prix de perdre ce raccourci une fois assignee.
static int g_mouseWheelForwardSel = 0;
static int g_mouseWheelBackwardSel = 0;

static bool MouseButtonTargetGetter(void*, int idx, const char** out_text) {
    if (idx < 0 || idx >= kMouseButtonTargetCount)
        return false;
    *out_text = Tr(kMouseButtonTargets[idx].labelId);
    return true;
}

static int FindMouseButtonTargetIndex(uint8_t ButtonsReport::* field) {
    const char* name = GetButtonFieldName(field);
    if (!name)
        return 0;
    for (int i = 1; i < kMouseButtonTargetCount; i++) {
        if (strcmp(kMouseButtonTargets[i].fieldName, name) == 0)
            return i;
    }
    return 0; // hors de la liste restreinte - voir le commentaire au-dessus
}

static void LoadMouseSettingsOnce() {
    if (g_mouseSettingsLoaded)
        return;
    g_mouseSettingsLoaded = true;

    // Reset a "Par defaut" avant de relire le fichier - sans ca, un
    // peripherique qui n'a pas (encore) d'entree pour un idx donne (ou pas
    // de mapping du tout, m == nullptr) HERITAIT silencieusement des valeurs
    // du peripherique precedemment selectionne, encore en memoire dans ces
    // globales statiques. Bug reel trouve le 2026-08-03 en tracant la perte
    // des entrees molette (idx 3/4) : en changeant de souris dans le
    // selecteur (Jalon 8), les combos "Clic gauche/droit/milieu" et "Molette
    // avant/arriere" gardaient l'affichage de l'ANCIENNE souris, et un
    // "Enregistrer boutons souris" declenche dans cet etat ecrivait ces
    // valeurs heritees sur le mauvais peripherique.
    g_mouseBtnLeftSel = 0;
    g_mouseBtnRightSel = 0;
    g_mouseBtnMiddleSel = 0;
    g_mouseWheelForwardSel = 0;
    g_mouseWheelBackwardSel = 0;

    LoadMappingsFromFile("HDD:\\X360RemapStudio\\X360Remap.json");
    HidDeviceMapping* m = FindMapping(g_selectedMouseVid, g_selectedMousePid);
    if (m) {
        if (m->mouseSensitivity > 0) g_mouseSensitivity = m->mouseSensitivity;
        g_mouseInvertY = m->invertMouseY;
        g_mouseDeadzone = m->deadzone;

        for (uint8_t i = 0; i < m->buttonMapCount; i++) {
            const HidButtonMapEntry& e = m->buttonMap[i];
            if (e.idx == 0) g_mouseBtnLeftSel = FindMouseButtonTargetIndex(e.field);
            else if (e.idx == 1) g_mouseBtnRightSel = FindMouseButtonTargetIndex(e.field);
            else if (e.idx == 2) g_mouseBtnMiddleSel = FindMouseButtonTargetIndex(e.field);
            else if (e.idx == 3) g_mouseWheelForwardSel = FindMouseButtonTargetIndex(e.field);
            else if (e.idx == 4) g_mouseWheelBackwardSel = FindMouseButtonTargetIndex(e.field);
        }
    }
}

// --- Profils par jeu (Jalon 7) ---------------------------------------------
// Un profil surcharge sensibilite/deadzone/invertY/courbe pour UN jeu precis
// (identifie par son titleId Xbox), par-dessus les reglages globaux de
// l'onglet "Reglages souris" - voir ResolveEffectiveMouseSettings dans
// hiddriver/mapping.h, cable cote plugin dans hiddriver/main.cpp
// (XInputdReadStateHook). L'appli tourne HORS d'un jeu (lancee depuis
// Aurora, pas depuis l'interieur d'un titre) : contrairement au plugin, elle
// ne peut donc pas lire XamGetCurrentTitleId() pour deviner "le jeu qu'on
// est en train de configurer". Corrige le 2026-08-02 (demande utilisateur -
// "ce n'est pas a l'utilisateur de chercher [le Title ID]") : le plugin
// detecte deja chaque changement de jeu en temps reel et persiste
// desormais chaque nouveau titleId vu dans un petit historique
// (hiddriver/known_titles.h, HDD:\X360RemapStudio\known_titles.txt) - lu ici
// via ListKnownTitleIds() pour proposer un combo "jeux detectes" qui remplit
// le champ automatiquement. La saisie manuelle en hexadecimal reste
// disponible en repli (un jeu jamais lance avec le plugin actif n'apparait
// pas encore dans l'historique). Ecran de selection dynamique du Jalon 8
// (VID/PID) est un besoin different (choisir un PERIPHERIQUE, pas un jeu) -
// pas fusionne ici.
static bool g_profilesLoaded = false;
static std::vector<MouseProfileInfo> g_profiles;

// Historique des jeux vus (voir le commentaire ci-dessus) - charge une fois,
// rafraichi a l'ouverture de l'onglet comme g_profiles (RefreshProfiles).
static bool g_knownTitlesLoaded = false;
static std::vector<uint32_t> g_knownTitleIds;
static int g_knownTitlePickIdx = 0;
// Espace d'index NATIF du combo (0 = "Nouveau profil", 1..N = g_profiles[i-1])
// - reste dans cet espace d'un bout a l'autre, y compris entre les frames.
// Le convertir en -1/i "ailleurs" et le reconvertir a chaque frame (premiere
// version de ce fichier) cassait le contrat d'ImGui::Combo, qui s'attend a
// garder le meme entier tel quel entre deux frames pour rester coherent avec
// son getter - corrige avant meme un premier build, voir la revue de ce
// fichier.
static int g_profileComboIdx = 0;
static char g_profileTitleIdHex[9] = "";
// Nom lisible du jeu (2026-08-02, retour utilisateur reel - voir
// HidKeyCodeToAsciiChar plus bas et MouseProfileInfo::gameName). Saisi de la
// meme facon que g_profileTitleIdHex (une touche HID par frame, pas de
// machinerie clavier ImGui - voir le commentaire au-dessus du champ Title ID
// dans DrawProfilesTab), mais avec l'alphabet complet plutot que l'hexa.
static char g_profileGameName[32] = "";
static int g_profileSensitivity = 10000;
static int g_profileDeadzone = 0;
static bool g_profileInvertY = false;
static int g_profileCurveTypeIdx = 0; // 0 = Lineaire, 1 = Exponentielle
static float g_profileCurveExponent = 1.5f;

static void RefreshProfiles() {
    g_profiles = ListMouseProfiles(g_selectedMouseVid, g_selectedMousePid);
}

static void LoadProfilesOnce() {
    if (g_profilesLoaded)
        return;
    g_profilesLoaded = true;
    RefreshProfiles();
}

static void RefreshKnownTitles() {
    g_knownTitleIds = ListKnownTitleIds();
}

static void LoadKnownTitlesOnce() {
    if (g_knownTitlesLoaded)
        return;
    g_knownTitlesLoaded = true;
    RefreshKnownTitles();
}

// Retrouve le gameName deja enregistre pour ce titleId parmi les profils
// existants (g_profiles), ou "" si aucun profil n'a encore ete cree pour ce
// jeu - cas le plus courant ici, cette liste sert justement A CREER le
// premier profil. Simple recherche lineaire (au plus quelques dizaines de
// profils, pas un souci de perf).
static const char* FindKnownGameName(uint32_t titleId) {
    for (size_t i = 0; i < g_profiles.size(); i++) {
        if (g_profiles[i].titleId == titleId)
            return g_profiles[i].gameName;
    }
    return "";
}

static bool KnownTitleComboGetter(void*, int idx, const char** out_text) {
    static char s_label[48]; // un seul combo actif a la fois, meme precaution que ProfileComboGetter
    if (idx < 0 || idx >= (int)g_knownTitleIds.size())
        return false;
    uint32_t titleId = g_knownTitleIds[idx];
    const char* known = FindKnownGameName(titleId);
    if (known[0] != '\0') {
        _snprintf(s_label, sizeof(s_label) - 1, "%s (%08X)", known, (unsigned int)titleId);
    } else {
        _snprintf(s_label, sizeof(s_label) - 1, "%08X", (unsigned int)titleId);
    }
    s_label[sizeof(s_label) - 1] = '\0';
    *out_text = s_label;
    return true;
}

// Remplit les champs d'edition a partir d'un profil existant, en retombant
// sur les reglages globaux (deja charges par LoadMouseSettingsOnce, voir
// g_mouseSensitivity/g_mouseDeadzone/g_mouseInvertY plus haut) pour tout
// champ a sa sentinelle "herite" - c'est ce que ce profil applique REELLEMENT
// tant qu'on ne le modifie pas, pas une valeur arbitraire.
static void LoadProfileIntoEditor(const MouseProfileInfo& p) {
    _snprintf(g_profileTitleIdHex, sizeof(g_profileTitleIdHex) - 1, "%08X", (unsigned int)p.titleId);
    g_profileTitleIdHex[sizeof(g_profileTitleIdHex) - 1] = '\0';

    _snprintf(g_profileGameName, sizeof(g_profileGameName) - 1, "%s", p.gameName);
    g_profileGameName[sizeof(g_profileGameName) - 1] = '\0';

    g_profileSensitivity = (p.mouseSensitivity != 0) ? p.mouseSensitivity : g_mouseSensitivity;
    g_profileInvertY = (p.invertMouseY != -1) ? (p.invertMouseY != 0) : g_mouseInvertY;
    g_profileDeadzone = (p.deadzone != -1) ? p.deadzone : g_mouseDeadzone;
    if (p.curveType == CURVE_EXPONENTIAL) {
        g_profileCurveTypeIdx = 1;
        g_profileCurveExponent = p.curveExponent;
    } else {
        g_profileCurveTypeIdx = 0;
    }
}

static void ResetProfileEditorForNew() {
    g_profileTitleIdHex[0] = '\0';
    g_profileGameName[0] = '\0';
    g_profileSensitivity = g_mouseSensitivity;
    g_profileInvertY = g_mouseInvertY;
    g_profileDeadzone = g_mouseDeadzone;
    g_profileCurveTypeIdx = 0;
    g_profileCurveExponent = 1.5f;
}

// Page HID Keyboard/Keypad (0x07) - memes codes que hiddriver/mapping.h et
// le tableau du README. '\0' si le code ne correspond a aucun chiffre
// hexadecimal (touche ignoree pour ce champ).
static char HidKeyCodeToHexChar(int32_t code) {
    if (code >= 0x1E && code <= 0x26) return (char)('1' + (code - 0x1E)); // Keyboard 1..9
    if (code == 0x27) return '0';                                        // Keyboard 0
    if (code >= 0x04 && code <= 0x09) return (char)('A' + (code - 0x04)); // Keyboard a..f
    return '\0';
}

// Meme page HID que HidKeyCodeToHexChar ci-dessus, mais alphabet complet
// (lettres a..z + chiffres + espace) pour le champ "Nom du jeu" (2026-08-02,
// retour utilisateur reel : "comment comprendre que c'est de ce jeu la si
// c'est pas ecrit Tomb Raider"). Minuscules uniquement : le snapshot
// input_state.json ne transporte qu'UN SEUL code de touche par tick, pas
// l'etat des touches modificatrices (Shift) - voir InputStateSnapshot dans
// input_state_reader.h, aucun champ pour ca. Suffisant pour reconnaitre un
// nom de jeu au premier coup d'oeil, largement mieux que le Title ID en hex
// seul, sans avoir a cabler tout io.KeyMap/AddInputCharacter d'ImGui pour un
// seul champ (meme choix que HidKeyCodeToHexChar juste au-dessus).
static char HidKeyCodeToAsciiChar(int32_t code) {
    if (code >= 0x04 && code <= 0x1D) return (char)('a' + (code - 0x04)); // Keyboard a..z
    if (code >= 0x1E && code <= 0x26) return (char)('1' + (code - 0x1E)); // Keyboard 1..9
    if (code == 0x27) return '0';                                        // Keyboard 0
    if (code == 0x2C) return ' ';                                        // Barre d'espace
    return '\0';
}

static bool ProfileComboGetter(void*, int idx, const char** out_text) {
    // 32 (gameName) + " (" + 8 (hex) + ")" + marge, ou juste le hex seul si
    // gameName est vide (2026-08-02 - avant ce champ, seul le Title ID en hex
    // etait affiche ici, illisible pour un humain, voir le commentaire de
    // g_profileGameName plus haut).
    static char s_label[48]; // un seul combo actif a la fois, pas de risque de reecriture concurrente
    if (idx == 0) {
        *out_text = Tr(STR_PROFILES_COMBO_NEW);
        return true;
    }
    int profileIdx = idx - 1;
    if (profileIdx < 0 || profileIdx >= (int)g_profiles.size())
        return false;
    const MouseProfileInfo& p = g_profiles[profileIdx];
    if (p.gameName[0] != '\0') {
        _snprintf(s_label, sizeof(s_label) - 1, "%s (%08X)", p.gameName, (unsigned int)p.titleId);
    } else {
        _snprintf(s_label, sizeof(s_label) - 1, "%08X", (unsigned int)p.titleId);
    }
    s_label[sizeof(s_label) - 1] = '\0';
    *out_text = s_label;
    return true;
}

// --- Ecran wizard clavier --------------------------------------------------
// Logique deja testee (application/tests/test_wizard_session.cpp) - cette
// fonction ne fait que lire l'etat courant et dessiner, aucune decision
// n'est prise ici.
static void DrawWizardTab() {
    if (!g_wizard) {
        if (ImGui::Button(Tr(STR_WIZARD_BTN_START))) {
            // 180 = 60fps * 3s, PAS le defaut de 60 (calibre pour l'ancien
            // assistant manette a 50ms/tick, voir wizard_session.h) - avec
            // 60 ici, une etape se faisait sauter au bout d'~1s au lieu des
            // "3 secondes" annoncees juste en dessous, ce qui pouvait vider
            // silencieusement le wizard entier si chaque touche etait
            // relachee un peu tard (bug reel du 2026-08-02).
            std::vector<AssistantStep> steps = DefaultKeyboardWizardSteps();
            AppLog("Wizard clavier demarre (%d etapes)", (int)steps.size());
            g_wizard = new WizardSession(g_selectedKeyboardVid, g_selectedKeyboardPid, steps, 180);
        }
        return;
    }

    const InputStateSnapshot& snap = g_lastSnapshot;
    int32_t code = snap.valid ? snap.keyCode : MappingAssistant::NO_CODE;

    std::string prompt, message;
    g_wizard->Tick(code, &prompt, &message);

    if (!snap.valid) {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
            Tr(STR_WIZARD_NO_KEYBOARD));
    }

    // Progression en direct - permet de voir tout de suite si une etape
    // vient d'etre ignoree (skip) au lieu de le decouvrir seulement une fois
    // "termine" affiche, avec un resultat vide ou incomplet.
    ImGui::Text(Tr(STR_WIZARD_STEP_PROGRESS_FMT),
        (int)g_wizard->CurrentStepIndex() + 1, (int)g_wizard->TotalSteps(), (int)g_wizard->ResultsCount());

    ImGui::Text(Tr(STR_WIZARD_PRESS_KEY_FOR_FMT), prompt.c_str());
    if (!message.empty())
        ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.3f, 1.0f), "%s", message.c_str());
    ImGui::Separator();
    ImGui::TextDisabled(Tr(STR_WIZARD_HOLD_TO_SKIP));

    if (g_wizard->IsComplete()) {
        // static plutot qu'un membre de WizardSession : evite d'appeler
        // SaveResult() a CHAQUE FRAME tant que l'utilisateur n'a pas clique
        // "OK" (bug reel corrige le 2026-08-02 - en plus d'etre inutile,
        // ca aurait aussi pollue X360Remap_studio.log d'une ligne par frame). Reset
        // au clic "OK" plus bas, pret pour le prochain wizard.
        static bool s_saveAttempted = false;
        static bool s_saveOk = false;
        if (!s_saveAttempted) {
            s_saveAttempted = true;
            s_saveOk = g_wizard->SaveResult();
            size_t count = g_wizard->ResultsCount();
            AppLog(s_saveOk ? "Wizard clavier: %d touches enregistrees" : "Wizard clavier: ECHEC enregistrement (%d touches resolues)",
                (int)count);
            XexUtils::Xam::XNotify(s_saveOk ? "Configuration clavier enregistree" : "ECHEC sauvegarde configuration clavier");
        }

        if (s_saveOk) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
                Tr(STR_WIZARD_SAVED_FMT), (int)g_wizard->ResultsCount());
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                Tr(STR_WIZARD_SAVE_FAILED));
        }
        if (ImGui::Button(Tr(STR_WIZARD_BTN_OK))) {
            delete g_wizard;
            g_wizard = nullptr;
            s_saveAttempted = false;
            s_saveOk = false;
        }
    } else if (g_wizard->WasCancelled()) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), Tr(STR_WIZARD_CANCELLED));
        if (ImGui::Button(Tr(STR_WIZARD_BTN_OK))) {
            delete g_wizard;
            g_wizard = nullptr;
        }
    }
}

// --- Ecran assistant de calibration souris (2026-08-03) --------------------
// Idee de l'utilisateur : plutot que de deviner la disposition du paquet
// d'une souris (impossible depuis le VID/PID) ou de lire son descripteur HID
// (possible mais via la chaine USB asynchrone responsable de tous les gels
// historiques), on la fait DECOUVRIR par l'utilisateur - exactement le
// principe du wizard clavier.
//
// Toute la logique vit dans CalibrationSession (testee en g++). Cet ecran ne
// fait que l'alimenter en octets bruts et dessiner son etat.
//
// PILOTE A LA MANETTE, et c'est structurel : pendant la calibration la souris
// n'est justement PAS encore calibree - sur le modele qu'on cherche a
// reparer, son curseur est inutilisable. Naviguer l'assistant a la souris
// serait donc impossible precisement dans le cas ou il sert. La souris ne
// fait que produire les gestes demandes.
static bool g_calibSaveAttempted = false;
static bool g_calibSaveOk = false;

static void DrawCalibrationTab() {
    ImGui::TextWrapped(Tr(STR_CALIB_INTRO));
    ImGui::TextDisabled(Tr(STR_CALIB_NAV_HINT));
    ImGui::Separator();

    if (!g_calib) {
        ImGui::Text(Tr(STR_CALIB_TARGET_MOUSE_FMT), g_selectedMouseVid, g_selectedMousePid);
        ImGui::TextDisabled(Tr(STR_CALIB_TARGET_MOUSE_HINT));
        ImGui::Spacing();
        if (ImGui::Button(Tr(STR_CALIB_BTN_START))) {
            g_calib = new CalibrationSession(g_selectedMouseVid, g_selectedMousePid);
            g_calibSaveAttempted = false;
            g_calibSaveOk = false;
            AppLog("Calibration souris demarree (%04X:%04X)", g_selectedMouseVid, g_selectedMousePid);
        }
        ImGui::Spacing();
        ImGui::Separator();
        // Le cercle vicieux explique dans ShouldForceBootProtocol
        // (hiddriver/main.cpp) : une souris a paquet long est forcee en mode
        // boot tant qu'elle n'est pas calibree, or ce mode SUPPRIME la
        // molette. L'assistant ne verrait donc jamais de molette a detecter.
        // Il faut observer la souris dans son format natif au moins le temps
        // d'une calibration - d'ou ce bouton, qui evite a l'utilisateur de
        // creer le fichier a la main via XeCLI.
        // Depuis le 2026-08-03, le plugin lit le DESCRIPTEUR de chaque souris
        // et n'a plus besoin de calibration dans la quasi-totalite des cas.
        // Ce mode legacy ne sert plus que de filet : un modele qui se
        // comporterait mal avec son propre descripteur peut etre ramene a
        // l'ancienne lecture figee.
        // ETAT AFFICHE EN PERMANENCE (2026-08-04). Sans ca, ces deux boutons
        // sont un piege : l'utilisateur a active le mode de secours, l'a
        // oublie, et a passe une session entiere a chercher pourquoi sa
        // molette ne marchait plus - alors que l'interface connaissait la
        // reponse et ne la montrait pas. Un reglage qui modifie le
        // comportement DOIT afficher son etat courant.
        {
            std::ifstream fbp("HDD:\\X360RemapStudio\\force_boot_protocol.txt", std::ios::binary);
            bool secoursActif = false;
            if (fbp.is_open()) {
                char c = 0;
                fbp.get(c);
                secoursActif = (c == '1');
            }
            if (secoursActif) {
                ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.2f, 1.0f),
                    Tr(STR_CALIB_FALLBACK_ACTIVE));
                ImGui::TextWrapped(Tr(STR_CALIB_FALLBACK_ACTIVE_HINT));
            } else {
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f),
                    Tr(STR_CALIB_AUTO_ACTIVE));
            }
        }

        ImGui::TextWrapped(Tr(STR_CALIB_FALLBACK_EXPLAIN));
        if (ImGui::Button(Tr(STR_CALIB_BTN_FORCE_FALLBACK))) {
            std::ofstream f("HDD:\\X360RemapStudio\\force_boot_protocol.txt",
                std::ios::binary | std::ios::trunc);
            bool ok = f.is_open();
            if (ok) f << "1";
            AppLog(ok ? "Mode de secours active - redemarrage requis"
                      : "ECHEC ecriture force_boot_protocol.txt");
            XexUtils::Xam::XNotify(ok ? "Mode de secours - redemarre la console"
                                      : "ECHEC - impossible d'ecrire le fichier");
        }
        if (ImGui::Button(Tr(STR_CALIB_BTN_RESTORE_AUTO))) {
            bool ok = (std::remove("HDD:\\X360RemapStudio\\force_boot_protocol.txt") == 0);
            AppLog(ok ? "Lecture automatique retablie (fichier supprime)"
                      : "Deja en lecture automatique");
            XexUtils::Xam::XNotify(ok ? "Automatique - redemarre la console"
                                      : "Deja en mode automatique");
        }

        ImGui::Spacing();
        ImGui::TextDisabled(Tr(STR_CALIB_CLEAR_HINT));
        if (ImGui::Button(Tr(STR_CALIB_BTN_CLEAR))) {
            bool ok = ClearMouseReportLayout(g_selectedMouseVid, g_selectedMousePid);
            AppLog(ok ? "Calibration effacee (%04X:%04X)" : "Aucune calibration a effacer (%04X:%04X)",
                g_selectedMouseVid, g_selectedMousePid);
            XexUtils::Xam::XNotify(ok ? "Calibration effacee" : "Aucune calibration enregistree");
        }
        return;
    }

    // Alimentation : les octets bruts viennent du meme snapshot deja lu une
    // fois par frame dans main(), pas d'une lecture disque supplementaire.
    const InputStateSnapshot& snap = g_lastSnapshot;
    if (snap.valid && snap.rawLen > 0)
        g_calib->Feed(snap.raw, snap.rawLen, snap.tick);

    if (!snap.valid || snap.rawLen == 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
            Tr(STR_CALIB_NO_DATA));
    }

    if (g_calib->Finished()) {
        if (g_calib->Succeeded()) {
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), Tr(STR_CALIB_SUCCESS));
            ImGui::TextWrapped("%s", g_calib->Summary().c_str());
            ImGui::Spacing();

            if (!g_calibSaveAttempted) {
                if (ImGui::Button(Tr(STR_CALIB_BTN_SAVE))) {
                    g_calibSaveAttempted = true;
                    g_calibSaveOk = ApplyMouseReportLayout(
                        g_calib->Vid(), g_calib->Pid(), g_calib->Layout());
                    AppLog(g_calibSaveOk ? "Calibration enregistree (%04X:%04X)"
                                         : "ECHEC enregistrement calibration (%04X:%04X)",
                        g_calib->Vid(), g_calib->Pid());
                    XexUtils::Xam::XNotify(g_calibSaveOk ? "Calibration enregistree"
                                                         : "ECHEC sauvegarde calibration");
                }
            } else {
                ImGui::TextColored(
                    g_calibSaveOk ? ImVec4(0.3f, 1.0f, 0.3f, 1.0f) : ImVec4(1.0f, 0.3f, 0.3f, 1.0f),
                    g_calibSaveOk ? Tr(STR_CALIB_SAVED_APPLIED)
                                  : Tr(STR_CALIB_SAVE_FAILED));
            }
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", g_calib->Prompt());
            ImGui::TextWrapped("%s", g_calib->Summary().c_str());
        }

        ImGui::Spacing();
        if (ImGui::Button(Tr(STR_CALIB_BTN_CLOSE))) {
            delete g_calib;
            g_calib = nullptr;
            g_calibSaveAttempted = false;
            g_calibSaveOk = false;
        }
        return;
    }

    // Etape en cours : consigne + jauge de progression.
    ImGui::Spacing();
    ImGui::TextWrapped("%s", g_calib->Prompt());
    ImGui::Spacing();

    // La jauge est le coeur de l'ergonomie ici : elle ne se remplit que si le
    // paquet change REELLEMENT par rapport au repos. Un utilisateur qui ne
    // fait pas le geste voit une jauge vide et comprend tout de suite, au lieu
    // d'enchainer les etapes et d'enregistrer une disposition fausse.
    float p = g_calib->Progress();
    ImGui::ProgressBar(p, ImVec2(-1.0f, 0.0f));
    ImGui::TextDisabled(p >= 1.0f ? Tr(STR_CALIB_GESTURE_DONE)
                                  : Tr(STR_CALIB_GESTURE_CONTINUE));

    ImGui::Spacing();
    if (p >= 1.0f) {
        if (ImGui::Button(Tr(STR_CALIB_BTN_VALIDATE_STEP)))
            g_calib->Advance();
    } else {
        ImGui::TextDisabled(Tr(STR_CALIB_VALIDATE_DISABLED_HINT));
    }

    if (g_calib->CanSkip()) {
        ImGui::SameLine();
        if (ImGui::Button(Tr(STR_CALIB_BTN_NO_WHEEL)))
            g_calib->SkipWheel();
    }

    ImGui::Spacing();
    if (ImGui::Button(Tr(STR_CALIB_BTN_CANCEL))) {
        g_calib->Cancel();
        AppLog("Calibration souris annulee");
    }
}

// --- Ecran reglages souris -------------------------------------------------
// Exactement le cas d'usage cite par l'utilisateur pour ImGui : glissieres
// pour la sensibilite plutot que de coder des boutons +/- a la main.
static void DrawMouseSettingsTab() {
    LoadMouseSettingsOnce();

    // Selecteur de langue (2026-08-07) - voir application/i18n.h/.cpp. Les
    // noms "Francais"/"English" restent volontairement EN DUR (pas de Tr())
    // : un selecteur de langue affiche conventionnellement chaque langue
    // dans SA PROPRE langue, quelle que soit la langue actuellement active -
    // sinon "English" deviendrait illisible pour un francophone qui veut
    // justement basculer vers l'anglais (et inversement pour un anglophone).
    {
        Lang currentLang = GetLanguage();
        if (ImGui::RadioButton("Francais", currentLang == LANG_FR))
            SaveLanguagePref(LANG_FR);
        ImGui::SameLine();
        if (ImGui::RadioButton("English", currentLang == LANG_EN))
            SaveLanguagePref(LANG_EN);
        // TODO PT-BR: ajouter un 3eme RadioButton ici une fois les glyphes accentues geres
    }
    ImGui::Separator();

    ImGui::SliderInt(Tr(STR_SETTINGS_SENSITIVITY_LABEL), &g_mouseSensitivity, 1000, 30000);
    ImGui::SliderInt(Tr(STR_SETTINGS_DEADZONE_LABEL), &g_mouseDeadzone, 0, 10000);
    ImGui::Checkbox(Tr(STR_SETTINGS_INVERT_Y_LABEL), &g_mouseInvertY);

    ImGui::Separator();
    if (ImGui::Button(Tr(STR_SETTINGS_BTN_SAVE))) {
        bool ok = ApplyMouseSettings(g_selectedMouseVid, g_selectedMousePid, g_mouseSensitivity, g_mouseInvertY, g_mouseDeadzone);
        AppLog(ok ? "Reglages souris enregistres (sensibilite=%d deadzone=%d invertY=%d)" : "ECHEC enregistrement reglages souris",
            g_mouseSensitivity, g_mouseDeadzone, g_mouseInvertY ? 1 : 0);

        // DIAG (2026-08-03) - ce bouton ne devrait JAMAIS toucher buttonMap,
        // seulement sensibilite/deadzone/invertY. On verifie ici que le
        // buttonMapCount du device n'a pas change pour confirmer/infirmer
        // qu'un save "settings" ecrase par erreur les boutons/molette d'un
        // save precedent - voir PROJECT_NOTES.md, section molette 2026-08-03.
        int onDiskCount = GetMouseButtonMapCountOnDisk(g_selectedMouseVid, g_selectedMousePid);
        AppLog("DIAG reglages: vid=%04x pid=%04x buttonMapCount_disque=%d",
            g_selectedMouseVid, g_selectedMousePid, onDiskCount);

        XexUtils::Xam::XNotify(ok ? "Reglages souris enregistres" : "ECHEC sauvegarde reglages souris");
    }

    // Mapping des clics souris - demande utilisateur du 2026-08-02
    // ("pourquoi y'a pas un mapping pour les buttons souris"). Voir le
    // commentaire au-dessus de kMouseButtonTargets pour la portee/limites.
    ImGui::Separator();
    ImGui::TextUnformatted(Tr(STR_SETTINGS_MOUSE_BUTTONS_HEADER));
    ImGui::Combo(Tr(STR_SETTINGS_CLICK_LEFT_LABEL), &g_mouseBtnLeftSel, MouseButtonTargetGetter, nullptr, kMouseButtonTargetCount);
    ImGui::Combo(Tr(STR_SETTINGS_CLICK_RIGHT_LABEL), &g_mouseBtnRightSel, MouseButtonTargetGetter, nullptr, kMouseButtonTargetCount);
    ImGui::Combo(Tr(STR_SETTINGS_CLICK_MIDDLE_LABEL), &g_mouseBtnMiddleSel, MouseButtonTargetGetter, nullptr, kMouseButtonTargetCount);
    ImGui::TextDisabled(Tr(STR_SETTINGS_CLICK_DEFAULT_HINT));

    ImGui::Spacing();
    ImGui::Combo(Tr(STR_SETTINGS_WHEEL_FORWARD_LABEL), &g_mouseWheelForwardSel, MouseButtonTargetGetter, nullptr, kMouseButtonTargetCount);
    ImGui::Combo(Tr(STR_SETTINGS_WHEEL_BACKWARD_LABEL), &g_mouseWheelBackwardSel, MouseButtonTargetGetter, nullptr, kMouseButtonTargetCount);
    ImGui::TextDisabled(Tr(STR_SETTINGS_WHEEL_DEFAULT_HINT));

    if (ImGui::Button(Tr(STR_SETTINGS_BTN_SAVE_BUTTONS))) {
        std::vector<MouseButtonBinding> bindings;
        if (kMouseButtonTargets[g_mouseBtnLeftSel].fieldName) {
            MouseButtonBinding b;
            b.idx = 0;
            b.fieldName = kMouseButtonTargets[g_mouseBtnLeftSel].fieldName;
            bindings.push_back(b);
        }
        if (kMouseButtonTargets[g_mouseBtnRightSel].fieldName) {
            MouseButtonBinding b;
            b.idx = 1;
            b.fieldName = kMouseButtonTargets[g_mouseBtnRightSel].fieldName;
            bindings.push_back(b);
        }
        if (kMouseButtonTargets[g_mouseBtnMiddleSel].fieldName) {
            MouseButtonBinding b;
            b.idx = 2;
            b.fieldName = kMouseButtonTargets[g_mouseBtnMiddleSel].fieldName;
            bindings.push_back(b);
        }
        if (kMouseButtonTargets[g_mouseWheelForwardSel].fieldName) {
            MouseButtonBinding b;
            b.idx = 3; // convention "molette avant" - voir FindWheelOverride dans hiddriver/main.cpp
            b.fieldName = kMouseButtonTargets[g_mouseWheelForwardSel].fieldName;
            bindings.push_back(b);
        }
        if (kMouseButtonTargets[g_mouseWheelBackwardSel].fieldName) {
            MouseButtonBinding b;
            b.idx = 4; // convention "molette arriere"
            b.fieldName = kMouseButtonTargets[g_mouseWheelBackwardSel].fieldName;
            bindings.push_back(b);
        }

        bool ok = ApplyMouseButtonMapping(g_selectedMouseVid, g_selectedMousePid, bindings);
        AppLog(ok ? "Boutons souris enregistres (%d entrees personnalisees)" : "ECHEC enregistrement boutons souris",
            (int)bindings.size());

        // DIAG (2026-08-03) - relit immediatement le fichier pour verifier
        // que ce qui vient d'etre demande est bien ce qui a atterri sur le
        // disque. bindings.size() ci-dessus est ce que l'appli a CALCULE
        // AVANT l'appel (peut differer si GetButtonFieldPtr rejette un nom,
        // ou si une action ulterieure ecrase le fichier) - onDiskCount est
        // la verite disque, relue juste apres. Voir PROJECT_NOTES.md.
        int onDiskCount = GetMouseButtonMapCountOnDisk(g_selectedMouseVid, g_selectedMousePid);
        AppLog("DIAG boutons: vid=%04x pid=%04x demande=%d disque=%d",
            g_selectedMouseVid, g_selectedMousePid, (int)bindings.size(), onDiskCount);

        XexUtils::Xam::XNotify(ok ? "Boutons souris enregistres" : "ECHEC sauvegarde boutons souris");
    }

    // Reglages par defaut ("comme un BIOS") - demande utilisateur du
    // 2026-08-02 apres avoir cru perdre son mapping clavier en testant le
    // wizard. Porte sur TOUT X360Remap.json (clavier + souris), pas juste
    // cet onglet - placee ici car c'est le seul endroit avec un bouton
    // "Enregistrer" existant, mais le libelle le precise explicitement.
    ImGui::Separator();
    ImGui::TextUnformatted(Tr(STR_SETTINGS_DEFAULTS_HEADER));
    if (ImGui::Button(Tr(STR_SETTINGS_BTN_SAVE_AS_DEFAULT))) {
        bool ok = SaveConfigAsDefault();
        AppLog(ok ? "Reglages actuels sauvegardes comme defaut" : "ECHEC sauvegarde des reglages par defaut");
        XexUtils::Xam::XNotify(ok ? "Reglages actuels sauvegardes comme defaut" : "Echec sauvegarde des reglages par defaut");
    }
    if (!HasConfigDefault()) {
        ImGui::TextUnformatted(Tr(STR_SETTINGS_NO_DEFAULT_SAVED));
    } else if (ImGui::Button(Tr(STR_SETTINGS_BTN_RESTORE_DEFAULT))) {
        bool ok = RestoreConfigDefault();
        if (ok) {
            g_mouseSettingsLoaded = false; // force le rechargement des sliders/combos
        }
        AppLog(ok ? "Reglages par defaut restaures" : "ECHEC restauration des reglages par defaut");
        XexUtils::Xam::XNotify(ok ? "Reglages par defaut restaures" : "Echec restauration des reglages par defaut");
    }
}

// --- Ecran profils par jeu (Jalon 7) ---------------------------------------
// Voir le commentaire au-dessus de g_profilesLoaded pour la limite connue
// (titleId saisi a la main, pas de detection automatique du jeu en cours
// depuis cet ecran).
static void DrawProfilesTab() {
    LoadMouseSettingsOnce(); // pour les valeurs "heritees" affichees ci-dessous
    LoadProfilesOnce();

    ImGui::TextWrapped(Tr(STR_PROFILES_INTRO));
    ImGui::TextDisabled(Tr(STR_PROFILES_F9_HINT));
    ImGui::Separator();

    int comboCount = (int)g_profiles.size() + 1; // +1 pour "Nouveau profil" (index 0)
    // Garde-fou si g_profiles a retreci depuis la derniere frame (ex: profil
    // supprime ailleurs) - eviter de laisser le combo pointer hors plage,
    // ce qu'ImGui ne clamp pas tout seul.
    if (g_profileComboIdx >= comboCount)
        g_profileComboIdx = 0;

    if (ImGui::Combo(Tr(STR_PROFILES_COMBO_LABEL), &g_profileComboIdx, ProfileComboGetter, nullptr, comboCount)) {
        if (g_profileComboIdx == 0) {
            ResetProfileEditorForNew();
        } else {
            LoadProfileIntoEditor(g_profiles[g_profileComboIdx - 1]);
        }
    }

    bool isNewProfile = (g_profileComboIdx == 0);

    if (isNewProfile) {
        // BUG REEL signale par l'utilisateur (2026-08-02) : ImGui::InputText
        // ne recevait AUCUN caractere - cette appli n'a jamais cable le
        // clavier physique vers la machinerie texte d'ImGui
        // (io.AddInputCharacter/io.KeyMap, jamais initialises, voir
        // UpdateGamepadNavigation plus bas qui ne gere QUE la manette). Le
        // seul canal clavier existant est le snapshot MONO-TOUCHE de
        // input_state.json (deja utilise par le wizard, deja confirme
        // fonctionnel sur hardware) - reutilise directement ici plutot que
        // de cabler toute la machinerie clavier d'ImGui pour un seul champ :
        // une touche HID par frame, detection de front montant (une pression
        // = un caractere, pas de repetition tant que la touche reste
        // enfoncee) - meme principe que MappingAssistant, en plus simple.

        // Combo "Games detected by the plugin" RETIRE (2026-08-08, decision
        // utilisateur : "ça sert a rien il faut le supprimer si il detect 4
        // sur 60 jeux par exemple"). Le principe (historique des jeux vus
        // par le plugin, hiddriver/known_titles.h) ne couvre que les jeux
        // deja lances avec le plugin actif depuis l'installation - une
        // bibliotheque de 60 jeux n'en montre que quelques-uns tant qu'ils
        // n'ont pas tous ete lances au moins une fois, ce qui le rend peu
        // utile en pratique comme point d'entree principal. La saisie
        // manuelle du Title ID (hex, ci-dessous) reste le seul chemin.
        // LoadKnownTitlesOnce/RefreshKnownTitles/g_knownTitleIds/
        // g_knownTitlePickIdx/KnownTitleComboGetter restent dans le fichier
        // (inoffensifs, plus appeles) - a retirer une prochaine fois si
        // confirme definitivement inutile plutot que de tout retoucher dans
        // la meme session qu'un correctif calibration.

        // Le snapshot input_state.json ne porte qu'UNE touche par tick (voir
        // le commentaire au-dessus) - impossible de remplir le Title ID et le
        // nom du jeu en meme temps sans savoir lequel doit recevoir la
        // frappe. Bascule explicite via deux boutons radio, cliquables a la
        // souris (le curseur souris est deja cable, voir UpdateMouseCursor) -
        // ajoute le 2026-08-02 en meme temps que le champ "Nom du jeu"
        // (retour utilisateur reel : Title ID en hex seul illisible pour un
        // humain).
        static int s_activeProfileField = 0; // 0 = Title ID, 1 = Nom du jeu
        if (ImGui::RadioButton(Tr(STR_PROFILES_RADIO_TYPE_TITLEID), s_activeProfileField == 0)) s_activeProfileField = 0;
        ImGui::SameLine();
        if (ImGui::RadioButton(Tr(STR_PROFILES_RADIO_TYPE_GAMENAME), s_activeProfileField == 1)) s_activeProfileField = 1;

        static int32_t s_lastProfileKeyCode = -1;
        int32_t code = g_lastSnapshot.valid ? g_lastSnapshot.keyCode : -1;
        if (code != s_lastProfileKeyCode && code != -1) {
            if (s_activeProfileField == 0) {
                size_t len = strlen(g_profileTitleIdHex);
                char hexChar = HidKeyCodeToHexChar(code);
                if (hexChar != '\0' && len < 8) {
                    g_profileTitleIdHex[len] = hexChar;
                    g_profileTitleIdHex[len + 1] = '\0';
                } else if (code == 42 /* Retour arriere, meme code HID que documente dans README.md */ && len > 0) {
                    g_profileTitleIdHex[len - 1] = '\0';
                }
            } else {
                size_t len = strlen(g_profileGameName);
                char asciiChar = HidKeyCodeToAsciiChar(code);
                if (asciiChar != '\0' && len < sizeof(g_profileGameName) - 1) {
                    g_profileGameName[len] = asciiChar;
                    g_profileGameName[len + 1] = '\0';
                } else if (code == 42 && len > 0) {
                    g_profileGameName[len - 1] = '\0';
                }
            }
        }
        s_lastProfileKeyCode = code;

        // ReadOnly : le contenu est entierement pilote par le bloc ci-dessus,
        // pas par la saisie native d'ImGui (jamais cablee, voir plus haut) -
        // afficher quand meme via InputText (pas juste ImGui::Text) pour
        // garder l'apparence d'un champ de saisie a l'ecran.
        ImGui::InputText(Tr(STR_PROFILES_INPUT_TITLEID), g_profileTitleIdHex, sizeof(g_profileTitleIdHex),
            ImGuiInputTextFlags_ReadOnly);
        ImGui::TextDisabled(Tr(STR_PROFILES_HINT_TITLEID_CHARS));
        ImGui::InputText(Tr(STR_PROFILES_INPUT_GAME_NAME), g_profileGameName, sizeof(g_profileGameName),
            ImGuiInputTextFlags_ReadOnly);
        ImGui::TextDisabled(Tr(STR_PROFILES_HINT_GAME_NAME_CHARS));
    } else {
        ImGui::Text(Tr(STR_PROFILES_TITLEID_DISPLAY_FMT), g_profileTitleIdHex);

        // Un profil existant a deja son Title ID (non modifiable, c'est la
        // cle) - seul le nom peut etre tape/edite ici, pas de bascule
        // necessaire contrairement au bloc "Nouveau profil" ci-dessus.
        static int32_t s_lastExistingGameNameKeyCode = -1;
        int32_t code = g_lastSnapshot.valid ? g_lastSnapshot.keyCode : -1;
        if (code != s_lastExistingGameNameKeyCode && code != -1) {
            size_t len = strlen(g_profileGameName);
            char asciiChar = HidKeyCodeToAsciiChar(code);
            if (asciiChar != '\0' && len < sizeof(g_profileGameName) - 1) {
                g_profileGameName[len] = asciiChar;
                g_profileGameName[len + 1] = '\0';
            } else if (code == 42 && len > 0) {
                g_profileGameName[len - 1] = '\0';
            }
        }
        s_lastExistingGameNameKeyCode = code;

        ImGui::InputText(Tr(STR_PROFILES_INPUT_GAME_NAME), g_profileGameName, sizeof(g_profileGameName),
            ImGuiInputTextFlags_ReadOnly);
        ImGui::TextDisabled(Tr(STR_PROFILES_HINT_GAME_NAME_CHARS));
    }

    ImGui::SliderInt(Tr(STR_PROFILES_SENSITIVITY_LABEL), &g_profileSensitivity, 1000, 30000);
    ImGui::SliderInt(Tr(STR_PROFILES_DEADZONE_LABEL), &g_profileDeadzone, 0, 10000);
    ImGui::Checkbox(Tr(STR_PROFILES_INVERT_Y_LABEL), &g_profileInvertY);

    const char* curveNames[] = { Tr(STR_PROFILES_CURVE_LINEAR), Tr(STR_PROFILES_CURVE_EXPONENTIAL) };
    ImGui::Combo(Tr(STR_PROFILES_CURVE_COMBO_LABEL), &g_profileCurveTypeIdx, curveNames, 2);
    if (g_profileCurveTypeIdx == 1) {
        ImGui::SliderFloat(Tr(STR_PROFILES_EXPONENT_LABEL), &g_profileCurveExponent, 1.0f, 3.0f);
        ImGui::TextDisabled(Tr(STR_PROFILES_EXPONENT_HINT));
    }

    ImGui::Separator();
    if (ImGui::Button(Tr(STR_PROFILES_BTN_SAVE))) {
        uint32_t titleId = (uint32_t)strtoul(g_profileTitleIdHex, nullptr, 16);
        uint8_t curveType = (g_profileCurveTypeIdx == 1) ? CURVE_EXPONENTIAL : CURVE_LINEAR;
        // g_profileGameName vide n'efface PAS un nom deja enregistre (meme
        // convention "vide = ne pas toucher" que le raccourci F9, voir
        // ApplyMouseProfile dans config_writer.h) - un champ laisse vide en
        // renommant un profil existant conserve donc son ancien nom plutot
        // que de le vider silencieusement.
        bool ok = ApplyMouseProfile(g_selectedMouseVid, g_selectedMousePid, titleId,
            g_profileSensitivity, g_profileInvertY, g_profileDeadzone,
            curveType, g_profileCurveExponent, g_profileGameName);
        AppLog(ok ? "Profil %08X enregistre (nom=%s sensibilite=%d deadzone=%d invertY=%d courbe=%d)"
                  : "ECHEC enregistrement profil %08X",
            titleId, g_profileGameName, g_profileSensitivity, g_profileDeadzone, g_profileInvertY ? 1 : 0, (int)curveType);
        XexUtils::Xam::XNotify(ok ? "Profil enregistre" : "ECHEC sauvegarde du profil - Title ID invalide ?");
        if (ok) {
            RefreshProfiles();
            // Retrouve l'index du profil qu'on vient d'enregistrer (nouveau ou
            // modifie) pour que le combo reste sur cette entree plutot que de
            // retomber silencieusement sur "Nouveau profil".
            g_profileComboIdx = 0;
            for (size_t i = 0; i < g_profiles.size(); i++) {
                if (g_profiles[i].titleId == titleId) {
                    g_profileComboIdx = (int)i + 1; // +1 : espace natif du combo, voir g_profileComboIdx
                    break;
                }
            }
        }
    }

    if (!isNewProfile) {
        ImGui::SameLine();
        if (ImGui::Button(Tr(STR_PROFILES_BTN_DELETE))) {
            uint32_t titleId = (uint32_t)strtoul(g_profileTitleIdHex, nullptr, 16);
            bool ok = RemoveMouseProfile(g_selectedMouseVid, g_selectedMousePid, titleId);
            AppLog(ok ? "Profil %08X supprime" : "ECHEC suppression profil %08X", titleId);
            XexUtils::Xam::XNotify(ok ? "Profil supprime" : "ECHEC suppression du profil");
            if (ok) {
                RefreshProfiles();
                g_profileComboIdx = 0;
                ResetProfileEditorForNew();
            }
        }
    }

    if (g_profiles.empty()) {
        ImGui::Separator();
        ImGui::TextUnformatted(Tr(STR_PROFILES_NONE_SAVED));
    }
}

// --- Navigation manette via XexUtils::Input --------------------------------
// XexUtils::Input::GetInput() ne retourne jamais nullptr (voir Input.cpp -
// pointe toujours vers un slot statique interne, meme manette deconnectee) et
// fournit deja wButtons/PressedButtons debounces et ThumbLeftX/Y normalises
// en [-1,+1] avec deadzone appliquee - plus besoin de refaire cette logique
// a la main comme dans la toute premiere version de ce fichier.
static void UpdateGamepadNavigation(ImGuiIO& io) {
    XexUtils::Input::Gamepad* pad = XexUtils::Input::GetInput(0);

    // wButtons reste a 0 si la manette est deconnectee (voir GetInput) - pas
    // besoin de detecter explicitement l'absence de manette pour ce cas.
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

    const WORD b = pad->wButtons;
    io.NavInputs[ImGuiNavInput_Activate]  = (b & XINPUT_GAMEPAD_A) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_Cancel]    = (b & XINPUT_GAMEPAD_B) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadUp]    = (b & XINPUT_GAMEPAD_DPAD_UP) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadDown]  = (b & XINPUT_GAMEPAD_DPAD_DOWN) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadLeft]  = (b & XINPUT_GAMEPAD_DPAD_LEFT) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_DpadRight] = (b & XINPUT_GAMEPAD_DPAD_RIGHT) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_FocusPrev] = (b & XINPUT_GAMEPAD_LEFT_SHOULDER) ? 1.0f : 0.0f;
    io.NavInputs[ImGuiNavInput_FocusNext] = (b & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? 1.0f : 0.0f;

    // Deja normalise et deadzone par GetInput() - simple clamp au signe pour
    // ImGui (qui attend une magnitude positive par direction).
    io.NavInputs[ImGuiNavInput_LStickLeft]  = (pad->ThumbLeftX < 0.0f) ? -pad->ThumbLeftX : 0.0f;
    io.NavInputs[ImGuiNavInput_LStickRight] = (pad->ThumbLeftX > 0.0f) ?  pad->ThumbLeftX : 0.0f;
    io.NavInputs[ImGuiNavInput_LStickUp]    = (pad->ThumbLeftY > 0.0f) ?  pad->ThumbLeftY : 0.0f;
    io.NavInputs[ImGuiNavInput_LStickDown]  = (pad->ThumbLeftY < 0.0f) ? -pad->ThumbLeftY : 0.0f;

    // PressedButtons (edge, pas held) pour eviter d'annuler le wizard a
    // chaque frame ou BACK reste maintenu.
    if (pad->PressedButtons & XINPUT_GAMEPAD_BACK) {
        if (g_wizard) g_wizard->Cancel();
    }
}

// --- Curseur souris (2026-08-01) --------------------------------------------
// Ajoute suite au retour utilisateur du 2026-08-01 : la navigation manette
// seule (UpdateGamepadNavigation ci-dessus) etait jugee peu pratique pour
// choisir entre les deux onglets - et vu que cette appli configure
// justement une souris, autant qu'elle se pilote avec. Xbox 360 n'a pas de
// curseur systeme comme un bureau PC - ImGui dessine son propre curseur
// logiciel (io.MouseDrawCursor, deja active dans main() plus bas) a partir
// d'io.MousePos, qu'on met a jour ici a la main.
//
// mouseDX/mouseDY (voir input_state_reader.h) sont un DELTA accumule cote
// plugin.xex depuis la derniere lecture, pas une position absolue - on les
// ajoute donc a une position cumulee ici, note en pixels ecran (0,0 = coin
// haut-gauche, meme convention qu'io.MousePos). g_mouseSensitivityCursor est
// un facteur d'echelle simple, distinct du reglage "Sensibilite" du stick
// droit (onglet Reglages souris, qui modifie X360Remap.json cote manette) -
// celui-ci n'affecte que la vitesse du curseur menu, pas le jeu.
static float g_cursorX = 640.0f;
static float g_cursorY = 360.0f;
static bool g_cursorInitialized = false;
// 0.02f (valeur de depart arbitraire) confirme trop lent sur console reelle
// le 2026-08-01 (curseur bouge mais a peine) - x15 pour un ressenti plus
// proche d'un curseur de bureau normal. Encore une estimation, pas une
// valeur mesuree - a ajuster de nouveau si besoin.
static const float kCursorSensitivity = 0.3f;

static void UpdateMouseCursor(ImGuiIO& io, const InputStateSnapshot& snap, float screenWidth, float screenHeight) {
    if (!g_cursorInitialized) {
        // Centre de l'ecran au premier appel seulement - les frames
        // suivantes partent de la position deja cumulee, pas de reset.
        g_cursorX = screenWidth * 0.5f;
        g_cursorY = screenHeight * 0.5f;
        g_cursorInitialized = true;
    }

    if (snap.valid) {
        g_cursorX += (float)snap.mouseDX * kCursorSensitivity;
        g_cursorY += (float)snap.mouseDY * kCursorSensitivity;
        if (g_cursorX < 0.0f) g_cursorX = 0.0f;
        if (g_cursorY < 0.0f) g_cursorY = 0.0f;
        if (g_cursorX > screenWidth)  g_cursorX = screenWidth;
        if (g_cursorY > screenHeight) g_cursorY = screenHeight;

        io.MousePos = ImVec2(g_cursorX, g_cursorY);
        // bit0 = clic gauche, convention HID boot-mouse deja utilisee partout
        // ailleurs dans ce projet (voir hiddriver/main.cpp,
        // HidFillBootMouseState).
        io.MouseDown[0] = (snap.mouseButtonsMask & 0x01) != 0;
        io.MouseDown[1] = (snap.mouseButtonsMask & 0x02) != 0;
        io.MouseDown[2] = (snap.mouseButtonsMask & 0x04) != 0;
    } else {
        // Pas de plugin.xex charge / pas de souris branchee - curseur hors
        // ecran plutot que fige a une position potentiellement trompeuse
        // (ImGui traite une position negative comme "souris absente").
        io.MousePos = ImVec2(-1.0f, -1.0f);
        io.MouseDown[0] = io.MouseDown[1] = io.MouseDown[2] = false;
    }
}

// --- Selection de peripherique (Jalon 8, application/ROADMAP_APPLICATION.md) --
// Remplace le VID/PID en dur (kKeyboardVid/kMouseVid, repli par defaut
// toujours propose en index 0 de chaque combo) par une vraie liste lue dans
// known_devices.txt via ListKnownDevices() (config_writer.h) - historique que
// hiddriver.xex alimente lui-meme des qu'il classe un device en souris/
// clavier a l'enumeration USB, PAS une configuration explicite via cette
// appli (voir le commentaire de KnownDeviceInfo dans config_writer.h pour
// l'historique de cette decision, 2026-08-03 : la premiere version listait
// seulement les devices deja configures ici, et une saisie manuelle de
// VID/PID a ete explicitement rejetee par l'utilisateur). isMouse est un
// booleen exact (classification reelle du plugin) - deux listes filtrees
// simples, sans l'ambiguite d'heuristique de la premiere version.
static bool g_devicesLoaded = false;
static std::vector<KnownDeviceInfo> g_knownDevices;
// Indices DANS g_knownDevices, pas des VID/PID directement - evite de
// dupliquer les infos device, juste un filtre. Reconstruits par
// RefreshKnownDevices() (jamais modifies frame par frame ailleurs).
static std::vector<int> g_keyboardDeviceIndices;
static std::vector<int> g_mouseDeviceIndices;
// Espace d'index NATIF du combo : 0 = "Par defaut", 1..N = g_xDeviceIndices[i-1]
// - meme convention que g_profileComboIdx plus haut (voir son commentaire),
// pour la meme raison (ImGui::Combo veut un entier stable entre les frames).
static int g_keyboardComboIdx = 0;
static int g_mouseComboIdx = 0;

static void RefreshKnownDevices() {
    g_knownDevices = ListKnownDevices();
    g_keyboardDeviceIndices.clear();
    g_mouseDeviceIndices.clear();
    for (size_t i = 0; i < g_knownDevices.size(); i++) {
        const KnownDeviceInfo& d = g_knownDevices[i];
        if (!d.isMouse) g_keyboardDeviceIndices.push_back((int)i);
        if (d.isMouse) g_mouseDeviceIndices.push_back((int)i);
    }
}

static void LoadDevicesOnce() {
    if (g_devicesLoaded)
        return;
    g_devicesLoaded = true;
    RefreshKnownDevices();
}

// Libelle d'un peripherique dans les listes deroulantes. Le NOM d'abord, le
// VID:PID seulement en repli - remarque de l'utilisateur (2026-08-03) :
// afficher un matricule hexadecimal a quelqu'un qui veut juste choisir sa
// souris n'a aucun sens, d'autant qu'on interroge deja le peripherique pour
// savoir lire ses donnees. Le nom vient du descripteur de chaine USB, recupere
// par le plugin et stocke dans known_devices.txt.
static void FormatDeviceLabel(char* out, size_t outSize, const KnownDeviceInfo& d) {
    if (d.name[0] != '\0')
        _snprintf(out, outSize - 1, "%hs", d.name);
    else
        _snprintf(out, outSize - 1, Tr(STR_DEVICE_NAME_UNAVAILABLE_FMT), d.vid, d.pid);
    out[outSize - 1] = '\0';
}

static bool KeyboardComboGetter(void*, int idx, const char** out_text) {
    static char s_label[48]; // un seul combo actif a la fois, meme precaution que ProfileComboGetter
    if (idx == 0) {
        _snprintf(s_label, sizeof(s_label) - 1, Tr(STR_DEVICE_DEFAULT_LABEL_FMT), kKeyboardVid, kKeyboardPid);
        s_label[sizeof(s_label) - 1] = '\0';
        *out_text = s_label;
        return true;
    }
    int di = idx - 1;
    if (di < 0 || di >= (int)g_keyboardDeviceIndices.size())
        return false;
    const KnownDeviceInfo& d = g_knownDevices[g_keyboardDeviceIndices[di]];
    FormatDeviceLabel(s_label, sizeof(s_label), d);
    *out_text = s_label;
    return true;
}

static bool MouseComboGetter(void*, int idx, const char** out_text) {
    static char s_label[48];
    if (idx == 0) {
        _snprintf(s_label, sizeof(s_label) - 1, Tr(STR_DEVICE_DEFAULT_LABEL_FMT), kMouseVid, kMousePid);
        s_label[sizeof(s_label) - 1] = '\0';
        *out_text = s_label;
        return true;
    }
    int di = idx - 1;
    if (di < 0 || di >= (int)g_mouseDeviceIndices.size())
        return false;
    const KnownDeviceInfo& d = g_knownDevices[g_mouseDeviceIndices[di]];
    FormatDeviceLabel(s_label, sizeof(s_label), d);
    *out_text = s_label;
    return true;
}

static void DrawDeviceSelector() {
    LoadDevicesOnce();

    ImGui::Text(Tr(STR_DEVICE_HEADER));
    if (ImGui::Combo(Tr(STR_DEVICE_KEYBOARD_LABEL), &g_keyboardComboIdx, KeyboardComboGetter, nullptr,
                      (int)g_keyboardDeviceIndices.size() + 1)) {
        if (g_keyboardComboIdx == 0) {
            g_selectedKeyboardVid = kKeyboardVid;
            g_selectedKeyboardPid = kKeyboardPid;
        } else {
            const KnownDeviceInfo& d = g_knownDevices[g_keyboardDeviceIndices[g_keyboardComboIdx - 1]];
            g_selectedKeyboardVid = d.vid;
            g_selectedKeyboardPid = d.pid;
        }
        // Aucun ecran ne met en cache un etat charge specifiquement pour le
        // clavier (contrairement a la souris, voir g_mouseSettingsLoaded/
        // g_profilesLoaded ci-dessous) - le wizard relit toujours a l'ouverture,
        // rien a invalider ici.
    }
    ImGui::SameLine();
    if (ImGui::Combo(Tr(STR_DEVICE_MOUSE_LABEL), &g_mouseComboIdx, MouseComboGetter, nullptr,
                      (int)g_mouseDeviceIndices.size() + 1)) {
        if (g_mouseComboIdx == 0) {
            g_selectedMouseVid = kMouseVid;
            g_selectedMousePid = kMousePid;
        } else {
            const KnownDeviceInfo& d = g_knownDevices[g_mouseDeviceIndices[g_mouseComboIdx - 1]];
            g_selectedMouseVid = d.vid;
            g_selectedMousePid = d.pid;
        }
        // Force le rechargement des ecrans "Reglages souris"/"Profils" au
        // prochain affichage - sans ca, ils garderaient affiches les reglages
        // de l'ANCIEN device jusqu'a la fermeture de l'appli (LoadMouseSettingsOnce/
        // LoadProfilesOnce ne rechargent que si ce flag est a false).
        g_mouseSettingsLoaded = false;
        g_profilesLoaded = false;
    }
    ImGui::SameLine();
    if (ImGui::Button(Tr(STR_DEVICE_BTN_REFRESH))) {
        RefreshKnownDevices();
    }
    ImGui::TextDisabled(Tr(STR_DEVICE_ONE_DEFAULT_HINT));
    ImGui::Separator();
}

// --- Onglet "Comment ca marche" (2026-08-11) --------------------------------
// Demande utilisateur : "page how it works... explique comment l'application
// marche et les raccourcis de la liaison rapide rien de complique on laisse
// simple". Pas de nouvel assistant, pas d'etat a gerer - juste du texte
// statique, relu depuis i18n.h/.cpp comme le reste de l'appli.
static void DrawHowItWorksTab() {
    ImGui::TextWrapped(Tr(STR_HOWTO_INTRO));
    ImGui::Spacing();
    ImGui::BulletText(Tr(STR_HOWTO_STEP_PLUG));
    ImGui::BulletText(Tr(STR_HOWTO_STEP_WIZARD));
    ImGui::BulletText(Tr(STR_HOWTO_STEP_SETTINGS));
    ImGui::BulletText(Tr(STR_HOWTO_STEP_PROFILES));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Bleu (kColorMouseAccent) et vert (kColorKeyboardAccent) utilises ici
    // juste comme deux couleurs de section distinctes pour separer
    // visuellement "liaison rapide" de "sauvegarde F9" - aucun lien avec
    // leur usage clavier/souris habituel (voir theme.h).
    ImGui::TextColored(kColorMouseAccent, Tr(STR_HOWTO_QUICKBIND_HEADER));
    ImGui::TextWrapped(Tr(STR_HOWTO_QUICKBIND_TEXT));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(kColorKeyboardAccent, Tr(STR_HOWTO_F9_HEADER));
    ImGui::TextWrapped(Tr(STR_HOWTO_F9_TEXT));
}

// --- Onglet "Soutenir le projet" (2026-08-11) -------------------------------
// Demande utilisateur : message du developpeur + liens de don (PayPal/
// Ko-fi/GitHub/YouTube), dans un style volontairement "plus attractif et
// revendeur" que le reste de l'interface (plutot neutre/technique
// partout ailleurs) - exception assumee, une seule page.
//
// Chaque lien passe par IsLinkIntact() (donation_links.h) avant d'etre
// affiche - meme principe de protection legere qu'evoque pour le QR code
// de don : si quelqu'un patche la chaine dans le binaire compile sans
// repatcher le hash juste a cote, ce lien precis disparait plutot que
// d'afficher une valeur alteree. Ce n'est PAS une protection contre
// quelqu'un qui recompile depuis les sources (droit legitime sur un fork
// GPL) - juste un obstacle a un patch binaire a la main.
//
// Pas de QR code pour l'instant (2026-08-11) : en attente du fichier SVG
// que HB doit fournir - voir PROJECT_NOTES.md. Cette page n'affiche que du
// texte lisible depuis la TV ; le QR s'ajoutera a cote sans restructurer
// cette fonction.
static void DrawSupportRow(const char* label, const char* value, uint32_t expectedHash, const ImVec4& accent) {
    if (!IsLinkIntact(value, expectedHash)) {
        ImGui::TextDisabled(Tr(STR_SUPPORT_LINK_HIDDEN_HINT));
        return;
    }
    ImGui::TextColored(accent, "%s", label);
    ImGui::SameLine();
    ImGui::TextWrapped("%s", value);
}

static void DrawSupportTab() {
    // Badge "en cours de developpement" en ambre - premiere chose lue en
    // haut de la page, avant meme le message.
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.2f, 1.0f));
    ImGui::TextWrapped("[ %s ]", Tr(STR_SUPPORT_BADGE));
    ImGui::PopStyleColor();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextWrapped(Tr(STR_SUPPORT_MESSAGE));
    ImGui::Spacing();
    ImGui::TextColored(kColorKeyboardAccent, Tr(STR_SUPPORT_CTA));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    DrawSupportRow(Tr(STR_SUPPORT_PAYPAL_LABEL), kPayPalEmail, kPayPalEmailHash, kColorMouseAccent);
    DrawSupportRow(Tr(STR_SUPPORT_KOFI_LABEL), kKofiUrl, kKofiUrlHash, kColorKeyboardAccent);
    DrawSupportRow(Tr(STR_SUPPORT_GITHUB_LABEL), kGithubUrl, kGithubUrlHash, kColorMouseAccent);
    DrawSupportRow(Tr(STR_SUPPORT_YOUTUBE_LABEL), kYoutubeUrl, kYoutubeUrlHash, kColorKeyboardAccent);
}

// --- Diagnostic visuel (2026-08-01) -----------------------------------------
// Ajoute suite a un retour utilisateur ou ni le clavier (deja signale plus
// tot) ni le nouveau curseur souris ne semblaient recevoir de donnees, alors
// que hiddriver.xex les detecte bien cote log. Plutot que deviner encore une
// fois entre "input_state.json mal ecrit", "mal lu", ou "jamais rafraichi",
// affiche directement ce que l'appli voit reellement chaque frame - "tick"
// notamment: s'il reste fige d'une capture d'ecran a l'autre, le fichier
// n'est jamais rafraichi (bug cote ecriture ou tout simplement jamais
// atteint) ; s'il avance mais que les autres champs restent a leurs valeurs
// par defaut, le souci est plus cible. A retirer une fois le probleme
// identifie et corrige.
static void DrawDebugOverlay(const InputStateSnapshot& snap) {
    XexUtils::Input::Gamepad* pad = XexUtils::Input::GetInput(0);
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
        "DEBUG snap: valid=%d key=%d mBtn=%d mDX=%d mDY=%d mMask=%d tick=%u",
        snap.valid ? 1 : 0, (int)snap.keyCode, (int)snap.mouseButton,
        (int)snap.mouseDX, (int)snap.mouseDY, (int)snap.mouseButtonsMask,
        (unsigned int)snap.tick);
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f),
        "DEBUG pad: wButtons=0x%04x LX=%.2f LY=%.2f",
        (unsigned int)pad->wButtons, pad->ThumbLeftX, pad->ThumbLeftY);
    ImGui::Separator();
}

// --- Point d'entree ---------------------------------------------------------
void __cdecl main() {
    // Points de controle ajoutes le 2026-08-01 pour diagnostiquer un "rien ne
    // s'affiche" reporte au premier lancement reel. Passaient a l'origine par
    // XexUtils::Xam::XNotify (notification visible a l'ecran, systeme
    // d'Aurora/Xam, independant de notre propre device D3D9 - donc visible
    // meme si la creation du device ou l'init ImGui echoue silencieusement,
    // return anticipe sans aucune trace jusque-la).
    //
    // Rebascules en AppLog silencieux le 2026-08-09 (signale par HB : "je
    // veux mettre au silence les notifications qui se lancent avec l'app") -
    // le rendu est confirme stable depuis, ces points de repere n'avaient
    // plus de raison de s'afficher a l'ecran a CHAQUE lancement. Seuls les
    // ECHECs (creation D3D9/device/ImGui) restent en XNotify visible plus
    // bas : ce sont les seuls cas ou l'utilisateur a besoin de savoir que
    // quelque chose s'est reellement mal passe - les etapes de succes
    // (creation reussie, boucle demarree...) n'apportent rien a l'ecran et
    // restent desormais uniquement dans le journal.
    AppLog("App: main() demarre");

    // Monte "HDD:" en lien symbolique AVANT tout acces fichier (2026-08-01,
    // cause reelle identifiee du bug "clavier non detecte + curseur immobile")
    // : contrairement a hiddriver.xex, un plugin DashLaunch qui tourne DANS
    // le processus xam.xex du dashboard - lequel a deja "hdd:" mappe comme
    // lien symbolique SYSTEME - application.xex est un titre lance de facon
    // independante, avec son propre namespace d'objets prive, et n'herite
    // d'aucun mapping "HDD:". Sans cet appel, TOUT std::ifstream/ofstream
    // vers "HDD:\..." echoue silencieusement (is_open() == false) - confirme
    // par le nouveau diagnostic visuel (DrawDebugOverlay : valid=0, tick=0,
    // alors que le fichier existe bel et bien et est rafraichi cote plugin).
    // Reutilise XexUtils::Fs::MountHdd() (deja vendorise, voir Filesystem.cpp)
    // plutot que reimplementer ObCreateSymbolicLink a la main. Si le lien
    // existe deja (peu probable ici mais inoffensif), l'appel echoue avec
    // une collision - pas traite comme une erreur bloquante, juste notifie.
    if (FAILED(XexUtils::Fs::MountHdd())) {
        // Cas courant et inoffensif (lien deja monte) plutot qu'une vraie
        // panne - reste silencieux comme les autres points de repere de ce
        // bloc (voir plus haut). Le journal garde la trace si besoin de
        // diagnostiquer un vrai probleme HDD: plus tard.
        AppLog("App: MountHdd a echoue (deja monte ?)");
    }

    // X360Remap.json/input_state.json vivent maintenant dans un sous-dossier
    // dedie (HDD:\X360RemapStudio\) plutot qu'a la racine du disque dur -
    // demande utilisateur du 2026-08-02 pour ne pas polluer HDD:\. Cree le
    // dossier s'il n'existe pas encore (premier lancement) ; s'il existe deja,
    // CreateDirectoryA echoue avec ERROR_ALREADY_EXISTS, ce qui est normal et
    // pas une erreur bloquante. hiddriver.xex cree ce meme dossier de son
    // cote (voir hiddriver/main.cpp) puisque les deux .xex y ecrivent.
    if (!CreateDirectoryA("HDD:\\X360RemapStudio", nullptr)) {
        DWORD err = GetLastError();
        if (err != ERROR_ALREADY_EXISTS) {
            XexUtils::Xam::XNotify("App: creation dossier X360RemapStudio echouee");
        }
    }

    // Langue de l'UI (2026-08-07, voir application/i18n.h) - lue une seule
    // fois ici, apres le montage HDD: et la creation du dossier
    // X360RemapStudio ci-dessus (meme ordre de dependance que
    // force_boot_protocol.txt, qui suppose lui aussi HDD: deja monte).
    // Retombe sur LANG_FR (comportement actuel avant ce systeme) si le
    // fichier est absent ou son contenu non reconnu - voir LoadLanguagePref().
    LoadLanguagePref();

    // Creation du device D3D9 - meme schema que n'importe quel homebrew Xbox
    // 360 (snes360-enhanced, reference GPL-3.0 citee dans ARCHITECTURE.md).
    // Contrairement au plan XUI initial, application.xex possede son propre
    // device : pas besoin de hooker celui d'Aurora, on prend l'ecran comme
    // n'importe quel jeu/emulateur homebrew le temps de tourner.
    LPDIRECT3D9 pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) {
        // Echec reel, garde en visible - l'utilisateur doit savoir que
        // l'application n'a pas pu demarrer du tout.
        XexUtils::Xam::XNotify("App: ECHEC Direct3DCreate9");
        return;
    }
    AppLog("App: Direct3D9 cree");

    D3DPRESENT_PARAMETERS pp = {};
    pp.BackBufferWidth = 1280;
    pp.BackBufferHeight = 720;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.BackBufferCount = 1;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    pp.Windowed = FALSE;
    // D3DPRESENT_INTERVAL_ONE_OR_IMMEDIATE n'existe pas en D3D9 standard
    // (erreur reelle "undeclared identifier" au build) - probablement
    // confondu avec une constante D3D9Ex PC. D3DPRESENT_INTERVAL_ONE
    // (vsync, ~60Hz) est le choix standard pour une interface, evite le
    // tearing et une boucle qui tourne sans limite.
    pp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    LPDIRECT3DDEVICE9 pDevice = nullptr;
    if (FAILED(pD3D->CreateDevice(0, D3DDEVTYPE_HAL, nullptr, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &pDevice))) {
        // Echec reel, garde en visible - meme raisonnement que ECHEC
        // Direct3DCreate9 ci-dessus.
        XexUtils::Xam::XNotify("App: ECHEC CreateDevice");
        pD3D->Release();
        return;
    }
    AppLog("App: Device D3D9 cree");

    ImGui::CreateContext();
    // Theme visuel "X360Remap Studio" (2026-08-08, debut de la refonte
    // demandee par HB - voir theme.h et ETAT_DU_PROJET.md section 3). Doit
    // s'appliquer avant la premiere ImGui::NewFrame() plus bas ; APRES
    // CreateContext() puisque ImGui::GetStyle() n'existe qu'une fois le
    // contexte cree.
    ApplyX360RemapTheme();
    ImGuiIO& io = ImGui::GetIO();
    // Navigation manette gardee en secours (utile si la souris n'est pas
    // branchee pendant la config) - le curseur souris (UpdateMouseCursor,
    // 2026-08-01) est desormais le moyen principal, voir sa note en tete.
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.DisplaySize = ImVec2((float)pp.BackBufferWidth, (float)pp.BackBufferHeight);
    // Xbox 360 n'a pas de curseur systeme - ImGui doit dessiner le sien a
    // partir d'io.MousePos (voir UpdateMouseCursor).
    io.MouseDrawCursor = true;

    // Police custom "Inter" (2026-08-08, voir theme.h) - APRES avoir pose
    // io.DisplaySize/ConfigFlags (elle ne les lit pas, mais autant garder
    // toute la configuration io.* groupee) et AVANT ImGui_ImplDX9_Init() :
    // l'atlas de police doit etre construit avant que l'implementation DX9
    // ne cree sa texture a partir de lui. 24px : taille de depart pour le
    // texte courant, a ajuster une fois la mise en page de la maquette 2 en
    // place (la consigne HB de 41px visait le texte d'instruction principal
    // du wizard, pas le texte courant general).
    LoadX360RemapFont(24.0f);

    if (!ImGui_ImplDX9_Init(pDevice)) {
        // Echec reel, garde en visible - meme raisonnement que les deux
        // echecs D3D9 ci-dessus.
        XexUtils::Xam::XNotify("App: ECHEC ImGui_ImplDX9_Init");
        pDevice->Release();
        pD3D->Release();
        return;
    }
    AppLog("App: ImGui pret, boucle de rendu demarree");

    bool firstFramePresented = false;
    bool running = true;
    while (running) {
        // Une seule lecture de input_state.json par frame, partagee par
        // DrawWizardTab/UpdateMouseCursor/DrawDebugOverlay (voir g_lastSnapshot).
        g_lastSnapshot = ReadInputState();

        // Defilement a la molette (2026-08-09, demande utilisateur : "je vais
        // pas cliquer sur les fleches ou presser sur la barre"). io.MouseWheel
        // doit etre pose AVANT ImGui::NewFrame() - c'est ce que NewFrame lit
        // pour mettre a jour le defilement de la fenetre/zone sous le
        // curseur. g_lastSnapshot.wheelDelta est deja le delta accumule
        // depuis la derniere lecture (voir input_state_reader.h), pas une
        // valeur absolue - directement utilisable tel quel.
        io.MouseWheel = (float)g_lastSnapshot.wheelDelta;

        UpdateGamepadNavigation(io);
        UpdateMouseCursor(io, g_lastSnapshot, (float)pp.BackBufferWidth, (float)pp.BackBufferHeight);

        ImGui_ImplDX9_NewFrame();
        ImGui::NewFrame();

        // Zone sure TV : marge de 7% tout autour (2026-08-09, demande
        // utilisateur : "je veux que l'affichage ne prenne pas tout
        // l'ecran"). Remplace l'ancienne marge fixe de 40px, qui ne
        // respectait pas la consigne deja posee dans ETAT_DU_PROJET.md
        // section 3 (40px ~= 3-5% de 1280x720 selon l'axe, sous les 7%
        // vises) - insuffisant sur une TV avec overscan notable.
        const float kSafeZoneMargin = 0.07f;
        float marginX = (float)pp.BackBufferWidth * kSafeZoneMargin;
        float marginY = (float)pp.BackBufferHeight * kSafeZoneMargin;
        ImGui::SetNextWindowPos(ImVec2(marginX, marginY));
        ImGui::SetNextWindowSize(ImVec2((float)pp.BackBufferWidth - marginX * 2.0f, (float)pp.BackBufferHeight - marginY * 2.0f));
        ImGui::Begin(Tr(STR_APP_WINDOW_TITLE), nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        DrawDebugOverlay(g_lastSnapshot);
        DrawDeviceSelector();

        if (ImGui::BeginTabBar("MainTabs")) {
            if (ImGui::BeginTabItem(Tr(STR_APP_TAB_WIZARD))) {
                g_activeTab = TAB_WIZARD;
                DrawWizardTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(Tr(STR_APP_TAB_SETTINGS))) {
                g_activeTab = TAB_MOUSE_SETTINGS;
                DrawMouseSettingsTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(Tr(STR_APP_TAB_PROFILES))) {
                g_activeTab = TAB_PROFILES;
                DrawProfilesTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(Tr(STR_APP_TAB_CALIBRATION))) {
                g_activeTab = TAB_CALIBRATION;
                DrawCalibrationTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(Tr(STR_APP_TAB_HOWTO))) {
                g_activeTab = TAB_HOWTO;
                DrawHowItWorksTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(Tr(STR_APP_TAB_SUPPORT))) {
                g_activeTab = TAB_SUPPORT;
                DrawSupportTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();

        pDevice->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 20, 20, 24), 1.0f, 0);
        if (SUCCEEDED(pDevice->BeginScene())) {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            pDevice->EndScene();
        }
        pDevice->Present(nullptr, nullptr, nullptr, nullptr);

        // Point de controle une seule fois (pas a chaque frame, ca
        // spammerait des notifications en continu) - confirme que la
        // boucle de rendu tourne reellement jusqu'au bout au moins une
        // fois, distinct de "la boucle demarre" ci-dessus (qui ne prouve
        // pas que BeginScene/Clear/Present ne plantent pas).
        if (!firstFramePresented) {
            firstFramePresented = true;
            AppLog("App: premiere frame presentee");
        }

        // Pas de condition de sortie explicite pour ce premier jet - a
        // ajouter (ex: combinaison de boutons, ou bouton "Quitter" dans un
        // futur menu principal) une fois l'ecran unique confirme buildable.
    }

    delete g_wizard;
    ImGui_ImplDX9_Shutdown();
    ImGui::DestroyContext();
    pDevice->Release();
    pD3D->Release();
}
