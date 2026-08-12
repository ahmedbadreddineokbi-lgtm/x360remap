#include "lang.h"
#include <fstream>
#include <string>
#include <iterator>

// FileLog() is defined in main.cpp (no header of its own - forward-declared
// wherever it is used outside main.cpp, same as here). Same signature/
// convention as every other FileLog() call site in this project.
void FileLog(const char* fmt, ...);

Lang g_pluginLang = LANG_FR;

// [LANG_FR][...] chaines existantes de main.cpp copiees verbatim (aucune
// reformulation), y compris l'absence d'accents deja en usage dans ce
// projet (meme convention que application/i18n.cpp). [LANG_EN][...]
// traductions/originaux anglais - ton technique/terse identique, pas de
// texte marketing. Les gabarits swprintf (%s/%hs/%04x/%d/...) gardent
// exactement les memes specificateurs, dans le meme ordre, dans les deux
// lignes : les appels swprintf existants dans main.cpp n'ont pas a changer.
static const wchar_t* g_notifyStrings[LANG_COUNT][STR_NOTIFY_COUNT] = {
	// LANG_FR
	{
		/* STR_MOUSE_DETECTED_FMT        */ L"Souris detectee. VID:%04x PID:%04x map:%hs btns:%d sens:%d slot:%d",
		/* STR_KEYBOARD_READY_FMT        */ L"Clavier pret. WASD=stick gauche, IJKL=stick droit, ZXCV=ABXY, Entree=Start (slot %d)",
		/* STR_JSON_RELOADED             */ L"X360Remap.json recharge",
		/* STR_JSON_INVALID              */ L"X360Remap.json invalide - verifier la syntaxe",
		/* STR_PROFILE_SAVED             */ L"Profil enregistre pour ce jeu",
		/* STR_PROFILE_SAVE_FAILED       */ L"Echec de l'enregistrement du profil",
		/* STR_QUICKBIND_CANCELED        */ L"Liaison rapide annulee",
		/* STR_QUICKBIND_ARMED           */ L"Liaison rapide : appuie sur le bouton manette a lier",
		/* STR_QUICKBIND_TARGET_FMT      */ L"Cible : %s - appuie sur la touche ou le clic a associer",
		/* STR_QUICKBIND_BOUND_FMT       */ L"%s lie pour ce jeu",
		/* STR_QUICKBIND_BIND_FAILED_FMT */ L"Echec de la liaison de %s",
		/* STR_UNKNOWN_CONTROLLER        */ L"Manette inconnue connectee. Demarrage du processus de mapping...",
		/* STR_PRESS_BUTTON_FMT          */ L"Appuie sur %hs sur la manette (maintenir 3s pour passer)",
		/* STR_MAPPING_SKIPPED           */ L"Mapping ignore",
		/* STR_MAPPING_COMPLETE          */ L"Mapping termine ! Manette prete.",
	},
	// LANG_EN
	{
		/* STR_MOUSE_DETECTED_FMT        */ L"Mouse detected. VID:%04x PID:%04x map:%hs btns:%d sens:%d slot:%d",
		/* STR_KEYBOARD_READY_FMT        */ L"Keyboard ready. WASD=left stick, IJKL=right stick, ZXCV=ABXY, Enter=Start (slot %d)",
		/* STR_JSON_RELOADED             */ L"X360Remap.json reloaded",
		/* STR_JSON_INVALID              */ L"X360Remap.json invalid - check syntax",
		/* STR_PROFILE_SAVED             */ L"Profile saved for this game",
		/* STR_PROFILE_SAVE_FAILED       */ L"Failed to save profile",
		/* STR_QUICKBIND_CANCELED        */ L"Quick bind canceled",
		/* STR_QUICKBIND_ARMED           */ L"Quick bind: press the controller button to bind",
		/* STR_QUICKBIND_TARGET_FMT      */ L"Target: %s - press the key or click to assign",
		/* STR_QUICKBIND_BOUND_FMT       */ L"%s bound for this game",
		/* STR_QUICKBIND_BIND_FAILED_FMT */ L"Failed to bind %s",
		/* STR_UNKNOWN_CONTROLLER        */ L"Unknown controller connected. Starting mapping process...",
		/* STR_PRESS_BUTTON_FMT          */ L"Press %hs on controller (hold 3s to skip)",
		/* STR_MAPPING_SKIPPED           */ L"Mapping skipped",
		/* STR_MAPPING_COMPLETE          */ L"Mapping complete! Controller ready.",
	},
};

// Meme pattern std::ifstream que LoadBootProtocolSetting() (voir main.cpp) :
// un seul fichier, un seul octet significatif, lu une fois au demarrage.
// Meme fichier et meme convention "0"/"1" qu'application/i18n.cpp
// (LoadLanguagePref/SaveLanguagePref) - "1" quelque part dans le contenu =
// LANG_EN, sinon (fichier absent, "0", contenu non reconnu) = LANG_FR.
void LoadPluginLanguageSetting() {
	std::ifstream in("HDD:\\X360RemapStudio\\X360Remap_lang.txt", std::ios::binary);
	if (!in.is_open()) {
		FileLog("X360Remap_lang.txt absent - notifications du plugin en FR (par defaut)");
		g_pluginLang = LANG_FR;
		return;
	}
	std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	in.close();

	g_pluginLang = LANG_FR;
	for (size_t i = 0; i < content.size(); i++) {
		if (content[i] == '1') { g_pluginLang = LANG_EN; break; }
	}
	FileLog("X360Remap_lang.txt lu - notifications du plugin en %hs",
		g_pluginLang == LANG_EN ? "EN" : "FR");
}

const wchar_t* WTr(NotifyStrId id) {
	if (id < 0 || id >= STR_NOTIFY_COUNT)
		return L"";
	const wchar_t* s = g_notifyStrings[g_pluginLang][id];
	if (!s)
		s = g_notifyStrings[LANG_FR][id];
	return s;
}
