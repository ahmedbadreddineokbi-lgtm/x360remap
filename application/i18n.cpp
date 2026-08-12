#include "i18n.h"
#include <fstream>

// Meme convention que force_boot_protocol.txt (voir DrawCalibrationTab,
// application/main.cpp) : un seul fichier, un seul octet, lu/ecrit via
// std::ifstream/ofstream vers HDD:\X360RemapStudio\. "0" = LANG_FR,
// "1" = LANG_EN. Ce chemin suppose HDD: deja monte (XexUtils::Fs::MountHdd(),
// voir main()) - LoadLanguagePref() est appelee apres ce montage, comme tous
// les autres acces fichier de l'appli.
static const char* kLangPrefPath = "HDD:\\X360RemapStudio\\X360Remap_lang.txt";

static Lang g_currentLang = LANG_FR;

// [LANG_FR][...] copie verbatim des chaines existantes de main.cpp/
// wizard_session.cpp/calibration_session.cpp - AUCUNE reformulation, y
// compris l'absence d'accents (voir le commentaire en tete de i18n.h).
// [LANG_EN][...] traductions naturelles et concises, meme ton technique/
// terse que l'original FR (appli de configuration, pas une brochure).
static const char* g_strings[LANG_COUNT][STR_COUNT] = {
    // ================= LANG_FR =================
    {
        // --- DrawWizardTab ---
        /* STR_WIZARD_BTN_START            */ "Demarrer la configuration clavier",
        /* STR_WIZARD_NO_KEYBOARD          */ "Aucun clavier detecte (plugin.xex charge ? clavier branche ?)",
        /* STR_WIZARD_STEP_PROGRESS_FMT    */ "Etape %d / %d (assignees : %d)",
        /* STR_WIZARD_PRESS_KEY_FOR_FMT    */ "Appuie sur une touche pour : %s",
        /* STR_WIZARD_HOLD_TO_SKIP         */ "Maintiens 3 secondes pour passer une etape - BACK pour annuler",
        /* STR_WIZARD_SAVED_FMT            */ "Configuration enregistree (%d touches).",
        /* STR_WIZARD_SAVE_FAILED          */ "ECHEC de la sauvegarde - rien n'a ete ecrit sur le disque.",
        /* STR_WIZARD_BTN_OK               */ "OK",
        /* STR_WIZARD_CANCELLED            */ "Annule - rien n'a ete enregistre.",

        // --- DrawCalibrationTab ---
        /* STR_CALIB_INTRO                 */ "Cet assistant apprend la disposition des donnees envoyees par ta souris. Utile si le curseur se comporte mal, si les clics ne repondent pas, ou si la molette ne fait rien.",
        /* STR_CALIB_NAV_HINT              */ "Navigation a la MANETTE - la souris sert uniquement a faire les gestes demandes.",
        /* STR_CALIB_TARGET_MOUSE_FMT      */ "Souris ciblee : %04X:%04X",
        /* STR_CALIB_TARGET_MOUSE_HINT     */ "(choisie dans le selecteur en haut de l'ecran)",
        /* STR_CALIB_BTN_START             */ "Demarrer la calibration",
        /* STR_CALIB_FALLBACK_ACTIVE       */ "MODE DE SECOURS ACTIF - lecture figee, molette indisponible.",
        /* STR_CALIB_FALLBACK_ACTIVE_HINT  */ "C'est probablement pourquoi ta molette ne fonctionne pas. Reviens a la lecture automatique ci-dessous, puis redemarre la console.",
        /* STR_CALIB_AUTO_ACTIVE           */ "Lecture automatique active (recommande) - molette geree.",
        /* STR_CALIB_FALLBACK_EXPLAIN      */ "Mode de secours : a n'utiliser QUE si ta souris se comporte mal en lecture automatique. Il desactive la lecture du descripteur et supprime la molette.",
        /* STR_CALIB_BTN_FORCE_FALLBACK    */ "Forcer l'ancienne lecture (mode de secours)",
        /* STR_CALIB_BTN_RESTORE_AUTO      */ "Revenir a la lecture automatique (recommande)",
        /* STR_CALIB_CLEAR_HINT            */ "Si une calibration existante rend les choses pires, tu peux l'effacer :",
        /* STR_CALIB_BTN_CLEAR             */ "Effacer la calibration de cette souris",
        /* STR_CALIB_NO_DATA               */ "Aucune donnee souris recue (plugin charge ? souris branchee ?)",
        /* STR_CALIB_SUCCESS               */ "Calibration reussie.",
        /* STR_CALIB_BTN_SAVE              */ "Enregistrer cette calibration",
        /* STR_CALIB_SAVED_APPLIED         */ "Enregistree - le plugin l'applique des le prochain rapport.",
        /* STR_CALIB_SAVE_FAILED           */ "ECHEC - rien n'a ete ecrit sur le disque.",
        /* STR_CALIB_BTN_CLOSE             */ "Fermer",
        /* STR_CALIB_GESTURE_DONE          */ "Geste enregistre - valide pour continuer.",
        /* STR_CALIB_GESTURE_CONTINUE      */ "Continue le geste jusqu'a ce que la jauge soit pleine.",
        /* STR_CALIB_BTN_VALIDATE_STEP     */ "Valider cette etape",
        /* STR_CALIB_VALIDATE_DISABLED_HINT*/ "(Valider sera disponible une fois la jauge pleine)",
        /* STR_CALIB_BTN_NO_WHEEL          */ "Pas de molette sur cette souris",
        /* STR_CALIB_BTN_CANCEL            */ "Annuler",

        // --- DrawMouseSettingsTab ---
        /* STR_SETTINGS_SENSITIVITY_LABEL    */ "Sensibilite",
        /* STR_SETTINGS_DEADZONE_LABEL       */ "Zone morte (deadzone)",
        /* STR_SETTINGS_INVERT_Y_LABEL       */ "Inverser l'axe vertical",
        /* STR_SETTINGS_BTN_SAVE             */ "Enregistrer",
        /* STR_SETTINGS_MOUSE_BUTTONS_HEADER */ "Boutons souris",
        /* STR_SETTINGS_CLICK_LEFT_LABEL     */ "Clic gauche",
        /* STR_SETTINGS_CLICK_RIGHT_LABEL    */ "Clic droit",
        /* STR_SETTINGS_CLICK_MIDDLE_LABEL   */ "Clic milieu",
        /* STR_SETTINGS_CLICK_DEFAULT_HINT   */ "\"Par defaut\" = gauche->RT, droit->LT, milieu->clic stick droit",
        /* STR_SETTINGS_WHEEL_FORWARD_LABEL  */ "Molette avant",
        /* STR_SETTINGS_WHEEL_BACKWARD_LABEL */ "Molette arriere",
        /* STR_SETTINGS_WHEEL_DEFAULT_HINT   */ "\"Par defaut\" = navigation D-Pad Haut/Bas (menus Aurora) - perdue si assignee ici",
        /* STR_SETTINGS_BTN_SAVE_BUTTONS     */ "Enregistrer boutons souris",
        /* STR_SETTINGS_DEFAULTS_HEADER      */ "Reglages par defaut (clavier + souris)",
        /* STR_SETTINGS_BTN_SAVE_AS_DEFAULT  */ "Sauvegarder comme reglages par defaut",
        /* STR_SETTINGS_NO_DEFAULT_SAVED     */ "(aucun reglage par defaut sauvegarde pour le moment)",
        /* STR_SETTINGS_BTN_RESTORE_DEFAULT  */ "Restaurer les reglages par defaut",
        /* STR_SETTINGS_TARGET_DEFAULT       */ "Par defaut",
        /* STR_SETTINGS_TARGET_A             */ "A",
        /* STR_SETTINGS_TARGET_B             */ "B",
        /* STR_SETTINGS_TARGET_X             */ "X",
        /* STR_SETTINGS_TARGET_Y             */ "Y",
        /* STR_SETTINGS_TARGET_LB            */ "LB",
        /* STR_SETTINGS_TARGET_RB            */ "RB",
        /* STR_SETTINGS_TARGET_LT            */ "LT",
        /* STR_SETTINGS_TARGET_RT            */ "RT",
        /* STR_SETTINGS_TARGET_L3            */ "L3 (clic stick gauche)",
        /* STR_SETTINGS_TARGET_R3            */ "R3 (clic stick droit)",
        /* STR_SETTINGS_TARGET_START         */ "Start",
        /* STR_SETTINGS_TARGET_BACK          */ "Back",
        /* STR_SETTINGS_TARGET_DPAD_UP       */ "D-Pad Haut",
        /* STR_SETTINGS_TARGET_DPAD_DOWN     */ "D-Pad Bas",
        /* STR_SETTINGS_TARGET_DPAD_LEFT     */ "D-Pad Gauche",
        /* STR_SETTINGS_TARGET_DPAD_RIGHT    */ "D-Pad Droite",

        // --- DrawProfilesTab ---
        /* STR_PROFILES_INTRO                     */ "Un profil surcharge la sensibilite/deadzone/inversion/courbe pour UN jeu precis, au-dessus des reglages globaux de l'onglet \"Reglages souris\". Renseigne le Title ID du jeu (8 chiffres hexadecimaux).",
        /* STR_PROFILES_F9_HINT                   */ "Astuce : maintiens F9 quelques secondes EN JEU pour figer les reglages actuels comme profil de ce jeu, sans repasser par cet ecran.",
        /* STR_PROFILES_COMBO_LABEL               */ "Profil",
        /* STR_PROFILES_COMBO_NEW                 */ "+ Nouveau profil",
        /* STR_PROFILES_KNOWN_TITLES_COMBO_LABEL  */ "Jeux detectes par le plugin",
        /* STR_PROFILES_KNOWN_TITLES_HINT         */ "Jeux deja lances au moins une fois avec le plugin actif",
        /* STR_PROFILES_NO_KNOWN_TITLES           */ "Aucun jeu detecte pour le moment - lance-en un une fois avec le plugin actif, ou tape le Title ID a la main ci-dessous",
        /* STR_PROFILES_RADIO_TYPE_TITLEID        */ "Taper le Title ID",
        /* STR_PROFILES_RADIO_TYPE_GAMENAME       */ "Taper le nom du jeu",
        /* STR_PROFILES_INPUT_TITLEID             */ "Title ID (hex)",
        /* STR_PROFILES_HINT_TITLEID_CHARS        */ "0-9, A-F - Retour arriere pour effacer - Ex: 4D5308BC",
        /* STR_PROFILES_INPUT_GAME_NAME           */ "Nom du jeu (optionnel)",
        /* STR_PROFILES_HINT_GAME_NAME_CHARS      */ "a-z, 0-9, espace - juste pour reconnaitre ce profil dans la liste",
        /* STR_PROFILES_TITLEID_DISPLAY_FMT       */ "Title ID : %s",
        /* STR_PROFILES_SENSITIVITY_LABEL         */ "Sensibilite (profil)",
        /* STR_PROFILES_DEADZONE_LABEL            */ "Zone morte (profil)",
        /* STR_PROFILES_INVERT_Y_LABEL            */ "Inverser l'axe vertical (profil)",
        /* STR_PROFILES_CURVE_LINEAR              */ "Lineaire",
        /* STR_PROFILES_CURVE_EXPONENTIAL         */ "Exponentielle",
        /* STR_PROFILES_CURVE_COMBO_LABEL         */ "Courbe de sensibilite",
        /* STR_PROFILES_EXPONENT_LABEL            */ "Exposant",
        /* STR_PROFILES_EXPONENT_HINT             */ "1.0 = comme lineaire ; plus haut = plus de precision pres du centre",
        /* STR_PROFILES_BTN_SAVE                  */ "Enregistrer le profil",
        /* STR_PROFILES_BTN_DELETE                */ "Supprimer ce profil",
        /* STR_PROFILES_NONE_SAVED                */ "(aucun profil enregistre pour le moment)",

        // --- DrawDeviceSelector ---
        /* STR_DEVICE_HEADER                 */ "Peripheriques actifs (Clavier / Souris)",
        /* STR_DEVICE_KEYBOARD_LABEL         */ "Clavier",
        /* STR_DEVICE_MOUSE_LABEL            */ "Souris",
        /* STR_DEVICE_BTN_REFRESH            */ "Actualiser##devices",
        /* STR_DEVICE_ONE_DEFAULT_HINT       */ "Un seul \"Par defaut\" dans les deux listes ? Debranche/rebranche le peripherique une fois avec hiddriver.xex actif - il apparaitra ici (Actualiser pour rafraichir sans relancer l'appli).",
        /* STR_DEVICE_NAME_UNAVAILABLE_FMT   */ "%04X:%04X (nom indisponible)",
        /* STR_DEVICE_DEFAULT_LABEL_FMT      */ "Par defaut (%04X:%04X)",

        // --- main() : titre fenetre / barre d'onglets ---
        /* STR_APP_WINDOW_TITLE  */ "Configuration clavier / souris",
        /* STR_APP_TAB_WIZARD    */ "Wizard clavier",
        /* STR_APP_TAB_SETTINGS  */ "Reglages souris",
        /* STR_APP_TAB_PROFILES  */ "Profils",
        /* STR_APP_TAB_CALIBRATION */ "Calibration souris",
        /* STR_APP_TAB_HOWTO       */ "Comment ca marche",
        /* STR_APP_TAB_SUPPORT     */ "Soutenir le projet",

        // --- wizard_session.cpp : DefaultKeyboardWizardSteps ---
        /* STR_WIZSTEP_A            */ "A",
        /* STR_WIZSTEP_B            */ "B",
        /* STR_WIZSTEP_X            */ "X",
        /* STR_WIZSTEP_Y            */ "Y",
        /* STR_WIZSTEP_LB           */ "LB",
        /* STR_WIZSTEP_RB           */ "RB",
        /* STR_WIZSTEP_LT           */ "LT",
        /* STR_WIZSTEP_RT           */ "RT",
        /* STR_WIZSTEP_L3           */ "L3 (clic stick gauche)",
        /* STR_WIZSTEP_R3           */ "R3 (clic stick droit)",
        /* STR_WIZSTEP_START        */ "Start",
        /* STR_WIZSTEP_BACK         */ "Back",
        /* STR_WIZSTEP_GUIDE        */ "Guide (bouton Xbox)",
        /* STR_WIZSTEP_DPAD_UP      */ "D-Pad Haut",
        /* STR_WIZSTEP_DPAD_DOWN    */ "D-Pad Bas",
        /* STR_WIZSTEP_DPAD_LEFT    */ "D-Pad Gauche",
        /* STR_WIZSTEP_DPAD_RIGHT   */ "D-Pad Droite",
        /* STR_WIZSTEP_LSTICK_UP    */ "Stick gauche - Haut",
        /* STR_WIZSTEP_LSTICK_DOWN  */ "Stick gauche - Bas",
        /* STR_WIZSTEP_LSTICK_LEFT  */ "Stick gauche - Gauche",
        /* STR_WIZSTEP_LSTICK_RIGHT */ "Stick gauche - Droite",

        // --- calibration_session.cpp : Prompt() ---
        /* STR_CALSTEP_REST       */ "Ne touche a rien - laisse la souris immobile.",
        /* STR_CALSTEP_RIGHT      */ "Deplace la souris franchement vers la DROITE, plusieurs fois.",
        /* STR_CALSTEP_LEFT       */ "Maintenant vers la GAUCHE, aussi franchement.",
        /* STR_CALSTEP_DOWN       */ "Deplace la souris vers le BAS (vers toi).",
        /* STR_CALSTEP_UP         */ "Maintenant vers le HAUT.",
        /* STR_CALSTEP_WHEEL_FWD  */ "Tourne la molette vers l'AVANT, quelques crans.",
        /* STR_CALSTEP_WHEEL_BACK */ "Tourne la molette vers l'ARRIERE.",
        /* STR_CALSTEP_CLICK_LEFT */ "Maintiens le CLIC GAUCHE enfonce.",
        /* STR_CALSTEP_DONE       */ "Calibration terminee.",
        /* STR_CALSTEP_FAILED     */ "Calibration echouee - rien n'a ete enregistre.",

        // --- calibration_session.cpp : Summary() ---
        /* STR_CALSUM_FAIL_FMT      */ "Echec (paquet de %d octets). Detecte : X=%hs, Y=%hs, molette=%hs, bouton=%hs. Un axe non detecte vient presque toujours d'un geste trop court : refais-le plus ample et plus longtemps, en ligne droite.",
        /* STR_CALSUM_YES           */ "oui",
        /* STR_CALSUM_NO_CAPS       */ "NON",
        /* STR_CALSUM_NO            */ "non",
        /* STR_CALSUM_WHEEL_NONE    */ "aucune molette detectee",
        /* STR_CALSUM_WHEEL_BYTE_FMT*/ "molette octet %d",
        /* STR_CALSUM_SUCCESS_FMT   */ "Paquet de %d octets : X octet %d (%d o.), Y octet %d (%d o.), boutons octet %d, %hs.",

        // --- DrawHowItWorksTab ---
        /* STR_HOWTO_INTRO             */ "Comment utiliser l'application, en resume :",
        /* STR_HOWTO_STEP_PLUG         */ "1. Branche ton clavier et/ou ta souris sur un port USB de la console.",
        /* STR_HOWTO_STEP_WIZARD       */ "2. Onglet \"Wizard clavier\" : assigne tes touches une par une.",
        /* STR_HOWTO_STEP_SETTINGS     */ "3. Onglet \"Reglages souris\" : sensibilite, zone morte, boutons de la souris.",
        /* STR_HOWTO_STEP_PROFILES     */ "4. Onglet \"Profils\" (optionnel) : des reglages differents pour un jeu precis.",
        /* STR_HOWTO_QUICKBIND_HEADER  */ "Liaison rapide (en jeu, sans revenir a cette appli)",
        /* STR_HOWTO_QUICKBIND_TEXT    */ "Maintiens BACK + START environ 1,5 seconde sur la manette. Appuie ensuite sur le bouton de la manette a remplacer, puis sur la touche clavier (ou le clic souris) qui doit le remplacer. C'est fait - la liaison s'applique immediatement, pour ce jeu.",
        /* STR_HOWTO_F9_HEADER         */ "Sauvegarde rapide (en jeu)",
        /* STR_HOWTO_F9_TEXT           */ "Maintiens F9 quelques secondes en jeu pour enregistrer la sensibilite/zone morte/inversion actuelles comme profil de ce jeu, sans repasser par l'onglet Profils.",

        // --- DrawSupportTab ---
        /* STR_SUPPORT_BADGE           */ "EN COURS DE DEVELOPPEMENT",
        /* STR_SUPPORT_MESSAGE         */ "X360Remap est developpe seul, sur mon temps libre, et reste gratuit et open-source. Si l'application te fait gagner en confort de jeu, un encouragement - meme petit - m'aide enormement a continuer a l'ameliorer.",
        /* STR_SUPPORT_CTA             */ "Chaque contribution compte - merci de faire partie de l'aventure !",
        /* STR_SUPPORT_PAYPAL_LABEL    */ "PayPal",
        /* STR_SUPPORT_KOFI_LABEL      */ "Ko-fi",
        /* STR_SUPPORT_GITHUB_LABEL    */ "GitHub (code source, suivre le projet)",
        /* STR_SUPPORT_YOUTUBE_LABEL   */ "YouTube (demos, tutoriels)",
        /* STR_SUPPORT_LINK_HIDDEN_HINT*/ "(lien temporairement indisponible)",
    },

    // ================= LANG_EN =================
    {
        // --- DrawWizardTab ---
        /* STR_WIZARD_BTN_START            */ "Start keyboard configuration",
        /* STR_WIZARD_NO_KEYBOARD          */ "No keyboard detected (plugin.xex loaded? keyboard plugged in?)",
        /* STR_WIZARD_STEP_PROGRESS_FMT    */ "Step %d / %d (assigned: %d)",
        /* STR_WIZARD_PRESS_KEY_FOR_FMT    */ "Press a key for: %s",
        /* STR_WIZARD_HOLD_TO_SKIP         */ "Hold 3 seconds to skip a step - BACK to cancel",
        /* STR_WIZARD_SAVED_FMT            */ "Configuration saved (%d keys).",
        /* STR_WIZARD_SAVE_FAILED          */ "SAVE FAILED - nothing was written to disk.",
        /* STR_WIZARD_BTN_OK               */ "OK",
        /* STR_WIZARD_CANCELLED            */ "Cancelled - nothing was saved.",

        // --- DrawCalibrationTab ---
        /* STR_CALIB_INTRO                 */ "This wizard learns the layout of the data your mouse sends. Useful if the cursor misbehaves, if clicks don't respond, or if the wheel does nothing.",
        /* STR_CALIB_NAV_HINT              */ "Navigate with the CONTROLLER - the mouse is only used to perform the requested gestures.",
        /* STR_CALIB_TARGET_MOUSE_FMT      */ "Target mouse: %04X:%04X",
        /* STR_CALIB_TARGET_MOUSE_HINT     */ "(chosen in the selector at the top of the screen)",
        /* STR_CALIB_BTN_START             */ "Start calibration",
        /* STR_CALIB_FALLBACK_ACTIVE       */ "FALLBACK MODE ACTIVE - fixed reading, wheel unavailable.",
        /* STR_CALIB_FALLBACK_ACTIVE_HINT  */ "This is probably why your wheel isn't working. Switch back to automatic reading below, then restart the console.",
        /* STR_CALIB_AUTO_ACTIVE           */ "Automatic reading active (recommended) - wheel handled.",
        /* STR_CALIB_FALLBACK_EXPLAIN      */ "Fallback mode: use ONLY if your mouse misbehaves in automatic reading. It disables descriptor reading and removes the wheel.",
        /* STR_CALIB_BTN_FORCE_FALLBACK    */ "Force the old reading (fallback mode)",
        /* STR_CALIB_BTN_RESTORE_AUTO      */ "Switch back to automatic reading (recommended)",
        /* STR_CALIB_CLEAR_HINT            */ "If an existing calibration makes things worse, you can clear it:",
        /* STR_CALIB_BTN_CLEAR             */ "Clear this mouse's calibration",
        /* STR_CALIB_NO_DATA               */ "No mouse data received (plugin loaded? mouse plugged in?)",
        /* STR_CALIB_SUCCESS               */ "Calibration successful.",
        /* STR_CALIB_BTN_SAVE              */ "Save this calibration",
        /* STR_CALIB_SAVED_APPLIED         */ "Saved - the plugin applies it starting with the next report.",
        /* STR_CALIB_SAVE_FAILED           */ "FAILED - nothing was written to disk.",
        /* STR_CALIB_BTN_CLOSE             */ "Close",
        /* STR_CALIB_GESTURE_DONE          */ "Gesture recorded - valid, continue.",
        /* STR_CALIB_GESTURE_CONTINUE      */ "Keep doing the gesture until the gauge is full.",
        /* STR_CALIB_BTN_VALIDATE_STEP     */ "Validate this step",
        /* STR_CALIB_VALIDATE_DISABLED_HINT*/ "(Validate will be available once the gauge is full)",
        /* STR_CALIB_BTN_NO_WHEEL          */ "No wheel on this mouse",
        /* STR_CALIB_BTN_CANCEL            */ "Cancel",

        // --- DrawMouseSettingsTab ---
        /* STR_SETTINGS_SENSITIVITY_LABEL    */ "Sensitivity",
        /* STR_SETTINGS_DEADZONE_LABEL       */ "Dead zone",
        /* STR_SETTINGS_INVERT_Y_LABEL       */ "Invert vertical axis",
        /* STR_SETTINGS_BTN_SAVE             */ "Save",
        /* STR_SETTINGS_MOUSE_BUTTONS_HEADER */ "Mouse buttons",
        /* STR_SETTINGS_CLICK_LEFT_LABEL     */ "Left click",
        /* STR_SETTINGS_CLICK_RIGHT_LABEL    */ "Right click",
        /* STR_SETTINGS_CLICK_MIDDLE_LABEL   */ "Middle click",
        /* STR_SETTINGS_CLICK_DEFAULT_HINT   */ "\"Default\" = left->RT, right->LT, middle->right stick click",
        /* STR_SETTINGS_WHEEL_FORWARD_LABEL  */ "Wheel forward",
        /* STR_SETTINGS_WHEEL_BACKWARD_LABEL */ "Wheel backward",
        /* STR_SETTINGS_WHEEL_DEFAULT_HINT   */ "\"Default\" = D-Pad Up/Down navigation (Aurora menus) - lost if assigned here",
        /* STR_SETTINGS_BTN_SAVE_BUTTONS     */ "Save mouse buttons",
        /* STR_SETTINGS_DEFAULTS_HEADER      */ "Default settings (keyboard + mouse)",
        /* STR_SETTINGS_BTN_SAVE_AS_DEFAULT  */ "Save as default settings",
        /* STR_SETTINGS_NO_DEFAULT_SAVED     */ "(no default settings saved yet)",
        /* STR_SETTINGS_BTN_RESTORE_DEFAULT  */ "Restore default settings",
        /* STR_SETTINGS_TARGET_DEFAULT       */ "Default",
        /* STR_SETTINGS_TARGET_A             */ "A",
        /* STR_SETTINGS_TARGET_B             */ "B",
        /* STR_SETTINGS_TARGET_X             */ "X",
        /* STR_SETTINGS_TARGET_Y             */ "Y",
        /* STR_SETTINGS_TARGET_LB            */ "LB",
        /* STR_SETTINGS_TARGET_RB            */ "RB",
        /* STR_SETTINGS_TARGET_LT            */ "LT",
        /* STR_SETTINGS_TARGET_RT            */ "RT",
        /* STR_SETTINGS_TARGET_L3            */ "L3 (left stick click)",
        /* STR_SETTINGS_TARGET_R3            */ "R3 (right stick click)",
        /* STR_SETTINGS_TARGET_START         */ "Start",
        /* STR_SETTINGS_TARGET_BACK          */ "Back",
        /* STR_SETTINGS_TARGET_DPAD_UP       */ "D-Pad Up",
        /* STR_SETTINGS_TARGET_DPAD_DOWN     */ "D-Pad Down",
        /* STR_SETTINGS_TARGET_DPAD_LEFT     */ "D-Pad Left",
        /* STR_SETTINGS_TARGET_DPAD_RIGHT    */ "D-Pad Right",

        // --- DrawProfilesTab ---
        /* STR_PROFILES_INTRO                     */ "A profile overrides sensitivity/deadzone/invert/curve for ONE specific game, on top of the global settings in the \"Mouse settings\" tab. Enter the game's Title ID (8 hex digits).",
        /* STR_PROFILES_F9_HINT                   */ "Tip: hold F9 for a few seconds IN-GAME to freeze the current settings as this game's profile, without going through this screen.",
        /* STR_PROFILES_COMBO_LABEL               */ "Profile",
        /* STR_PROFILES_COMBO_NEW                 */ "+ New profile",
        /* STR_PROFILES_KNOWN_TITLES_COMBO_LABEL  */ "Games detected by the plugin",
        /* STR_PROFILES_KNOWN_TITLES_HINT         */ "Games already launched at least once with the plugin active",
        /* STR_PROFILES_NO_KNOWN_TITLES           */ "No game detected yet - launch one with the plugin active, or type the Title ID by hand below",
        /* STR_PROFILES_RADIO_TYPE_TITLEID        */ "Type the Title ID",
        /* STR_PROFILES_RADIO_TYPE_GAMENAME       */ "Type the game name",
        /* STR_PROFILES_INPUT_TITLEID             */ "Title ID (hex)",
        /* STR_PROFILES_HINT_TITLEID_CHARS        */ "0-9, A-F - Backspace to clear - Ex: 4D5308BC",
        /* STR_PROFILES_INPUT_GAME_NAME           */ "Game name (optional)",
        /* STR_PROFILES_HINT_GAME_NAME_CHARS      */ "a-z, 0-9, space - just to recognize this profile in the list",
        /* STR_PROFILES_TITLEID_DISPLAY_FMT       */ "Title ID: %s",
        /* STR_PROFILES_SENSITIVITY_LABEL         */ "Sensitivity (profile)",
        /* STR_PROFILES_DEADZONE_LABEL            */ "Dead zone (profile)",
        /* STR_PROFILES_INVERT_Y_LABEL            */ "Invert vertical axis (profile)",
        /* STR_PROFILES_CURVE_LINEAR              */ "Linear",
        /* STR_PROFILES_CURVE_EXPONENTIAL         */ "Exponential",
        /* STR_PROFILES_CURVE_COMBO_LABEL         */ "Sensitivity curve",
        /* STR_PROFILES_EXPONENT_LABEL            */ "Exponent",
        /* STR_PROFILES_EXPONENT_HINT             */ "1.0 = same as linear; higher = more precision near center",
        /* STR_PROFILES_BTN_SAVE                  */ "Save profile",
        /* STR_PROFILES_BTN_DELETE                */ "Delete this profile",
        /* STR_PROFILES_NONE_SAVED                */ "(no profile saved yet)",

        // --- DrawDeviceSelector ---
        /* STR_DEVICE_HEADER                 */ "Active devices (Keyboard / Mouse)",
        /* STR_DEVICE_KEYBOARD_LABEL         */ "Keyboard",
        /* STR_DEVICE_MOUSE_LABEL            */ "Mouse",
        /* STR_DEVICE_BTN_REFRESH            */ "Refresh##devices",
        /* STR_DEVICE_ONE_DEFAULT_HINT       */ "Only one \"Default\" in both lists? Unplug/replug the device once with hiddriver.xex active - it will appear here (Refresh to reload without restarting the app).",
        /* STR_DEVICE_NAME_UNAVAILABLE_FMT   */ "%04X:%04X (name unavailable)",
        /* STR_DEVICE_DEFAULT_LABEL_FMT      */ "Default (%04X:%04X)",

        // --- main() : titre fenetre / barre d'onglets ---
        /* STR_APP_WINDOW_TITLE  */ "Keyboard / mouse configuration",
        /* STR_APP_TAB_WIZARD    */ "Keyboard wizard",
        /* STR_APP_TAB_SETTINGS  */ "Mouse settings",
        /* STR_APP_TAB_PROFILES  */ "Profiles",
        /* STR_APP_TAB_CALIBRATION */ "Mouse calibration",
        /* STR_APP_TAB_HOWTO       */ "How it works",
        /* STR_APP_TAB_SUPPORT     */ "Support the project",

        // --- wizard_session.cpp : DefaultKeyboardWizardSteps ---
        /* STR_WIZSTEP_A            */ "A",
        /* STR_WIZSTEP_B            */ "B",
        /* STR_WIZSTEP_X            */ "X",
        /* STR_WIZSTEP_Y            */ "Y",
        /* STR_WIZSTEP_LB           */ "LB",
        /* STR_WIZSTEP_RB           */ "RB",
        /* STR_WIZSTEP_LT           */ "LT",
        /* STR_WIZSTEP_RT           */ "RT",
        /* STR_WIZSTEP_L3           */ "L3 (left stick click)",
        /* STR_WIZSTEP_R3           */ "R3 (right stick click)",
        /* STR_WIZSTEP_START        */ "Start",
        /* STR_WIZSTEP_BACK         */ "Back",
        /* STR_WIZSTEP_GUIDE        */ "Guide (Xbox button)",
        /* STR_WIZSTEP_DPAD_UP      */ "D-Pad Up",
        /* STR_WIZSTEP_DPAD_DOWN    */ "D-Pad Down",
        /* STR_WIZSTEP_DPAD_LEFT    */ "D-Pad Left",
        /* STR_WIZSTEP_DPAD_RIGHT   */ "D-Pad Right",
        /* STR_WIZSTEP_LSTICK_UP    */ "Left stick - Up",
        /* STR_WIZSTEP_LSTICK_DOWN  */ "Left stick - Down",
        /* STR_WIZSTEP_LSTICK_LEFT  */ "Left stick - Left",
        /* STR_WIZSTEP_LSTICK_RIGHT */ "Left stick - Right",

        // --- calibration_session.cpp : Prompt() ---
        /* STR_CALSTEP_REST       */ "Don't touch anything - leave the mouse still.",
        /* STR_CALSTEP_RIGHT      */ "Move the mouse firmly to the RIGHT, several times.",
        /* STR_CALSTEP_LEFT       */ "Now to the LEFT, just as firmly.",
        /* STR_CALSTEP_DOWN       */ "Move the mouse DOWN (toward you).",
        /* STR_CALSTEP_UP         */ "Now UP.",
        /* STR_CALSTEP_WHEEL_FWD  */ "Turn the wheel FORWARD, a few notches.",
        /* STR_CALSTEP_WHEEL_BACK */ "Turn the wheel BACKWARD.",
        /* STR_CALSTEP_CLICK_LEFT */ "Hold the LEFT CLICK down.",
        /* STR_CALSTEP_DONE       */ "Calibration complete.",
        /* STR_CALSTEP_FAILED     */ "Calibration failed - nothing was saved.",

        // --- calibration_session.cpp : Summary() ---
        /* STR_CALSUM_FAIL_FMT      */ "Failed (packet of %d bytes). Detected: X=%hs, Y=%hs, wheel=%hs, button=%hs. An undetected axis is almost always caused by too short a gesture: redo it wider and longer, in a straight line.",
        /* STR_CALSUM_YES           */ "yes",
        /* STR_CALSUM_NO_CAPS       */ "NO",
        /* STR_CALSUM_NO            */ "no",
        /* STR_CALSUM_WHEEL_NONE    */ "no wheel detected",
        /* STR_CALSUM_WHEEL_BYTE_FMT*/ "wheel byte %d",
        /* STR_CALSUM_SUCCESS_FMT   */ "Packet of %d bytes: X byte %d (%d b.), Y byte %d (%d b.), buttons byte %d, %hs.",

        // --- DrawHowItWorksTab ---
        /* STR_HOWTO_INTRO             */ "How to use the application, in short:",
        /* STR_HOWTO_STEP_PLUG         */ "1. Plug your keyboard and/or mouse into a USB port on the console.",
        /* STR_HOWTO_STEP_WIZARD       */ "2. \"Keyboard wizard\" tab: assign your keys one by one.",
        /* STR_HOWTO_STEP_SETTINGS     */ "3. \"Mouse settings\" tab: sensitivity, dead zone, mouse buttons.",
        /* STR_HOWTO_STEP_PROFILES     */ "4. \"Profiles\" tab (optional): different settings for one specific game.",
        /* STR_HOWTO_QUICKBIND_HEADER  */ "Quick bind (in-game, no need to come back to this app)",
        /* STR_HOWTO_QUICKBIND_TEXT    */ "Hold BACK + START for about 1.5 seconds on the controller. Then press the controller button you want to replace, followed by the keyboard key (or mouse click) that should replace it. Done - the binding applies immediately, for this game.",
        /* STR_HOWTO_F9_HEADER         */ "Quick save (in-game)",
        /* STR_HOWTO_F9_TEXT           */ "Hold F9 for a few seconds in-game to save the current sensitivity/dead zone/invert settings as this game's profile, without going through the Profiles tab.",

        // --- DrawSupportTab ---
        /* STR_SUPPORT_BADGE           */ "WORK IN PROGRESS",
        /* STR_SUPPORT_MESSAGE         */ "X360Remap is built solo, on my free time, and stays free and open-source. If this app makes your gaming more comfortable, a little encouragement goes a long way in helping me keep improving it.",
        /* STR_SUPPORT_CTA             */ "Every bit helps - thanks for being part of the journey!",
        /* STR_SUPPORT_PAYPAL_LABEL    */ "PayPal",
        /* STR_SUPPORT_KOFI_LABEL      */ "Ko-fi",
        /* STR_SUPPORT_GITHUB_LABEL    */ "GitHub (source code, follow the project)",
        /* STR_SUPPORT_YOUTUBE_LABEL   */ "YouTube (demos, tutorials)",
        /* STR_SUPPORT_LINK_HIDDEN_HINT*/ "(link temporarily unavailable)",
    },
};

const char* Tr(StrId id) {
    if (id < 0 || id >= STR_COUNT)
        return "";
    const char* s = g_strings[g_currentLang][id];
    if (s)
        return s;
    // Repli sur LANG_FR si jamais la case courante est vide - ne devrait pas
    // arriver (tableau rempli integralement pour LANG_FR/LANG_EN ci-dessus)
    // mais evite un ImGui::Text(nullptr) si une future langue (LANG_PT_BR)
    // est ajoutee a l'enum sans que sa ligne du tableau soit completee.
    s = g_strings[LANG_FR][id];
    return s ? s : "";
}

Lang GetLanguage() {
    return g_currentLang;
}

void SetLanguage(Lang lang) {
    if (lang < 0 || lang >= LANG_COUNT)
        return;
    g_currentLang = lang;
}

void LoadLanguagePref() {
    std::ifstream f(kLangPrefPath, std::ios::binary);
    if (!f.is_open()) {
        g_currentLang = LANG_FR;
        return;
    }
    char c = 0;
    f.get(c);
    if (c == '1')
        g_currentLang = LANG_EN;
    else
        g_currentLang = LANG_FR; // "0", contenu non reconnu, ou fichier vide
}

void SaveLanguagePref(Lang lang) {
    if (lang < 0 || lang >= LANG_COUNT)
        return;
    std::ofstream f(kLangPrefPath, std::ios::binary | std::ios::trunc);
    if (f.is_open())
        f << (lang == LANG_EN ? "1" : "0");
    SetLanguage(lang);
}
