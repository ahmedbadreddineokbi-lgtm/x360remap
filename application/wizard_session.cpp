#include "wizard_session.h"
#include "config_writer.h"
#include "i18n.h"

WizardSession::WizardSession(uint16_t vid, uint16_t pid, std::vector<AssistantStep> steps,
                              uint32_t holdTicksToSkip)
    : vid_(vid), pid_(pid), assistant_(std::move(steps), holdTicksToSkip) {}

void WizardSession::Tick(int32_t inputCode, std::string* promptOut, std::string* messageOut) {
    if (!assistant_.IsActive())
        return;

    assistant_.Tick(inputCode, messageOut);
    if (promptOut)
        *promptOut = assistant_.CurrentPrompt();
}

bool WizardSession::SaveResult(const char* configPathOverride) {
    if (!assistant_.IsComplete())
        return false;

    // Iterateur + variable nommee plutot que for-range et push_back({...})
    // (initialisation par accolades sur un argument de fonction, C++11/
    // VS2013) - le toolset XDK Xbox 360 (VS2010) ne supporte ni l'un ni
    // l'autre, voir mapping_assistant.cpp pour le contexte complet.
    std::vector<HidKeyMapEntry> entries;
    entries.reserve(assistant_.Results().size());
    const std::vector<AssistantResult>& results = assistant_.Results();
    for (std::vector<AssistantResult>::const_iterator it = results.begin(); it != results.end(); ++it) {
        HidKeyMapEntry entry;
        entry.code = static_cast<uint8_t>(it->code);
        entry.action = it->action;
        entries.push_back(entry);
    }

    if (configPathOverride)
        return ApplyKeyboardMapping(vid_, pid_, entries, configPathOverride);
    return ApplyKeyboardMapping(vid_, pid_, entries);
}

// Petit helper local pour construire chaque AssistantStep par init-liste
// nommee (valide C++03) plutot qu'un temporaire brace-init passe directement
// - voir la note plus haut sur le toolset XDK Xbox 360 (VS2010).
static AssistantStep MakeStep(const char* label, uint8_t action) {
    AssistantStep step;
    step.label = label;
    step.action = action;
    return step;
}

std::vector<AssistantStep> DefaultKeyboardWizardSteps() {
    // vector::push_back un par un plutot qu'un retour par
    // std::initializer_list (constructeur vector(initializer_list<T>),
    // C++11/VS2013) - meme raison que les autres corrections de ce fichier.
    std::vector<AssistantStep> steps;
    steps.reserve(25);
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_A), KEYACT_A));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_B), KEYACT_B));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_X), KEYACT_X));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_Y), KEYACT_Y));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_LB), KEYACT_LB));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_RB), KEYACT_RB));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_LT), KEYACT_LT));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_RT), KEYACT_RT));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_L3), KEYACT_L3));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_R3), KEYACT_R3));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_START), KEYACT_START));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_BACK), KEYACT_BACK));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_GUIDE), KEYACT_GUIDE));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_DPAD_UP), KEYACT_DPAD_UP));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_DPAD_DOWN), KEYACT_DPAD_DOWN));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_DPAD_LEFT), KEYACT_DPAD_LEFT));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_DPAD_RIGHT), KEYACT_DPAD_RIGHT));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_LSTICK_UP), KEYACT_LSTICK_UP));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_LSTICK_DOWN), KEYACT_LSTICK_DOWN));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_LSTICK_LEFT), KEYACT_LSTICK_LEFT));
    steps.push_back(MakeStep(Tr(STR_WIZSTEP_LSTICK_RIGHT), KEYACT_LSTICK_RIGHT));
    // Stick droit VOLONTAIREMENT absent du wizard clavier (demande
    // utilisateur du 2026-08-02) : le stick droit (camera/visee) est deja
    // couvert par la souris, pas de raison d'occuper 4 touches clavier
    // dessus aussi. Reste assignable a la main dans X360Remap.json si
    // vraiment necessaire, juste plus propose par defaut ici.
    return steps;
}
