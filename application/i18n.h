#pragma once
// Systeme de traduction FR/EN de l'interface (application.xex uniquement -
// hiddriver.xex, le plugin, n'a pas d'UI). Fichier volontairement en C++
// standard PUR (pas de windows.h/xtl.h, pas d'ImGui, pas de _snprintf) : il
// doit rester includable par les suites de tests portables g++
// (application/tests/test_wizard_session.cpp, test_calibration_session.cpp)
// exactement comme wizard_session.h/calibration_session.h.
//
// Toutes les chaines FR existantes de l'appli sont ecrites SANS accents
// (ex: "Demarrer", "Peripheriques", "Sensibilite") - contournement
// deliberement conserve ici : la police par defaut d'ImGui utilisee par
// application.xex est ASCII seul, sans configuration de plage de glyphes
// (glyph ranges). Ne PAS ajouter d'accents aux chaines FR tant que ce travail
// de police n'est pas fait (phase UI-overhaul ulterieure) - ce fichier ne
// touche ni au chargement de police ni aux glyph ranges.
//
// Portugais (Bresil) : prevu par l'architecture (voir Lang/LANG_PT_BR
// ci-dessous) mais PAS ajoute maintenant, pour la meme raison que le
// contournement sans-accents ci-dessus - le portugais a lui aussi besoin de
// glyphes accentues que la police actuelle ne sait pas afficher.

#include <cstddef>

// LANG_PT_BR (portugais Bresil) sera insere ICI, juste avant LANG_COUNT,
// une fois le travail de plage de glyphes (glyph ranges) fait sur la police
// ImGui - le portugais a besoin des memes glyphes accentues que le francais
// aurait besoin s'il n'avait pas ete ecrit sans accents. Ne pas ajouter cette
// valeur avant que la police puisse effectivement les afficher.
enum Lang {
    LANG_FR = 0,
    LANG_EN = 1,
    LANG_COUNT
};

// Un StrId par chaine d'UI distincte, groupe par ecran/fichier d'origine.
// Les commentaires de section suivent exactement le catalogue de la passe de
// recherche prealable (voir le rapport qui a precede ce fichier) : onglet
// wizard, onglet calibration, onglet reglages, onglet profils, selecteur de
// peripherique, titre fenetre/barre d'onglets (tous dans application/main.cpp),
// puis les libelles d'etape de application/wizard_session.cpp et enfin les
// consignes/resume de application/calibration_session.cpp.
enum StrId {
    // --- main.cpp : DrawWizardTab (onglet "Wizard clavier") ---
    STR_WIZARD_BTN_START,
    STR_WIZARD_NO_KEYBOARD,
    STR_WIZARD_STEP_PROGRESS_FMT,
    STR_WIZARD_PRESS_KEY_FOR_FMT,
    STR_WIZARD_HOLD_TO_SKIP,
    STR_WIZARD_SAVED_FMT,
    STR_WIZARD_SAVE_FAILED,
    STR_WIZARD_BTN_OK,
    STR_WIZARD_CANCELLED,

    // --- main.cpp : DrawCalibrationTab (onglet "Calibration souris") ---
    STR_CALIB_INTRO,
    STR_CALIB_NAV_HINT,
    STR_CALIB_TARGET_MOUSE_FMT,
    STR_CALIB_TARGET_MOUSE_HINT,
    STR_CALIB_BTN_START,
    STR_CALIB_FALLBACK_ACTIVE,
    STR_CALIB_FALLBACK_ACTIVE_HINT,
    STR_CALIB_AUTO_ACTIVE,
    STR_CALIB_FALLBACK_EXPLAIN,
    STR_CALIB_BTN_FORCE_FALLBACK,
    STR_CALIB_BTN_RESTORE_AUTO,
    STR_CALIB_CLEAR_HINT,
    STR_CALIB_BTN_CLEAR,
    STR_CALIB_NO_DATA,
    STR_CALIB_SUCCESS,
    STR_CALIB_BTN_SAVE,
    STR_CALIB_SAVED_APPLIED,
    STR_CALIB_SAVE_FAILED,
    STR_CALIB_BTN_CLOSE,
    STR_CALIB_GESTURE_DONE,
    STR_CALIB_GESTURE_CONTINUE,
    STR_CALIB_BTN_VALIDATE_STEP,
    STR_CALIB_VALIDATE_DISABLED_HINT,
    STR_CALIB_BTN_NO_WHEEL,
    STR_CALIB_BTN_CANCEL,

    // --- main.cpp : DrawMouseSettingsTab (onglet "Reglages souris") ---
    STR_SETTINGS_SENSITIVITY_LABEL,
    STR_SETTINGS_DEADZONE_LABEL,
    STR_SETTINGS_INVERT_Y_LABEL,
    STR_SETTINGS_BTN_SAVE,
    STR_SETTINGS_MOUSE_BUTTONS_HEADER,
    STR_SETTINGS_CLICK_LEFT_LABEL,
    STR_SETTINGS_CLICK_RIGHT_LABEL,
    STR_SETTINGS_CLICK_MIDDLE_LABEL,
    STR_SETTINGS_CLICK_DEFAULT_HINT,
    STR_SETTINGS_WHEEL_FORWARD_LABEL,
    STR_SETTINGS_WHEEL_BACKWARD_LABEL,
    STR_SETTINGS_WHEEL_DEFAULT_HINT,
    STR_SETTINGS_BTN_SAVE_BUTTONS,
    STR_SETTINGS_DEFAULTS_HEADER,
    STR_SETTINGS_BTN_SAVE_AS_DEFAULT,
    STR_SETTINGS_NO_DEFAULT_SAVED,
    STR_SETTINGS_BTN_RESTORE_DEFAULT,
    // Libelles de kMouseButtonTargets (combos "Clic gauche/droit/milieu" et
    // "Molette avant/arriere") - section separee des libelles d'etape de
    // wizard_session.cpp meme quand le texte se ressemble (ex: "Start"),
    // ce sont deux tableaux independants dans deux fichiers differents.
    STR_SETTINGS_TARGET_DEFAULT,
    STR_SETTINGS_TARGET_A,
    STR_SETTINGS_TARGET_B,
    STR_SETTINGS_TARGET_X,
    STR_SETTINGS_TARGET_Y,
    STR_SETTINGS_TARGET_LB,
    STR_SETTINGS_TARGET_RB,
    STR_SETTINGS_TARGET_LT,
    STR_SETTINGS_TARGET_RT,
    STR_SETTINGS_TARGET_L3,
    STR_SETTINGS_TARGET_R3,
    STR_SETTINGS_TARGET_START,
    STR_SETTINGS_TARGET_BACK,
    STR_SETTINGS_TARGET_DPAD_UP,
    STR_SETTINGS_TARGET_DPAD_DOWN,
    STR_SETTINGS_TARGET_DPAD_LEFT,
    STR_SETTINGS_TARGET_DPAD_RIGHT,

    // --- main.cpp : DrawProfilesTab (onglet "Profils") ---
    STR_PROFILES_INTRO,
    STR_PROFILES_F9_HINT,
    STR_PROFILES_COMBO_LABEL,
    STR_PROFILES_COMBO_NEW,
    STR_PROFILES_KNOWN_TITLES_COMBO_LABEL,
    STR_PROFILES_KNOWN_TITLES_HINT,
    STR_PROFILES_NO_KNOWN_TITLES,
    STR_PROFILES_RADIO_TYPE_TITLEID,
    STR_PROFILES_RADIO_TYPE_GAMENAME,
    STR_PROFILES_INPUT_TITLEID,
    STR_PROFILES_HINT_TITLEID_CHARS,
    STR_PROFILES_INPUT_GAME_NAME,
    STR_PROFILES_HINT_GAME_NAME_CHARS,
    STR_PROFILES_TITLEID_DISPLAY_FMT,
    STR_PROFILES_SENSITIVITY_LABEL,
    STR_PROFILES_DEADZONE_LABEL,
    STR_PROFILES_INVERT_Y_LABEL,
    STR_PROFILES_CURVE_LINEAR,
    STR_PROFILES_CURVE_EXPONENTIAL,
    STR_PROFILES_CURVE_COMBO_LABEL,
    STR_PROFILES_EXPONENT_LABEL,
    STR_PROFILES_EXPONENT_HINT,
    STR_PROFILES_BTN_SAVE,
    STR_PROFILES_BTN_DELETE,
    STR_PROFILES_NONE_SAVED,

    // --- main.cpp : DrawDeviceSelector ---
    STR_DEVICE_HEADER,
    STR_DEVICE_KEYBOARD_LABEL,
    STR_DEVICE_MOUSE_LABEL,
    STR_DEVICE_BTN_REFRESH,
    STR_DEVICE_ONE_DEFAULT_HINT,
    STR_DEVICE_NAME_UNAVAILABLE_FMT,
    STR_DEVICE_DEFAULT_LABEL_FMT,

    // --- main.cpp : titre fenetre / barre d'onglets (main()) ---
    STR_APP_WINDOW_TITLE,
    STR_APP_TAB_WIZARD,
    STR_APP_TAB_SETTINGS,
    STR_APP_TAB_PROFILES,
    STR_APP_TAB_CALIBRATION,
    // Ajoutes 2026-08-11 (demande utilisateur : deux pages supplementaires -
    // "comment ca marche" et une page de soutien/dons)
    STR_APP_TAB_HOWTO,
    STR_APP_TAB_SUPPORT,

    // --- wizard_session.cpp : DefaultKeyboardWizardSteps (libelles d'etape) ---
    STR_WIZSTEP_A,
    STR_WIZSTEP_B,
    STR_WIZSTEP_X,
    STR_WIZSTEP_Y,
    STR_WIZSTEP_LB,
    STR_WIZSTEP_RB,
    STR_WIZSTEP_LT,
    STR_WIZSTEP_RT,
    STR_WIZSTEP_L3,
    STR_WIZSTEP_R3,
    STR_WIZSTEP_START,
    STR_WIZSTEP_BACK,
    STR_WIZSTEP_GUIDE,
    STR_WIZSTEP_DPAD_UP,
    STR_WIZSTEP_DPAD_DOWN,
    STR_WIZSTEP_DPAD_LEFT,
    STR_WIZSTEP_DPAD_RIGHT,
    STR_WIZSTEP_LSTICK_UP,
    STR_WIZSTEP_LSTICK_DOWN,
    STR_WIZSTEP_LSTICK_LEFT,
    STR_WIZSTEP_LSTICK_RIGHT,

    // --- calibration_session.cpp : Prompt() (consigne par etape) ---
    STR_CALSTEP_REST,
    STR_CALSTEP_RIGHT,
    STR_CALSTEP_LEFT,
    STR_CALSTEP_DOWN,
    STR_CALSTEP_UP,
    STR_CALSTEP_WHEEL_FWD,
    STR_CALSTEP_WHEEL_BACK,
    STR_CALSTEP_CLICK_LEFT,
    STR_CALSTEP_DONE,
    STR_CALSTEP_FAILED,

    // --- calibration_session.cpp : Summary() (resume construit via _snprintf) ---
    STR_CALSUM_FAIL_FMT,
    STR_CALSUM_YES,
    STR_CALSUM_NO_CAPS,
    STR_CALSUM_NO,
    STR_CALSUM_WHEEL_NONE,
    STR_CALSUM_WHEEL_BYTE_FMT,
    STR_CALSUM_SUCCESS_FMT,

    // --- main.cpp : DrawHowItWorksTab (onglet "Comment ca marche",
    // ajoute 2026-08-11) - reste simple, texte uniquement, pas de nouvel
    // ecran/wizard : explique le fonctionnement general + la liaison
    // rapide (Back+Start) + le raccourci F9. ---
    STR_HOWTO_INTRO,
    STR_HOWTO_STEP_PLUG,
    STR_HOWTO_STEP_WIZARD,
    STR_HOWTO_STEP_SETTINGS,
    STR_HOWTO_STEP_PROFILES,
    STR_HOWTO_QUICKBIND_HEADER,
    STR_HOWTO_QUICKBIND_TEXT,
    STR_HOWTO_F9_HEADER,
    STR_HOWTO_F9_TEXT,

    // --- main.cpp : DrawSupportTab (onglet "Soutenir le projet",
    // ajoute 2026-08-11) - message du developpeur + liens de don, voir
    // donation_links.h pour les chaines de lien elles-memes (non traduites,
    // ce sont des identifiants/URLs, pas du texte d'interface). ---
    STR_SUPPORT_BADGE,
    STR_SUPPORT_MESSAGE,
    STR_SUPPORT_CTA,
    STR_SUPPORT_PAYPAL_LABEL,
    STR_SUPPORT_KOFI_LABEL,
    STR_SUPPORT_GITHUB_LABEL,
    STR_SUPPORT_YOUTUBE_LABEL,
    STR_SUPPORT_LINK_HIDDEN_HINT,

    STR_COUNT
};

// Renvoie la chaine de l'id donne pour la langue active (GetLanguage()). Si
// jamais la case courante est nulle (table mal remplie), retombe sur LANG_FR
// plutot que de renvoyer nullptr - un ImGui::Text(nullptr) planterait.
const char* Tr(StrId id);

Lang GetLanguage();

// En memoire uniquement - pas d'ecriture disque ici (voir SaveLanguagePref
// pour la variante qui persiste). Utile pour changer la langue sans vouloir
// forcement la rendre permanente (ex: tests).
void SetLanguage(Lang lang);

// A appeler une fois au demarrage de l'appli (voir main(), application/main.cpp) :
// lit la langue sauvegardee sur disque et l'applique. Si le fichier est
// absent ou son contenu non reconnu, reste sur LANG_FR (comportement actuel
// avant l'ajout de ce systeme).
void LoadLanguagePref();

// Ecrit la langue choisie sur disque ET l'applique immediatement (appelle
// SetLanguage) - c'est ce que l'UI doit appeler quand l'utilisateur choisit
// une langue (voir le selecteur de langue dans DrawMouseSettingsTab).
void SaveLanguagePref(Lang lang);
