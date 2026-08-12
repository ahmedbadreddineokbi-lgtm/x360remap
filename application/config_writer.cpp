#include "config_writer.h"
#include "../hiddriver/known_titles.h"
#include "../hiddriver/known_devices.h"
#include "../hiddriver/mouse_calibration.h" // IsLayoutUsable, pour refuser une disposition bancale
#include <memory>
#include <fstream>
#include <iterator>
#include <string.h>

// Sous-dossier dedie de l'appli plutot que la racine du disque dur - evite
// de polluer HDD:\ avec des fichiers qui n'ont de sens que pour cet outil.
// Le plugin (hiddriver/main.cpp) utilise le meme chemin en dur de son cote -
// voir le commentaire de MountHdd() dans application/main.cpp pour le rappel
// que ce n'est PAS un dossier prive (les deux .xex tournent dans des
// process/namespaces differents), juste une convention de rangement partagee.
static const char* kDefaultConfigPath = "HDD:\\X360RemapStudio\\X360Remap.json";
static const char* kDefaultBackupPath = "HDD:\\X360RemapStudio\\X360Remap_default.json";
// Meme chemin en dur que hiddriver/main.cpp (voir sa section
// "Jalon 7 (suite)" pres de la detection de changement de jeu) - simple
// convention de rangement partagee, pas de dossier prive (voir le
// commentaire de kDefaultConfigPath au-dessus).
static const char* kDefaultKnownTitlesPath = "HDD:\\X360RemapStudio\\known_titles.txt";
// Meme convention - ecrit uniquement par hiddriver.xex (Jalon 8, suite), lu
// en lecture seule ici (voir le commentaire de ListKnownDevices dans
// config_writer.h).
static const char* kDefaultKnownDevicesPath = "HDD:\\X360RemapStudio\\known_devices.txt";

bool ApplyKeyboardMapping(uint16_t vid, uint16_t pid, const std::vector<HidKeyMapEntry>& keys) {
    return ApplyKeyboardMapping(vid, pid, keys, kDefaultConfigPath);
}

bool ApplyKeyboardMapping(uint16_t vid, uint16_t pid, const std::vector<HidKeyMapEntry>& keys,
                           const char* configPath) {
    // Load whatever is on disk right now. A missing/unreadable file just
    // means we start from an empty mapping set - not an error worth failing
    // on, since the whole point of this call is to create/update an entry.
    LoadMappingsFromFile(configPath);

    // Iterateur plutot que for-range (C++11/VS2012, non supporte par le
    // toolset XDK Xbox 360 - voir mapping_assistant.cpp pour l'explication
    // complete, meme correction appliquee ici).
    HidDeviceMapping* existing = nullptr;
    for (std::vector<HidDeviceMapping>::iterator it = g_dynamicMappings.begin(); it != g_dynamicMappings.end(); ++it) {
        if (it->vendorId == vid && it->productId == pid) {
            existing = &(*it);
            break;
        }
    }

    std::unique_ptr<DynamicMappingData> data(new DynamicMappingData());
    data->keyEntries = keys;

    if (existing) {
        existing->keyMap = data->keyEntries.data();
        existing->keyMapCount = static_cast<uint8_t>(data->keyEntries.size());
    } else {
        HidDeviceMapping fresh = {};
        fresh.vendorId = vid;
        fresh.productId = pid;
        fresh.keyMap = data->keyEntries.data();
        fresh.keyMapCount = static_cast<uint8_t>(data->keyEntries.size());
        g_dynamicMappings.push_back(fresh);
    }

    // g_dynamicData owns the backing storage every HidDeviceMapping::keyMap
    // pointer in g_dynamicMappings points into (see mapping.h) - must stay
    // alive at least until SaveMappingsToFile has read out of it below.
    g_dynamicData.push_back(std::move(data));

    return SaveMappingsToFile(configPath);
}

// Copie brute octet-par-octet - pas besoin de reparser le JSON, on ne fait
// que dupliquer le fichier tel quel. std::ifstream/ofstream en binaire pour
// ne rien perdre/transformer (pas de conversion de fin de ligne).
static bool CopyFileRaw(const char* fromPath, const char* toPath) {
    std::ifstream in(fromPath, std::ios::binary);
    if (!in.is_open()) {
        return false;
    }

    std::ofstream out(toPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }

    out << in.rdbuf();
    return in.good() || in.eof();
}

bool SaveConfigAsDefault() {
    return SaveConfigAsDefault(kDefaultConfigPath, kDefaultBackupPath);
}

bool SaveConfigAsDefault(const char* configPath, const char* backupPath) {
    // "Sauvegarder les reglages actuels comme reglages par defaut", comme un
    // profil BIOS - copie l'etat courant de X360Remap.json vers un fichier a
    // part, sans y toucher tant qu'on ne demande pas explicitement de
    // restaurer.
    return CopyFileRaw(configPath, backupPath);
}

bool RestoreConfigDefault() {
    return RestoreConfigDefault(kDefaultConfigPath, kDefaultBackupPath);
}

bool RestoreConfigDefault(const char* configPath, const char* backupPath) {
    // Echoue proprement (false) si aucun defaut n'a jamais ete sauvegarde -
    // ne doit surtout pas creer un X360Remap.json vide/casse dans ce cas.
    return CopyFileRaw(backupPath, configPath);
}

bool HasConfigDefault() {
    return HasConfigDefault(kDefaultBackupPath);
}

bool HasConfigDefault(const char* backupPath) {
    std::ifstream in(backupPath, std::ios::binary);
    return in.is_open();
}

bool ApplyMouseButtonMapping(uint16_t vid, uint16_t pid, const std::vector<MouseButtonBinding>& bindings) {
    return ApplyMouseButtonMapping(vid, pid, bindings, kDefaultConfigPath);
}

bool ApplyMouseButtonMapping(uint16_t vid, uint16_t pid, const std::vector<MouseButtonBinding>& bindings,
                              const char* configPath) {
    LoadMappingsFromFile(configPath);

    HidDeviceMapping* existing = nullptr;
    for (std::vector<HidDeviceMapping>::iterator it = g_dynamicMappings.begin(); it != g_dynamicMappings.end(); ++it) {
        if (it->vendorId == vid && it->productId == pid) {
            existing = &(*it);
            break;
        }
    }

    std::unique_ptr<DynamicMappingData> data(new DynamicMappingData());
    data->buttonEntries.reserve(bindings.size());
    for (std::vector<MouseButtonBinding>::const_iterator it = bindings.begin(); it != bindings.end(); ++it) {
        uint8_t ButtonsReport::* field = GetButtonFieldPtr(it->fieldName);
        if (!field)
            continue; // nom invalide - voir le commentaire de ApplyMouseButtonMapping dans config_writer.h
        HidButtonMapEntry entry;
        entry.idx = it->idx;
        entry.field = field;
        data->buttonEntries.push_back(entry);
    }

    if (existing) {
        existing->buttonMap = data->buttonEntries.data();
        existing->buttonMapCount = static_cast<uint8_t>(data->buttonEntries.size());
    } else {
        HidDeviceMapping fresh = {};
        fresh.vendorId = vid;
        fresh.productId = pid;
        fresh.buttonMap = data->buttonEntries.data();
        fresh.buttonMapCount = static_cast<uint8_t>(data->buttonEntries.size());
        g_dynamicMappings.push_back(fresh);
    }

    g_dynamicData.push_back(std::move(data));

    return SaveMappingsToFile(configPath);
}

int GetMouseButtonMapCountOnDisk(uint16_t vid, uint16_t pid) {
    return GetMouseButtonMapCountOnDisk(vid, pid, kDefaultConfigPath);
}

int GetMouseButtonMapCountOnDisk(uint16_t vid, uint16_t pid, const char* configPath) {
    LoadMappingsFromFile(configPath);
    HidDeviceMapping* m = FindMapping(vid, pid);
    if (!m)
        return -1;
    return static_cast<int>(m->buttonMapCount);
}

bool ApplyMouseReportLayout(uint16_t vid, uint16_t pid, const HidReportLayout& layout) {
    return ApplyMouseReportLayout(vid, pid, layout, kDefaultConfigPath);
}

bool ApplyMouseReportLayout(uint16_t vid, uint16_t pid, const HidReportLayout& layout,
                             const char* configPath) {
    // Refus net plutot qu'ecriture d'offsets bancals : le plugin lit ces
    // valeurs dans le contexte USB, une disposition incoherente le ferait lire
    // hors du paquet a chaque rapport.
    if (!IsLayoutUsable(layout))
        return false;

    LoadMappingsFromFile(configPath);

    HidDeviceMapping* existing = FindMapping(vid, pid);
    if (existing) {
        existing->reportLayout = layout;
        existing->reportLayout.valid = true;
    } else {
        HidDeviceMapping fresh = {};
        fresh.vendorId = vid;
        fresh.productId = pid;
        fresh.reportLayout = layout;
        fresh.reportLayout.valid = true;
        g_dynamicMappings.push_back(fresh);
    }

    return SaveMappingsToFile(configPath);
}

bool ClearMouseReportLayout(uint16_t vid, uint16_t pid) {
    return ClearMouseReportLayout(vid, pid, kDefaultConfigPath);
}

bool ClearMouseReportLayout(uint16_t vid, uint16_t pid, const char* configPath) {
    LoadMappingsFromFile(configPath);
    HidDeviceMapping* existing = FindMapping(vid, pid);
    if (!existing)
        return false;
    memset(&existing->reportLayout, 0, sizeof(existing->reportLayout));
    existing->reportLayout.valid = false;
    return SaveMappingsToFile(configPath);
}

bool ApplyMouseSettings(uint16_t vid, uint16_t pid, int32_t sensitivity, bool invertY, int32_t deadzone) {
    return ApplyMouseSettings(vid, pid, sensitivity, invertY, deadzone, kDefaultConfigPath);
}

bool ApplyMouseSettings(uint16_t vid, uint16_t pid, int32_t sensitivity, bool invertY, int32_t deadzone,
                         const char* configPath) {
    LoadMappingsFromFile(configPath);

    HidDeviceMapping* existing = nullptr;
    for (std::vector<HidDeviceMapping>::iterator it = g_dynamicMappings.begin(); it != g_dynamicMappings.end(); ++it) {
        if (it->vendorId == vid && it->productId == pid) {
            existing = &(*it);
            break;
        }
    }

    if (existing) {
        if (sensitivity > 0) existing->mouseSensitivity = sensitivity;
        existing->invertMouseY = invertY;
        if (deadzone >= 0) existing->deadzone = deadzone;
    } else {
        HidDeviceMapping fresh = {};
        fresh.vendorId = vid;
        fresh.productId = pid;
        fresh.mouseSensitivity = sensitivity > 0 ? sensitivity : 0;
        fresh.invertMouseY = invertY;
        fresh.deadzone = deadzone >= 0 ? deadzone : 0;
        g_dynamicMappings.push_back(fresh);
    }

    return SaveMappingsToFile(configPath);
}

std::vector<MouseProfileInfo> ListMouseProfiles(uint16_t vid, uint16_t pid) {
    return ListMouseProfiles(vid, pid, kDefaultConfigPath);
}

std::vector<MouseProfileInfo> ListMouseProfiles(uint16_t vid, uint16_t pid, const char* configPath) {
    std::vector<MouseProfileInfo> result;

    LoadMappingsFromFile(configPath);
    HidDeviceMapping* m = FindMapping(vid, pid);
    if (!m)
        return result;

    result.reserve(m->profileCount);
    for (uint8_t i = 0; i < m->profileCount; i++) {
        const HidDeviceProfile& p = m->profiles[i];
        MouseProfileInfo info;
        info.titleId = p.titleId;
        strncpy(info.gameName, p.gameName, sizeof(info.gameName) - 1);
        info.gameName[sizeof(info.gameName) - 1] = '\0';
        info.mouseSensitivity = p.mouseSensitivity;
        info.invertMouseY = p.invertMouseY;
        info.deadzone = p.deadzone;
        info.curveType = p.curveType;
        info.curveExponent = p.curveExponent;
        result.push_back(info);
    }
    return result;
}

bool ApplyMouseProfile(uint16_t vid, uint16_t pid, uint32_t titleId,
                        int32_t sensitivity, bool invertY, int32_t deadzone,
                        uint8_t curveType, float curveExponent,
                        const char* gameName) {
    return ApplyMouseProfile(vid, pid, titleId, sensitivity, invertY, deadzone, curveType, curveExponent, gameName, kDefaultConfigPath);
}

bool ApplyMouseProfile(uint16_t vid, uint16_t pid, uint32_t titleId,
                        int32_t sensitivity, bool invertY, int32_t deadzone,
                        uint8_t curveType, float curveExponent,
                        const char* gameName,
                        const char* configPath) {
    // Delegue a SetMouseProfile (hiddriver/mapping.cpp) - meme logique
    // desormais partagee avec hiddriver.xex, qui l'appelle directement pour
    // le raccourci "sauvegarder le profil depuis le jeu" (Jalon 7, suite) -
    // une seule source de verite pour "qu'est-ce qu'enregistrer un profil
    // veut dire", pas de logique dupliquee a faire diverger entre les deux
    // .xex. Cette fonction reste le seul point qui fait le I/O disque
    // (Load/Save), SetMouseProfile ne manipule que la memoire.
    LoadMappingsFromFile(configPath);
    if (!SetMouseProfile(vid, pid, titleId, sensitivity, invertY, deadzone, curveType, curveExponent, gameName))
        return false;
    return SaveMappingsToFile(configPath);
}

bool RemoveMouseProfile(uint16_t vid, uint16_t pid, uint32_t titleId) {
    return RemoveMouseProfile(vid, pid, titleId, kDefaultConfigPath);
}

std::vector<uint32_t> ListKnownTitleIds() {
    return ListKnownTitleIds(kDefaultKnownTitlesPath);
}

std::vector<uint32_t> ListKnownTitleIds(const char* path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
        return std::vector<uint32_t>(); // pas encore de fichier - liste vide, pas une erreur

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return ParseKnownTitleIds(content);
}

std::vector<KnownDeviceInfo> ListKnownDevices() {
    return ListKnownDevices(kDefaultKnownDevicesPath);
}

std::vector<KnownDeviceInfo> ListKnownDevices(const char* knownDevicesPath) {
    std::vector<KnownDeviceInfo> result;

    std::ifstream in(knownDevicesPath, std::ios::binary);
    if (!in.is_open())
        return result; // pas encore de fichier - aucun device jamais vu par le plugin, liste vide (pas une erreur)

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::vector<KnownDeviceEntry> entries = ParseKnownDevices(content);

    result.reserve(entries.size());
    for (std::vector<KnownDeviceEntry>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
        KnownDeviceInfo info;
        info.vid = it->vid;
        info.pid = it->pid;
        info.isMouse = it->isMouse;
        size_t n = 0;
        for (; n < sizeof(info.name) - 1 && it->name[n] != '\0'; n++)
            info.name[n] = it->name[n];
        info.name[n] = '\0';
        result.push_back(info);
    }
    return result;
}

bool RemoveMouseProfile(uint16_t vid, uint16_t pid, uint32_t titleId, const char* configPath) {
    LoadMappingsFromFile(configPath);

    HidDeviceMapping* existing = nullptr;
    for (std::vector<HidDeviceMapping>::iterator it = g_dynamicMappings.begin(); it != g_dynamicMappings.end(); ++it) {
        if (it->vendorId == vid && it->productId == pid) {
            existing = &(*it);
            break;
        }
    }
    if (!existing)
        return false; // device inconnu - rien a supprimer

    std::unique_ptr<DynamicMappingData> data(new DynamicMappingData());
    for (uint8_t i = 0; i < existing->profileCount; i++) {
        if (existing->profiles[i].titleId != titleId) {
            data->profileEntries.push_back(existing->profiles[i]);
        }
    }

    // BUG REEL attrape par TestRemoveMouseProfile (titleId inexistant) :
    // sans cette verification, la boucle ci-dessus recopie simplement TOUS
    // les profils inchanges quand aucun ne correspond a titleId, et la
    // fonction renvoyait quand meme true (SaveMappingsToFile reussit
    // toujours sur un contenu inchange) au lieu de signaler qu'il n'y avait
    // rien a supprimer.
    if (data->profileEntries.size() == existing->profileCount)
        return false; // aucun profil de ce titleId - rien retire, pas de resauvegarde inutile

    existing->profiles = data->profileEntries.empty() ? nullptr : data->profileEntries.data();
    existing->profileCount = static_cast<uint8_t>(data->profileEntries.size());

    g_dynamicData.push_back(std::move(data));

    return SaveMappingsToFile(configPath);
}
