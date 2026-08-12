#pragma once
// Systeme de traduction FR/EN pour les notifications a l'ecran (XNotifyUI)
// de hiddriver.xex (le plugin). Contrairement a application/i18n.h, ce
// fichier N'A PAS besoin de rester portable/g++ (hiddriver n'est pas compile
// par la suite de tests portable de application/) : xtl.h et les types
// wide-char de la XDK sont utilisables librement ici.
//
// hiddriver.xex et application.xex sont deux binaires .xex separes, sans
// memoire partagee : chacun lit independamment le meme fichier de
// preference sur HDD: pour rester synchronise (voir LoadPluginLanguageSetting
// ci-dessous, et SaveLanguagePref/LoadLanguagePref dans application/i18n.h/
// i18n.cpp pour le cote qui ECRIT ce fichier).
//
// LANG_FR/LANG_EN doivent garder exactement les memes valeurs numeriques que
// application/i18n.h (Lang) : les deux binaires lisent le meme octet "0"/"1"
// sur disque avec la meme convention.
//
// Memes regles que application/i18n.h : chaines FR volontairement SANS
// accents (ASCII pur) - contournement police conserve ici aussi, meme si ce
// plugin n'utilise pas ImGui : XNotifyUI utilise la police systeme du
// dashboard, dont le comportement avec les caracteres accentues n'a pas ete
// verifie sur hardware. Par prudence/coherence avec le reste du projet, on
// garde la meme convention ASCII-only.
enum Lang { LANG_FR = 0, LANG_EN = 1, LANG_COUNT };

// Un NotifyStrId par gabarit XNotifyUI distinct dans hiddriver/main.cpp.
// Regroupe dans l'ordre d'apparition des appels XNotifyUI() dans main.cpp.
enum NotifyStrId {
	// MappingManagerThreadProc : notification souris/clavier detecte
	STR_MOUSE_DETECTED_FMT,          // "Mouse detected. VID:%04x PID:%04x map:%hs btns:%d sens:%d slot:%d"
	STR_KEYBOARD_READY_FMT,          // "Keyboard ready. ... (slot %d)"

	// MappingManagerThreadProc : hot-reload de X360Remap.json
	STR_JSON_RELOADED,               // "X360Remap.json reloaded"
	STR_JSON_INVALID,                // "X360Remap.json invalid - check syntax"

	// MappingManagerThreadProc : raccourci F9 "sauvegarder le profil"
	STR_PROFILE_SAVED,               // "Profil enregistre pour ce jeu"
	STR_PROFILE_SAVE_FAILED,         // "Echec de l'enregistrement du profil"

	// MappingManagerThreadProc : liaison rapide (quick bind)
	STR_QUICKBIND_CANCELED,          // "Liaison rapide annulee"
	STR_QUICKBIND_ARMED,             // "Liaison rapide : appuie sur le bouton manette a lier"
	STR_QUICKBIND_TARGET_FMT,        // "Cible : %s - appuie sur la touche ou le clic a associer"
	STR_QUICKBIND_BOUND_FMT,         // "%s lie pour ce jeu"
	STR_QUICKBIND_BIND_FAILED_FMT,   // "Echec de la liaison de %s"

	// MappingThreadProc : assistant de mapping manette
	STR_UNKNOWN_CONTROLLER,          // "Unknown controller connected. Starting mapping process..."
	STR_PRESS_BUTTON_FMT,            // "Press %hs on controller (hold 3s to skip)"
	STR_MAPPING_SKIPPED,             // "Mapping skipped"
	STR_MAPPING_COMPLETE,            // "Mapping complete! Controller ready."

	STR_NOTIFY_COUNT
};

// Langue active des notifications du plugin. Par defaut LANG_FR, comme
// g_currentLang dans application/i18n.cpp, jusqu'a ce que
// LoadPluginLanguageSetting() lise le fichier de preference (ou que ce
// fichier soit absent/non reconnu, auquel cas on reste sur LANG_FR).
extern Lang g_pluginLang;

// Lu une seule fois au demarrage (voir l'appel unique juste a cote de
// LoadBootProtocolSetting() dans main.cpp) - PAS a chaque tick du thread de
// polling. Meme fichier, meme convention "0"/"1" qu'application/i18n.h :
// "0" = LANG_FR, "1" = LANG_EN, absent/non reconnu = LANG_FR. Meme pattern
// std::ifstream que LoadBootProtocolSetting() (voir main.cpp) pour rester
// coherent avec le reste du fichier.
void LoadPluginLanguageSetting();

// Renvoie la chaine large (wchar_t) de l'id donne pour g_pluginLang. Si la
// case courante est nulle (table mal remplie), retombe sur la ligne LANG_FR
// plutot que de renvoyer nullptr - un XNotifyUI(nullptr) serait dangereux.
const wchar_t* WTr(NotifyStrId id);
