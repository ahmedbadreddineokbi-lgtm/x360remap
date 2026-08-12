// Harnais de test de la machine a etats de l'assistant de calibration.
// Rejoue un parcours complet avec des paquets synthetiques, exactement comme
// le ferait un utilisateur devant l'ecran - y compris les cas ou il triche
// (valider sans rien faire) ou ou sa souris n'a pas de molette.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../calibration_session.h"

static int g_failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("  FAIL (%s:%d): %s\n", __FILE__, __LINE__, #cond);  \
            g_failures++;                                                    \
        }                                                                    \
    } while (0)

static uint32_t g_tick = 1000;

// Envoie n fois le meme paquet, avec un tick different a chaque fois (le
// plugin en produit un nouveau a chaque tour de boucle).
static void FeedN(CalibrationSession& s, const uint8_t* b, uint8_t len, int n) {
    for (int i = 0; i < n; i++)
        s.Feed(b, len, g_tick++);
}

// Parcours nominal sur une souris boot classique 4 octets.
static void TestFullRunClassicMouse() {
    std::printf("TestFullRunClassicMouse\n");

    CalibrationSession s(0x0461, 0x4E35);
    CHECK(s.Step() == CALSTEP_REST);

    uint8_t rest[4]  = {0x00, 0x00, 0x00, 0x00};
    FeedN(s, rest, 4, CALIB_SAMPLES_REST);
    CHECK(s.CanAdvance());
    s.Advance();
    CHECK(s.Step() == CALSTEP_RIGHT);

    uint8_t right[4] = {0x00, 0x0A, 0x00, 0x00};
    FeedN(s, right, 4, CALIB_SAMPLES_MOTION);
    s.Advance();
    CHECK(s.Step() == CALSTEP_LEFT);

    uint8_t left[4]  = {0x00, 0xF6, 0x00, 0x00}; // -10
    FeedN(s, left, 4, CALIB_SAMPLES_MOTION);
    s.Advance();

    uint8_t down[4]  = {0x00, 0x00, 0x08, 0x00};
    FeedN(s, down, 4, CALIB_SAMPLES_MOTION);
    s.Advance();

    uint8_t up[4]    = {0x00, 0x00, 0xF8, 0x00}; // -8
    FeedN(s, up, 4, CALIB_SAMPLES_MOTION);
    s.Advance();
    CHECK(s.Step() == CALSTEP_WHEEL_FWD);

    uint8_t wf[4]    = {0x00, 0x00, 0x00, 0x01};
    FeedN(s, wf, 4, CALIB_SAMPLES_WHEEL);
    s.Advance();

    uint8_t wb[4]    = {0x00, 0x00, 0x00, 0xFF};
    FeedN(s, wb, 4, CALIB_SAMPLES_WHEEL);
    s.Advance();
    CHECK(s.Step() == CALSTEP_CLICK_LEFT);

    uint8_t click[4] = {0x01, 0x00, 0x00, 0x00};
    FeedN(s, click, 4, CALIB_SAMPLES_CLICK);
    s.Advance();

    CHECK(s.Finished());
    CHECK(s.Succeeded());
    const HidReportLayout& l = s.Layout();
    CHECK(l.valid);
    CHECK(l.xOffset == 1 && l.xSize == 1);
    CHECK(l.yOffset == 2 && l.ySize == 1);
    CHECK(l.wheelOffset == 3);
    CHECK(l.buttonsOffset == 0);
    CHECK(!s.Summary().empty());

    std::printf("  parcours complet : disposition 4 octets correctement apprise\n");
}

// Le verrou anti-triche : valider sans avoir fait le geste ne doit rien faire.
// C'est ce qui empeche d'enregistrer une disposition fausse sans s'en rendre
// compte - le choix explicite d'un assistant plus long mais sur.
static void TestCannotSkipWithoutDoingTheGesture() {
    std::printf("TestCannotSkipWithoutDoingTheGesture\n");

    CalibrationSession s(1, 2);
    CHECK(!s.CanAdvance());
    s.Advance();
    CHECK(s.Step() == CALSTEP_REST); // n'a pas bouge

    uint8_t rest[4] = {0x00, 0x00, 0x00, 0x00};
    FeedN(s, rest, 4, CALIB_SAMPLES_REST);
    s.Advance();
    CHECK(s.Step() == CALSTEP_RIGHT);

    // Envoyer des paquets IDENTIQUES au repos ne doit pas remplir la jauge :
    // l'utilisateur n'a rien bouge.
    FeedN(s, rest, 4, 50);
    CHECK(s.Progress() == 0.0f);
    s.Advance();
    CHECK(s.Step() == CALSTEP_RIGHT); // toujours bloque

    std::printf("  la jauge ne se remplit pas sans geste reel, et valider reste sans effet\n");
}

// Un meme paquet renvoye avec le meme tick ne compte qu'une fois : le plugin
// reecrit input_state.json en continu, y compris sans nouveau rapport USB.
static void TestDuplicateTickIgnored() {
    std::printf("TestDuplicateTickIgnored\n");

    CalibrationSession s(1, 2);
    uint8_t rest[4] = {0x00, 0x00, 0x00, 0x00};
    for (int i = 0; i < 30; i++)
        s.Feed(rest, 4, 777); // meme tick a chaque fois
    CHECK(s.Progress() < 1.0f);

    std::printf("  un tick inchange n'alimente pas la calibration\n");
}

// Une souris sans molette doit aboutir a une calibration VALIDE, simplement
// sans molette - pas a un echec.
static void TestMouseWithoutWheelStillSucceeds() {
    std::printf("TestMouseWithoutWheelStillSucceeds\n");

    CalibrationSession s(3, 4);
    uint8_t rest[4]  = {0x00, 0x00, 0x00, 0x00};
    FeedN(s, rest, 4, CALIB_SAMPLES_REST);
    s.Advance();

    uint8_t right[4] = {0x00, 0x05, 0x00, 0x00};
    FeedN(s, right, 4, CALIB_SAMPLES_MOTION); s.Advance();
    uint8_t left[4]  = {0x00, 0xFB, 0x00, 0x00};
    FeedN(s, left, 4, CALIB_SAMPLES_MOTION); s.Advance();
    uint8_t down[4]  = {0x00, 0x00, 0x05, 0x00};
    FeedN(s, down, 4, CALIB_SAMPLES_MOTION); s.Advance();
    uint8_t up[4]    = {0x00, 0x00, 0xFB, 0x00};
    FeedN(s, up, 4, CALIB_SAMPLES_MOTION); s.Advance();

    // Etapes molette : l'utilisateur tourne mais rien ne change dans le
    // paquet - cette souris n'en emet pas. La jauge reste vide, donc valider
    // est impossible : sans echappatoire, l'utilisateur serait coince au
    // milieu de l'assistant.
    CHECK(s.Step() == CALSTEP_WHEEL_FWD);
    CHECK(!s.CanAdvance());
    CHECK(s.CanSkip());

    s.SkipWheel();
    CHECK(s.Step() == CALSTEP_CLICK_LEFT); // saute les DEUX etapes molette
    CHECK(!s.CanSkip());                   // et pas ailleurs qu'a la molette

    uint8_t click[4] = {0x01, 0x00, 0x00, 0x00};
    FeedN(s, click, 4, CALIB_SAMPLES_CLICK);
    s.Advance();

    CHECK(s.Succeeded());
    CHECK(s.Layout().valid);
    CHECK(s.Layout().wheelOffset == REPORT_FIELD_ABSENT); // valide, sans molette
    CHECK(s.Layout().xOffset == 1 && s.Layout().yOffset == 2);

    std::printf("  souris sans molette : echappatoire, et disposition valide sans molette\n");
}

// Le cas qui motive toute la fonctionnalite : paquet 8 octets, identifiant de
// rapport en tete, axes 16 bits. C'est le profil de la 2e souris de
// l'utilisateur, celle dont le curseur etait casse.
static void TestLongReportMouse() {
    std::printf("TestLongReportMouse\n");

    CalibrationSession s(0x046A, 0xB092);
    uint8_t rest[8] = {0x02, 0x00, 0,0, 0,0, 0x00, 0x00};
    FeedN(s, rest, 8, CALIB_SAMPLES_REST);
    s.Advance();

    uint8_t right[8] = {0x02, 0x00, 0x2C, 0x01, 0,0, 0x00, 0x00}; // X = +300
    FeedN(s, right, 8, CALIB_SAMPLES_MOTION); s.Advance();
    uint8_t left[8]  = {0x02, 0x00, 0xD4, 0xFE, 0,0, 0x00, 0x00}; // X = -300
    FeedN(s, left, 8, CALIB_SAMPLES_MOTION); s.Advance();
    uint8_t down[8]  = {0x02, 0x00, 0,0, 0x90, 0x01, 0x00, 0x00}; // Y = +400
    FeedN(s, down, 8, CALIB_SAMPLES_MOTION); s.Advance();
    uint8_t up[8]    = {0x02, 0x00, 0,0, 0x70, 0xFE, 0x00, 0x00}; // Y = -400
    FeedN(s, up, 8, CALIB_SAMPLES_MOTION); s.Advance();
    uint8_t wf[8]    = {0x02, 0x00, 0,0, 0,0, 0x01, 0x00};
    FeedN(s, wf, 8, CALIB_SAMPLES_WHEEL); s.Advance();
    uint8_t wb[8]    = {0x02, 0x00, 0,0, 0,0, 0xFF, 0x00};
    FeedN(s, wb, 8, CALIB_SAMPLES_WHEEL); s.Advance();
    uint8_t click[8] = {0x02, 0x01, 0,0, 0,0, 0x00, 0x00};
    FeedN(s, click, 8, CALIB_SAMPLES_CLICK); s.Advance();

    CHECK(s.Succeeded());
    const HidReportLayout& l = s.Layout();
    CHECK(l.reportSize == 8);
    CHECK(l.xOffset == 2 && l.xSize == 2);
    CHECK(l.yOffset == 4 && l.ySize == 2);
    CHECK(l.wheelOffset == 6);
    CHECK(l.buttonsOffset == 1); // et PAS 0, qui est l'identifiant de rapport

    std::printf("  souris a paquet long : identifiant ignore, axes 16 bits et boutons corrects\n");
}

// LES DEUX DEFAUTS REELS remontes par l'utilisateur apres son premier test
// sur console (2026-08-03), reproduits ensemble :
//   1. la souris n'est jamais parfaitement immobile au repos - le capteur
//      bruite, la main la frole. L'ancienne version disqualifiait tout octet
//      qui frémissait une seule fois, donc X et Y, donc AUCUNE detection.
//   2. pour tourner la molette, la main fait bouger la souris : les octets
//      X et Y varient pendant l'etape molette et etaient pris pour elle.
static void TestNoisyRestAndWheelContamination() {
    std::printf("TestNoisyRestAndWheelContamination\n");

    CalibrationSession s(0x046A, 0xB092);

    // Repos BRUITE : X et Y frémissent de +/-1, comme sur du vrai materiel.
    for (int i = 0; i < CALIB_SAMPLES_REST; i++) {
        uint8_t r[8] = {0x02, 0x00, (uint8_t)(i % 2 ? 0x01 : 0xFF), 0x00,
                        (uint8_t)(i % 3 ? 0x00 : 0x01), 0x00, 0x00, 0x00};
        s.Feed(r, 8, g_tick++);
    }
    CHECK(s.CanAdvance());
    s.Advance();

    uint8_t right[8] = {0x02, 0x00, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00}; // X +40
    FeedN(s, right, 8, CALIB_SAMPLES_MOTION); s.Advance();
    uint8_t left[8]  = {0x02, 0x00, 0xD8, 0xFF, 0x00, 0x00, 0x00, 0x00}; // X -40
    FeedN(s, left, 8, CALIB_SAMPLES_MOTION); s.Advance();
    uint8_t down[8]  = {0x02, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00}; // Y +30
    FeedN(s, down, 8, CALIB_SAMPLES_MOTION); s.Advance();
    uint8_t up[8]    = {0x02, 0x00, 0x00, 0x00, 0xE2, 0xFF, 0x00, 0x00}; // Y -30
    FeedN(s, up, 8, CALIB_SAMPLES_MOTION); s.Advance();

    CHECK(s.Step() == CALSTEP_WHEEL_FWD);

    // Molette avant : le cran vaut +1 sur l'octet 6, MAIS la main fait aussi
    // bouger la souris - X derive de +5. Sans exclusion, X gagnerait le score
    // (amplitude 5 contre 1) et serait pris pour la molette.
    uint8_t wf[8] = {0x02, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x00};
    FeedN(s, wf, 8, CALIB_SAMPLES_WHEEL); s.Advance();
    uint8_t wb[8] = {0x02, 0x00, 0xFB, 0xFF, 0x00, 0x00, 0xFF, 0x00}; // X -5, molette -1
    FeedN(s, wb, 8, CALIB_SAMPLES_WHEEL); s.Advance();

    uint8_t click[8] = {0x02, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00}; // clic + petit mouvement
    FeedN(s, click, 8, CALIB_SAMPLES_CLICK); s.Advance();

    CHECK(s.Succeeded());
    const HidReportLayout& l = s.Layout();
    CHECK(l.xOffset == 2);      // detecte MALGRE le bruit au repos
    CHECK(l.yOffset == 4);
    CHECK(l.wheelOffset == 6);  // et PAS 2 : le mouvement parasite est exclu
    CHECK(l.buttonsOffset == 1);

    std::printf("  repos bruite tolere, et le mouvement parasite n'est plus pris pour la molette\n");
}

static void TestCancelLeavesNothing() {
    std::printf("TestCancelLeavesNothing\n");
    CalibrationSession s(1, 2);
    s.Cancel();
    CHECK(s.Finished());
    CHECK(!s.Succeeded());
    CHECK(!s.Layout().valid);
    std::printf("  annulation : aucune disposition produite\n");
}

int main() {
    TestFullRunClassicMouse();
    TestCannotSkipWithoutDoingTheGesture();
    TestDuplicateTickIgnored();
    TestMouseWithoutWheelStillSucceeds();
    TestLongReportMouse();
    TestNoisyRestAndWheelContamination();
    TestCancelLeavesNothing();

    if (g_failures == 0) {
        std::printf("\nALL TESTS PASSED\n");
        return 0;
    }
    std::printf("\n%d FAILURE(S)\n", g_failures);
    return 1;
}
