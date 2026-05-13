#include "GameEngine.h"
#include "RendererHelpers.h"
#include "PhysicsHelpers.h"
#include "MusicSystem.h"
#include "GameDataTypes.h"
#include "DialogueSystem.h"

#include <iostream>
#include <filesystem> 
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <array>
#include <deque>
#include <memory>
#include <limits>
#include <functional>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>


#ifndef TINYGLTF_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#endif
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#ifndef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#include "tiny_gltf.h"

using namespace std;

Objectives mesuemObjectives;

static std::vector<RoomLock> g_roomLocks;
static std::vector<KeyPickup> g_keyPickups;
static std::vector<ClueNote> g_clueNotes;
static std::vector<SafePuzzle> g_safes;
static std::vector<SymbolPuzzle> g_symbols;
static std::vector<int> g_safeBoxIndices;
static std::vector<int> g_pedestalBoxIndices;
static Image g_stairWallOverlay;
static bool g_stairWallOverlayReady = false;
static std::unordered_set<std::string> g_playerKeys;
static std::vector<int> g_foundNotes;


static std::string g_accessPopup;
static Uint32 g_accessPopupUntil = 0;

static bool g_codeEntryActive = false;
static int g_codeEntryLockIndex = -1;
static std::string g_codeEntryBuffer;
static int g_safeEntryIndex = -1;
static int g_symbolEntryIndex = -1;
static int g_symbolState[3] = {0,0,0};
static int g_symbolFocus = 0;
static bool g_notesOpen = false;
static int g_notesSelected = 0;
static int g_notesBodyScroll = 0;
static bool g_caveFinalNoteCollected = false;
static int g_notesCollectedRun = 0;
static float g_runElapsedSeconds = 0.0f;
static bool g_caveQuizActive = false;
static bool g_caveQuizPassed = false;
static int g_caveQuizQuestionIndex = 0;
static std::vector<CaveQuizQuestion> g_caveQuiz;
static bool g_caveTimeoutActive = false;
static float g_caveTimeoutTimer = 0.0f;
static constexpr float kCaveTimeoutDuration = 10.0f;
static int g_caveTimeoutStage = 0;
static float g_caveTimeoutFlashTimer = 0.0f;
static constexpr float kCaveTimeoutFlashDuration = 0.85f;
static std::string g_caveTimeoutLine = "Time's up. There was never an escape, thank you for letting go";
static size_t g_caveTimeoutTypedChars = 0;
static float g_caveTimeoutTypingAccumulator = 0.0f;
static float g_caveTimeoutCreditsTimer = 0.0f;
static std::vector<MindTrapPhase> g_mindTrapPhases;
static bool g_mindTrapActive = false;
static bool g_mindTrapTriggerConsumed = false;
static int g_mindTrapPhaseIndex = 0;
static bool g_mindTrapShowingResult = false;
static std::string g_mindTrapLastResult;
static float g_mindTrapResultTimer = 0.0f;
static float g_mindTrapResultDuration = 2.8f;
static float g_mindTrapFlickerTimer = 0.0f;
static float g_mindTrapWhiteFlashTimer = 0.0f;
static bool g_mindTrapAdvanceOnResult = false;
static bool g_mindTrapExitOnResult = false;
static bool g_mindTrapReadyToExit = false;
static std::vector<std::string> g_mindTrapTerminalLog;
static std::deque<std::string> g_mindTrapTypeQueue;
static std::string g_mindTrapTypingLine;
static size_t g_mindTrapTypingChars = 0;
static float g_mindTrapTypingAccumulator = 0.0f;
static float g_mindTrapTypingCharsPerSecond = 54.0f;
static float g_mindTrapPostLinePause = 0.0f;
static bool g_mindTrapAwaitingChoice = false;
static int g_mindTrapSelectedOption = 0;
static bool g_mindTrapAdvanceAfterResult = false;
static bool g_mindTrapFinalizeAfterResult = false;
static int g_mindTrapLastHoveredOption = -1;
static float g_mindTrapHoverTimer = 0.0f;
static bool g_mindTrapForcedCorrectionActive = false;
static float g_mindTrapForcedCorrectionTimer = 0.0f;
static int g_mindTrapForcedOption = -1;
static float g_mindTrapChoiceJitterTimer = 0.0f;
static bool g_mindTrapTearActive = false;
static float g_mindTrapTearTimer = 0.0f;
static bool g_museumPuzzleInitialized = false;
static bool g_unlockAllDoorsOverride = false;
static bool g_solventLabUnlockCutsceneActive = false;
static int g_solventLabUnlockCutsceneStage = 0;
static float g_solventLabUnlockCutsceneTimer = 0.0f;
static float g_solventLabUnlockTurnStartYaw = 0.0f;
static float g_solventLabUnlockWhiteFlash = 0.0f;
static int g_solventLabMonsterModelIndex = -1;
static bool g_solventCoolerEntryActive = false;
static std::string g_solventCoolerBuffer;
static bool g_redPigmentDispensed = false;
static bool g_redPigmentDispenseCutsceneActive = false;
static float g_redPigmentDispenseCutsceneTimer = 0.0f;
static int g_redPigmentDispenseModelIndex = -1;
static float g_redPigmentDispenseBaseYaw = 0.0f;
static bool g_restorationWingUnlocked = false;
static bool g_wakeCutsceneActive = false;
static float g_wakeCutsceneTimer = 0.0f;
static constexpr float kWakeCutsceneDuration = 11.35f;
static int g_wakeCutsceneStage = 0;
static float g_wakeCutsceneStageTimer = 0.0f;
static float g_wakeCutsceneInitialYaw = 0.0f;
static float g_wakeCutsceneTurnStartYaw = 0.0f;
static float g_wakeCutsceneReturnStartYaw = 0.0f;
static float g_wakeDarknessOverride = -1.0f;
static std::shared_ptr<sf::Sound> g_wakeGeneratorHumSound;
static int g_wakeMonsterModelIndex = -1;
static bool g_startupBriefingActive = false;
static int g_startupBriefingStage = 0;
static float g_startupBriefingTimer = 0.0f;
static std::vector<std::string> g_startupBriefingLog;
static std::deque<std::string> g_startupBriefingTypeQueue;
static std::string g_startupBriefingTypingLine;
static size_t g_startupBriefingTypingChars = 0;
static float g_startupBriefingTypingAccumulator = 0.0f;
static float g_startupBriefingPostLinePause = 0.0f;
static bool g_startupBriefingAwaitingContinue = false;
static bool g_rosesCutsceneActive = false;
static int g_rosesCutsceneStage = 0;
static float g_rosesCutsceneTimer = 0.0f;
static float g_rosesCutsceneInitialYaw = 0.0f;
static float g_rosesCutsceneTurnStartYaw = 0.0f;
static float g_rosesCutsceneReturnStartYaw = 0.0f;
static int g_rosesMonsterModelIndex = -1;
static std::shared_ptr<sf::Sound> g_rosesGeneratorHumSound;
static float g_rosesDarknessOverride = -1.0f;
static bool g_generatorNeedsGasLinePlayed = false;
static bool g_gasCanCollected = false;
static bool g_generatorFueled = false;
static int g_generatorModelIndex = -1;
static int g_gasCanModelIndex = -1;
static bool g_powerRestoreFlickerActive = false;
static float g_powerRestoreFlickerTimer = 0.0f;
static constexpr float kPowerRestoreFlickerDuration = 0.85f;
static constexpr float kMuseumGeneratorX = 7.41f;
static constexpr float kMuseumGeneratorY = 7.19f;
static constexpr float kMuseumGasCanX = 16.1f;
static constexpr float kMuseumGasCanY = 2.1f;

static std::unordered_map<std::string, std::shared_ptr<CpuModel>> g_cpuModelCache;
static std::vector<WorldModelInstance> g_worldModels;

static std::shared_ptr<CpuModel> loadCpuModel( const std::string &modelPath );
static float modelScaleOverride( const std::string &modelPath );
static int addWorldModelInstance(
    const std::string &modelPath,
    float x,
    float y,
    float scale,
    Uint32 tint,
    float yaw,
    float pitch,
    float roll,
    bool spinYaw,
    float spinSpeed,
    float heightOffset );

#include "CutsceneController.h"

static WeaponPickup g_revolverPickup;
static bool g_endGameStateAllowRevolver = false;
static CombatState g_combatState;
static bool g_directorDeskUnlocked = false;
static DialogueSystem g_dialogue;
static CutsceneController g_cutsceneController;
static bool g_showHeldWeapon = false;
static int g_heldRevolverModelIndex = -1;
static bool g_revolverAiming = false;
static float g_revolverShotCooldown = 0.0f;
static float g_revolverRecoilTimer = 0.0f;
static constexpr float kRevolverRecoilDuration = 0.30f;
static bool g_revolverInspectCutsceneActive = false;
static float g_revolverInspectCutsceneTimer = 0.0f;
static constexpr float kRevolverInspectCutsceneDuration = 3.5f;
static int g_revolverInspectModelIndex = -1;
static float g_revolverInspectBaseYaw = 0.0f;
static std::string g_weaponSoundBaseFolder;
static sf::SoundBuffer g_gunshotBuffer;
static sf::SoundBuffer g_shellDropBuffer;
static bool g_weaponSoundBuffersReady = false;
static std::vector<std::shared_ptr<sf::Sound>> g_activeWeaponSounds;
static std::vector<float> g_pendingShellDropTimers;
static std::string g_generatorStartSoundBaseFolder;
static sf::SoundBuffer g_generatorStartBuffer;
static bool g_generatorStartBufferReady = false;
static std::shared_ptr<sf::Sound> g_generatorStartSound;
static float g_generatorStartSoundTimer = 0.0f;
static constexpr float kGeneratorStartSoundDuration = 3.0f;
static constexpr float kGeneratorStartFadeOutDuration = 2.0f;
static constexpr float kGeneratorStartSoundVolume = 100.0f;
static sf::SoundBuffer g_whisperBuffer;
static bool g_whisperBufferReady = false;
static std::string g_whisperBaseFolder;
static float g_whisperTimer = 0.0f;
static float g_whisperNextDelay = 14.0f;
static bool g_firstLockedDoorDialogueShown = false;
static constexpr float kRevolverFacingYawOffset = 1.5f;
static constexpr float kRevolverScreenShakeX = 16.0f;
static constexpr float kRevolverScreenShakeY = 11.0f;

static std::string g_currentLevelFolder;
static bool g_levelEditorMode = false;
static int g_editorAssetIndex = 0;
static int g_editorSelectedModel = -1;
static constexpr const char* kEditorModelsLegacyFile = "editor_models.txt";
static std::string g_currentEditorModelsFile = kEditorModelsLegacyFile;
static std::vector<EditorAssetDef> g_editorAssetCatalog;
static Uint32 g_editorCatalogLastScanTick = 0;
static constexpr Uint32 kEditorCatalogRefreshMs = 5000;

static std::string resolveAssetModelPath( const std::string &assetName );
static std::filesystem::path editorModelsPathForLevel();
static std::filesystem::path findAssetsRoot();
static void refreshEditorAssetCatalog( bool force = false );
static void clearPuzzleState();

#include "GameplaySystems.h"
#include "GameplayUiRendering.h"

static float g_randomFootstepTimer = 0.0f;
static constexpr float kRandomFootstepCheckInterval = 40.0f;  // Check every 40 seconds
static constexpr float kRandomFootstepChance = 0.40f;          // 40% chance


static void updateRandomFootsteps(Engine& engineContext, float dt, const std::string& baseLevelFolder) {
    if (engineContext.currentLevel != Levels::MUSEUM && engineContext.currentLevel != Levels::MUSEUM_UPPER) {
        g_randomFootstepTimer = 0.0f;
        return;
    }

    g_randomFootstepTimer += dt;
    if (g_randomFootstepTimer < kRandomFootstepCheckInterval) {
        return;
    }

    g_randomFootstepTimer = 0.0f;

    // 40% chance to play footstep
    const float roll = float(std::rand()) / float(RAND_MAX);
    if (roll > kRandomFootstepChance) {
        return;
    }

    // Pick random location on map
    if (engineContext.map.width <= 0 || engineContext.map.height <= 0) {
        return;
    }

    int randomTx = std::rand() % engineContext.map.width;
    int randomTy = std::rand() % engineContext.map.height;

    // Make sure tile is walkable
    const int tile = engineContext.map.tiles[randomTy * engineContext.map.width + randomTx];
    if (tile != 0) {
        return;  // Wall or obstacle
    }

    // Pick a location near the tile that's far from the player
    const float randomX = float(randomTx) + 0.3f + (float(std::rand()) / float(RAND_MAX)) * 0.4f;
    const float randomY = float(randomTy) + 0.3f + (float(std::rand()) / float(RAND_MAX)) * 0.4f;

    // Only play if far enough from player
    const float dx = engineContext.positionX - randomX;
    const float dy = engineContext.positionY - randomY;
    const float distSq = dx * dx + dy * dy;
    if (distSq < (8.0f * 8.0f)) {
        return;  // Too close to player
    }

    // Play directional footstep sound at this location
    playFootstep(baseLevelFolder);
}

static void registerWepingStatue(int modelIndex);

static sf::SoundBufferRecorder g_volumeSpikeRecorder;
static float g_lastAverageAmplitude = 0.0f;
static float g_volumeSpikeCheckTimer = 0.0f;
static constexpr float kVolumeSpikeCheckInterval = 0.5f;  // Check every 0.5 seconds
static constexpr float kVolumeSpikeThreshold = 0.35f;     // Spike threshold (normalized 0-1)
static constexpr float kVolumeSpikeBaselineMultiplier = 1.8f;

static void updateActiveRecording(Engine& engineContext, float dt) {
    if (!config::autoMusicVolume) {
        return;
    }

    // Start recording if not already
    static bool recordingStarted = false;
    if (!recordingStarted && sf::SoundBufferRecorder::isAvailable()) {
        if (g_volumeSpikeRecorder.start(22050)) {
            recordingStarted = true;
        }
    }

    if (!recordingStarted) {
        return;
    }

    g_volumeSpikeCheckTimer += dt;
    if (g_volumeSpikeCheckTimer < kVolumeSpikeCheckInterval) {
        return;
    }

    g_volumeSpikeCheckTimer = 0.0f;

    const sf::SoundBuffer& buf = g_volumeSpikeRecorder.getBuffer();
    const auto* samples = buf.getSamples();
    size_t count = buf.getSampleCount();

    if (!samples || count == 0) {
        return;
    }

    // Calculate RMS of recent samples
    const size_t sampleWindow = std::min(size_t(4410), count);  // ~0.2 seconds at 22050 Hz
    const size_t startIdx = count > sampleWindow ? count - sampleWindow : 0;

    double sumSq = 0.0;
    for (size_t i = startIdx; i < count; ++i) {
        double s = double(samples[i]) / 32768.0;
        sumSq += s * s;
    }
    double rms = std::sqrt(sumSq / double(sampleWindow));
    float currentAmplitude = float(std::clamp(rms, 0.0, 1.0));

    // Detect spike: current is significantly higher than baseline
    const bool isSpike = currentAmplitude > (g_lastAverageAmplitude * kVolumeSpikeBaselineMultiplier) &&
        currentAmplitude > kVolumeSpikeThreshold;

    // Smooth average
    g_lastAverageAmplitude = g_lastAverageAmplitude * 0.7f + currentAmplitude * 0.3f;

    if (isSpike) {
        // Play dialogue about being too loud
        const bool canSpeak =
            !g_dialogue.isActive() &&
            !g_cutsceneController.isCameraLockActive() &&
            !g_revolverInspectCutsceneActive &&
            !g_wakeCutsceneActive &&
            !g_codeEntryActive &&
            !g_notesOpen &&
            !g_caveQuizActive;

        if (canSpeak) {
            g_dialogue.start({ { "I'm being too loud, it might hear me", 3.0f } });
        }
    }
}




static float g_doorCreakTimer = 0.0f;
static constexpr float kDoorCreakCheckInterval = 120.0f;  // 2 minutes
static constexpr float kDoorCreakChance = 0.30f;           // 30% chance

static void updateRandomDoorCreaks(Engine& engineContext, float dt, const std::string& baseLevelFolder) {
    if (engineContext.currentLevel != Levels::MUSEUM && engineContext.currentLevel != Levels::MUSEUM_UPPER) {
        g_doorCreakTimer = 0.0f;
        return;
    }

    g_doorCreakTimer += dt;
    if (g_doorCreakTimer < kDoorCreakCheckInterval) {
        return;
    }

    g_doorCreakTimer = 0.0f;

    // 30% chance to play door creak
    const float roll = float(std::rand()) / float(RAND_MAX);
    if (roll > kDoorCreakChance) {
        return;
    }

    playDoorCreak(baseLevelFolder);
}




static void updatePanicAttack(Engine& engineContext, float dt) {
    if (!g_panicAttack.active)
    {
        return;
    }

    g_panicAttack.timer += dt;
    const float progress = std::clamp(g_panicAttack.timer / g_panicAttack.duration, 0.0f, 1.0f);

    if (progress >= 1.0f)
    {
        g_panicAttack.active = false;
        g_panicAttack.blurIntensity = 0.0f;
        stopHeartbeat();
        return;
    }

    const float rampUp = std::clamp(progress / 0.18f, 0.0f, 1.0f);
    const float rampDown = std::clamp((1.0f - progress) / 0.20f, 0.0f, 1.0f);
    g_panicAttack.blurIntensity = std::min(rampUp, rampDown) * kPanicAttackBlurMax;
}



static float getPanicAttackBlurIntensity() {
    return g_panicAttack.blurIntensity;
}


static void updateHorrorElements(Engine& engineContext, float dt, const std::string& baseLevelFolder) {
    updateRandomFootsteps(engineContext, dt, baseLevelFolder);
    updateActiveRecording(engineContext, dt);
    updateWepingStatues(engineContext, dt);
    updateRandomDoorCreaks(engineContext, dt, baseLevelFolder);
    updatePanicAttack(engineContext, dt);
}

static void applyPanicAttackBlur(Engine& engineContext, float intensity) {
    if (intensity <= 0.001f) return;
    if (engineContext.backbuffer.size() != size_t(RENDER_W * RENDER_H)) return;

    const float normalized = std::clamp(intensity / std::max(0.001f, kPanicAttackBlurMax), 0.0f, 1.0f);
    const int radius = std::clamp(1 + int(std::round(normalized * 3.0f)), 1, 4);
    const float mix = std::clamp(0.22f + normalized * 0.58f, 0.0f, 0.80f);

    static std::vector<Uint32> scratch;
    static std::vector<Uint32> temp;

    scratch = engineContext.backbuffer;
    temp.resize(scratch.size());

    auto sample = [](const std::vector<Uint32>& src, int x, int y) -> Uint32 {
        x = std::clamp(x, 0, RENDER_W - 1);
        y = std::clamp(y, 0, RENDER_H - 1);
        return src[y * RENDER_W + x];
        };

    for (int y = 0; y < RENDER_H; ++y)
    {
        for (int x = 0; x < RENDER_W; ++x)
        {
            float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f, sumW = 0.0f;
            for (int ox = -radius; ox <= radius; ++ox)
            {
                const float w = float(radius + 1 - std::abs(ox));
                const Uint32 c = sample(scratch, x + ox, y);
                sumR += float((c >> 16) & 255) * w;
                sumG += float((c >> 8) & 255) * w;
                sumB += float(c & 255) * w;
                sumW += w;
            }

            const int idx = y * RENDER_W + x;
            temp[idx] = rgb(
                Uint8(std::clamp(sumR / std::max(0.001f, sumW), 0.0f, 255.0f)),
                Uint8(std::clamp(sumG / std::max(0.001f, sumW), 0.0f, 255.0f)),
                Uint8(std::clamp(sumB / std::max(0.001f, sumW), 0.0f, 255.0f)));
        }
    }

    for (int y = 0; y < RENDER_H; ++y)
    {
        for (int x = 0; x < RENDER_W; ++x)
        {
            float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f, sumW = 0.0f;
            for (int oy = -radius; oy <= radius; ++oy)
            {
                const float w = float(radius + 1 - std::abs(oy));
                const Uint32 c = sample(temp, x, y + oy);
                sumR += float((c >> 16) & 255) * w;
                sumG += float((c >> 8) & 255) * w;
                sumB += float(c & 255) * w;
                sumW += w;
            }

            const int idx = y * RENDER_W + x;
            const Uint32 blurred = rgb(
                Uint8(std::clamp(sumR / std::max(0.001f, sumW), 0.0f, 255.0f)),
                Uint8(std::clamp(sumG / std::max(0.001f, sumW), 0.0f, 255.0f)),
                Uint8(std::clamp(sumB / std::max(0.001f, sumW), 0.0f, 255.0f)));

            const Uint32 original = scratch[idx];
            const float invMix = 1.0f - mix;

            const float orR = float((original >> 16) & 255);
            const float orG = float((original >> 8) & 255);
            const float orB = float(original & 255);

            const float blR = float((blurred >> 16) & 255);
            const float blG = float((blurred >> 8) & 255);
            const float blB = float(blurred & 255);

            engineContext.backbuffer[idx] = rgb(
                Uint8(std::clamp(orR * invMix + blR * mix, 0.0f, 255.0f)),
                Uint8(std::clamp(orG * invMix + blG * mix, 0.0f, 255.0f)),
                Uint8(std::clamp(orB * invMix + blB * mix, 0.0f, 255.0f)));
        }
    }
}

static void applyCaveTimeoutEffect( Engine &engineContext, float progress ) {
    progress = std::clamp( progress, 0.0f, 1.0f );
    if (progress <= 0.0f) return;

    const float desat = std::clamp( progress * 1.1f, 0.0f, 1.0f );
    const float staticAmp = std::clamp( 0.08f + progress * 0.42f, 0.08f, 0.55f );
    const float fade = std::clamp( 1.0f - progress, 0.0f, 1.0f );
    const Uint32 timeSeed = SDL_GetTicks();

    for (int y = 0; y < RENDER_H; ++y)
    {
        for (int x = 0; x < RENDER_W; ++x)
        {
            const int idx = y * RENDER_W + x;
            const Uint32 c = engineContext.backbuffer[ idx ];
            float r = float( (c >> 16) & 255 );
            float g = float( (c >> 8) & 255 );
            float b = float( c & 255 );
            const float lum = 0.299f * r + 0.587f * g + 0.114f * b;

            r = r * (1.0f - desat) + lum * desat;
            g = g * (1.0f - desat) + lum * desat;
            b = b * (1.0f - desat) + lum * desat;

            const Uint32 noiseSeed = (Uint32(x) * 73856093u) ^ (Uint32(y) * 19349663u) ^ (timeSeed * 83492791u);
            const float noise = (float( noiseSeed & 255 ) / 255.0f - 0.5f) * 2.0f;
            const float staticOffset = noise * 255.0f * staticAmp;

            r = (r + staticOffset) * fade;
            g = (g + staticOffset) * fade;
            b = (b + staticOffset) * fade;

            engineContext.backbuffer[ idx ] = rgb(
                Uint8( std::clamp( r, 0.0f, 255.0f ) ),
                Uint8( std::clamp( g, 0.0f, 255.0f ) ),
                Uint8( std::clamp( b, 0.0f, 255.0f ) ) );
        }
    }
}

static void updateCaveTimeoutEndingSequence( float dt ) {
    if (g_caveTimeoutStage == 1)
    {
        g_caveTimeoutFlashTimer += dt;
        if (g_caveTimeoutFlashTimer >= kCaveTimeoutFlashDuration)
        {
            g_caveTimeoutFlashTimer = kCaveTimeoutFlashDuration;
            g_caveTimeoutStage = 2;
            g_caveTimeoutTypedChars = 0;
            g_caveTimeoutTypingAccumulator = 0.0f;
            g_caveTimeoutCreditsTimer = 0.0f;
        }
        return;
    }

    if (g_caveTimeoutStage == 2)
    {
        auto charCost = []( char c ) -> float {
            if (c == ' ') return 0.45f;
            if (c == '.' || c == ',' || c == ':' || c == ';') return 1.7f;
            if (c == '?' || c == '!') return 2.1f;
            return 1.0f;
        };

        g_caveTimeoutTypingAccumulator += dt * 48.0f;
        while (g_caveTimeoutTypedChars < g_caveTimeoutLine.size())
        {
            const float cost = charCost( g_caveTimeoutLine[ g_caveTimeoutTypedChars ] );
            if (g_caveTimeoutTypingAccumulator < cost) break;
            ++g_caveTimeoutTypedChars;
            g_caveTimeoutTypingAccumulator -= cost;
        }

        if (g_caveTimeoutTypedChars >= g_caveTimeoutLine.size())
        {
            g_caveTimeoutCreditsTimer += dt;
            if (g_caveTimeoutCreditsTimer >= 1.0f)
            {
                g_caveTimeoutStage = 3;
            }
        }
    }
}




int main( int argc, char **argv ) {
    (void)argc; (void)argv;
    std::srand( (unsigned int)std::time( nullptr ) );

    if (!SDL_Init( SDL_INIT_VIDEO ))
    {
        std::fprintf( stderr, "SDL_Init failed: %s\n", SDL_GetError() );
        return 1;
    }

    Engine engineContext;
    engineContext.backbuffer.resize( RENDER_W * RENDER_H );
    engineContext.window = SDL_CreateWindow( "Still Life", RENDER_W * WIN_SCALE, RENDER_H * WIN_SCALE, 0 );
    SDL_SetWindowPosition( engineContext.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED );

    SDL_SetWindowRelativeMouseMode(engineContext.window, false);


    if (!engineContext.window)
    {
        std::fprintf( stderr, "SDL_CreateWindow: %s\n", SDL_GetError() ); return 1;
    }
    engineContext.renderer = SDL_CreateRenderer( engineContext.window, nullptr );     // 2 args in SDL3
    SDL_SetRenderVSync( engineContext.renderer, 1 );                   // optional vsync

    if (!engineContext.renderer)
    {
        std::fprintf( stderr, "SDL_CreateRenderer: %s\n", SDL_GetError() );
        return 1;
    }

    g_logicalThreadCount = std::max( 1u, std::thread::hardware_concurrency() );
    g_detectedThreadCount = estimateCpuRasterWorkerThreads( g_logicalThreadCount );
    initRasterWorkerPool();

    configureRuntimeGpuProfile( engineContext.renderer );

    engineContext.backtexure = SDL_CreateTexture( engineContext.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, RENDER_W, RENDER_H );
    applyPresentationFilter( engineContext );



    engineContext.hasFloor = true;
    engineContext.hasCeiling = true;

    std::filesystem::path cwd = std::filesystem::current_path();

    auto findProjectRoot = [&]( std::filesystem::path start ) {
        for (int i = 0; i < 6; ++i)
        {
            if (std::filesystem::exists( start / "levels" / "museum" / "map.txt" )) return start;
            if (std::filesystem::exists( start / "CCP Art Final" / "levels" / "museum" / "map.txt" )) return start / "CCP Art Final";
            if (!start.has_parent_path()) break;
            std::filesystem::path parent = start.parent_path();
            if (parent == start) break;
            start = parent;
        }
        return cwd;
        };
    std::filesystem::path assetRoot = findProjectRoot( cwd );

    std::vector<LevelDef> levels = {
    {"Museum Entrance", (assetRoot / "levels" / "entrance").string(), "map.txt", 11.5f, 15.5f, 270.f, Levels::ENTRANCE, rgb( 230, 238, 255 ), 0.78f, false, "Check in at the desk"},
    {"Museum Ground", (assetRoot / "levels" / "museum").string(), "map.txt", 10.0f, 9.0f, 90.f, Levels::MUSEUM, rgb( 255, 242, 220 ), 0.72f, true, "Explore and view the art"},
    {"Museum Upper", (assetRoot / "levels" / "museum_upper").string(), "map.txt", 3.8f, 9.3f, 0.f, Levels::MUSEUM_UPPER, rgb( 205, 225, 255 ), 0.66f, true, "Figure out what's going on"},
    {"Transition", (assetRoot / "levels" / "transition").string(), "map.txt", 1.5f, 4.5f, 270.f, Levels::TRANSITION, rgb( 235, 235, 235 ), 0.62f, false, "Proceed through the tunnels"},
    {"Cave", (assetRoot / "levels" / "cave").string(), "map.txt", 2.5f, 2.5f, 90.0f, Levels::CAVE, rgb( 200, 215, 255 ), 0.58f, false, "Escape"}


    };

    const std::string phoneCutsceneAsset = resolveFirstExistingAsset( { "Phone.glb", "phone.glb", "MessageBoard.glb" } );

    calibrateMusicVolumeFromMic();

    engineContext.currentLevel = Levels::MUSEUM;
    int curLevel = engineContext.currentLevel;
    if (!loadLevel( engineContext, levels[ curLevel ] )) return 1;

    std::vector<float2> floors, doors, walls;
    for (int ty = 0; ty < engineContext.map.height; ++ty)
    {
        for (int tx = 0; tx < engineContext.map.width; ++tx)
        {
            int t = engineContext.map.tiles[ ty * engineContext.map.width + tx ]; // 0 empty, 1 wall, 2 door
            float2 prop = tileCenter( tx, ty );
            if (t == 0) floors.push_back( prop );
            else if (t == 1) walls.push_back( prop );
            else if (t == 2) doors.push_back( prop );
        }
    }


    GameState currentState = debug::showMenuInital ? STATE_MENU : STATE_GAME;
    if (currentState == STATE_GAME)
    {
        startWakeCutscene( engineContext );
    }

    int currentMenuSelection = 0; // 0=Play, 1=Music, 2=Volume, 3=viewbobbing, 4=AA, 5=quality, 6=gpu mode, 7=multithreading, 8=school, 9=quit
    const int numMenuOptions = 10;
    float musicVolume = getMusicVolume(); 
    g_notesCollectedRun = 0;
    g_runElapsedSeconds = 0.0f;
  


    // Main loop
    bool running = true; 
    Uint32 prev = SDL_GetTicks();
    static int __fpsCounter = 0;
    static float __fpsAccum = 0.0f;
    while (running)
    {

        Uint32 now = SDL_GetTicks();
        float dt = (now - prev) / 1000.0f;
        prev = now;
        if (dt > 0.05f) dt = 0.05f;
        // Update FPS accumulator and publish to engineContext every 0.5s
        __fpsCounter++;
        __fpsAccum += dt;
        if (__fpsAccum >= 0.5f)
        {
            engineContext.fps = float(__fpsCounter) / __fpsAccum;
            __fpsCounter = 0;
            __fpsAccum = 0.0f;
        }

        updateMusicListener( engineContext.positionX, engineContext.positionY, engineContext.directionX, engineContext.directionY );
        {
            const sf::Vector3f source = getMusicSourcePosition();
            updateAudioOcclusion( engineContext, source.x, source.y );
        }

        if (currentState == STATE_GAME) 
        {
            g_runElapsedSeconds += dt;
            if (g_caveTimeoutActive)
            {
                g_caveTimeoutTimer += dt;
                if (g_caveTimeoutTimer >= kCaveTimeoutDuration)
                {
                    g_caveTimeoutActive = false;
                    g_caveTimeoutTimer = 0.0f;
                    g_caveTimeoutStage = 1;
                    g_caveTimeoutFlashTimer = 0.0f;
                    currentState = STATE_ENDING;
                }
            }
            else if (g_caveTimerActive && !g_caveQuizPassed)
            {
                g_caveTimerSeconds -= dt;
                if (g_caveTimerSeconds <= 0.0f)
                {
                    g_caveTimerActive = false;
                    g_caveTimerSeconds = 0.0f;
                    g_caveTimeoutActive = true;
                    g_caveTimeoutTimer = 0.0f;
                    g_caveTimeoutStage = 0;
                    g_caveTimeoutFlashTimer = 0.0f;
                    g_caveTimeoutTypedChars = 0;
                    g_caveTimeoutTypingAccumulator = 0.0f;
                    g_caveTimeoutCreditsTimer = 0.0f;
                }
            }
        }
        updateCaveTimeoutEndingSequence( dt );
        // Input
        SDL_Event ev;
        float actualSpeed = MOVE_SPEED;

        static float walkTime = 0.f;
        bool stepTriggered = false;
        const bool movingLastFrame = engineContext.isMoving;
        if (movingLastFrame)
        {
            walkTime += dt * MOVE_SPEED * 5.0f;
            float sinValue = std::sin( walkTime );

            if (config::useMusic)
            {
   
                if (std::abs( sinValue ) > 0.999f)
                {
                    if (!stepTriggered)
                    {
                        playFootstep( levels[ engineContext.currentLevel ].folder );
                        stepTriggered = true; 
                    }
                }
                else 
                {
                    stepTriggered = false;
                }
            }
        }
        const bool allowViewBob =
            config::viewBobbing &&
            !g_cutsceneController.isCameraLockActive() &&
            !g_wakeCutsceneActive &&
            !g_solventLabUnlockCutsceneActive;
        g_cutsceneController.updateViewBobShake( allowViewBob, movingLastFrame, dt, MOVE_SPEED );
        engineContext.isMoving = false;

        updateMusicStream();
        g_dialogue.update( dt );
        g_cutsceneController.update( engineContext, g_dialogue, dt );
        if (currentState == STATE_GAME || currentState == STATE_MIND_TRAP)
        {
            updateWhisperAmbience( engineContext, dt );
        }

        if (currentState == STATE_MIND_TRAP)
        {
            updateMindTrapSequence( engineContext, levels, currentState, dt );
        }

        if (currentState == STATE_GAME)
        {
            updateSolventLabUnlockCutscene( engineContext, currentState, dt );
            updateRedPigmentDispenseCutscene( engineContext, dt );
            updateHorrorElements( engineContext, dt, levels[ engineContext.currentLevel ].folder );
        }

        g_revolverShotCooldown = std::max( 0.0f, g_revolverShotCooldown - dt );
        g_revolverRecoilTimer = std::max( 0.0f, g_revolverRecoilTimer - dt );

        for (auto &t : g_pendingShellDropTimers)
        {
            t -= dt;
        }
        for (int i = (int)g_pendingShellDropTimers.size() - 1; i >= 0; --i)
        {
            if (g_pendingShellDropTimers[ i ] <= 0.0f)
            {
                if (g_weaponSoundBuffersReady)
                {
                    playWeaponBufferedSound( g_shellDropBuffer, 90.0f );
                }
                g_pendingShellDropTimers.erase( g_pendingShellDropTimers.begin() + i );
            }
        }

        if (g_generatorStartSound && g_generatorStartSound->getStatus() == sf::SoundSource::Status::Playing)
        {
            g_generatorStartSoundTimer = std::max( 0.0f, g_generatorStartSoundTimer - dt );

            if (g_generatorStartSoundTimer <= kGeneratorStartFadeOutDuration)
            {
                const float fadeT = std::clamp( g_generatorStartSoundTimer / kGeneratorStartFadeOutDuration, 0.0f, 1.0f );
                g_generatorStartSound->setVolume( kGeneratorStartSoundVolume * fadeT );
            }

            if (g_generatorStartSoundTimer <= 0.0f)
            {
                g_generatorStartSound->stop();
                g_generatorStartSound.reset();
            }
        }
        else
        {
            g_generatorStartSoundTimer = 0.0f;
            g_generatorStartSound.reset();
        }

        g_activeWeaponSounds.erase(
            std::remove_if( g_activeWeaponSounds.begin(), g_activeWeaponSounds.end(), []( const std::shared_ptr<sf::Sound> &s ) {
                return !s || s->getStatus() == sf::SoundSource::Status::Stopped;
            } ),
            g_activeWeaponSounds.end() );

        if (g_revolverInspectCutsceneActive)
        {
            g_revolverAiming = false;
            g_revolverInspectCutsceneTimer += dt;
            engineContext.pitchOffset = 6.0f;

            const float yaw = std::atan2( engineContext.directionY, engineContext.directionX );
            const float px = engineContext.positionX + engineContext.directionX * 0.62f;
            const float py = engineContext.positionY + engineContext.directionY * 0.62f;

            if (g_revolverInspectModelIndex < 0 || g_revolverInspectModelIndex >= (int)g_worldModels.size())
            {
                g_revolverInspectModelIndex = addWorldModelInstance(
                    resolveFirstExistingAsset( { "Revolver.glb", "revolver.glb", "SurgicalKnife.glb" } ),
                    px,
                    py,
                    0.24f,
                    rgb( 185, 190, 198 ),
                    g_revolverInspectBaseYaw,
                    0.20f,
                    -0.12f,
                    true,
                    3.4f,
                    0.42f );
            }

            if (g_revolverInspectModelIndex >= 0 && g_revolverInspectModelIndex < (int)g_worldModels.size())
            {
                auto &inspectModel = g_worldModels[ g_revolverInspectModelIndex ];
                inspectModel.visible = true;
                inspectModel.x = px;
                inspectModel.y = py;
                inspectModel.heightOffset = 0.42f;
                inspectModel.yaw = g_revolverInspectBaseYaw;
                inspectModel.pitch = 0.20f;
                inspectModel.roll = -0.12f;
                inspectModel.spinYaw = true;
                inspectModel.spinSpeed = 3.4f;
            }

            if (g_revolverInspectCutsceneTimer >= kRevolverInspectCutsceneDuration && !g_dialogue.isActive())
            {
                g_revolverInspectCutsceneActive = false;
                g_revolverInspectCutsceneTimer = 0.0f;
                engineContext.pitchOffset = 0.0f;
                if (g_revolverInspectModelIndex >= 0 && g_revolverInspectModelIndex < (int)g_worldModels.size())
                {
                    g_worldModels[ g_revolverInspectModelIndex ].visible = false;
                }
                g_revolverInspectModelIndex = -1;
                g_revolverInspectBaseYaw = 0.0f;
            }
        }

        if (g_interactionAnim.active)
        {
            g_interactionAnim.t += dt;
            float ip = std::clamp( g_interactionAnim.t / std::max( 0.001f, g_interactionAnim.duration ), 0.0f, 1.0f );
            float amp = (g_interactionAnim.type == InteractionAnimType::KEY_USE) ? 3.5f : 1.2f;
            engineContext.pitchOffset = std::sin( ip * 3.14159265f ) * amp;
            if (g_interactionAnim.t >= g_interactionAnim.duration)
            {
                g_interactionAnim.active = false;
                g_interactionAnim.t = 0.0f;
                engineContext.pitchOffset = 0.0f;
            }
        }

        if (g_powerRestoreFlickerActive)
        {
            g_powerRestoreFlickerTimer += dt;
            if (g_powerRestoreFlickerTimer >= kPowerRestoreFlickerDuration)
            {
                g_powerRestoreFlickerActive = false;
                g_powerRestoreFlickerTimer = 0.0f;
            }
        }

        if (g_cutsceneController.isCameraLockActive())
        {
            engineContext.pitchOffset = g_cutsceneController.forcedPitchOffset();
        }

        if (g_levelTransition.active)
        {
            g_levelTransition.t += dt;
            if (!g_levelTransition.switched && g_levelTransition.t >= (g_levelTransition.duration * 0.5f))
            {
                handleLevelChange( engineContext, levels, g_levelTransition.targetLevel );
                g_levelTransition.switched = true;
            }
            if (g_levelTransition.t >= g_levelTransition.duration)
            {
                g_levelTransition.active = false;
                g_levelTransition.switched = false;
                g_levelTransition.t = 0.0f;
            }
        }

        if (g_wakeCutsceneActive)
        {
            g_wakeCutsceneTimer += dt;
            g_wakeCutsceneStageTimer += dt;

            auto setYaw = [&]( float yaw ) {
                engineContext.directionX = std::cos( yaw );
                engineContext.directionY = std::sin( yaw );
                engineContext.planeX = -engineContext.directionY * FOV_TAN;
                engineContext.planeY = engineContext.directionX * FOV_TAN;
                engineContext.yaw = yaw * (180.0f / 3.14159265f);
                if (engineContext.yaw > 360.0f) engineContext.yaw -= 360.0f;
                if (engineContext.yaw < 0.0f) engineContext.yaw += 360.0f;
            };

            if (g_wakeCutsceneStage == 0)
            {
                const float openDuration = 2.0f;
                const float p = std::clamp( g_wakeCutsceneStageTimer / openDuration, 0.0f, 1.0f );
                const float openEase = 1.0f - std::pow( 1.0f - p, 2.2f );
                engineContext.pitchOffset = (1.0f - openEase) * 30.0f;
                g_wakeDarknessOverride = std::clamp( 1.0f - openEase, 0.0f, 1.0f );
                g_cutsceneController.updateTurnHeadShake( dt, false, 0.0f );

                if (p >= 0.86f)
                {
                    g_wakeCutsceneStage = 1;
                    g_wakeCutsceneStageTimer = 0.0f;
                    g_wakeDarknessOverride = std::clamp( 1.0f - openEase, 0.0f, 1.0f );

                    if (!g_wakeGeneratorHumSound)
                    {
                        refreshGeneratorStartSoundBuffer( g_currentLevelFolder );
                        if (g_generatorStartBufferReady)
                        {
                            g_wakeGeneratorHumSound = playDirectionalBufferedSound(
                                g_generatorStartBuffer,
                                kMuseumGeneratorX,
                                kMuseumGeneratorY,
                                60.0f,
                                8.5f,
                                0.03f,
                                true );
                        }
                    }
                }
            }
            else if (g_wakeCutsceneStage == 1)
            {
                const float preTurnDelay = 3.45f;
                const float p = std::clamp( g_wakeCutsceneStageTimer / preTurnDelay, 0.0f, 1.0f );
                const float uDip = 4.0f * p * (1.0f - p);
                engineContext.pitchOffset = uDip * 1.3f;
                g_wakeDarknessOverride = std::clamp( 0.18f * (1.0f - p), 0.0f, 0.18f );
                g_cutsceneController.updateTurnHeadShake( dt, false, 0.0f );

                if (g_wakeCutsceneStageTimer >= preTurnDelay)
                {
                    g_wakeCutsceneStage = 2;
                    g_wakeCutsceneStageTimer = 0.0f;
                    g_wakeCutsceneTurnStartYaw = std::atan2( engineContext.directionY, engineContext.directionX );
                }
            }
            else if (g_wakeCutsceneStage == 2)
            {
                const float turnDuration = 1.60f;
                const float p = std::clamp( g_wakeCutsceneStageTimer / turnDuration, 0.0f, 1.0f );
                const float ease = p * p * (3.0f - 2.0f * p);
                const float uDip = 4.0f * p * (1.0f - p);
                const float targetYaw = std::atan2( kMuseumGeneratorY - engineContext.positionY, kMuseumGeneratorX - engineContext.positionX );
                const float d = std::atan2( std::sin( targetYaw - g_wakeCutsceneTurnStartYaw ), std::cos( targetYaw - g_wakeCutsceneTurnStartYaw ) );
                setYaw( g_wakeCutsceneTurnStartYaw + d * ease );
                engineContext.pitchOffset = uDip * 2.6f;
                g_cutsceneController.updateTurnHeadShake( dt, false, 0.0f );

                if (g_wakeMonsterModelIndex < 0)
                {
                    constexpr float kWakeMonsterX = 8.3f;
                    constexpr float kWakeMonsterY = 7.5f;
                    const float monsterYaw = std::atan2( kMuseumGeneratorY - kWakeMonsterY, kMuseumGeneratorX - kWakeMonsterX ) - 0.18f;
                    g_wakeMonsterModelIndex = addWorldModelInstance(
                        resolveFirstExistingAsset( { "Monster.glb", "monster.glb" } ),
                        kWakeMonsterX,
                        kWakeMonsterY,
                        0.96f,
                        rgb( 120, 120, 120 ),
                        monsterYaw + 0.6f,
                        0.0f,
                        0.0f,
                        false,
                        0.0f,
                        -0.05f );
                }
                if (g_wakeMonsterModelIndex >= 0 && g_wakeMonsterModelIndex < (int)g_worldModels.size())
                {
                    g_worldModels[ g_wakeMonsterModelIndex ].visible = true;
                }

                if (p >= 1.0f)
                {
                    g_wakeCutsceneStage = 3;
                    g_wakeCutsceneStageTimer = 0.0f;
                    g_wakeDarknessOverride = 0.0f;
                }
            }
            else if (g_wakeCutsceneStage == 3)
            {
                engineContext.pitchOffset = 0.0f;
                g_wakeDarknessOverride = 0.0f;
                g_cutsceneController.updateTurnHeadShake( dt, false, 0.0f );

                if (g_wakeGeneratorHumSound)
                {
                    const float fadeP = std::clamp( g_wakeCutsceneStageTimer / 1.7f, 0.0f, 1.0f );
                    g_wakeGeneratorHumSound->setVolume( 55.0f * (1.0f - fadeP) );
                }

                if (g_wakeCutsceneStageTimer >= 0.008f)
                {
                    g_wakeCutsceneStage = 4;
                    g_wakeCutsceneStageTimer = 0.0f;
                    g_wakeDarknessOverride = 1.0f;
                    if (g_wakeMonsterModelIndex >= 0 && g_wakeMonsterModelIndex < (int)g_worldModels.size())
                    {
                        g_worldModels[ g_wakeMonsterModelIndex ].visible = false;
                    }
                    if (g_wakeGeneratorHumSound)
                    {
                        g_wakeGeneratorHumSound->stop();
                        g_wakeGeneratorHumSound.reset();
                    }
                }
            }
            else if (g_wakeCutsceneStage == 4)
            {
                engineContext.pitchOffset = 0.0f;
                g_wakeDarknessOverride = 1.0f;
                g_cutsceneController.updateTurnHeadShake( dt, false, 0.0f );

                if (g_wakeCutsceneStageTimer >= 1.60f)
                {
                    g_wakeCutsceneStage = 5;
                    g_wakeCutsceneStageTimer = 0.0f;
                    g_wakeCutsceneReturnStartYaw = std::atan2( engineContext.directionY, engineContext.directionX );
                }
            }
            else
            {
                const float returnDuration = 4.00f;
                const float p = std::clamp( g_wakeCutsceneStageTimer / returnDuration, 0.0f, 1.0f );
                const float ease = p * p * (3.0f - 2.0f * p);
                const float uDip = 4.0f * p * (1.0f - p);
                g_wakeDarknessOverride = 1.0f + (0.14f - 1.0f) * ease;
                engineContext.pitchOffset = uDip * 1.1f;
                g_cutsceneController.updateTurnHeadShake( dt, false, 0.0f );

                const float yawDelta = std::atan2( std::sin( g_wakeCutsceneInitialYaw - g_wakeCutsceneReturnStartYaw ), std::cos( g_wakeCutsceneInitialYaw - g_wakeCutsceneReturnStartYaw ) );
                setYaw( g_wakeCutsceneReturnStartYaw + yawDelta * ease );

                if (p >= 1.0f)
                {
                    g_wakeCutsceneActive = false;
                    g_wakeCutsceneTimer = 0.0f;
                    g_wakeCutsceneStage = 0;
                    g_wakeCutsceneStageTimer = 0.0f;
                    g_wakeDarknessOverride = -1.0f;
                    g_cutsceneController.clearHeadShake();
                    engineContext.pitchOffset = 0.0f;
                }
            }

            if (!g_wakeCutsceneActive)
            {
                if (g_wakeMonsterModelIndex >= 0 && g_wakeMonsterModelIndex < (int)g_worldModels.size())
                {
                    g_worldModels[ g_wakeMonsterModelIndex ].visible = false;
                }
                g_wakeMonsterModelIndex = -1;
                if (g_wakeGeneratorHumSound)
                {
                    g_wakeGeneratorHumSound->stop();
                    g_wakeGeneratorHumSound.reset();
                }
            }
        }
        SDL_SetWindowRelativeMouseMode(engineContext.window, true);

        while (SDL_PollEvent( &ev ))
        {
            if (ev.type == SDL_EVENT_QUIT)
            {
                running = false;
            }

            if (currentState == STATE_MENU)
            {
                if (ev.type == SDL_EVENT_KEY_DOWN)
                {
                    switch (ev.key.key)
                    {
                    case SDLK_ESCAPE:
                        currentState = STATE_GAME;
                        break;
                    case SDLK_UP:
                        currentMenuSelection = (currentMenuSelection - 1 + numMenuOptions) % numMenuOptions;
                        break;
                    case SDLK_DOWN:
                        currentMenuSelection = (currentMenuSelection + 1) % numMenuOptions;
                        break;
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER:
                        if (currentMenuSelection == 0) // "Play"
                        {
                   

                            startNewMuseumRun( engineContext, levels );
                            currentState = STATE_GAME;
                        }
                       else if (currentMenuSelection == 9) // "Quit"
                        {
                            exit( 0 );
                        }
                        break;
                    case SDLK_LEFT:
                        if (currentMenuSelection == 1) // Music Toggle
                        {
                            config::useMusic = !config::useMusic;
                        }
                        else if (currentMenuSelection == 2) // Volume Down
                        {
                            musicVolume = std::max( 0.f, musicVolume - 10.f );
                            setMusicVolume( musicVolume );
                        }
                        else if (currentMenuSelection == 3)
                        {
							config::viewBobbing = !config::viewBobbing;
                        }
                        else if (currentMenuSelection == 4)
                        {
                            config::antiAliasingMode = (getAntiAliasingMode() + 2) % 3;
                            applyPresentationFilter( engineContext );
                        }
                        else if (currentMenuSelection == 5)
                        {
                            config::modelQualityPreset = (config::modelQualityPreset + 2) % 3;
                            applyQualityPresetFromConfig();
                        }
                        else if (currentMenuSelection == 6)
                        {
                            config::gpuRenderMode = (config::gpuRenderMode + 2) % 3;
                        }
                        else if (currentMenuSelection == 7)
                        {
                            g_multithreadingEnabled = !g_multithreadingEnabled;
                        }
                        else if (currentMenuSelection == 8)
                        {
                            config::schoolMode = !config::schoolMode;
                            if (config::schoolMode)
                            {
                                g_pendingShellDropTimers.clear();
                            }
                        }
                        break;
                    case SDLK_RIGHT:
                        if (currentMenuSelection == 1) // Music Toggle
                        {
                            config::useMusic = !config::useMusic;
                        }
                        else if (currentMenuSelection == 2) // Volume Up
                        {
                            musicVolume = std::min( 100.f, musicVolume + 10.f );
                            setMusicVolume( musicVolume );
                        }
                        else if (currentMenuSelection == 3)
						{
							config::viewBobbing = !config::viewBobbing;
						}
                        else if (currentMenuSelection == 4)
                        {
                            config::antiAliasingMode = (getAntiAliasingMode() + 1) % 3;
                            applyPresentationFilter( engineContext );
                        }
                        else if (currentMenuSelection == 5)
                        {
                            config::modelQualityPreset = (config::modelQualityPreset + 1) % 3;
                            applyQualityPresetFromConfig();
                        }
                        else if (currentMenuSelection == 6)
                        {
                            config::gpuRenderMode = (config::gpuRenderMode + 1) % 3;
                        }
                        else if (currentMenuSelection == 7)
                        {
                            g_multithreadingEnabled = !g_multithreadingEnabled;
                        }
                        else if (currentMenuSelection == 8)
                        {
                            config::schoolMode = !config::schoolMode;
                            if (config::schoolMode)
                            {
                                g_pendingShellDropTimers.clear();
                            }
                        }
                        break;
                    }
                }
            }
            else if (currentState == STATE_GAME)
            {
                if (g_levelEditorMode && ev.type == SDL_EVENT_MOUSE_MOTION)
                {
                    if (g_editorSelectedModel >= 0 && g_editorSelectedModel < (int)g_worldModels.size() && g_worldModels[ g_editorSelectedModel ].editorPlaced)
                    {
                        constexpr float kMouseMoveSensitivity = 0.004f;
                        float rightX = -engineContext.directionY;
                        float rightY = engineContext.directionX;
                        float dx = (float)ev.motion.xrel * kMouseMoveSensitivity * rightX
                            + (float)(-ev.motion.yrel) * kMouseMoveSensitivity * engineContext.directionX;
                        float dy = (float)ev.motion.xrel * kMouseMoveSensitivity * rightY
                            + (float)(-ev.motion.yrel) * kMouseMoveSensitivity * engineContext.directionY;
                        nudgeSelectedEditorModel( dx, dy, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
                    }
                    continue;
                }
                else if (!g_levelEditorMode && ev.type == SDL_EVENT_MOUSE_MOTION) {
                    const float lookSpeedY = 1.0f; 
                    engineContext.pitchOffset -= ev.motion.yrel * lookSpeedY;

                    engineContext.pitchOffset = std::clamp(engineContext.pitchOffset, -250.0f, 250.0f);
                }

                if (g_levelEditorMode && ev.type == SDL_EVENT_MOUSE_WHEEL)
                {
                    if (g_editorSelectedModel >= 0 && g_editorSelectedModel < (int)g_worldModels.size() && g_worldModels[ g_editorSelectedModel ].editorPlaced)
                    {
                        constexpr float kMouseWheelZStep = 0.05f;
                        nudgeSelectedEditorModel( 0.0f, 0.0f, (float)ev.wheel.y * kMouseWheelZStep, 0.0f, 0.0f, 0.0f, 0.0f );
                    }
                    continue;
                }

                if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN && g_endGameStateAllowRevolver)
                {
                    const bool inputBlocked = g_codeEntryActive || g_notesOpen || g_caveQuizActive || g_levelTransition.active || g_interactionAnim.active || g_levelEditorMode || g_cutsceneController.isCameraLockActive() || g_revolverInspectCutsceneActive || g_wakeCutsceneActive;

                    if (ev.button.button == SDL_BUTTON_RIGHT)
                    {
                        if (!inputBlocked && g_combatState.active && g_combatState.hasRevolver)
                        {
                            g_revolverAiming = true;
                        }
                        continue;
                    }

                    if (ev.button.button == SDL_BUTTON_LEFT && g_endGameStateAllowRevolver && g_showHeldWeapon)
                    {
                        if (!inputBlocked && g_combatState.active && g_combatState.hasRevolver)
                        {
                            if (g_revolverShotCooldown <= 0.0f)
                            {
                                if (g_combatState.loadedAmmo > 0)
                                {
                                    g_combatState.loadedAmmo--;
                                    g_revolverShotCooldown = 0.32f;
                                    g_revolverRecoilTimer = kRevolverRecoilDuration;
                                    playRevolverShotSequence( levels[ engineContext.currentLevel ].folder );
                                }
                              
                            }
                        }
                        continue;
                    }
                }

                if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP && g_endGameStateAllowRevolver)
                {
                    if (ev.button.button == SDL_BUTTON_RIGHT)
                    {
                        g_revolverAiming = false;
                        continue;
                    }
                }

                if (g_solventCoolerEntryActive && ev.type == SDL_EVENT_TEXT_INPUT)
                {
                    bool submitFromTextEvent = false;
                    for (const char *p = ev.text.text; *p != '\0'; ++p)
                    {
                        if (*p == '\r' || *p == '\n')
                        {
                            submitFromTextEvent = true;
                            continue;
                        }

                        if ((int)g_solventCoolerBuffer.size() >= 52) break;
                        const char c = char( std::toupper( unsigned char( *p ) ) );
                        const bool allowed =
                            (c >= 'A' && c <= 'Z') ||
                            (c >= '0' && c <= '9') ||
                            c == ' ' || c == '\'';
                        if (allowed)
                        {
                            g_solventCoolerBuffer.push_back( c );
                        }
                    }

                    if (submitFromTextEvent)
                    {
                        if (isSolventCoolerPhraseCorrect( g_solventCoolerBuffer ))
                        {
                            g_solventCoolerEntryActive = false;
                            g_solventCoolerBuffer.clear();
                            SDL_StopTextInput( engineContext.window );
                            showAccessPopup( "Dispensing pigment.", 2200 );
                            startRedPigmentDispenseCutscene( engineContext );
                        }
                        else
                        {
                            showAccessPopup( "Phrase rejected.", 1800 );
                        }
                    }

                    continue;
                }

                if (ev.type == SDL_EVENT_KEY_DOWN)
                {
                    if (g_levelTransition.active || g_wakeCutsceneActive)
                    {
                        continue;
                    }

                    if (g_cutsceneController.isCameraLockActive())
                    {
                        continue;
                    }

                    if (g_revolverInspectCutsceneActive)
                    {
                        continue;
                    }

                    if (g_solventCoolerEntryActive)
                    {
                        if (ev.key.key == SDLK_ESCAPE)
                        {
                            g_solventCoolerEntryActive = false;
                            g_solventCoolerBuffer.clear();
                            SDL_StopTextInput( engineContext.window );
                        }
                        else if (ev.key.key == SDLK_BACKSPACE)
                        {
                            if (!g_solventCoolerBuffer.empty())
                            {
                                g_solventCoolerBuffer.pop_back();
                            }
                        }
                        else if (ev.key.key == SDLK_RETURN || ev.key.key == SDLK_KP_ENTER || ev.key.scancode == SDL_SCANCODE_RETURN || ev.key.scancode == SDL_SCANCODE_KP_ENTER)
                        {
                            if (isSolventCoolerPhraseCorrect( g_solventCoolerBuffer ))
                            {
                                g_solventCoolerEntryActive = false;
                                g_solventCoolerBuffer.clear();
                                SDL_StopTextInput( engineContext.window );
                                showAccessPopup( "Authorization accepted.", 2200 );
                                startRedPigmentDispenseCutscene( engineContext );
                            }
                            else
                            {
                                showAccessPopup( "Phrase rejected.", 1800 );
                            }
                        }
                        continue;
                    }

                    if (ev.key.key == SDLK_F2)
                    {
                        g_levelEditorMode = !g_levelEditorMode;
                        if (g_levelEditorMode)
                        {
                            g_notesOpen = false;
                            g_codeEntryActive = false;
                            g_caveQuizActive = false;
                            g_editorSelectedModel = findNearestEditorModel( engineContext, 6.0f );
                            showAccessPopup( "Level editor enabled." );
                        }
                        else
                        {
                            showAccessPopup( "Level editor disabled." );
                        }
                        continue;
                    }

                    if (ev.key.key == SDLK_F3)
                    {
                        g_unlockAllDoorsOverride = true;
                        applyUnlockAllDoorsOverride( engineContext );
                        showAccessPopup( "Door override enabled", 2200 );
                        continue;
                    }

                    if (g_levelEditorMode)
                    {
                        const bool fine = (ev.key.mod & SDL_KMOD_SHIFT) != 0;
                        const float moveStep = fine ? 0.01f : 0.05f;
                        const float zStep = fine ? 0.01f : 0.05f;
                        const float rotStep = fine ? 0.05f : 0.18f;
                        const float sizeStep = fine ? 0.02f : 0.08f;

                        if (ev.key.key == SDLK_LEFTBRACKET)
                        {
                            const int n = (int)editorAssetCatalog().size();
                            g_editorAssetIndex = (g_editorAssetIndex - 1 + n) % n;
                        }
                        else if (ev.key.key == SDLK_RIGHTBRACKET)
                        {
                            const int n = (int)editorAssetCatalog().size();
                            g_editorAssetIndex = (g_editorAssetIndex + 1) % n;
                        }
                        else if (ev.key.key == SDLK_RETURN || ev.key.key == SDLK_KP_ENTER)
                        {
                            int idx = placeEditorModel( engineContext );
                            if (idx >= 0) showAccessPopup( "Placed " + editorAssetCatalog()[ g_editorAssetIndex ].label + ".", 1200 );
                        }
                        else if (ev.key.key == SDLK_TAB)
                        {
                            g_editorSelectedModel = findNearestEditorModel( engineContext, 6.0f );
                            if (g_editorSelectedModel < 0) showAccessPopup( "No nearby editor model." );
                        }
                        else if (ev.key.key == SDLK_DELETE || ev.key.key == SDLK_BACKSPACE)
                        {
                            deleteSelectedEditorModel();
                            showAccessPopup( "Deleted selected model.", 1200 );
                        }
                        else if ((ev.key.mod & SDL_KMOD_CTRL) && (ev.key.key == SDLK_S))
                        {
                            saveEditorModelsForLevel();
                            showAccessPopup( "Editor models saved.", 1500 );
                        }
                        else if (ev.key.key == SDLK_W)
                        {
                            nudgeSelectedEditorModel( 0.0f, -moveStep, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
                        }
                        else if (ev.key.key == SDLK_S)
                        {
                            nudgeSelectedEditorModel( 0.0f, moveStep, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
                        }
                        else if (ev.key.key == SDLK_A)
                        {
                            nudgeSelectedEditorModel( -moveStep, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
                        }
                        else if (ev.key.key == SDLK_D)
                        {
                            nudgeSelectedEditorModel( moveStep, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
                        }
                        else if (ev.key.key == SDLK_R)
                        {
                            nudgeSelectedEditorModel( 0.0f, 0.0f, zStep, 0.0f, 0.0f, 0.0f, 0.0f );
                        }
                        else if (ev.key.key == SDLK_F)
                        {
                            nudgeSelectedEditorModel( 0.0f, 0.0f, -zStep, 0.0f, 0.0f, 0.0f, 0.0f );
                        }
                        else if (ev.key.key == SDLK_Q)
                        {
                            nudgeSelectedEditorModel( 0.0f, 0.0f, 0.0f, -rotStep, 0.0f, 0.0f, 0.0f );
                        }
                        else if (ev.key.key == SDLK_E)
                        {
                            nudgeSelectedEditorModel( 0.0f, 0.0f, 0.0f, rotStep, 0.0f, 0.0f, 0.0f );
                        }
                        else if (ev.key.key == SDLK_Z)
                        {
                            nudgeSelectedEditorModel( 0.0f, 0.0f, 0.0f, 0.0f, -rotStep, 0.0f, 0.0f );
                        }
                        else if (ev.key.key == SDLK_X)
                        {
                            nudgeSelectedEditorModel( 0.0f, 0.0f, 0.0f, 0.0f, rotStep, 0.0f, 0.0f );
                        }
                        else if (ev.key.key == SDLK_C)
                        {
                            nudgeSelectedEditorModel( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -rotStep, 0.0f );
                        }
                        else if (ev.key.key == SDLK_V)
                        {
                            nudgeSelectedEditorModel( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, rotStep, 0.0f );
                        }
                        else if (ev.key.key == SDLK_MINUS)
                        {
                            nudgeSelectedEditorModel( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -sizeStep );
                        }
                        else if (ev.key.key == SDLK_EQUALS)
                        {
                            nudgeSelectedEditorModel( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, sizeStep );
                        }
                        continue;
                    }

                    if (g_notesOpen)
                    {
                        if (ev.key.key == SDLK_N || ev.key.key == SDLK_ESCAPE)
                        {
                            g_notesOpen = false;
                        }
                        else if (!g_foundNotes.empty())
                        {
                            if (ev.key.key == SDLK_UP)
                            {
                                g_notesSelected = std::max( 0, g_notesSelected - 1 );
                                g_notesBodyScroll = 0;
                            }
                            else if (ev.key.key == SDLK_DOWN)
                            {
                                g_notesSelected = std::min( (int)g_foundNotes.size() - 1, g_notesSelected + 1 );
                                g_notesBodyScroll = 0;
                            }
                            else if (ev.key.key == SDLK_PAGEUP || ev.key.key == SDLK_W)
                            {
                                g_notesBodyScroll = std::max( 0, g_notesBodyScroll - 2 );
                            }
                            else if (ev.key.key == SDLK_PAGEDOWN || ev.key.key == SDLK_S)
                            {
                                g_notesBodyScroll += 2;
                            }
                            else if (ev.key.key == SDLK_HOME)
                            {
                                g_notesBodyScroll = 0;
                            }
                        }
                        continue;
                    }

                    if (g_interactionAnim.active)
                    {
                        continue;
                    }

                    if (g_caveQuizActive)
                    {
                        if (ev.key.key == SDLK_ESCAPE)
                        {
                            g_caveQuizActive = false;
                        }
                        else if (ev.key.key >= SDLK_1 && ev.key.key <= SDLK_4)
                        {
                            int selected = (int)(ev.key.key - SDLK_1);
                            if (!g_caveQuiz.empty() && g_caveQuizQuestionIndex >= 0 && g_caveQuizQuestionIndex < (int)g_caveQuiz.size())
                            {
                                const auto &q = g_caveQuiz[ g_caveQuizQuestionIndex ];
                                if (selected == q.correctOption)
                                {
                                    g_caveQuizQuestionIndex++;
                                    if (g_caveQuizQuestionIndex >= (int)g_caveQuiz.size())
                                    {
                                        g_caveQuizPassed = true;
                                        g_caveQuizActive = false;
                                        showAccessPopup( "The warden statue steps aside. You may proceed.", 2600 );
                                        currentState = STATE_ENDING;
                                    }
                                    else
                                    {
                                        showAccessPopup( "Correct.", 1200 );
                                    }
                                }
                                else
                                {
                                    playFailedDoorOpen( levels[ engineContext.currentLevel ].folder );
                                    showAccessPopup( "Incorrect.", 2200 );
                                }
                            }
                        }
                        continue;
                    }

                    if (g_codeEntryActive)
                    {
                        if (g_symbolEntryIndex >= 0)
                        {
                            if (ev.key.key == SDLK_LEFT)
                            {
                                g_symbolFocus = (g_symbolFocus - 1 + 3) % 3;
                            }
                            else if (ev.key.key == SDLK_RIGHT)
                            {
                                g_symbolFocus = (g_symbolFocus + 1) % 3;
                            }
                            else if (ev.key.key == SDLK_UP)
                            {
                                g_symbolState[g_symbolFocus] = (g_symbolState[g_symbolFocus] + 1) % 4;
                            }
                            else if (ev.key.key == SDLK_DOWN)
                            {
                                g_symbolState[g_symbolFocus] = (g_symbolState[g_symbolFocus] - 1 + 4) % 4;
                            }
                            else if (ev.key.key == SDLK_ESCAPE)
                            {
                                g_codeEntryActive = false;
                                g_symbolEntryIndex = -1;
                            }
                            else if (ev.key.key == SDLK_RETURN || ev.key.key == SDLK_KP_ENTER)
                            {
                                auto &sym = g_symbols[ g_symbolEntryIndex ];
                                if (g_symbolState[0] == sym.targetCombo[0] && g_symbolState[1] == sym.targetCombo[1] && g_symbolState[2] == sym.targetCombo[2])
                                {
                                    sym.solved = true;
                                  //  showAccessPopup( sym.name + " solved! Obtained " + sym.rewardKey, 2800 );
                                    g_playerKeys.insert( sym.rewardKey );
                                    triggerInteractionAnim( InteractionAnimType::ITEM_PICKUP, "ACQUIRED " + sym.rewardKey, 0.8f );
                                    g_dialogue.start( {
                                        {"How do people get these keys normally?", 2.8f}
                                        } );
                                }
                                else
                                {
                                    showAccessPopup( "Pedestal is jammed." );
                                }
                                g_codeEntryActive = false;
                                g_symbolEntryIndex = -1;
                            }
                        }
                        else
                        {
                            if (ev.key.key >= SDLK_0 && ev.key.key <= SDLK_9 && g_codeEntryBuffer.size() < 4)
                            {
                                g_codeEntryBuffer.push_back( char( '0' + (ev.key.key - SDLK_0) ) );
                            }
                            else if (ev.key.key >= SDLK_KP_0 && ev.key.key <= SDLK_KP_9 && g_codeEntryBuffer.size() < 4)
                            {
                                g_codeEntryBuffer.push_back( char( '0' + (ev.key.key - SDLK_KP_0) ) );
                            }
                            else if (ev.key.key == SDLK_BACKSPACE && !g_codeEntryBuffer.empty())
                            {
                                g_codeEntryBuffer.pop_back();
                            }
                            else if (ev.key.key == SDLK_ESCAPE)
                            {
                                g_codeEntryActive = false;
                                g_codeEntryBuffer.clear();
                                g_codeEntryLockIndex = -1;
                                g_safeEntryIndex = -1;
                            }
                            else if ((ev.key.key == SDLK_RETURN || ev.key.key == SDLK_KP_ENTER))
                            {
                                if (g_codeEntryLockIndex >= 0 && g_codeEntryLockIndex < (int)g_roomLocks.size())
                                {
                                    auto &lock = g_roomLocks[ g_codeEntryLockIndex ];
                                    if (g_codeEntryBuffer == lock.requirement)
                                    {
                                        lock.unlocked = true;
                                        int idx = lock.ty * engineContext.map.width + lock.tx;
                                        if ((unsigned)idx < (unsigned)engineContext.map.tiles.size() && engineContext.map.tiles[ idx ] == 2)
                                        {
                                            engineContext.map.tiles[ idx ] = 0;
                                        }
                                        showAccessPopup( lock.roomName + " unlocked.", 1800 );
                                        playDoorCreak(levels[engineContext.currentLevel].folder);
                                        triggerInteractionAnim( InteractionAnimType::DOOR_USE, "ACCESS CODE ACCEPTED", 0.7f );
                                    }
                                    else
                                    {

                                        playFailedDoorOpen(levels[engineContext.currentLevel].folder);
                                        showAccessPopup( "Wrong code" );
                                    }
                                }
                                else if (g_safeEntryIndex >= 0 && g_safeEntryIndex < (int)g_safes.size())
                                {
                                    auto &safe = g_safes[ g_safeEntryIndex ];
                                    if (g_codeEntryBuffer == safe.code)
                                    {
                                        safe.solved = true;
                                        showAccessPopup( safe.safeName + " unlocked! Obtained " + safe.rewardKey, 2800 );
                                        g_playerKeys.insert( safe.rewardKey );
                                        triggerInteractionAnim( InteractionAnimType::ITEM_PICKUP, "ACQUIRED " + safe.rewardKey, 0.8f );
                                    }
                                    else
                                    {
                                        showAccessPopup( "Safe combination incorrect." );
                                    }
                                }
                                g_codeEntryActive = false;
                                g_codeEntryBuffer.clear();
                                g_codeEntryLockIndex = -1;
                                g_safeEntryIndex = -1;
                            }
                        }
                        continue;
                    }

                    if (ev.key.key == SDLK_ESCAPE)
                    {
                        currentState = STATE_MENU;
                    }
                    else if (ev.key.key == SDLK_N)
                    {
                        g_notesOpen = !g_notesOpen;
                        if (g_notesOpen)
                        {
                            g_notesSelected = std::clamp( g_notesSelected, 0, std::max( 0, (int)g_foundNotes.size() - 1 ) );
                            g_notesBodyScroll = 0;
                        }
                    }
                    else if (ev.key.key == SDLK_H && g_endGameStateAllowRevolver)
                    {
                        if (g_combatState.active && g_combatState.hasRevolver)
                        {
                            g_showHeldWeapon = !g_showHeldWeapon;
                            //showAccessPopup( g_showHeldWeapon ? "Revolver shown." : "Revolver hidden.", 1200 );
                        }
                    }
                    else if (ev.key.key == SDLK_F1)
                    {
                        engineContext.showHelp = !engineContext.showHelp;
                    }
                    else if (ev.key.scancode == SDL_SCANCODE_E)
                    {
                        if (engineContext.currentLevel == Levels::MUSEUM_UPPER && isPlayerNearSolventCooler( engineContext ) &&
                            !g_playerKeys.contains( "RED PIGMENT" ) && !g_redPigmentDispenseCutsceneActive)
                        {
                            g_solventCoolerEntryActive = true;
                            g_solventCoolerBuffer.clear();
                            SDL_StartTextInput( engineContext.window );
                           // showAccessPopup( "Solvent Cooler awaiting authorization phrase.", 1800 );
                            continue;
                        }

                        if (engineContext.currentLevel == Levels::MUSEUM && isPlayerNearPoint( engineContext, kUpperEntryX, kUpperEntryY, kUpperEntryRadius ))
                        {
                            triggerInteractionAnim( InteractionAnimType::DOOR_USE, "ASCENDING TO UPPER GALLERY", 0.9f );
                            beginLevelTransition( Levels::MUSEUM_UPPER, 1.0f );
                            continue;
                        }

                        if (engineContext.currentLevel == Levels::MUSEUM_UPPER && isPlayerNearPoint( engineContext, 3.5f, 9.3f, 1.1f ))
                        {
                            triggerInteractionAnim( InteractionAnimType::DOOR_USE, "RETURNING TO GROUND FLOOR", 0.9f );
                            beginLevelTransition( Levels::MUSEUM, 1.0f );
                            continue;
                        }


                  
                        if (engineContext.currentLevel == Levels::MUSEUM && isPlayerNearGasCan( engineContext ))
                        {
                            g_gasCanCollected = true;
                            if (g_gasCanModelIndex >= 0 && g_gasCanModelIndex < (int)g_worldModels.size())
                            {
                                g_worldModels[ g_gasCanModelIndex ].visible = false;
                            }
                            showAccessPopup( "Acquired gas can.", 1700 );
                            triggerInteractionAnim( InteractionAnimType::ITEM_PICKUP, "ACQUIRED GAS CAN", 0.55f );
                            continue;
                        }

                        if (engineContext.currentLevel == Levels::MUSEUM && isPlayerNearGenerator( engineContext ))
                        {
                            if (g_generatorFueled)
                            {
                               // showAccessPopup( "Generator is fueled.", 1200 );
                                continue;
                            }

                            if (!g_gasCanCollected)
                            {
                                if (!g_generatorNeedsGasLinePlayed)
                                {
                                    g_generatorNeedsGasLinePlayed = true;
                                    g_dialogue.start( {
                                        {"Looks like it needs some gas", 2.4f}
                                        } );
                                }
                                else
                                {
                                    showAccessPopup( "Generator is empty.", 1800 );
                                }
                                continue;
                            }

                            g_generatorFueled = true;
                            g_gasCanCollected = false;
                            g_powerRestoreFlickerActive = true;
                            g_powerRestoreFlickerTimer = 0.0f;
                            playGeneratorStartSound( levels[ engineContext.currentLevel ].folder );

                            if (g_generatorModelIndex >= 0 && g_generatorModelIndex < (int)g_worldModels.size())
                            {
                                g_worldModels[ g_generatorModelIndex ].tint = rgb( 225, 225, 205 );
                            }

                            triggerInteractionAnim( InteractionAnimType::KEY_USE, "RESTORING POWER", 0.85f );
                            g_dialogue.start( {
                                {"That should get the lights back on", 2.1f}
                                } );
                            showAccessPopup("Power has been restored to all levels", 3200);

                            continue;
                        }

                        int nearbyKey = getNearbyKeyPickup( engineContext );
                        if (nearbyKey >= 0)
                        {
                            auto &k = g_keyPickups[ nearbyKey ];
                            k.collected = true;
                            g_playerKeys.insert( k.keyName );
                            if (k.propIndex >= 0 && k.propIndex < (int)engineContext.props.size())
                            {
                                engineContext.props[ k.propIndex ].scale = 0.0f;
                            }
                            if (k.modelIndex >= 0 && k.modelIndex < (int)g_worldModels.size())
                            {
                                g_worldModels[ k.modelIndex ].visible = false;
                            }
                            showAccessPopup( "Acquired " + k.keyName + ".", 1800 );
                            playPickup(levels[engineContext.currentLevel].folder);
                            triggerInteractionAnim( InteractionAnimType::ITEM_PICKUP, "ACQUIRED " + k.keyName, 0.50f );
                            hideExactInteractableVisualsAt(engineContext, k.x, k.y);
                            continue;
                        }

                        if (isRevolverNearby( engineContext ))
                        {
                            g_revolverPickup.collected = true;
                            if (g_revolverPickup.modelIndex >= 0 && g_revolverPickup.modelIndex < (int)g_worldModels.size())
                            {
                                g_worldModels[ g_revolverPickup.modelIndex ].visible = false;
                            }
                            
                            g_revolverPickup.modelIndex = -1;
                            g_combatState.active = true;
                            g_combatState.hasRevolver = true;
                            g_combatState.loadedAmmo = 6;
                            g_combatState.reserveAmmo = 0;
                            g_revolverInspectCutsceneActive = true;
                            g_revolverInspectCutsceneTimer = 0.0f;
                            g_revolverInspectBaseYaw = std::atan2( engineContext.directionY, engineContext.directionX ) + kRevolverFacingYawOffset;
                            g_revolverAiming = false;
                            g_dialogue.start( {
                                {"Why would the director have this?", 2.5f}
                                } );
                            showAccessPopup( "Revolver acquired.", 1700 );
                            triggerInteractionAnim( InteractionAnimType::ITEM_PICKUP, "REVOLVER ACQUIRED", 0.6f );
                            continue;
                        }

                        int nearbyNote = getNearbyClueNote( engineContext );
                        if (nearbyNote >= 0)
                        {
                             auto &n = g_clueNotes[ nearbyNote ];
                             n.collected = true;
                             g_foundNotes.push_back( nearbyNote );
                             g_notesCollectedRun++;
                             if (n.propIndex >= 0 && n.propIndex < (int)engineContext.props.size())
                             {
                                engineContext.props[ n.propIndex ].scale = 0.0f;
                            }
                             if (n.modelIndex >= 0 && n.modelIndex < (int)g_worldModels.size())
                             {
                                 g_worldModels[ n.modelIndex ].visible = false;
                             }
                             showAccessPopup( "Collected note: " + n.title, 2200 );
 							 playPaperRustle( levels[ engineContext.currentLevel ].folder );
                             triggerInteractionAnim( InteractionAnimType::NOTE_COLLECT, "READING NOTE", 0.5f );

                            hideExactInteractableVisualsAt(engineContext, n.x, n.y);
                            if (g_cutsceneController.canTriggerPhoneCutscene() &&
                                engineContext.currentLevel == Levels::MUSEUM &&
                                n.title == "Missed Calls")
                            {
                                g_cutsceneController.triggerPhoneCutscene( engineContext, g_dialogue, phoneCutsceneAsset );
                            }

                            if (engineContext.currentLevel == Levels::CAVE)
                            {
                                if (n.title == "Last Journal Fragment")
                                {
                                    g_caveFinalNoteCollected = true;
                                    showAccessPopup( "Journal fragment recovered. Return to the warden statue.", 2400 );
                                }
                            }
                            continue;
                        }

                        if (engineContext.currentLevel == Levels::CAVE && isPlayerNearCaveStatue( engineContext ))
                        {
                            if (!g_caveFinalNoteCollected)
                            {
                                showAccessPopup( "The statue whispers: Bring me the final journal fragment.", 2400 );
                            }
                            else if (g_caveQuizPassed)
                            {
                                currentState = STATE_ENDING;
                            }
                            else
                            {
                                g_caveQuizQuestionIndex = 0;
                                g_caveQuizActive = true;
                            }
                            continue;
                        }


                        int id = pickArtworkUnderCrosshair( engineContext );
                        if (id < 0) id = findNearestArtwork( engineContext ); // optional fallback

                        if (id >= 0) // We are looking at a valid artwork
                        {
                            if (engineContext.placardOpen && engineContext.openArtId == id)
                            {
                                // Placard is open -> close it, open journal
                                engineContext.placardOpen = false;
                                engineContext.lastPlacardTick = SDL_GetTicks(); // Refresh timer
                            }
                            else
                            {
         
                                engineContext.openArtId = id;
                                engineContext.placardOpen = true;
                                engineContext.lastPlacardTick = SDL_GetTicks();
								mesuemObjectives.markViewed(id);


                            }
                        }
                        else // Not looking at any art
                        {
                            // Close whatever is open
                            engineContext.placardOpen = false;
                            engineContext.openArtId = -1;
                            if (engineContext.inRangeOfStatue && !engineContext.statueChatActive)
                            {
                                engineContext.statueChatActive = true;
                                engineContext.statueChatStartTick = SDL_GetTicks();
                            }
                        }
                    }
                    else if (ev.key.scancode == SDL_SCANCODE_M) {
                     //   currentState = STATE_ENDING;
                    }
                    else if (ev.key.scancode == SDL_SCANCODE_F)
                    {
                        if (isPlayerNearDirectorDesk( engineContext ) && !g_directorDeskUnlocked)
                        {
                            if (g_playerKeys.contains( "DIRECTOR'S KEY" ))
                            {
                                g_directorDeskUnlocked = true;
                                if (!g_combatState.hasRevolver)
                                {
                                    g_combatState.active = true;
                                    g_combatState.hasRevolver = true;
                                    g_combatState.loadedAmmo = 6;
                                    g_combatState.reserveAmmo = 0;
                                    g_revolverInspectCutsceneActive = true;
                                    g_revolverInspectCutsceneTimer = 0.0f;
                                    g_revolverInspectBaseYaw = std::atan2( engineContext.directionY, engineContext.directionX ) + kRevolverFacingYawOffset;
                                    g_revolverAiming = false;
                                    g_dialogue.start( {
                                        {"Why would the director have this? Eh, Doesn't matter.", 2.5f}
                                        } );
                                    showAccessPopup( "Director's Desk unlocked", 1900 );
                                    triggerInteractionAnim( InteractionAnimType::ITEM_PICKUP, "REVOLVER ACQUIRED", 0.6f );
                                }
                                else
                                {
                                    showAccessPopup( "Director's Desk already searched.", 1800 );
                                }
                            }
                            else
                            {
                                showAccessPopup( "The desk is locked", 2200 );
                            }
                            continue;
                        }

                        int tx = 0;
                        int ty = 0;
                        if (engineContext.currentLevel == Levels::MUSEUM_UPPER && getDoorAheadTile( engineContext, tx, ty ) && isRestorationGateDoorTile( tx, ty ) && !g_restorationWingUnlocked)
                        {
                            if (hasRestorationPigments())
                            {
                                g_restorationWingUnlocked = true;
                                int idx = ty * engineContext.map.width + tx;
                                if ((unsigned)idx < (unsigned)engineContext.map.tiles.size() && engineContext.map.tiles[ idx ] == 2)
                                {
                                    engineContext.map.tiles[ idx ] = 0;
                                }
                                showAccessPopup( "Access override.", 2400 );
                                triggerInteractionAnim( InteractionAnimType::KEY_USE, "RESTORATION WING UNSEALED", 1.0f );
                                startSolventLabUnlockCutscene( engineContext );
                            }
                            else
                            {
                                showAccessPopup( "Seal active.", 2300 );
                            }
                            continue;
                        }

                        int nearbySafe = getNearbySafe( engineContext );
                        if (engineContext.currentLevel == Levels::MUSEUM && nearbySafe >= 0)
                        {
                            g_codeEntryActive = true;
                            g_safeEntryIndex = nearbySafe;
                            g_codeEntryLockIndex = -1;
                            g_symbolEntryIndex = -1;
                            g_codeEntryBuffer.clear();
                            continue;
                        }

                        int nearbySymbol = getNearbySymbol( engineContext );
                        if (engineContext.currentLevel == Levels::MUSEUM && nearbySymbol >= 0)
                        {
                            g_codeEntryActive = true;
                            g_symbolEntryIndex = nearbySymbol;
                            g_codeEntryLockIndex = -1;
                            g_safeEntryIndex = -1;
                            g_symbolFocus = 0;
                            g_codeEntryBuffer.clear();
                            continue;
                        }

                        if (isMuseumLikeLevel( engineContext.currentLevel ) && getDoorAheadTile( engineContext, tx, ty ))
                        {
                            int lockIndex = findDoorLockIndex( engineContext.currentLevel, tx, ty );
                            if (lockIndex >= 0 && !g_roomLocks[ lockIndex ].unlocked)
                            {
                                if (!g_firstLockedDoorDialogueShown)
                                {
                                    g_dialogue.start( { { "Why is this locked?", 2.0f } } );
                                    g_firstLockedDoorDialogueShown = true;
                                }

                                auto &lock = g_roomLocks[ lockIndex ];
                                if (lock.type == LockType::KEY)
                                {
                                    if (g_playerKeys.contains( lock.requirement ))
                                    {
                                        lock.unlocked = true;
                                        showAccessPopup( lock.requirement + " used.", 1600 );
                                        triggerInteractionAnim( InteractionAnimType::KEY_USE, "USING " + lock.requirement, 1.25f );
                                    }
                                    else
                                    {
                                        showAccessPopup( lock.requirement + " required." );
                                        continue;
                                    }
                                }
                                else
                                {
                                    g_codeEntryActive = true;
                                    g_codeEntryLockIndex = lockIndex;
                                    g_codeEntryBuffer.clear();
                                    continue;
                                }
                            }
                        }

                        bool toggled = toggleDoorAhead( engineContext );
                        if (toggled)
                        {
                            playDoorCreak( levels[ engineContext.currentLevel ].folder );
                            triggerInteractionAnim( InteractionAnimType::DOOR_USE, "OPENING DOOR", 0.45f );
                        }

                        if (engineContext.currentLevel == Levels::CAVE && isPlayerNearPoint(engineContext, kCaveFinalDoorSequenceX, kCaveFinalDoorSequenceY, 1.5f))
                        {
                            g_endGameStateAllowRevolver = true;
                            g_showHeldWeapon = true;

                            continue;
                        }


   
                        if (toggled && engineContext.currentLevel == Levels::TRANSITION)
                        {
                            handleLevelChange( engineContext, levels, Levels::CAVE );
                        }
                    }
           
                    else if (ev.key.scancode == SDL_SCANCODE_K)
                    {
                        handleLevelChange( engineContext, levels, static_cast<Levels>((static_cast<int>(engineContext.currentLevel) + 1) % static_cast<int>(Levels::COUNT)));
                    }
                    else if (ev.key.scancode == SDL_SCANCODE_L)
                    {
                        startMindTrapSequence(engineContext);
                        currentState = STATE_MIND_TRAP;
                    }
                //    else if (ev.key.scancode == SDL_SCANCODE_J)
                //    {
                //        tryTriggerPanicAttack(engineContext, levels[engineContext.currentLevel].folder);
                   // }
                }
            }
            else if (currentState == STATE_ENDING)
            {
                if (ev.type == SDL_EVENT_KEY_DOWN)
                {
                    if (ev.key.key == SDLK_R)
                    {
                        startNewMuseumRun( engineContext, levels );
                        currentState = STATE_GAME;
                    }
                    else if (ev.key.key == SDLK_ESCAPE)
                    {
                        currentState = STATE_MENU;
                    }
                }
            }
            else if (currentState == STATE_MIND_TRAP)
            {
                if (ev.type == SDL_EVENT_KEY_DOWN)
                {
                    if ((ev.key.key == SDLK_UP || ev.key.key == SDLK_W) && g_mindTrapAwaitingChoice && !g_mindTrapForcedCorrectionActive)
                    {
                        g_mindTrapSelectedOption = (g_mindTrapSelectedOption + 2) % 3;
                    }
                    else if ((ev.key.key == SDLK_DOWN || ev.key.key == SDLK_S) && g_mindTrapAwaitingChoice && !g_mindTrapForcedCorrectionActive)
                    {
                        g_mindTrapSelectedOption = (g_mindTrapSelectedOption + 1) % 3;
                    }
                    else if ((ev.key.key == SDLK_RETURN || ev.key.key == SDLK_KP_ENTER) && g_mindTrapAwaitingChoice)
                    {
                        commitMindTrapChoice( g_mindTrapSelectedOption );
                    }
                    else if (ev.key.key == SDLK_ESCAPE && !g_mindTrapReadyToExit && mindTrapTypewriterIdle())
                    {
                        g_mindTrapAwaitingChoice = false;
                        pushMindTrapTerminalLine( "> EXIT" );
                        pushMindTrapTerminalLine( "[ENTER]" );
                        queueMindTrapTerminalLine( "EXIT COMMAND DENIED." );
                    }
                }
            }
        }

 

        if (currentState == STATE_GAME)
        {
            const bool *ks = SDL_GetKeyboardState( nullptr );
            float ms = actualSpeed * dt;
            if (ks[ SDL_SCANCODE_LSHIFT ])
            {
                ms += 0.8f * dt;
            }
            if (g_codeEntryActive || g_notesOpen || g_caveQuizActive || g_levelTransition.active || g_interactionAnim.active || g_levelEditorMode || g_cutsceneController.isCameraLockActive() || g_revolverInspectCutsceneActive || g_wakeCutsceneActive) ms = 0.0f;
            if (g_solventLabUnlockCutsceneActive || g_caveTimeoutActive || g_caveTimeoutStage > 0) ms = 0.0f;
            float ts = TURN_SPEED * dt;
            if (g_cutsceneController.isCameraLockActive() || g_revolverInspectCutsceneActive || g_wakeCutsceneActive || g_caveTimeoutActive || g_caveTimeoutStage > 0) ts = 0.0f;
            if (ks[ SDL_SCANCODE_LEFT ])
            {
              /*  float ang = -ts;
                engineContext.yaw += ang;
                if (engineContext.yaw > 360)
                {
                    engineContext.yaw = 0;
                }
                float ndx = engineContext.directionX * std::cos( ang ) - engineContext.directionY * std::sin( ang );
                float ndy = engineContext.directionX * std::sin( ang ) + engineContext.directionY * std::cos( ang );
                engineContext.directionX = ndx;
                engineContext.directionY = ndy;
                // re-derive plane to stay perfectly perpendicular and correct FOV
                engineContext.planeX = -engineContext.directionY * FOV_TAN;
                engineContext.planeY = engineContext.directionX * FOV_TAN;
                */
            }
            if (ks[ SDL_SCANCODE_RIGHT ])
            {
                /*float ang = ts;
                engineContext.yaw += ang;
                if (engineContext.yaw < 0)
                {
                    engineContext.yaw = 360;
                }
                float ndx = engineContext.directionX * std::cos( ang ) - engineContext.directionY * std::sin( ang );
                float ndy = engineContext.directionX * std::sin( ang ) + engineContext.directionY * std::cos( ang );
                engineContext.directionX = ndx;
                engineContext.directionY = ndy;
                engineContext.planeX = -engineContext.directionY * FOV_TAN;
                engineContext.planeY = engineContext.directionX * FOV_TAN;*/
            }
            // move: W/S
            float nx = engineContext.positionX, ny = engineContext.positionY;
            if (ks[ SDL_SCANCODE_W ])
            {
                nx += engineContext.directionX * ms;
                ny += engineContext.directionY * ms;
                engineContext.isMoving = true;
            }
            if (ks[ SDL_SCANCODE_S ])
            {
                nx -= engineContext.directionX * ms;
                ny -= engineContext.directionY * ms;
                engineContext.isMoving = true;
            }
            // strafe: A/D
            if (ks[ SDL_SCANCODE_A ])
            {

                float ang = -ts;
                engineContext.yaw += ang;
                if (engineContext.yaw > 360)
                {
                    engineContext.yaw = 0;
                }
                float ndx = engineContext.directionX * std::cos(ang) - engineContext.directionY * std::sin(ang);
                float ndy = engineContext.directionX * std::sin(ang) + engineContext.directionY * std::cos(ang);
                engineContext.directionX = ndx;
                engineContext.directionY = ndy;
                // re-derive plane to stay perfectly perpendicular and correct FOV
                engineContext.planeX = -engineContext.directionY * FOV_TAN;
                engineContext.planeY = engineContext.directionX * FOV_TAN;


               // nx += engineContext.directionY * ms;
                //ny += -engineContext.directionX * ms;
               // engineContext.isMoving = true;
            }
            if (ks[ SDL_SCANCODE_D ])
            {

                float ang = ts;
                engineContext.yaw += ang;
                if (engineContext.yaw < 0)
                {
                    engineContext.yaw = 360;
                }
                float ndx = engineContext.directionX * std::cos(ang) - engineContext.directionY * std::sin(ang);
                float ndy = engineContext.directionX * std::sin(ang) + engineContext.directionY * std::cos(ang);
                engineContext.directionX = ndx;
                engineContext.directionY = ndy;
                engineContext.planeX = -engineContext.directionY * FOV_TAN;
                engineContext.planeY = engineContext.directionX * FOV_TAN;

                //nx += -engineContext.directionY * ms;
               // ny += engineContext.directionX * ms;
               // engineContext.isMoving = true;
            }
            auto pass = [&]( float x, float y ) {
                int mx = int( x ), my = int( y );
                if (mx < 0 || my < 0 || mx >= engineContext.map.width || my >= engineContext.map.height) return false;
                int t = engineContext.map.tiles[ my * engineContext.map.width + mx ];
                if (t != 0) return false;

             if (pointBlockedByWorldModel( x, y, 0.1f )) return false;


                /*
                // Quad collisions: inflate bench art tiny bit
                for (const auto &q : engineContext.quads)
                {
                    // Project point into local space of q
                    float u, v;
                    if (quadprop_local_uv( q, x, y, u, v ))
                    {
                        // treat inside (u,v) as blocked; shrink bounds slightly for easier navigation
                        const float pad = 0.02f;
                        if (u > pad && u < 1.0f - pad && v > pad && v < 1.0f - pad) return false;
                    }
                }
                */
                const float pad = 0.01f;
                for (const auto &box : engineContext.benches3D)
                {
                    // world -> bench local (rotate by -angle)
                    const float dx = x - box.centerX, dy = y - box.centerY;
                    const float c = std::cos( -box.angle ), s = std::sin( -box.angle );
                    const float u = dx * c - dy * s;  // along length
                    const float v = dx * s + dy * c;  // along depth

                    // Half extents, inflated for player radius
                    if (std::fabs( u ) < (box.halfLength + 0.3 + pad) &&
                        std::fabs( v ) < (box.halfDepth + 0.3 + pad))
                    {
                        return false; // blocked by bench body
                    }
                }

                return true;
                };
            // Use art radius
            float radius = 0.2f;
            if (pass( nx + radius, engineContext.positionY ) && pass( nx - radius, engineContext.positionY )) engineContext.positionX = nx;
            if (pass( engineContext.positionX, ny + radius ) && pass( engineContext.positionX, ny - radius )) engineContext.positionY = ny;
            engineContext.inRangeOfStatue = isPlayerNearStatue( engineContext );
            if (engineContext.statueChatActive)
            {
                Uint32 now = SDL_GetTicks();
                if (now - engineContext.statueChatStartTick > 8000)
                {
                    engineContext.statueChatActive = false; // Reset state
                }
            }
            {
                // Keep open while you keep looking at it; close after ~600ms of looking away
                static const Uint32 KEEP_MS = 600;

                int under = pickArtworkUnderCrosshair( engineContext );
                Uint32 now = SDL_GetTicks();

                if (engineContext.placardOpen)
                {
                    if (under == engineContext.openArtId)
                    {
                        engineContext.lastPlacardTick = now; // still looking at it: refresh timer
                    }
                    else if (now - engineContext.lastPlacardTick > KEEP_MS)
                    {
                        engineContext.placardOpen = false;
                        engineContext.openArtId = -1;
                    }
                }
            }

            if (isPlayerInsideUpperStudio( engineContext ) && !g_cutsceneController.hasTriggeredStudioCutscene() && !g_levelTransition.active)
            {
                g_cutsceneController.triggerStudioCutscene( engineContext, g_dialogue );
            }
        }
        const float headShakeYaw = g_cutsceneController.headShakeYawOffset();
        const float headShakePitch = g_cutsceneController.headShakePitchOffset();
        const float savedPitchOffset = engineContext.pitchOffset;
        const float savedDirX = engineContext.directionX;
        const float savedDirY = engineContext.directionY;
        const float savedPlaneX = engineContext.planeX;
        const float savedPlaneY = engineContext.planeY;
        const float savedYawDeg = engineContext.yaw;

        engineContext.pitchOffset = savedPitchOffset + headShakePitch;
        if (std::fabs( headShakeYaw ) > 0.00001f)
        {
            const float c = std::cos( headShakeYaw );
            const float s = std::sin( headShakeYaw );
            const float ndx = savedDirX * c - savedDirY * s;
            const float ndy = savedDirX * s + savedDirY * c;
            engineContext.directionX = ndx;
            engineContext.directionY = ndy;
            engineContext.planeX = -engineContext.directionY * FOV_TAN;
            engineContext.planeY = engineContext.directionX * FOV_TAN;
            float yaw = std::atan2( engineContext.directionY, engineContext.directionX ) * (180.0f / 3.14159265f);
            if (yaw < 0.0f) yaw += 360.0f;
            engineContext.yaw = yaw;
        }

        if (currentState == STATE_MIND_TRAP)
        {
            drawTextBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), rgb( 0, 0, 0 ) );
        }
        else
        {
            render( engineContext, dt );
            applyPostAAMode( engineContext );
            if (g_caveTimeoutActive && g_caveTimeoutStage == 0)
            {
                const float progress = std::clamp( g_caveTimeoutTimer / std::max( 0.001f, kCaveTimeoutDuration ), 0.0f, 1.0f );
                applyCaveTimeoutEffect( engineContext, progress );
            }
        }

        if (currentState == STATE_GAME)
        {

       

            if (!g_caveTimeoutActive)
            {
                renderGameplayUiPass( engineContext );
            }

            if (g_panicAttack.active)
            {
                applyPanicAttackBlur(engineContext, g_panicAttack.blurIntensity);
            }

        }
        else if (currentState == STATE_MENU)
        {
            renderMenu( engineContext,
                currentMenuSelection,
                musicVolume,
                config::useMusic,
                config::viewBobbing,
                getAntiAliasingMode(),
                config::modelQualityPreset,
                config::gpuRenderMode,
                g_multithreadingEnabled,
                g_detectedThreadCount,
                config::schoolMode );
        }
        else if (currentState == STATE_ENDING)
        {
            if (g_caveTimeoutStage > 0)
            {
                renderCaveTimeoutEndingScreen( engineContext );
            }
        }
        else if (currentState == STATE_MIND_TRAP)
        {
            renderMindTrapInterface( engineContext );
        }

        engineContext.pitchOffset = savedPitchOffset;
        engineContext.directionX = savedDirX;
        engineContext.directionY = savedDirY;
        engineContext.planeX = savedPlaneX;
        engineContext.planeY = savedPlaneY;
        engineContext.yaw = savedYawDeg;

        // Present to window (nearest-neighbor scale)
        SDL_UpdateTexture( engineContext.backtexure, nullptr, engineContext.backbuffer.data(), RENDER_W * 4  );
        SDL_RenderClear( engineContext.renderer );
        const float presentShotFx = (currentState == STATE_GAME)
            ? std::clamp( g_revolverRecoilTimer / std::max( 0.001f, kRevolverRecoilDuration ), 0.0f, 1.0f )
            : 0.0f;
        SDL_FRect dstRect{
            0.0f,
            0.0f,
            float( RENDER_W * WIN_SCALE ),
            float( RENDER_H * WIN_SCALE )
        };
        if (presentShotFx > 0.001f)
        {
            const float phase = SDL_GetTicks() * 0.001f;
            const float amp = std::pow( presentShotFx, 0.58f );
            dstRect.x = (std::sin( phase * 96.0f ) * 0.82f + std::cos( phase * 173.0f ) * 0.51f) * kRevolverScreenShakeX * amp;
            dstRect.y = (std::cos( phase * 109.0f ) * 0.88f + std::sin( phase * 159.0f ) * 0.47f) * kRevolverScreenShakeY * amp;
        }
        SDL_RenderTexture( engineContext.renderer, engineContext.backtexure, nullptr, &dstRect );
        if (currentState == STATE_GAME && isGpuModelRenderingEnabled())
        {
            renderWorldModelsGpu( engineContext, g_lastEffectivePitchOffset );

            if (g_wakeCutsceneActive)
            {
                const float p = std::clamp( g_wakeCutsceneTimer / std::max( 0.001f, kWakeCutsceneDuration ), 0.0f, 1.0f );
                const float eyeOpen = std::clamp( std::pow( p, 1.9f ), 0.0f, 1.0f );
                const int lidH = int( (RENDER_H * 0.5f) * (1.0f - eyeOpen) );

                SDL_SetRenderDrawBlendMode( engineContext.renderer, SDL_BLENDMODE_BLEND );

                if (lidH > 0)
                {
                    SDL_SetRenderDrawColor( engineContext.renderer, 0, 0, 0, 255 );
                    SDL_FRect topBar{ 0.0f, 0.0f, float( RENDER_W * WIN_SCALE ), float( lidH * WIN_SCALE ) };
                    SDL_FRect botBar{ 0.0f, float( (RENDER_H - lidH) * WIN_SCALE ), float( RENDER_W * WIN_SCALE ), float( lidH * WIN_SCALE ) };
                    SDL_RenderFillRect( engineContext.renderer, &topBar );
                    SDL_RenderFillRect( engineContext.renderer, &botBar );
                }

                const Uint8 fadeAlpha = Uint8( std::clamp( 0.55f - p * 0.55f, 0.0f, 0.55f ) * 255.0f );
                if (fadeAlpha > 0)
                {
                    SDL_SetRenderDrawColor( engineContext.renderer, 0, 0, 0, fadeAlpha );
                    SDL_FRect fadeRect{ 0.0f, 0.0f, float( RENDER_W * WIN_SCALE ), float( RENDER_H * WIN_SCALE ) };
                    SDL_RenderFillRect( engineContext.renderer, &fadeRect );
                }
            }
        }
        if (currentState == STATE_GAME)
        {
            const bool uiOverlayOpen = g_notesOpen || g_codeEntryActive || g_caveQuizActive || g_levelTransition.active || g_interactionAnim.active || g_levelEditorMode;
            if (!uiOverlayOpen)
            {
                renderModernCrosshairOverlay( engineContext );
                renderModernRevolverHudOverlay( engineContext );
            }
        }
        SDL_RenderPresent( engineContext.renderer );
    }

    SDL_DestroyTexture( engineContext.backtexure );
    shutdownRasterWorkerPool();
    SDL_DestroyRenderer( engineContext.renderer );
    SDL_DestroyWindow( engineContext.window );
    SDL_Quit();
    return 0;
}


















































