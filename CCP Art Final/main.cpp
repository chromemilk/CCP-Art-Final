#include "GameEngine.h"
#include "RendererHelpers.h"
#include "PhysicsHelpers.h"
#include "MusicSystem.h"
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

enum class LockType
{
    KEY,
    CODE
};

struct RoomLock
{
    int tx = 0;
    int ty = 0;
    std::string roomName;
    LockType type = LockType::KEY;
    std::string requirement;
    bool unlocked = false;
    Levels level = Levels::MUSEUM;
};

struct KeyPickup
{
    std::string keyName;
    float x = 0.f;
    float y = 0.f;
    bool collected = false;
    int propIndex = -1;
    int modelIndex = -1;
    Levels level = Levels::MUSEUM;
};

struct CaveQuizQuestion
{
    std::string question;
    std::array<std::string, 4> options;
    int correctOption = 0;
};

struct MindTrapPhase
{
    std::string prompt;
    std::array<std::string, 3> commands;
    std::array<std::string, 3> options;
    std::array<std::string, 3> results;
    int surrenderOption = -1;
};

struct ClueNote
{
    std::string title;
    std::string body;
    float x = 0.f;
    float y = 0.f;
    bool collected = false;
    int propIndex = -1;
    int modelIndex = -1;
    Levels level = Levels::MUSEUM;
};

struct SafePuzzle
{
    std::string safeName;
    std::string code;
    float x = 0.f;
    float y = 0.f;
    bool solved = false;
    std::string rewardKey;
};

struct SymbolPuzzle
{
    std::string name;
    int targetCombo[3] = {0,0,0}; // 0-based indices for symbols
    float x = 0.f;
    float y = 0.f;
    bool solved = false;
    std::string rewardKey;
};

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
static bool g_museumPuzzleInitialized = false;
static bool g_restorationWingUnlocked = false;
static bool g_unlockAllDoorsOverride = false;
static bool g_wakeCutsceneActive = false;
static float g_wakeCutsceneTimer = 0.0f;
static constexpr float kWakeCutsceneDuration = 6.5f;
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

struct WeaponPickup
{
    std::string weaponName;
    float x = 0.f;
    float y = 0.f;
    bool collected = false;
    int modelIndex = -1;
    Levels level = Levels::MUSEUM;
};

struct CombatState
{
    bool active = false;
    bool hasRevolver = false;
    int loadedAmmo = 0;
    int reserveAmmo = 0;
};

struct DialogueLine
{
    std::string text;
    float duration = 3.0f;
};

struct CpuModel
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec2> uvs;
    std::vector<uint32_t> indices;
    std::vector<Image> baseColorTextures;
    std::vector<SDL_Texture*> hwTextures;
    std::vector<int> triangleTextureIndex;
    std::vector<glm::vec4> triangleBaseColorFactor;
    glm::vec3 boundsMin{0.0f};
    glm::vec3 boundsMax{0.0f};
};

struct WorldModelInstance
{
    std::shared_ptr<CpuModel> model;
    float x = 0.0f;
    float y = 0.0f;
    float heightOffset = 0.0f;
    float scale = 1.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    bool spinYaw = false;
    float spinSpeed = 0.0f;
    Uint32 tint = rgb( 170, 170, 170 );
    bool visible = true;
    bool editorPlaced = false;
    std::string editorAssetName;
    float editorTargetScale = 1.0f;
};

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

class DialogueSystem
{
public:
    void start( std::vector<DialogueLine> lines ) {
        queue = std::move( lines );
        index = 0;
        lineTimer = queue.empty() ? 0.0f : std::max( 0.1f, queue[ 0 ].duration );
        active = !queue.empty();
    }

    void update( float dt ) {
        if (!active || queue.empty()) return;

        lineTimer -= dt;
        while (lineTimer <= 0.0f && active)
        {
            ++index;
            if (index >= queue.size())
            {
                clear();
                return;
            }
            lineTimer += std::max( 0.1f, queue[ index ].duration );
        }
    }

    bool isActive() const {
        return active;
    }

    const std::string &currentText() const {
        static const std::string kEmpty;
        if (!active || queue.empty() || index >= queue.size()) return kEmpty;
        return queue[ index ].text;
    }

    void clear() {
        queue.clear();
        index = 0;
        lineTimer = 0.0f;
        active = false;
    }

private:
    std::vector<DialogueLine> queue;
    size_t index = 0;
    float lineTimer = 0.0f;
    bool active = false;
};

class CutsceneController
{
public:
    bool isCameraLockActive() const {
        return phoneCutsceneActive || scriptedPanActive;
    }

    bool isPhoneCutsceneActive() const {
        return phoneCutsceneActive;
    }

    bool canTriggerPhoneCutscene() const {
        return !phoneCutsceneTriggered && !phoneCutsceneActive;
    }

    void triggerUpstairsGalleryCutscene( Engine &engineContext, DialogueSystem &dialogue ) {
        if (upstairsGalleryCutsceneTriggered) return;
        upstairsGalleryCutsceneTriggered = true;
        triggerScriptedPanCutscene( engineContext, dialogue, "I don't remember this being here...", 1.5f, 0.3f );
    }

    void triggerStudioCutscene( Engine &engineContext, DialogueSystem &dialogue ) {
        if (studioCutsceneTriggered) return;
        studioCutsceneTriggered = true;
        triggerScriptedPanCutscene( engineContext, dialogue, "What the hell happend here?", 1.f, 0.3f );
    }

    bool hasTriggeredStudioCutscene() const {
        return studioCutsceneTriggered;
    }

    void triggerPhoneCutscene( Engine &engineContext, DialogueSystem &dialogue, const std::string &phoneAssetPath ) {
        if (!canTriggerPhoneCutscene()) return;

        phoneCutsceneActive = true;
        elapsed = 0.0f;
        phoneCutsceneTriggered = true;

        const float yaw = std::atan2( engineContext.directionY, engineContext.directionX );
        const float px = engineContext.positionX + engineContext.directionX * kPhoneForward;
        const float py = engineContext.positionY + engineContext.directionY * kPhoneForward;

        phoneModelIndex = addWorldModelInstance(
            phoneAssetPath,
            px,
            py,
            0.22f,
            rgb( 210, 210, 220 ),
            yaw + kPhoneYawOffset,
            kPhonePitch,
            kPhoneRoll,
            false,
            0.0f,
            kPhoneHeight );

        dialogue.start( {
            {"Huh, that's weird... it was just at 100%. Wait... where am I, where is everyone? Ugh my head hurts", 6.5f}
            } );
    }

    void update( Engine &engineContext, DialogueSystem &dialogue, float dt ) {
        if (scriptedPanActive)
        {
            scriptedPanElapsed += dt;

            const float ang = scriptedPanSpeed * dt;
            const float ndx = engineContext.directionX * std::cos( ang ) - engineContext.directionY * std::sin( ang );
            const float ndy = engineContext.directionX * std::sin( ang ) + engineContext.directionY * std::cos( ang );
            engineContext.directionX = ndx;
            engineContext.directionY = ndy;
            engineContext.planeX = -engineContext.directionY * FOV_TAN;
            engineContext.planeY = engineContext.directionX * FOV_TAN;
            engineContext.yaw += ang * (180.0f / 3.14159265f);

            if (engineContext.yaw > 360.0f) engineContext.yaw -= 360.0f;
            if (engineContext.yaw < 0.0f) engineContext.yaw += 360.0f;

            if (scriptedPanElapsed >= scriptedPanDuration && !dialogue.isActive())
            {
                scriptedPanActive = false;
                scriptedPanElapsed = 0.0f;
            }
        }

        if (!phoneCutsceneActive) return;

        elapsed += dt;

        if (phoneModelIndex >= 0 && phoneModelIndex < (int)g_worldModels.size())
        {
            const float yaw = std::atan2( engineContext.directionY, engineContext.directionX );
            g_worldModels[ phoneModelIndex ].x = engineContext.positionX + engineContext.directionX * kPhoneForward;
            g_worldModels[ phoneModelIndex ].y = engineContext.positionY + engineContext.directionY * kPhoneForward;
            g_worldModels[ phoneModelIndex ].yaw = yaw + kPhoneYawOffset;
            g_worldModels[ phoneModelIndex ].pitch = kPhonePitch;
            g_worldModels[ phoneModelIndex ].roll = kPhoneRoll;
            g_worldModels[ phoneModelIndex ].heightOffset = kPhoneHeight;
        }

        if (elapsed >= durationSeconds && !dialogue.isActive())
        {
            stop( engineContext );
            mesuemObjectives.setMainObjective( "Figure out what happened." );
        }
    }

    float forcedPitchOffset() const {
        if (phoneCutsceneActive) return 74.0f;
        if (scriptedPanActive) return 2.0f;
        return 0.0f;
    }

    void reset() {
        phoneCutsceneActive = false;
        phoneCutsceneTriggered = false;
        elapsed = 0.0f;
        phoneModelIndex = -1;
        scriptedPanActive = false;
        scriptedPanElapsed = 0.0f;
        upstairsGalleryCutsceneTriggered = false;
        studioCutsceneTriggered = false;
    }

private:
    void triggerScriptedPanCutscene( Engine &engineContext, DialogueSystem &dialogue, const std::string &text, float duration, float panSpeed ) {
        if (scriptedPanActive || phoneCutsceneActive) return;
        scriptedPanActive = true;
        scriptedPanElapsed = 0.0f;
        scriptedPanDuration = std::max( 0.5f, duration );
        scriptedPanSpeed = panSpeed;
        dialogue.start( { { text, scriptedPanDuration } } );
        engineContext.pitchOffset = 2.0f;
    }

    void stop( Engine &engineContext ) {
        phoneCutsceneActive = false;
        elapsed = 0.0f;
        if (phoneModelIndex >= 0 && phoneModelIndex < (int)g_worldModels.size())
        {
            g_worldModels[ phoneModelIndex ].visible = false;
        }
        phoneModelIndex = -1;
        engineContext.pitchOffset = 0.0f;
    }

    bool phoneCutsceneActive = false;
    bool phoneCutsceneTriggered = false;
    float elapsed = 0.0f;
    float durationSeconds = 6.5f;
    int phoneModelIndex = -1;
    bool scriptedPanActive = false;
    float scriptedPanElapsed = 0.0f;
    float scriptedPanDuration = 2.8f;
    float scriptedPanSpeed = 0.45f;
    bool upstairsGalleryCutsceneTriggered = false;
    bool studioCutsceneTriggered = false;
    static constexpr float kPhoneForward = 0.45f;
    static constexpr float kPhoneHeight = 0.45f;
    static constexpr float kPhoneYawOffset = 1.5707963f;
    static constexpr float kPhonePitch = -0.1f;
    static constexpr float kPhoneRoll = 0.0f;
};

static WeaponPickup g_revolverPickup;
static CombatState g_combatState;
static bool g_directorDeskUnlocked = false;
static DialogueSystem g_dialogue;
static CutsceneController g_cutsceneController;
static bool g_showHeldWeapon = true;
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
static constexpr float kGeneratorStartFadeOutDuration = 1.0f;
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

struct EditorAssetDef
{
    std::string label;
    std::string assetName;
    float worldSize = 0.7f;
    Uint32 tint = rgb( 170, 170, 170 );
    float pitch = 0.0f;
};

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

static std::string makeEditorModelsFileNameForLevel( int levelId, const std::string &mapFile ) {
    std::string stem = std::filesystem::path( mapFile.empty() ? "map.txt" : mapFile ).stem().string();
    if (stem.empty()) stem = "map";
    return "editor_models_level_" + std::to_string( levelId ) + "_" + stem + ".txt";
}

static void refreshWeaponSoundBuffers( const std::string &baseFolder ) {
    if (g_weaponSoundBuffersReady && g_weaponSoundBaseFolder == baseFolder) return;

    g_weaponSoundBaseFolder = baseFolder;
    g_weaponSoundBuffersReady = false;

    const std::string gunshotPath = baseFolder + "\\gunshot.wav";
    const std::string shellDropPath = baseFolder + "\\shelldrop.wav";

    const bool gunOk = g_gunshotBuffer.loadFromFile( gunshotPath );
    const bool shellOk = g_shellDropBuffer.loadFromFile( shellDropPath );
    g_weaponSoundBuffersReady = gunOk && shellOk;
}

static void playWeaponBufferedSound( const sf::SoundBuffer &buffer, float volume = 100.0f ) {
    auto snd = std::make_shared<sf::Sound>( buffer );
    snd->setVolume( volume );
    snd->play();
    g_activeWeaponSounds.push_back( snd );
}

static void refreshGeneratorStartSoundBuffer( const std::string &baseFolder ) {
    if (g_generatorStartBufferReady && g_generatorStartSoundBaseFolder == baseFolder) return;

    g_generatorStartSoundBaseFolder = baseFolder;
    g_generatorStartBufferReady = g_generatorStartBuffer.loadFromFile( baseFolder + "\\GeneratorStart.wav" );
}

static void playGeneratorStartSound( const std::string &baseFolder ) {
    refreshGeneratorStartSoundBuffer( baseFolder );
    if (!g_generatorStartBufferReady) return;

    g_generatorStartSound = std::make_shared<sf::Sound>( g_generatorStartBuffer );
    g_generatorStartSound->setVolume( kGeneratorStartSoundVolume );
    g_generatorStartSound->play();
    g_activeWeaponSounds.push_back( g_generatorStartSound );
    g_generatorStartSoundTimer = kGeneratorStartSoundDuration;
}

static void playRevolverShotSequence( const std::string &baseFolder ) {
    if (config::schoolMode)
    {
        return;
    }

    refreshWeaponSoundBuffers( baseFolder );
    if (!g_weaponSoundBuffersReady) return;

    playWeaponBufferedSound( g_gunshotBuffer, 100.0f );
    g_pendingShellDropTimers.push_back( 0.34f );
}

static float randomRange01( float minValue, float maxValue ) {
    const float t = float( std::rand() ) / float( RAND_MAX );
    return minValue + (maxValue - minValue) * t;
}

static void resetWhisperAmbience() {
    g_whisperBufferReady = false;
    g_whisperBaseFolder.clear();
    g_whisperTimer = 0.0f;
    g_whisperNextDelay = randomRange01( 24.0f, 40.0f );
}

static void updateWhisperAmbience( Engine &engineContext, float dt ) {
    if (engineContext.currentLevel != Levels::MUSEUM && engineContext.currentLevel != Levels::MUSEUM_UPPER)
    {
        return;
    }

    if (g_currentLevelFolder.empty())
    {
        return;
    }

    if (g_whisperBaseFolder != g_currentLevelFolder)
    {
        g_whisperBaseFolder = g_currentLevelFolder;
        g_whisperBufferReady = g_whisperBuffer.loadFromFile( g_currentLevelFolder + "\\whisper.wav" );
        g_whisperTimer = 0.0f;
        g_whisperNextDelay = g_mindTrapActive ? randomRange01( 2.4f, 5.2f ) : randomRange01( 20.0f, 34.0f );
    }

    if (!g_whisperBufferReady) return;

    g_whisperTimer += dt;
    if (g_whisperTimer < g_whisperNextDelay) return;

    g_whisperTimer = 0.0f;
    g_whisperNextDelay = g_mindTrapActive ? randomRange01( 1.2f, 3.2f ) : randomRange01( 36.0f, 64.0f );

    playWeaponBufferedSound( g_whisperBuffer, g_mindTrapActive ? 16.0f : 9.0f );

    const bool shouldSpeak = (std::rand() % 100) < 70;
    const bool canSpeak =
        (g_mindTrapActive ||
        !g_dialogue.isActive() &&
        !g_cutsceneController.isCameraLockActive() &&
        !g_revolverInspectCutsceneActive &&
        !g_wakeCutsceneActive &&
        !g_codeEntryActive &&
        !g_notesOpen &&
        !g_caveQuizActive);
    if (!shouldSpeak || !canSpeak) return;

    static const std::array<std::string, 6> whisperLines = {
        "I'm losing it.",
        "What was that?",
        "I'm not alone down here.",
        "Did someone just whisper my name?",
        "No, no... keep it together.",
        "Something is following me."
    };

    const int index = std::rand() % (int)whisperLines.size();
    g_dialogue.start( { { whisperLines[ index ], 2.2f } } );
}

static const std::vector<EditorAssetDef> &editorAssetCatalog() {
    if (g_editorAssetCatalog.empty())
    {
        refreshEditorAssetCatalog( true );
    }
    return g_editorAssetCatalog;
}

static std::string toLowerCopy( std::string s ) {
    for (char &c : s) c = char( std::tolower( unsigned char( c ) ) );
    return s;
}

static void configureRuntimeGpuProfile( SDL_Renderer *renderer );

static EditorAssetDef makeEditorAssetDefForName( const std::string &assetName ) {
    EditorAssetDef out;
    out.assetName = assetName;
    out.label = std::filesystem::path( assetName ).stem().string();

    const std::string lower = toLowerCopy( out.label );
    out.worldSize = 0.78f;
    out.tint = rgb( 170, 170, 170 );
    out.pitch = 0.0f;

    if (lower.find( "plant" ) != std::string::npos) { out.worldSize = 0.95f; out.tint = rgb( 255, 255, 255 ); }
    else if (lower.find( "trash" ) != std::string::npos || lower.find( "can" ) != std::string::npos) { out.worldSize = 0.80f; out.tint = rgb( 140, 145, 150 ); }
    else if (lower.find( "bench" ) != std::string::npos) { out.worldSize = 0.72f; out.tint = rgb( 126, 96, 64 ); }
    else if (lower.find( "vase" ) != std::string::npos) { out.worldSize = 0.62f; out.tint = rgb( 188, 168, 130 ); }
    else if (lower.find( "pedestal" ) != std::string::npos) { out.worldSize = 0.78f; out.tint = rgb( 164, 156, 142 ); }
    else if (lower.find( "safe" ) != std::string::npos) { out.worldSize = 0.72f; out.tint = rgb( 150, 160, 174 ); }
    else if (lower.find( "desk" ) != std::string::npos) { out.worldSize = 0.82f; out.tint = rgb( 170, 150, 130 ); }
    else if (lower.find( "shelf" ) != std::string::npos) { out.worldSize = 0.82f; out.tint = rgb( 170, 160, 140 ); }
    else if (lower.find( "couch" ) != std::string::npos) { out.worldSize = 0.82f; out.tint = rgb( 130, 112, 78 ); }
    else if (lower.find( "box" ) != std::string::npos) { out.worldSize = 0.82f; out.tint = rgb( 184, 130, 98 ); }
    else if (lower.find( "paper" ) != std::string::npos) { out.worldSize = 0.22f; out.tint = rgb( 224, 214, 188 ); }
    else if (lower.find( "note" ) != std::string::npos) { out.worldSize = 0.25f; out.tint = rgb( 225, 214, 180 ); out.pitch = -1.5707963f; }

    return out;
}

static std::filesystem::path findAssetsRoot() {
    namespace fs = std::filesystem;
    fs::path start = fs::current_path();
    for (int i = 0; i < 7; ++i)
    {
        fs::path a = start / "assets";
        if (fs::exists( a ) && fs::is_directory( a )) return a;
        fs::path b = start / "CCP Art Final" / "assets";
        if (fs::exists( b ) && fs::is_directory( b )) return b;
        if (!start.has_parent_path()) break;
        fs::path parent = start.parent_path();
        if (parent == start) break;
        start = parent;
    }
    return fs::current_path() / "assets";
}

static void refreshEditorAssetCatalog( bool force ) {
    const Uint32 now = SDL_GetTicks();
    if (!force && (now - g_editorCatalogLastScanTick) < kEditorCatalogRefreshMs)
    {
        return;
    }
    g_editorCatalogLastScanTick = now;

    std::filesystem::path assetsRoot = findAssetsRoot();
    std::vector<EditorAssetDef> scanned;
    if (std::filesystem::exists( assetsRoot ) && std::filesystem::is_directory( assetsRoot ))
    {
        for (const auto &entry : std::filesystem::recursive_directory_iterator( assetsRoot ))
        {
            if (!entry.is_regular_file()) continue;
            std::string ext = toLowerCopy( entry.path().extension().string() );
            if (ext != ".glb" && ext != ".gltf") continue;

            std::string relName;
            std::error_code ec;
            auto rel = std::filesystem::relative( entry.path(), assetsRoot, ec );
            relName = ec ? entry.path().filename().string() : rel.generic_string();
            scanned.push_back( makeEditorAssetDefForName( relName ) );
        }
    }

    std::sort( scanned.begin(), scanned.end(), []( const EditorAssetDef &a, const EditorAssetDef &b ) {
        return a.label < b.label;
    } );

    scanned.erase( std::unique( scanned.begin(), scanned.end(), []( const EditorAssetDef &a, const EditorAssetDef &b ) {
        return a.assetName == b.assetName;
    } ), scanned.end() );

    if (scanned.empty())
    {
        scanned = {
            makeEditorAssetDefForName( "Plant.glb" ),
            makeEditorAssetDefForName( "Trashcan.glb" ),
            makeEditorAssetDefForName( "Bench.glb" ),
            makeEditorAssetDefForName( "Vase1.glb" ),
            makeEditorAssetDefForName( "Vase2.glb" ),
            makeEditorAssetDefForName( "Vase3.glb" ),
            makeEditorAssetDefForName( "Pedestal.glb" ),
            makeEditorAssetDefForName( "Safe.glb" ),
            makeEditorAssetDefForName( "Full Desk.glb" ),
            makeEditorAssetDefForName( "Shelf.glb" ),
            makeEditorAssetDefForName( "Couch.glb" ),
            makeEditorAssetDefForName( "Boxes.glb" ),
            makeEditorAssetDefForName( "Scattered Paper.glb" ),
            makeEditorAssetDefForName( "Note.glb" )
        };
    }

    g_editorAssetCatalog = std::move( scanned );
    if (!g_editorAssetCatalog.empty())
    {
        g_editorAssetIndex = std::clamp( g_editorAssetIndex, 0, (int)g_editorAssetCatalog.size() - 1 );
    }
    else
    {
        g_editorAssetIndex = 0;
    }
}

static void saveEditorModelsForLevel() {
    std::filesystem::path outPath = editorModelsPathForLevel();
    if (outPath.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::create_directories( outPath.parent_path(), ec );
    }

    std::ofstream out( outPath.string(), std::ios::trunc );
    if (!out.is_open())
    {
        return;
    }

    out << "# asset|x|y|z|size|yaw|pitch|roll|r|g|b\n";
    out.setf( std::ios::fixed );
    out.precision( 4 );

    for (const auto &inst : g_worldModels)
    {
        if (!inst.visible || !inst.editorPlaced || inst.editorAssetName.empty()) continue;

        out
            << inst.editorAssetName << "|"
            << inst.x << "|"
            << inst.y << "|"
            << inst.heightOffset << "|"
            << inst.editorTargetScale << "|"
            << inst.yaw << "|"
            << inst.pitch << "|"
            << inst.roll << "|"
            << int( (inst.tint >> 16) & 255 ) << "|"
            << int( (inst.tint >> 8) & 255 ) << "|"
            << int( inst.tint & 255 ) << "\n";
    }
}

static void loadEditorModelsForLevel() {
    std::filesystem::path inPath = editorModelsPathForLevel();
    std::ifstream in( inPath.string() );
    if (!in.is_open())
    {
        std::filesystem::path legacy = g_currentLevelFolder.empty()
            ? std::filesystem::path( kEditorModelsLegacyFile )
            : (std::filesystem::path( g_currentLevelFolder ) / kEditorModelsLegacyFile);

        if (legacy != inPath)
        {
            in.open( legacy.string() );
        }
        if (!in.is_open())
        {
            return;
        }
    }

    std::string line;
    while (std::getline( in, line ))
    {
        if (line.empty() || line[ 0 ] == '#') continue;

        std::vector<std::string> parts;
        std::stringstream ss( line );
        std::string part;
        while (std::getline( ss, part, '|' )) parts.push_back( part );
        if (parts.size() < 11) continue;

        const std::string assetName = parts[ 0 ];

        float x = std::stof( parts[ 1 ] );
        float y = std::stof( parts[ 2 ] );
        float z = std::stof( parts[ 3 ] );
        float size = std::stof( parts[ 4 ] );
        float yaw = std::stof( parts[ 5 ] );
        float pitch = std::stof( parts[ 6 ] );
        float roll = std::stof( parts[ 7 ] );
        int r = std::clamp( std::stoi( parts[ 8 ] ), 0, 255 );
        int g = std::clamp( std::stoi( parts[ 9 ] ), 0, 255 );
        int b = std::clamp( std::stoi( parts[ 10 ] ), 0, 255 );

        int idx = addWorldModelInstance(
            resolveAssetModelPath( assetName ),
            x,
            y,
            size,
            rgb( Uint8( r ), Uint8( g ), Uint8( b ) ),
            yaw,
            pitch,
            roll,
            false,
            0.0f,
            z );

        if (idx >= 0 && idx < (int)g_worldModels.size())
        {
            g_worldModels[ idx ].editorPlaced = true;
            g_worldModels[ idx ].editorAssetName = assetName;
            g_worldModels[ idx ].editorTargetScale = size;
        }
    }
}

static int placeEditorModel( Engine &engineContext ) {
    refreshEditorAssetCatalog();
    const auto &catalog = editorAssetCatalog();
    if (catalog.empty()) return -1;

    g_editorAssetIndex = (g_editorAssetIndex % (int)catalog.size() + (int)catalog.size()) % (int)catalog.size();
    const auto &asset = catalog[ g_editorAssetIndex ];

    const float yaw = std::atan2( engineContext.directionY, engineContext.directionX );

    constexpr float kEditorSpawnForward = 1.0f;
    constexpr float kEditorSpawnEyeLevel = 0.52f;
    const float spawnX = engineContext.positionX + engineContext.directionX * kEditorSpawnForward;
    const float spawnY = engineContext.positionY + engineContext.directionY * kEditorSpawnForward;

    int idx = addWorldModelInstance(
        resolveAssetModelPath( asset.assetName ),
        spawnX,
        spawnY,
        asset.worldSize,
        asset.tint,
        yaw,
        asset.pitch,
        0.0f,
        false,
        0.0f,
        kEditorSpawnEyeLevel );

    if (idx >= 0 && idx < (int)g_worldModels.size())
    {
        g_worldModels[ idx ].editorPlaced = true;
        g_worldModels[ idx ].editorAssetName = asset.assetName;
        g_worldModels[ idx ].editorTargetScale = asset.worldSize;
        g_editorSelectedModel = idx;
    }
    return idx;
}

static void deleteSelectedEditorModel() {
    if (g_editorSelectedModel < 0 || g_editorSelectedModel >= (int)g_worldModels.size()) return;
    if (!g_worldModels[ g_editorSelectedModel ].editorPlaced) return;
    g_worldModels.erase( g_worldModels.begin() + g_editorSelectedModel );
    g_editorSelectedModel = -1;
}

static void nudgeSelectedEditorModel( float dx, float dy, float dz, float dYaw, float dPitch, float dRoll, float dSize ) {
    if (g_editorSelectedModel < 0 || g_editorSelectedModel >= (int)g_worldModels.size()) return;
    auto &m = g_worldModels[ g_editorSelectedModel ];
    if (!m.editorPlaced) return;

    m.x += dx;
    m.y += dy;
    m.heightOffset = std::clamp( m.heightOffset + dz, -2.0f, 4.0f );

    auto snapNearHalfOrZero = []( float v )->float {
        if (std::fabs( v ) < 0.0001f) return 0.0f;
        const float halfRounded = std::round( v * 2.0f ) * 0.5f;
        if (std::fabs( v - halfRounded ) <= 0.03f) return halfRounded;
        return v;
    };

    m.x = snapNearHalfOrZero( m.x );
    m.y = snapNearHalfOrZero( m.y );
    m.heightOffset = snapNearHalfOrZero( m.heightOffset );

    m.yaw += dYaw;
    m.pitch += dPitch;
    m.roll += dRoll;

    if (std::fabs( dSize ) > 0.0f)
    {
        m.editorTargetScale = std::clamp( m.editorTargetScale + dSize, 0.05f, 3.0f );
        auto reload = loadCpuModel( resolveAssetModelPath( m.editorAssetName ) );
        if (reload)
        {
            const float modelSizeX = std::max( 0.0001f, std::fabs( reload->boundsMax.x - reload->boundsMin.x ) );
            const float modelSizeY = std::max( 0.0001f, std::fabs( reload->boundsMax.y - reload->boundsMin.y ) );
            const float modelSizeZ = std::max( 0.0001f, std::fabs( reload->boundsMax.z - reload->boundsMin.z ) );
            const float modelReferenceSize = std::max( { modelSizeX, modelSizeY, modelSizeZ } );
            const float overrideMul = modelScaleOverride( resolveAssetModelPath( m.editorAssetName ) );
            m.scale = std::clamp( (m.editorTargetScale / modelReferenceSize) * overrideMul, 0.01f, 2.0f );
        }
    }
}

static std::filesystem::path editorModelsPathForLevel() {
    const std::string fileName = g_currentEditorModelsFile.empty() ? std::string( kEditorModelsLegacyFile ) : g_currentEditorModelsFile;
    if (g_currentLevelFolder.empty())
    {
        return std::filesystem::path( fileName );
    }
    return std::filesystem::path( g_currentLevelFolder ) / fileName;
}

static int findNearestEditorModel( Engine const &engineContext, float radius = 3.0f ) {
    float best = radius * radius;
    int hit = -1;
    for (int i = 0; i < (int)g_worldModels.size(); ++i)
    {
        const auto &m = g_worldModels[ i ];
        if (!m.visible || !m.editorPlaced) continue;
        float dx = m.x - engineContext.positionX;
        float dy = m.y - engineContext.positionY;
        float d2 = dx * dx + dy * dy;
        if (d2 <= best)
        {
            best = d2;
            hit = i;
        }
    }
    return hit;
}

static bool pointBlockedByWorldModel( float px, float py, float playerRadius ) {
    for (int i = 0; i < (int)g_worldModels.size(); ++i)
    {
        if (i == g_heldRevolverModelIndex || i == g_revolverInspectModelIndex) continue;

        const auto &inst = g_worldModels[ i ];
        if (!inst.visible || !inst.model) continue;

        // Keep small floating/spinning pickups interactive.
        if (inst.spinYaw) continue;
        if (inst.heightOffset > 0.35f) continue;

        const float sx = std::max( 0.0001f, std::fabs( inst.model->boundsMax.x - inst.model->boundsMin.x ) );
        const float sz = std::max( 0.0001f, std::fabs( inst.model->boundsMax.z - inst.model->boundsMin.z ) );
        const float halfX = 0.5f * sx * inst.scale;
        const float halfZ = 0.5f * sz * inst.scale;
        const float rawRadius = std::max( 0.12f, std::max( halfX, halfZ ) );

        // Some imported models can report oversized bounds, creating huge invisible blockers.
        if (rawRadius > 2.2f)
        {
            continue;
        }

        const float intendedRadius = std::clamp( inst.editorTargetScale * 0.55f, 0.12f, 1.0f );
        const float modelRadius = std::min( rawRadius, intendedRadius );

        const float dx = px - inst.x;
        const float dy = py - inst.y;
        const float hitRadius = modelRadius + playerRadius;
        if ((dx * dx + dy * dy) < (hitRadius * hitRadius))
        {
            return true;
        }
    }
    return false;
}

static std::string resolveAssetModelPath( const std::string &assetName ) {
    namespace fs = std::filesystem;
    fs::path start = fs::current_path();
    for (int i = 0; i < 7; ++i)
    {
        fs::path a = start / "assets" / assetName;
        if (fs::exists( a )) return a.string();
        fs::path b = start / "CCP Art Final" / "assets" / assetName;
        if (fs::exists( b )) return b.string();
        if (!start.has_parent_path()) break;
        fs::path parent = start.parent_path();
        if (parent == start) break;
        start = parent;
    }
    return (fs::current_path() / "assets" / assetName).string();
}

static bool readAccessorVec3( const tinygltf::Model &gltf, int accessorIndex, std::vector<glm::vec3> &out ) {
    if (accessorIndex < 0 || accessorIndex >= (int)gltf.accessors.size()) return false;
    const auto &acc = gltf.accessors[ accessorIndex ];
    if (acc.type != TINYGLTF_TYPE_VEC3 || acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) return false;
    if (acc.bufferView < 0 || acc.bufferView >= (int)gltf.bufferViews.size()) return false;
    const auto &view = gltf.bufferViews[ acc.bufferView ];
    if (view.buffer < 0 || view.buffer >= (int)gltf.buffers.size()) return false;

    const auto &buf = gltf.buffers[ view.buffer ];
    size_t stride = acc.ByteStride( view );
    if (stride == 0) stride = sizeof( float ) * 3;
    const uint8_t *src = buf.data.data() + view.byteOffset + acc.byteOffset;
    out.resize( acc.count );
    for (size_t i = 0; i < acc.count; ++i)
    {
        const float *f = reinterpret_cast<const float *>( src + i * stride );
        out[ i ] = glm::vec3( f[ 0 ], f[ 1 ], f[ 2 ] );
    }
    return true;
}

static glm::mat4 buildNodeLocalMatrix( const tinygltf::Node &node ) {
    if (node.matrix.size() == 16)
    {
        return glm::make_mat4( node.matrix.data() );
    }

    glm::mat4 m( 1.0f );
    if (node.translation.size() == 3)
    {
        m = glm::translate( m, glm::vec3(
            (float)node.translation[ 0 ],
            (float)node.translation[ 1 ],
            (float)node.translation[ 2 ] ) );
    }
    if (node.rotation.size() == 4)
    {
        const glm::quat q(
            (float)node.rotation[ 3 ],
            (float)node.rotation[ 0 ],
            (float)node.rotation[ 1 ],
            (float)node.rotation[ 2 ] );
        m *= glm::mat4_cast( q );
    }
    if (node.scale.size() == 3)
    {
        m = glm::scale( m, glm::vec3(
            (float)node.scale[ 0 ],
            (float)node.scale[ 1 ],
            (float)node.scale[ 2 ] ) );
    }
    return m;
}

static bool readAccessorIndices( const tinygltf::Model &gltf, int accessorIndex, std::vector<uint32_t> &out );

static bool readAccessorVec2( const tinygltf::Model &gltf, int accessorIndex, std::vector<glm::vec2> &out ) {
    if (accessorIndex < 0 || accessorIndex >= (int)gltf.accessors.size()) return false;
    const auto &acc = gltf.accessors[ accessorIndex ];
    if (acc.type != TINYGLTF_TYPE_VEC2) return false;
    if (acc.bufferView < 0 || acc.bufferView >= (int)gltf.bufferViews.size()) return false;
    const auto &view = gltf.bufferViews[ acc.bufferView ];
    if (view.buffer < 0 || view.buffer >= (int)gltf.buffers.size()) return false;

    const auto &buf = gltf.buffers[ view.buffer ];
    size_t stride = acc.ByteStride( view );
    if (stride == 0)
    {
        size_t compSize = sizeof( float );
        if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE || acc.componentType == TINYGLTF_COMPONENT_TYPE_BYTE) compSize = sizeof( uint8_t );
        else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT || acc.componentType == TINYGLTF_COMPONENT_TYPE_SHORT) compSize = sizeof( uint16_t );
        else if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) return false;
        stride = compSize * 2;
    }

    const uint8_t *src = buf.data.data() + view.byteOffset + acc.byteOffset;
    out.resize( acc.count );
    for (size_t i = 0; i < acc.count; ++i)
    {
        const uint8_t *p = src + i * stride;
        if (acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
        {
            const float *f = reinterpret_cast<const float *>( p );
            out[ i ] = glm::vec2( f[ 0 ], f[ 1 ] );
        }
        else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
        {
            const uint8_t *u = reinterpret_cast<const uint8_t *>( p );
            out[ i ] = glm::vec2( float( u[ 0 ] ) / 255.0f, float( u[ 1 ] ) / 255.0f );
        }
        else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
        {
            const uint16_t *u = reinterpret_cast<const uint16_t *>( p );
            out[ i ] = glm::vec2( float( u[ 0 ] ) / 65535.0f, float( u[ 1 ] ) / 65535.0f );
        }
        else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_BYTE)
        {
            const int8_t *s = reinterpret_cast<const int8_t *>( p );
            out[ i ] = glm::vec2( std::clamp( float( s[ 0 ] ) / 127.0f, -1.0f, 1.0f ), std::clamp( float( s[ 1 ] ) / 127.0f, -1.0f, 1.0f ) );
            out[ i ] = out[ i ] * 0.5f + glm::vec2( 0.5f );
        }
        else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_SHORT)
        {
            const int16_t *s = reinterpret_cast<const int16_t *>( p );
            out[ i ] = glm::vec2( std::clamp( float( s[ 0 ] ) / 32767.0f, -1.0f, 1.0f ), std::clamp( float( s[ 1 ] ) / 32767.0f, -1.0f, 1.0f ) );
            out[ i ] = out[ i ] * 0.5f + glm::vec2( 0.5f );
        }
        else
        {
            return false;
        }
    }
    return true;
}

static int appendBaseColorTexture( const tinygltf::Model &gltf, const tinygltf::Primitive &primitive, CpuModel &out ) {
    if (primitive.material < 0 || primitive.material >= (int)gltf.materials.size()) return -1;

    const auto &mat = gltf.materials[ primitive.material ];
    int texIndex = mat.pbrMetallicRoughness.baseColorTexture.index;
    if (texIndex < 0 || texIndex >= (int)gltf.textures.size()) return -1;

    int imageIndex = gltf.textures[ texIndex ].source;
    if (imageIndex < 0 || imageIndex >= (int)gltf.images.size()) return -1;

    const auto &img = gltf.images[ imageIndex ];
    if (img.width <= 0 || img.height <= 0 || img.image.empty()) return -1;

    const int comp = std::max( 1, img.component );
    int bytesPerComponent = 1;
    if (img.bits > 0)
    {
        bytesPerComponent = std::max( 1, (img.bits + 7) / 8 );
    }
    else if (img.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT || img.pixel_type == TINYGLTF_COMPONENT_TYPE_SHORT)
    {
        bytesPerComponent = 2;
    }
    else if (img.pixel_type == TINYGLTF_COMPONENT_TYPE_FLOAT || img.pixel_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
    {
        bytesPerComponent = 4;
    }

    const size_t pixelStride = size_t( comp ) * size_t( bytesPerComponent );
    if (pixelStride == 0) return -1;

    Image tex;
    tex.width = img.width;
    tex.height = img.height;
    tex.pixels.resize( img.width * img.height );

    auto readChannel8 = [&]( const uint8_t *px, size_t pxBytes, int channel )->Uint8 {
        channel = std::clamp( channel, 0, comp - 1 );
        const size_t off = size_t( channel ) * size_t( bytesPerComponent );
        if (off >= pxBytes) return 255;

        const uint8_t *src = px + off;
        const size_t avail = pxBytes - off;

        if (bytesPerComponent == 1)
        {
            return src[ 0 ];
        }
        if (bytesPerComponent == 2)
        {
            if (avail < 2) return src[ 0 ];
            const uint16_t v = uint16_t( src[ 0 ] ) | (uint16_t( src[ 1 ] ) << 8);
            return Uint8( v / 257u );
        }
        if (bytesPerComponent == 4 && img.pixel_type == TINYGLTF_COMPONENT_TYPE_FLOAT)
        {
            if (avail < 4) return src[ 0 ];
            float f = 0.0f;
            std::memcpy( &f, src, sizeof( float ) );
            return Uint8( std::clamp( f * 255.0f, 0.0f, 255.0f ) );
        }

        return src[ 0 ];
        };

    for (int y = 0; y < img.height; ++y)
    {
        for (int x = 0; x < img.width; ++x)
        {
            const size_t i = size_t( y * img.width + x ) * pixelStride;
            if (i >= img.image.size())
            {
                tex.pixels[ y * img.width + x ] = rgb( 255, 255, 255 );
                continue;
            }
            const size_t pxBytes = std::min( pixelStride, img.image.size() - i );
            const uint8_t *px = img.image.data() + i;

            const Uint8 r = readChannel8( px, pxBytes, 0 );
            const Uint8 g = (comp > 1) ? readChannel8( px, pxBytes, 1 ) : r;
            const Uint8 b = (comp > 2) ? readChannel8( px, pxBytes, 2 ) : r;
            tex.pixels[ y * img.width + x ] = rgb( r, g, b );
        }
    }

    int newIndex = (int)out.baseColorTextures.size();
    out.baseColorTextures.push_back( std::move( tex ) );
    return newIndex;
}

static bool readAccessorColor3( const tinygltf::Model &gltf, int accessorIndex, std::vector<glm::vec3> &out ) {
    if (accessorIndex < 0 || accessorIndex >= (int)gltf.accessors.size()) return false;
    const auto &acc = gltf.accessors[ accessorIndex ];
    if (acc.bufferView < 0 || acc.bufferView >= (int)gltf.bufferViews.size()) return false;
    const auto &view = gltf.bufferViews[ acc.bufferView ];
    if (view.buffer < 0 || view.buffer >= (int)gltf.buffers.size()) return false;
    const auto &buf = gltf.buffers[ view.buffer ];

    const int comps = (acc.type == TINYGLTF_TYPE_VEC4) ? 4 : ((acc.type == TINYGLTF_TYPE_VEC3) ? 3 : 0);
    if (comps == 0) return false;

    size_t stride = acc.ByteStride( view );
    if (stride == 0)
    {
        size_t compSize = 4;
        if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) compSize = 1;
        else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) compSize = 2;
        stride = compSize * comps;
    }

    const uint8_t *src = buf.data.data() + view.byteOffset + acc.byteOffset;
    out.resize( acc.count, glm::vec3( 1.0f ) );

    for (size_t i = 0; i < acc.count; ++i)
    {
        const uint8_t *p = src + i * stride;
        auto readComp = [&]( int c )->float {
            if (acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                return reinterpret_cast<const float *>( p )[ c ];
            }
            if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
            {
                const float v = float( reinterpret_cast<const uint8_t *>( p )[ c ] );
                return acc.normalized ? (v / 255.0f) : v;
            }
            if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
            {
                const float v = float( reinterpret_cast<const uint16_t *>( p )[ c ] );
                return acc.normalized ? (v / 65535.0f) : v;
            }
            return 1.0f;
            };

        out[ i ].r = std::clamp( readComp( 0 ), 0.0f, 1.0f );
        out[ i ].g = std::clamp( readComp( 1 ), 0.0f, 1.0f );
        out[ i ].b = std::clamp( readComp( 2 ), 0.0f, 1.0f );
    }

    return true;
}

static void appendMeshPrimitiveToCpuModel(
    const tinygltf::Model &gltf,
    const tinygltf::Primitive &primitive,
    const glm::mat4 &world,
    CpuModel &out ) {
    if (primitive.mode != -1 && primitive.mode != TINYGLTF_MODE_TRIANGLES)
    {
        return;
    }

    auto posIt = primitive.attributes.find( "POSITION" );
    if (posIt == primitive.attributes.end()) return;

    std::vector<glm::vec3> pos;
    if (!readAccessorVec3( gltf, posIt->second, pos )) return;

    std::vector<glm::vec2> uv0;
    std::vector<glm::vec2> uv1;
    std::vector<glm::vec2> uv;
    bool hasValidUv = false;
    auto uv0It = primitive.attributes.find( "TEXCOORD_0" );
    if (uv0It != primitive.attributes.end())
    {
        hasValidUv = readAccessorVec2( gltf, uv0It->second, uv0 );
    }

    auto uv1It = primitive.attributes.find( "TEXCOORD_1" );
    bool hasValidUv1 = false;
    if (uv1It != primitive.attributes.end())
    {
        hasValidUv1 = readAccessorVec2( gltf, uv1It->second, uv1 );
    }

    int baseTexCoordSet = 0;
    if (primitive.material >= 0 && primitive.material < (int)gltf.materials.size())
    {
        baseTexCoordSet = gltf.materials[ primitive.material ].pbrMetallicRoughness.baseColorTexture.texCoord;
    }

    if (baseTexCoordSet == 1 && hasValidUv1 && uv1.size() == pos.size())
    {
        uv = uv1;
        hasValidUv = true;
    }
    else if (hasValidUv && uv0.size() == pos.size())
    {
        uv = uv0;
        hasValidUv = true;
    }

    if (uv.size() != pos.size())
    {
        uv.assign( pos.size(), glm::vec2( 0.0f ) );
        hasValidUv = false;
    }

    glm::vec4 materialBaseColorFactor( 1.0f );
    if (primitive.material >= 0 && primitive.material < (int)gltf.materials.size())
    {
        const auto &mat = gltf.materials[ primitive.material ];
        const auto &factor = mat.pbrMetallicRoughness.baseColorFactor;
        if (factor.size() >= 4)
        {
            materialBaseColorFactor = glm::vec4(
                (float)factor[ 0 ],
                (float)factor[ 1 ],
                (float)factor[ 2 ],
                (float)factor[ 3 ] );
        }
    }

    std::vector<glm::vec3> vcol;
    auto colorIt = primitive.attributes.find( "COLOR_0" );
    if (colorIt != primitive.attributes.end())
    {
        (void)readAccessorColor3( gltf, colorIt->second, vcol );
    }
    if (vcol.size() != pos.size())
    {
        vcol.assign( pos.size(), glm::vec3( 1.0f ) );
    }

    std::vector<uint32_t> idx;
    if (primitive.indices >= 0)
    {
        if (!readAccessorIndices( gltf, primitive.indices, idx )) return;
    }
    else
    {
        idx.resize( pos.size() );
        for (uint32_t i = 0; i < (uint32_t)pos.size(); ++i) idx[ i ] = i;
    }

    for (uint32_t i : idx)
    {
        if (i >= pos.size()) return;
    }

    const uint32_t base = (uint32_t)out.vertices.size();
    out.vertices.reserve( out.vertices.size() + pos.size() );
    out.colors.reserve( out.colors.size() + pos.size() );
    out.uvs.reserve( out.uvs.size() + pos.size() );
    for (const auto &v : pos)
    {
        const size_t vi = size_t( &v - pos.data() );
        const glm::vec3 tv = glm::vec3( world * glm::vec4( v, 1.0f ) );
        out.vertices.push_back( tv );
        out.colors.push_back( vcol[ vi ] );
        out.uvs.push_back( uv[ vi ] );
        out.boundsMin = glm::min( out.boundsMin, tv );
        out.boundsMax = glm::max( out.boundsMax, tv );
    }

    int primitiveTextureIndex = appendBaseColorTexture( gltf, primitive, out );
    if (!hasValidUv)
    {
        primitiveTextureIndex = -1;
    }

    out.indices.reserve( out.indices.size() + idx.size() );
    for (uint32_t i : idx)
    {
        out.indices.push_back( base + i );
    }

    const int triCount = (int)(idx.size() / 3);
    if (triCount > 0)
    {
        out.triangleTextureIndex.reserve( out.triangleTextureIndex.size() + triCount );
        out.triangleBaseColorFactor.reserve( out.triangleBaseColorFactor.size() + triCount );
        for (int t = 0; t < triCount; ++t)
        {
            out.triangleTextureIndex.push_back( primitiveTextureIndex );
            out.triangleBaseColorFactor.push_back( materialBaseColorFactor );
        }
    }
}

static bool readAccessorIndices( const tinygltf::Model &gltf, int accessorIndex, std::vector<uint32_t> &out ) {
    if (accessorIndex < 0 || accessorIndex >= (int)gltf.accessors.size()) return false;
    const auto &acc = gltf.accessors[ accessorIndex ];
    if (acc.bufferView < 0 || acc.bufferView >= (int)gltf.bufferViews.size()) return false;
    const auto &view = gltf.bufferViews[ acc.bufferView ];
    if (view.buffer < 0 || view.buffer >= (int)gltf.buffers.size()) return false;
    const auto &buf = gltf.buffers[ view.buffer ];

    size_t stride = acc.ByteStride( view );
    if (stride == 0)
    {
        stride = (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) ? 2 :
            (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE ? 1 : 4);
    }

    const uint8_t *src = buf.data.data() + view.byteOffset + acc.byteOffset;
    out.resize( acc.count );
    for (size_t i = 0; i < acc.count; ++i)
    {
        const uint8_t *p = src + i * stride;
        switch (acc.componentType)
        {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            out[ i ] = *reinterpret_cast<const uint8_t *>( p );
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            out[ i ] = *reinterpret_cast<const uint16_t *>( p );
            break;
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            out[ i ] = *reinterpret_cast<const uint32_t *>( p );
            break;
        default:
            return false;
        }
    }
    return true;
}

static std::shared_ptr<CpuModel> loadCpuModel( const std::string &modelPath ) {
    auto hit = g_cpuModelCache.find( modelPath );
    if (hit != g_cpuModelCache.end()) return hit->second;

    tinygltf::TinyGLTF loader;
    tinygltf::Model gltf;
    std::string warn;
    std::string err;

    bool ok = false;
    if (modelPath.size() >= 4 && modelPath.substr( modelPath.size() - 4 ) == ".glb")
        ok = loader.LoadBinaryFromFile( &gltf, &err, &warn, modelPath );
    else
        ok = loader.LoadASCIIFromFile( &gltf, &err, &warn, modelPath );

    if (!ok)
    {
        g_cpuModelCache[ modelPath ] = nullptr;
        return nullptr;
    }

    auto out = std::make_shared<CpuModel>();
    out->boundsMin = glm::vec3( std::numeric_limits<float>::max() );
    out->boundsMax = glm::vec3( std::numeric_limits<float>::lowest() );

    bool wroteAny = false;
    auto appendMeshByNode = [&]( int meshIndex, const glm::mat4 &world ) {
        if (meshIndex < 0 || meshIndex >= (int)gltf.meshes.size()) return;
        const auto &mesh = gltf.meshes[ meshIndex ];
        for (const auto &primitive : mesh.primitives)
        {
            const size_t beforeCount = out->vertices.size();
            appendMeshPrimitiveToCpuModel( gltf, primitive, world, *out );
            if (out->vertices.size() > beforeCount) wroteAny = true;
        }
    };

    std::function<void( int, const glm::mat4& )> walkNode;
    walkNode = [&]( int nodeIndex, const glm::mat4 &parentWorld ) {
        if (nodeIndex < 0 || nodeIndex >= (int)gltf.nodes.size()) return;
        const auto &node = gltf.nodes[ nodeIndex ];
        const glm::mat4 world = parentWorld * buildNodeLocalMatrix( node );
        appendMeshByNode( node.mesh, world );
        for (int child : node.children)
        {
            walkNode( child, world );
        }
    };

    int sceneIndex = gltf.defaultScene;
    if (sceneIndex < 0 && !gltf.scenes.empty()) sceneIndex = 0;
    if (sceneIndex >= 0 && sceneIndex < (int)gltf.scenes.size())
    {
        const auto &scene = gltf.scenes[ sceneIndex ];
        for (int rootNode : scene.nodes)
        {
            walkNode( rootNode, glm::mat4( 1.0f ) );
        }
    }

    if (!wroteAny)
    {
        for (size_t meshIndex = 0; meshIndex < gltf.meshes.size(); ++meshIndex)
        {
            appendMeshByNode( (int)meshIndex, glm::mat4( 1.0f ) );
        }
    }

    if (out->vertices.empty() || out->indices.size() < 3)
    {
        g_cpuModelCache[ modelPath ] = nullptr;
        return nullptr;
    }

    g_cpuModelCache[ modelPath ] = out;
    return out;
}

static float modelScaleOverride( const std::string &modelPath ) {
    const std::string lower = std::filesystem::path( modelPath ).filename().string();
    if (lower.find( "Plant" ) != std::string::npos) return 0.92f;
    if (lower.find( "Pedestal" ) != std::string::npos) return 0.95f;
    if (lower.find( "Vase2" ) != std::string::npos) return 0.58f;
    if (lower.find( "Vase1" ) != std::string::npos) return 0.86f;
    if (lower.find( "Vase3" ) != std::string::npos) return 0.82f;
    if (lower.find( "Bronze Key" ) != std::string::npos) return 1.30f;
    if (lower.find( "Silver Key" ) != std::string::npos) return 1.30f;
    if (lower.find( "Gold Key" ) != std::string::npos) return 1.30f;
    if (lower.find( "Iron Key" ) != std::string::npos) return 1.30f;
    return 1.0f;
}

static int addWorldModelInstance(
    const std::string &modelPath,
    float x,
    float y,
    float scale,
    Uint32 tint,
    float yaw = 0.0f,
    float pitch = 0.0f,
    float roll = 0.0f,
    bool spinYaw = false,
    float spinSpeed = 0.0f,
    float heightOffset = 0.0f ) {
    auto model = loadCpuModel( modelPath );
    if (!model) return -1;

    const float modelSizeX = std::max( 0.0001f, std::fabs( model->boundsMax.x - model->boundsMin.x ) );
    const float modelSizeY = std::max( 0.0001f, std::fabs( model->boundsMax.y - model->boundsMin.y ) );
    const float modelSizeZ = std::max( 0.0001f, std::fabs( model->boundsMax.z - model->boundsMin.z ) );
    const float modelReferenceSize = std::max( { modelSizeX, modelSizeY, modelSizeZ } );
    const float targetWorldSize = std::max( 0.02f, scale );
    const float overrideMul = modelScaleOverride( modelPath );

    WorldModelInstance inst;
    inst.model = std::move( model );
    inst.x = x;
    inst.y = y;
    inst.heightOffset = heightOffset;
    inst.scale = std::clamp( (targetWorldSize / modelReferenceSize) * overrideMul, 0.01f, 2.0f );
    inst.tint = tint;
    inst.yaw = yaw;
    inst.pitch = pitch;
    inst.roll = roll;
    inst.spinYaw = spinYaw;
    inst.spinSpeed = spinSpeed;
    inst.editorTargetScale = targetWorldSize;
    int index = (int)g_worldModels.size();
    g_worldModels.push_back( std::move( inst ) );
    return index;
}

static float g_caveTimerSeconds = 120.0f;
static bool g_caveTimerActive = false;
static constexpr float kUpperEntryX = 20.6f;
static constexpr float kUpperEntryY = 9.3f;
static constexpr float kUpperEntryRadius = 0.85f;

struct LevelDef
{
    string name;
    string folder;
    string mapFile = "map.txt";
    float spawnX = 2.0f, spawnY = 9.5f, spawnDirDeg = 0.f;
    int levelId = 0;
    Uint32 ambianceTint = rgb( 255, 255, 255 );
    float ambianceMul = 1.0f;
    bool isMuseumFloor = false;
    std::string objectiveLabel;
};

enum class InteractionAnimType
{
    NONE,
    ITEM_PICKUP,
    KEY_USE,
    NOTE_COLLECT,
    DOOR_USE
};

struct LevelTransitionState
{
    bool active = false;
    bool switched = false;
    float t = 0.0f;
    float duration = 1.05f;
    Levels targetLevel = Levels::MUSEUM;
};

struct InteractionAnimState
{
    bool active = false;
    float t = 0.0f;
    float duration = 0.55f;
    InteractionAnimType type = InteractionAnimType::NONE;
    std::string label;
};

static LevelTransitionState g_levelTransition;
static InteractionAnimState g_interactionAnim;
static bool g_perfLowMode = false;
static float g_wallRenderDistance = 42.0f;
static float g_worldModelRenderDistance = 22.0f;
static float g_horrorLightingMul = 0.72f;
static float g_horrorDarknessOverlay = 0.14f;
static int g_meshTriangleStride = 1;
static int g_meshRasterStep = 1;
static bool g_useGpuModelRendering = false;
static float g_lastEffectivePitchOffset = 0.0f;
static bool g_detectedPerfLowMode = false;
static float g_detectedWallRenderDistance = 42.0f;
static float g_detectedWorldModelRenderDistance = 22.0f;
static std::string g_rendererBackend = "unknown";
static bool g_multithreadingEnabled = true;
static unsigned int g_logicalThreadCount = std::max( 1u, std::thread::hardware_concurrency() );
static unsigned int g_detectedThreadCount = 1u;

static std::vector<std::thread> g_rasterWorkers;
static std::mutex g_rasterWorkMutex;
static std::condition_variable g_rasterWorkCv;
static std::condition_variable g_rasterDoneCv;
static bool g_rasterPoolShutdown = false;
static bool g_rasterJobAvailable = false;
static uint64_t g_rasterJobSerial = 0;
static unsigned int g_rasterJobWorkerCount = 0;
static unsigned int g_rasterJobCompleted = 0;
static Engine *g_rasterJobEngine = nullptr;
static std::vector<float> *g_rasterJobMeshDepth = nullptr;
static const std::vector<float> *g_rasterJobWallDepth = nullptr;
static float g_rasterJobPitchOffset = 0.0f;

static unsigned int estimateCpuRasterWorkerThreads( unsigned int logicalThreadCount ) {
    logicalThreadCount = std::max( 1u, logicalThreadCount );

    // Heuristic: on SMT/HT systems, using all logical threads for this memory-heavy raster path
    // can hurt due to sibling-core contention and cache pressure.
    unsigned int physicalLike = logicalThreadCount;
    if (logicalThreadCount >= 8 && (logicalThreadCount % 2u) == 0u)
    {
        physicalLike = logicalThreadCount / 2u;
    }

    return std::max( 1u, physicalLike );
}

static int getGpuRenderMode() {
    return std::clamp( config::gpuRenderMode, 0, 2 );
}

static int getAntiAliasingMode() {
    return std::clamp( config::antiAliasingMode, 0, 2 );
}

static bool isGpuModelRenderingEnabled() {
    return g_useGpuModelRendering && getGpuRenderMode() != 0;
}

static bool shouldGpuRenderModel( Engine const &engineContext, WorldModelInstance const &inst ) {
    if (!isGpuModelRenderingEnabled()) return false;
    if (getGpuRenderMode() == 2) return true; // Full

    // Smart mode: offload nearby or complex models to GPU, keep trivial/far meshes on CPU.
    const float dx = inst.x - engineContext.positionX;
    const float dy = inst.y - engineContext.positionY;
    const float distSq = dx * dx + dy * dy;
    const int triCount = inst.model ? int( inst.model->indices.size() / 3 ) : 0;

    return distSq <= (12.0f * 12.0f) || triCount >= 220;
}

static const char *gpuRenderModeLabel() {
    switch (getGpuRenderMode())
    {
    case 0: return "NONE";
    case 2: return "FULL";
    default: return "SMART";
    }
}

static void applyQualityPresetFromConfig();
static void applyPresentationFilter( Engine &engineContext );

static void configureRuntimeGpuProfile( SDL_Renderer *renderer ) {
    const char *rendererName = renderer ? SDL_GetRendererName( renderer ) : nullptr;
    g_rendererBackend = rendererName ? rendererName : "unknown";
    const std::string backendLower = toLowerCopy( g_rendererBackend );

    const bool softwareLike =
        backendLower.find( "software" ) != std::string::npos ||
        backendLower.find( "llvmpipe" ) != std::string::npos ||
        backendLower.find( "swiftshader" ) != std::string::npos ||
        backendLower.find( "warp" ) != std::string::npos;

    const bool gpuTierHigh =
        backendLower.find( "vulkan" ) != std::string::npos ||
        backendLower.find( "direct3d" ) != std::string::npos ||
        backendLower.find( "metal" ) != std::string::npos ||
        backendLower.find( "opengl" ) != std::string::npos;

    g_detectedPerfLowMode = softwareLike;
    g_useGpuModelRendering = !softwareLike && gpuTierHigh;
    g_detectedWallRenderDistance = gpuTierHigh ? 52.0f : 40.0f;
    g_detectedWorldModelRenderDistance = gpuTierHigh ? 26.0f : 20.0f;
    if (g_detectedPerfLowMode)
    {
        g_detectedWallRenderDistance = 26.0f;
        g_detectedWorldModelRenderDistance = 14.0f;
    }

    applyQualityPresetFromConfig();

    std::cout << "[Renderer] " << g_rendererBackend
        << " | gpuModelRendering=" << (g_useGpuModelRendering ? "true" : "false")
        << " | gpuMode=" << gpuRenderModeLabel()
        << " | cpuRasterThreads=" << g_detectedThreadCount << "/" << g_logicalThreadCount
        << " | perfLowMode=" << (g_perfLowMode ? "true" : "false")
        << " | wallRenderDistance=" << g_wallRenderDistance
        << " | modelRenderDistance=" << g_worldModelRenderDistance
        << std::endl;
}

static void applyQualityPresetFromConfig() {
    switch (config::modelQualityPreset)
    {
    case 0: // High
        g_perfLowMode = false;
        g_wallRenderDistance = std::clamp( g_detectedWallRenderDistance * 1.15f, 32.0f, 72.0f );
        g_worldModelRenderDistance = std::clamp( g_detectedWorldModelRenderDistance * 1.18f, 16.0f, 40.0f );
        g_meshTriangleStride = 1;
        g_meshRasterStep = 1;
        break;
    case 2: // Performance
        g_perfLowMode = true;
        g_wallRenderDistance = std::clamp( g_detectedWallRenderDistance * 0.85f, 18.0f, 44.0f );
        g_worldModelRenderDistance = std::clamp( g_detectedWorldModelRenderDistance * 0.72f, 10.0f, 26.0f );
        g_meshTriangleStride = 3;
        g_meshRasterStep = 2;
        break;
    default: // Balanced
        g_perfLowMode = g_detectedPerfLowMode;
        g_wallRenderDistance = g_detectedWallRenderDistance;
        g_worldModelRenderDistance = g_detectedWorldModelRenderDistance;
        g_meshTriangleStride = g_detectedPerfLowMode ? 2 : 1;
        g_meshRasterStep = g_detectedPerfLowMode ? 2 : 1;
        break;
    }
}

enum GameState
{
    STATE_MENU,
    STATE_GAME,
    STATE_ENDING,
    STATE_MIND_TRAP
};

static void showAccessPopup( const std::string &msg, Uint32 durationMs = 2200 ) {
    const bool isAcquiredMsg =
        (msg.find( "Acquired" ) != std::string::npos) ||
        (msg.find( "acquired" ) != std::string::npos) ||
        (msg.find( "ACQUIRED" ) != std::string::npos);
    if (isAcquiredMsg && (g_cutsceneController.isCameraLockActive() || g_revolverInspectCutsceneActive))
    {
        return;
    }

    g_accessPopup = msg;
    g_accessPopupUntil = SDL_GetTicks() + durationMs;
}

static bool isPlayerNearPoint( Engine const &engineContext, float px, float py, float tolerance = 1.0f ) {
    float dx = engineContext.positionX - px;
    float dy = engineContext.positionY - py;
    return (dx * dx + dy * dy) <= (tolerance * tolerance);
}

static bool isMuseumLikeLevel( Levels level ) {
    return level == Levels::MUSEUM || level == Levels::MUSEUM_UPPER;
}

static bool museumPowerFlickerLowPhase() {
    if (!g_powerRestoreFlickerActive) return false;
    const float t = g_powerRestoreFlickerTimer;
    return
        (t >= 0.06f && t <= 0.11f) ||
        (t >= 0.18f && t <= 0.24f) ||
        (t >= 0.33f && t <= 0.39f);
}

static float museumPowerLightMultiplierForLevel( Levels level ) {
    if (!isMuseumLikeLevel( level )) return 1.0f;
    if (!g_generatorFueled) return 0.56f;
    if (museumPowerFlickerLowPhase()) return 0.44f;
    return 1.12f;
}

static float museumPowerDarknessAddForLevel( Levels level ) {
    if (!isMuseumLikeLevel( level )) return 0.0f;
    if (!g_generatorFueled) return 0.14f;
    if (museumPowerFlickerLowPhase()) return 0.12f;
    return -0.06f;
}

static bool hasRestorationPigments() {
    return g_playerKeys.contains( "BLACK PIGMENT" ) &&
        g_playerKeys.contains( "BLUE PIGMENT" ) &&
        g_playerKeys.contains( "RED PIGMENT" );
}

static std::string resolveFirstExistingAsset( std::initializer_list<const char *> assetCandidates ) {
    for (const char *candidate : assetCandidates)
    {
        const std::string path = resolveAssetModelPath( candidate );
        if (std::filesystem::exists( path ))
        {
            return path;
        }
    }
    return resolveAssetModelPath( *assetCandidates.begin() );
}

static bool isPlayerNearDirectorDesk( Engine const &engineContext ) {
    if (engineContext.currentLevel != Levels::MUSEUM) return false;
    return isPlayerNearPoint( engineContext, 16.36f, 16.55f, 1.45f );
}

static bool isPlayerInsideUpperStudio( Engine const &engineContext ) {
    if (engineContext.currentLevel != Levels::MUSEUM_UPPER) return false;
    return engineContext.positionY < 7.0f && engineContext.positionX >= 7.0f && engineContext.positionX <= 15.0f;
}

static bool isRevolverNearby( Engine const &engineContext, float radius = 1.0f ) {
    if (g_revolverPickup.collected) return false;
    if (g_revolverPickup.level != engineContext.currentLevel) return false;
    float dx = engineContext.positionX - g_revolverPickup.x;
    float dy = engineContext.positionY - g_revolverPickup.y;
    return (dx * dx + dy * dy) <= (radius * radius);
}

static void updateHeldRevolverModel( Engine &engineContext ) {
    const bool shouldShow =
        g_combatState.active &&
        g_combatState.hasRevolver &&
        g_showHeldWeapon &&
        !g_cutsceneController.isCameraLockActive() &&
        !g_revolverInspectCutsceneActive;

    if (!shouldShow)
    {
        if (g_heldRevolverModelIndex >= 0 && g_heldRevolverModelIndex < (int)g_worldModels.size())
        {
            g_worldModels[ g_heldRevolverModelIndex ].visible = false;
        }
        return;
    }

    const float yaw = std::atan2( engineContext.directionY, engineContext.directionX );
    const float rightX = -engineContext.directionY;
    const float rightY = engineContext.directionX;

    const float recoil01 = std::clamp( g_revolverRecoilTimer / kRevolverRecoilDuration, 0.0f, 1.0f );
    const float recoilKick = std::pow( recoil01, 0.65f );

    float forward = 0.34f;
    float right = 0.20f;
    float height = 0.30f;
    float pitch = 0.06f;
    float roll = -0.22f;

    if (g_revolverAiming)
    {
        forward = 0.42f;
        right = 0.01f;
        height = 0.28f;
        pitch = 0.02f;
        roll = -0.04f;
    }

    if (g_revolverInspectCutsceneActive)
    {
        forward = 0.56f;
        right = 0.04f;
        height = 0.38f;
        pitch = 0.38f;
        roll = -0.16f;
    }

    forward -= 0.15f * recoilKick;
    height += 0.05f * recoilKick;
    pitch -= 1.05f * recoilKick;
    roll += 0.14f * recoilKick;

    const float px = engineContext.positionX + engineContext.directionX * forward + rightX * right;
    const float py = engineContext.positionY + engineContext.directionY * forward + rightY * right;

    if (g_heldRevolverModelIndex < 0 || g_heldRevolverModelIndex >= (int)g_worldModels.size())
    {
        g_heldRevolverModelIndex = addWorldModelInstance(
            resolveFirstExistingAsset( { "Revolver.glb", "revolver.glb", "SurgicalKnife.glb" } ),
            px,
            py,
            0.25f,
            rgb( 175, 180, 190 ),
            -yaw + kRevolverFacingYawOffset,
            pitch,
            roll,
            false,
            0.0f,
            height );
    }

    if (g_heldRevolverModelIndex >= 0 && g_heldRevolverModelIndex < (int)g_worldModels.size())
    {
        auto &held = g_worldModels[ g_heldRevolverModelIndex ];
        held.visible = true;
        held.x = px;
        held.y = py;
        held.yaw = -yaw + kRevolverFacingYawOffset;
        held.pitch = pitch;
        held.roll = roll;
        held.heightOffset = height;
    }
}

static bool isRestorationGateDoorTile( int tx, int ty ) {
    return tx == 16 && ty == 9;
}

static void applyUnlockAllDoorsOverride( Engine &engineContext ) {
    for (auto &lock : g_roomLocks)
    {
        lock.unlocked = true;
        int idx = lock.ty * engineContext.map.width + lock.tx;
        if ((unsigned)idx < (unsigned)engineContext.map.tiles.size() && engineContext.map.tiles[ idx ] == 2)
        {
            engineContext.map.tiles[ idx ] = 0;
        }
    }

    for (auto &tile : engineContext.map.tiles)
    {
        if (tile == 2)
        {
            tile = 0;
        }
    }

    g_restorationWingUnlocked = true;
}

static void beginLevelTransition( Levels target, float seconds = 1.05f ) {
    g_levelTransition.active = true;
    g_levelTransition.switched = false;
    g_levelTransition.t = 0.0f;
    g_levelTransition.duration = std::max( 0.2f, seconds );
    g_levelTransition.targetLevel = target;
}

static void triggerInteractionAnim( InteractionAnimType type, const std::string &label, float seconds = 0.55f ) {
    if (type == InteractionAnimType::ITEM_PICKUP &&
        ((label.find( "ACQUIRED" ) != std::string::npos) ||
            (label.find( "Acquired" ) != std::string::npos) ||
            (label.find( "acquired" ) != std::string::npos)) &&
        (g_cutsceneController.isCameraLockActive() || g_revolverInspectCutsceneActive))
    {
        return;
    }

    constexpr float kAnimDurationScale = 1.28f;
    g_interactionAnim.active = true;
    g_interactionAnim.t = 0.0f;
    g_interactionAnim.duration = std::max( 0.18f, seconds * kAnimDurationScale );
    g_interactionAnim.type = type;
    g_interactionAnim.label = label;
}

static void renderCaveQuiz( Engine &engineContext ) {
    if (!g_caveQuizActive || g_caveQuiz.empty()) return;
    if (g_caveQuizQuestionIndex < 0 || g_caveQuizQuestionIndex >= (int)g_caveQuiz.size()) return;

    const auto &q = g_caveQuiz[ g_caveQuizQuestionIndex ];

    int panelW = RENDER_W - 140;
    int panelH = 220;
    int x = (RENDER_W - panelW) / 2;
    int y = (RENDER_H - panelH) / 2;

    drawTextBox( engineContext, x, y, panelW, panelH, rgb( 12, 12, 16 ), rgb( 180, 150, 60 ) );
    std::string header = "Warden Statue " + std::to_string( g_caveQuizQuestionIndex + 1 ) + "/" + std::to_string( g_caveQuiz.size() );
    drawString16x16( engineContext, x + 16, y + 14, header, rgb( 255, 225, 120 ), panelW - 32, 1, 1, false );

    int cy = y + 46;
    cy = drawWrappedText( engineContext, x + 16, cy, q.question, rgb( 220, 220, 220 ), panelW - 32 );
    cy += 12;

    for (int i = 0; i < 4; ++i)
    {
        std::string line = std::to_string( i + 1 ) + ") " + q.options[ i ];
        drawString16x16( engineContext, x + 22, cy, line, rgb( 210, 210, 205 ), panelW - 44, 1, 1, false );
        cy += 24;
    }

    drawStringTinyScaled( engineContext, x + 16, y + panelH - 22, "Press 1-4 To Answer   ESC To Cancel", rgb( 130, 130, 145 ), 1, 1, 1, false );
}

static void renderMindTrapInterface( Engine &engineContext ) {
    if (!g_mindTrapActive) return;
    if (g_mindTrapPhaseIndex < 0 || g_mindTrapPhaseIndex >= (int)g_mindTrapPhases.size()) return;

    drawTextBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), rgb( 0, 0, 0 ) );

    const float flickerNoise =
        0.82f +
        0.12f * std::sin( g_mindTrapFlickerTimer * 33.0f ) +
        0.08f * std::sin( g_mindTrapFlickerTimer * 71.0f );
    const float flicker = std::clamp( flickerNoise, 0.38f, 1.0f );
    const Uint8 green = Uint8( std::clamp( 180.0f * flicker, 42.0f, 225.0f ) );
    const Uint8 greenDim = Uint8( std::clamp( 120.0f * flicker, 24.0f, 180.0f ) );
    const Uint32 mainText = rgb( 0, green, 22 );
    const Uint32 dimText = rgb( 0, greenDim, 12 );
    const Uint32 borderText = rgb( 0, Uint8( std::clamp( green + 18, 0, 255 ) ), 28 );

    for (int y = 8; y < RENDER_H; y += 4)
    {
        if (((y / 4) % 2) == 0)
        {
            drawTranslucentBox( engineContext, 0, y, RENDER_W, 1, rgb( 0, 44, 0 ), 0.16f );
        }
    }

    const bool twitch = (std::sin( g_mindTrapFlickerTimer * 19.0f ) > 0.94f);
    const int jitterX = twitch ? ((std::rand() % 3) - 1) : 0;

    int panelX = 52 + jitterX;
    int panelY = 54;
    int panelW = RENDER_W - 104;
    int panelH = RENDER_H - 108;

    drawTextBox( engineContext, panelX, panelY, panelW, panelH, rgb( 0, 0, 0 ), borderText );

    const std::string header = "INTERNAL DIAGNOSTIC // CONSCIOUSNESS THREAD";
    drawString16x16( engineContext, panelX + 18, panelY + 16, header, mainText, panelW - 36, 1, 1, false );

    const std::string phaseLabel = "PHASE " + std::to_string( g_mindTrapPhaseIndex + 1 ) + "/" + std::to_string( g_mindTrapPhases.size() );
    drawStringTinyScaled( engineContext, panelX + panelW - 132, panelY + 22, phaseLabel, dimText, 1, 1, 1, false );

    const int bodyX = panelX + 18;
    const int bodyY = panelY + 50;
    const int bodyW = panelW - 36;
    const int bodyH = panelH - 170;
    drawTextBox( engineContext, bodyX - 4, bodyY - 4, bodyW + 8, bodyH + 8, rgb( 0, 0, 0 ), rgb( 0, greenDim, 10 ) );

    const int lineStep = 22;
    const int visibleLines = std::max( 1, bodyH / lineStep );
    int start = 0;
    if ((int)g_mindTrapTerminalLog.size() > visibleLines)
    {
        start = (int)g_mindTrapTerminalLog.size() - visibleLines;
    }

    int ty = bodyY;
    for (int i = start; i < (int)g_mindTrapTerminalLog.size(); ++i)
    {
        drawStringTinyScaled( engineContext, bodyX, ty, g_mindTrapTerminalLog[ i ], dimText, 2, 1, 1, false );
        ty += lineStep;
        if (ty > bodyY + bodyH - lineStep) break;
    }

    if (!g_mindTrapTypingLine.empty() && ty <= bodyY + bodyH - lineStep)
    {
        const size_t count = std::min( g_mindTrapTypingChars, g_mindTrapTypingLine.size() );
        drawStringTinyScaled( engineContext, bodyX, ty, g_mindTrapTypingLine.substr( 0, count ), mainText, 2, 1, 1, false );
    }

    const MindTrapPhase &phase = g_mindTrapPhases[ g_mindTrapPhaseIndex ];
    int cmdY = panelY + panelH - 82;
    if (g_mindTrapAwaitingChoice)
    {
        const bool cursorOn = ((SDL_GetTicks() / 420) % 2) == 0;
        drawTextBox( engineContext, bodyX - 4, cmdY - 6, bodyW + 8, 76, rgb( 0, 0, 0 ), rgb( 0, greenDim, 12 ) );

        for (int i = 0; i < 3; ++i)
        {
            const bool selected = (i == g_mindTrapSelectedOption);
            const std::string marker = selected ? ">> " : "   ";
            const Uint32 col = selected ? mainText : dimText;
            drawStringTinyScaled( engineContext, bodyX, cmdY + i * 20, marker + phase.commands[ i ], col, 2, 1, 1, false );
        }

        std::string line = "MINDTRAP> " + phase.commands[ g_mindTrapSelectedOption ] + (cursorOn ? " _" : "  ");
        drawStringTinyScaled( engineContext, bodyX, cmdY + 60, line, mainText, 2, 1, 1, false );
    }
    else if (g_mindTrapReadyToExit)
    {
        drawStringTinyScaled( engineContext, bodyX, cmdY, "SYNC COMPLETE... TRANSFERRING CONTEXT", mainText, 2, 1, 1, false );
    }
    else
    {
        drawStringTinyScaled( engineContext, bodyX, cmdY, "TERMINAL BUSY...", dimText, 2, 1, 1, false );
    }

    if (g_mindTrapWhiteFlashTimer > 0.0f)
    {
        const float flash = std::clamp( g_mindTrapWhiteFlashTimer / 0.65f, 0.0f, 1.0f );
        const float alpha = std::clamp( std::pow( flash, 0.65f ), 0.0f, 1.0f );
        drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 255, 255, 255 ), alpha );
    }
}

static void renderInteractionAnimation( Engine &engineContext ) {
    if (!g_interactionAnim.active) return;

    float p = std::clamp( g_interactionAnim.t / std::max( 0.001f, g_interactionAnim.duration ), 0.0f, 1.0f );
    int w = 520;
    int h = 126;
    int camShiftX = int( std::sin( p * 3.14159265f ) * 14.0f );
    int x = (RENDER_W - w) / 2 + camShiftX;
    int y = RENDER_H - h - 28;

    drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), 0.18f );
    drawTextBox( engineContext, x, y, w, h, rgb( 8, 8, 12 ), rgb( 165, 138, 70 ) );
    drawString16x16( engineContext, x + 16, y + 12, g_interactionAnim.label, rgb( 235, 220, 170 ), w - 32, 1, 1, false );

    int stageY = y + 54;
    int stageH = 52;
    drawTextBox( engineContext, x + 14, stageY, w - 28, stageH, rgb( 12, 12, 16 ), rgb( 80, 80, 95 ) );

    if (g_interactionAnim.type == InteractionAnimType::KEY_USE)
    {
        int lockX = x + w - 118;
        int lockY = stageY + 12;
        drawTextBox( engineContext, lockX, lockY, 56, 28, rgb( 50, 44, 34 ), rgb( 170, 148, 96 ) );
        drawTextBox( engineContext, lockX + 18, lockY + 7, 20, 14, rgb( 18, 18, 18 ), rgb( 130, 110, 70 ) );

        float keyMotion = std::clamp( p * 0.85f, 0.0f, 1.0f );
        int keyStartX = x + 34;
        int keyTargetX = lockX + 10;
        int keyX = keyStartX + int( (keyTargetX - keyStartX) * keyMotion );
        int keyY = stageY + 26;

        Uint32 keyCol = rgb( 210, 176, 88 );
        for (int yy = -5; yy <= 5; ++yy)
        {
            for (int xx = -5; xx <= 5; ++xx)
            {
                if (xx * xx + yy * yy <= 25) putPix( engineContext, keyX + xx, keyY + yy, keyCol );
            }
        }
        for (int xx = 6; xx <= 30; ++xx) putPix( engineContext, keyX + xx, keyY, keyCol );
        putPix( engineContext, keyX + 26, keyY + 1, keyCol );
        putPix( engineContext, keyX + 27, keyY + 1, keyCol );
        putPix( engineContext, keyX + 26, keyY + 2, keyCol );

        if (p > 0.72f)
        {
            drawStringTinyScaled( engineContext, x + w - 176, y + h - 16, "Turning...", rgb( 220, 190, 120 ), 1, 1, 1, false );
        }
    }
    else if (g_interactionAnim.type == InteractionAnimType::NOTE_COLLECT)
    {
        int nx = x + 40;
        int ny = stageY + 6;
        drawTextBox( engineContext, nx, ny, 72, 40, rgb( 210, 198, 164 ), rgb( 120, 96, 64 ) );
        drawStringTinyScaled( engineContext, nx + 10, ny + 12, "Note", rgb( 70, 55, 36 ), 1, 1, 1, false );
    }
    else if (g_interactionAnim.type == InteractionAnimType::ITEM_PICKUP)
    {
        int cx = x + 70;
        int cy = stageY + 26;
        for (int yy = -8; yy <= 8; ++yy)
        {
            for (int xx = -8; xx <= 8; ++xx)
            {
                if (xx * xx + yy * yy <= 64) putPix( engineContext, cx + xx, cy + yy, rgb( 160, 200, 120 ) );
            }
        }
        drawStringTinyScaled( engineContext, x + 110, stageY + 22, "ACQUIRED", rgb( 180, 220, 145 ), 1, 1, 1, false );
    }
    else
    {
        int barX = x + 24;
        int barY = stageY + 18;
        int barW = w - 48;
        int barH = 16;
        drawTextBox( engineContext, barX, barY, barW, barH, rgb( 12, 12, 12 ), rgb( 90, 90, 105 ) );
        int fill = (int)((barW - 2) * p);
        for (int yy = barY + 1; yy < barY + barH - 1; ++yy)
        {
            for (int xx = barX + 1; xx < barX + 1 + fill; ++xx)
            {
                putPix( engineContext, xx, yy, rgb( 195, 165, 85 ) );
            }
        }
    }
}

static void renderLevelTransitionOverlay( Engine &engineContext ) {
    if (!g_levelTransition.active) return;

    float p = std::clamp( g_levelTransition.t / std::max( 0.001f, g_levelTransition.duration ), 0.0f, 1.0f );
    float fade = (p < 0.5f) ? (p * 2.0f) : ((1.0f - p) * 2.0f);
    drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), std::clamp( 0.95f - fade * 0.65f, 0.2f, 0.95f ) );
}

static void buildSimpleKeySprite( Image &img, Uint32 keyColor ) {
    img.width = 24;
    img.height = 24;
    img.pixels.assign( img.width * img.height, rgb( 255, 0, 255 ) );

    auto p = [&]( int x, int y, Uint32 c ) {
        if ((unsigned)x < (unsigned)img.width && (unsigned)y < (unsigned)img.height)
        {
            img.pixels[ y * img.width + x ] = c;
        }
        };

    for (int y = 6; y <= 14; ++y)
    {
        for (int x = 4; x <= 12; ++x)
        {
            int dx = x - 8;
            int dy = y - 10;
            if (dx * dx + dy * dy <= 14) p( x, y, keyColor );
        }
    }
    for (int x = 12; x <= 20; ++x) p( x, 10, keyColor );
    for (int x = 16; x <= 18; ++x)
    {
        p( x, 11, keyColor );
        p( x, 12, keyColor );
    }
    p( 19, 9, keyColor );
    p( 20, 9, keyColor );
}

static int addKeyPickupSprite( Engine &engineContext, float x, float y, const std::string &keyName, Uint32 keyColor ) {
    Image img;
    buildSimpleKeySprite( img, keyColor );
    int texId = (int)engineContext.propImages.size();
    engineContext.propImages.push_back( std::move( img ) );

    Prop prop;
    prop.x = x;
    prop.y = y;
    prop.kind = "PUZZLE_KEY";
    prop.filename = keyName;
    prop.textureID = texId;
    prop.scale = 0.45f;
    int propIndex = (int)engineContext.props.size();
    engineContext.props.push_back( std::move( prop ) );
    return propIndex;
}

static int addNotePickupSprite( Engine &engineContext, float x, float y, const std::string &noteName );

struct NotePickupVisual
{
    int propIndex = -1;
    int modelIndex = -1;
};

static NotePickupVisual addNotePickupModel( Engine &engineContext, float x, float y, const std::string &noteName ) {
    NotePickupVisual out;
    out.propIndex = addNotePickupSprite( engineContext, x, y, noteName );
    if (out.propIndex >= 0 && out.propIndex < (int)engineContext.props.size())
    {
        engineContext.props[ out.propIndex ].scale = 0.0f;
    }

    out.modelIndex = addWorldModelInstance(
        resolveAssetModelPath( "Note.glb" ),
        x,
        y,
        0.25f,
        rgb( 225, 214, 180 ),
        0.0f,
        0.0f,
        0.0f,
        true,
        1.4f );

    return out;
}

static ClueNote makeClueNote( Engine &engineContext, const std::string &title, const std::string &body, float x, float y, Levels level = Levels::MUSEUM ) {
    ClueNote note;
    note.title = title;
    note.body = body;
    note.x = x;
    note.y = y;
    note.collected = false;
    note.level = level;

    if (level != engineContext.currentLevel)
    {
        note.propIndex = -1;
        note.modelIndex = -1;
        return note;
    }

    NotePickupVisual vis = addNotePickupModel( engineContext, x, y, title );
    note.propIndex = vis.propIndex;
    note.modelIndex = vis.modelIndex;
    return note;
}

static KeyPickup addKeyPickupModelProxy( Engine &engineContext, const std::string &keyName, float x, float y, Uint32 keyColor, const std::string &modelAsset, Levels level = Levels::MUSEUM ) {
    KeyPickup out;
    out.keyName = keyName;
    out.x = x;
    out.y = y;
    out.collected = false;
    out.level = level;

    if (level != engineContext.currentLevel)
    {
        return out;
    }

    float roll = -1.5707963f;

    if (keyName == "BLACK PIGMENT" || keyName == "BLUE PIGMENT" || keyName == "RED PIGMENT")
    {
        roll = 0.0f;
	}

    int spriteIndex = addKeyPickupSprite( engineContext, x, y, keyName, keyColor );
    int modelIndex = addWorldModelInstance(
        resolveAssetModelPath( modelAsset ),
        x,
        y,
        0.17f,
        keyColor,
        0.0f,
        0.0f,
        roll,
        true,
        1.2f,
        0.2f);

    if (spriteIndex >= 0 && spriteIndex < (int)engineContext.props.size() && modelIndex >= 0)
    {
        engineContext.props[ spriteIndex ].scale = 0.0f;
    }

    out.propIndex = spriteIndex;
    out.modelIndex = modelIndex;
    return out;
}

static void buildStairWallOverlay( Image &img ) {
    img.width = 96;
    img.height = 96;
    img.pixels.assign( img.width * img.height, rgb( 255, 0, 255 ) );

    auto p = [&]( int x, int y, Uint32 c ) {
        if ((unsigned)x < (unsigned)img.width && (unsigned)y < (unsigned)img.height) img.pixels[ y * img.width + x ] = c;
    };

    Uint32 wallShadow = rgb( 38, 42, 52 );
    Uint32 wallEdge = rgb( 78, 84, 98 );
    Uint32 stepTop = rgb( 142, 148, 158 );
    Uint32 stepFront = rgb( 92, 98, 110 );

    for (int y = 12; y <= 92; ++y)
    {
        for (int x = 14; x <= 82; ++x)
        {
            p( x, y, wallShadow );
        }
    }
    for (int x = 14; x <= 82; ++x) { p( x, 12, wallEdge ); p( x, 92, wallEdge ); }
    for (int y = 12; y <= 92; ++y) { p( 14, y, wallEdge ); p( 82, y, wallEdge ); }

    int left = 24;
    int right = 72;
    int y = 86;
    for (int s = 0; s < 8; ++s)
    {
        int topY = y - 3;
        for (int xx = left; xx <= right; ++xx) p( xx, topY, stepTop );
        for (int yy = topY + 1; yy <= y; ++yy)
            for (int xx = left; xx <= right; ++xx)
                p( xx, yy, stepFront );

        left += 2;
        right -= 2;
        y -= 9;
    }
}

static void buildSimpleNoteSprite( Image &img ) {
    img.width = 24;
    img.height = 24;
    img.pixels.assign( img.width * img.height, rgb( 255, 0, 255 ) );

    Uint32 paper = rgb( 225, 214, 180 );
    Uint32 ink = rgb( 60, 50, 35 );

    auto p = [&]( int x, int y, Uint32 c ) {
        if ((unsigned)x < (unsigned)img.width && (unsigned)y < (unsigned)img.height)
        {
            img.pixels[ y * img.width + x ] = c;
        }
        };

    for (int y = 4; y <= 19; ++y)
    {
        for (int x = 5; x <= 18; ++x)
        {
            p( x, y, paper );
        }
    }
    for (int x = 7; x <= 16; ++x)
    {
        p( x, 8, ink );
        p( x, 11, ink );
        p( x, 14, ink );
    }
}

static int addNotePickupSprite( Engine &engineContext, float x, float y, const std::string &noteName ) {
    Image img;
    buildSimpleNoteSprite( img );
    int texId = (int)engineContext.propImages.size();
    engineContext.propImages.push_back( std::move( img ) );

    Prop prop;
    prop.x = x;
    prop.y = y;
    prop.kind = "CLUE_NOTE";
    prop.filename = noteName;
    prop.textureID = texId;
    prop.scale = 0.38f;
    int propIndex = (int)engineContext.props.size();
    engineContext.props.push_back( std::move( prop ) );
    return propIndex;
}

static void buildSafeSprite( Image &img ) {
    img.width = 48;
    img.height = 40;
    img.pixels.assign( img.width * img.height, rgb( 255, 0, 255 ) );

    Uint32 frontMid = rgb( 108, 116, 124 );
    Uint32 frontDark = rgb( 74, 80, 88 );
    Uint32 topCol = rgb( 142, 150, 160 );
    Uint32 sideCol = rgb( 84, 92, 102 );
    Uint32 rim = rgb( 52, 56, 62 );
    Uint32 dial = rgb( 188, 198, 210 );

    auto p = [&]( int x, int y, Uint32 c ) {
        if ((unsigned)x < (unsigned)img.width && (unsigned)y < (unsigned)img.height) img.pixels[ y * img.width + x ] = c;
    };

    // top face (faux 3D)
    for (int y = 4; y <= 10; ++y)
    {
        for (int x = 10; x <= 37; ++x)
        {
            p( x, y, topCol );
        }
    }

    // front face
    for (int y = 11; y <= 34; ++y)
    {
        for (int x = 8; x <= 37; ++x)
        {
            Uint32 c = (x < 15) ? frontDark : frontMid;
            p( x, y, c );
        }
    }

    // right side face
    for (int y = 11; y <= 34; ++y)
    {
        for (int x = 38; x <= 43; ++x)
        {
            p( x, y, sideCol );
        }
    }

    // rims
    for (int x = 8; x <= 43; ++x) { p( x, 11, rim ); p( x, 34, rim ); }
    for (int y = 11; y <= 34; ++y) { p( 8, y, rim ); p( 43, y, rim ); }

    // dial + handle
    for (int y = 19; y <= 25; ++y)
    {
        for (int x = 20; x <= 26; ++x)
        {
            p( x, y, dial );
        }
    }
    for (int x = 27; x <= 31; ++x) p( x, 22, dial );
}

static int addSafeSprite( Engine &engineContext, float x, float y, const std::string &name ) {
    Image img;
    buildSafeSprite( img );
    int texId = (int)engineContext.propImages.size();
    engineContext.propImages.push_back( std::move( img ) );

    Prop prop;
    prop.x = x;
    prop.y = y;
    prop.kind = "SAFE";
    prop.filename = name;
    prop.textureID = texId;
    prop.scale = 0.95f;
    int propIndex = (int)engineContext.props.size();
    engineContext.props.push_back( std::move( prop ) );
    return propIndex;
}

static void buildPedestalSprite( Image &img ) {
    img.width = 48;
    img.height = 56;
    img.pixels.assign( img.width * img.height, rgb( 255, 0, 255 ) );

    Uint32 light = rgb( 170, 162, 145 );
    Uint32 mid = rgb( 146, 136, 118 );
    Uint32 dark = rgb( 112, 102, 88 );

    auto p = [&]( int x, int y, Uint32 c ) {
        if ((unsigned)x < (unsigned)img.width && (unsigned)y < (unsigned)img.height) img.pixels[ y * img.width + x ] = c;
    };

    // top slab
    for (int y = 4; y <= 11; ++y)
        for (int x = 10; x <= 37; ++x)
            p( x, y, light );

    // shaft front
    for (int y = 12; y <= 45; ++y)
        for (int x = 14; x <= 33; ++x)
            p( x, y, (x < 22) ? mid : light );

    // shaft side
    for (int y = 12; y <= 45; ++y)
        for (int x = 34; x <= 39; ++x)
            p( x, y, dark );

    // base plinth
    for (int y = 46; y <= 53; ++y)
    {
        for (int x = 8; x <= 39; ++x)
        {
            p( x, y, (x < 24) ? dark : mid );
        }
    }
}

static int addPedestalSprite( Engine &engineContext, float x, float y, const std::string &name ) {
    Image img;
    buildPedestalSprite( img );
    int texId = (int)engineContext.propImages.size();
    engineContext.propImages.push_back( std::move( img ) );

    Prop prop;
    prop.x = x;
    prop.y = y;
    prop.kind = "PEDESTAL";
    prop.filename = name;
    prop.textureID = texId;
    prop.scale = 1.05f;
    int propIndex = (int)engineContext.props.size();
    engineContext.props.push_back( std::move( prop ) );
    return propIndex;
}

static BoxProp buildStairStepBox( float centerX, float centerY, float halfLength, float halfDepth, float height, float angle, const Image &tex ) {
    BoxProp step;
    step.centerX = centerX;
    step.centerY = centerY;
    step.halfLength = halfLength;
    step.halfDepth = halfDepth;
    step.height = height;
    step.angle = angle;
    step.sideTexure = tex;
    step.legTexure = tex;
    step.legHalf = 0.03f;
    step.legInsetLength = 0.02f;
    step.legInsetDepth = 0.02f;
    return step;
}

static Image makeSafeMetalTexture() {
    Image tex;
    tex.width = 64;
    tex.height = 64;
    tex.pixels.assign( 64 * 64, rgb( 140, 150, 162 ) );
    for (int y = 0; y < tex.height; ++y)
    {
        for (int x = 0; x < tex.width; ++x)
        {
            int n = ((x * 17 + y * 31) & 7) - 3;
            int shade = std::clamp( 150 + n - (x / 5), 132, 212 );
            tex.pixels[ y * tex.width + x ] = rgb( shade, shade + 6, shade + 14 );
        }
    }
    return tex;
}

static Image makeSafeDoorTexture() {
    Image tex;
    tex.width = 64;
    tex.height = 64;
    tex.pixels.assign( 64 * 64, rgb( 136, 146, 156 ) );
    for (int y = 0; y < tex.height; ++y)
    {
        for (int x = 0; x < tex.width; ++x)
        {
            int base = 132 + ((x + y) & 7);
            tex.pixels[ y * tex.width + x ] = rgb( base, base + 8, base + 14 );
        }
    }
    for (int y = 18; y <= 46; ++y)
    {
        for (int x = 18; x <= 46; ++x)
        {
            int dx = x - 32;
            int dy = y - 32;
            int r2 = dx * dx + dy * dy;
            if (r2 <= 100)
            {
                tex.pixels[ y * tex.width + x ] = rgb( 176, 188, 205 );
            }
            else if (r2 <= 144)
            {
                tex.pixels[ y * tex.width + x ] = rgb( 128, 138, 152 );
            }
        }
    }
    return tex;
}

static Image makeStoneTexture() {
    Image tex;
    tex.width = 64;
    tex.height = 64;
    tex.pixels.assign( 64 * 64, rgb( 172, 162, 144 ) );
    for (int y = 0; y < tex.height; ++y)
    {
        for (int x = 0; x < tex.width; ++x)
        {
            int n = ((x * 13 + y * 23 + (x * y) / 9) & 15) - 7;
            int shade = std::clamp( 168 + n, 138, 214 );
            tex.pixels[ y * tex.width + x ] = rgb( shade, shade - 8, shade - 20 );
        }
    }
    return tex;
}

static int addBoxMesh( Engine &engineContext, float x, float y, float halfLength, float halfDepth, float height, const Image &tex ) {
    BoxProp box;
    box.centerX = x;
    box.centerY = y;
    box.halfLength = halfLength;
    box.halfDepth = halfDepth;
    box.height = height;
    box.angle = 0.0f;
    box.sideTexure = tex;
    box.legTexure = tex;
    box.legHalf = 0.0f;
    box.legInsetLength = 0.0f;
    box.legInsetDepth = 0.0f;
    int index = (int)engineContext.benches3D.size();
    engineContext.benches3D.push_back( std::move( box ) );
    return index;
}

static void addSafe3D( Engine &engineContext, float x, float y ) {
    (void)engineContext;
    g_safeBoxIndices.push_back( addWorldModelInstance( resolveAssetModelPath( "Safe.glb" ), x, y, 0.72f, rgb( 255, 255, 255 ), 3.14, 0, 0, false, 0, -0.05f) );
}

static void addPedestal3D( Engine &engineContext, float x, float y ) {
    (void)engineContext;
    g_pedestalBoxIndices.push_back( addWorldModelInstance( resolveAssetModelPath( "Pedestal.glb" ), x, y, 0.46f, rgb( 164, 156, 142 ), 0, 0, false, 0, -0.05f) );
}

static void initMuseumPuzzle(Engine& engineContext) {
    g_roomLocks = {
        // Doors blocking the main 4 wings:
        {6, 9, "West Wing", LockType::KEY, "BRONZE KEY", false, Levels::MUSEUM},
        {10, 6, "North Wing", LockType::CODE, "0300", false, Levels::MUSEUM},
        {16, 9, "East Wing", LockType::KEY, "GOLD KEY", false, Levels::MUSEUM},
        {10, 12, "South Wing", LockType::CODE, "7391", false, Levels::MUSEUM},
        // Doors blocking the new 4 corner rooms:
        {5, 2, "NW Archives", LockType::KEY, "BRONZE KEY", false, Levels::MUSEUM}, // From NW
        {14, 3, "NE Vault", LockType::KEY, "IRON KEY", false, Levels::MUSEUM}, // From NW
        {5, 15, "SW Crypt", LockType::KEY, "SILVER KEY", false, Levels::MUSEUM}, // From SW
        {17, 13, "SE Office", LockType::KEY, "BRONZE KEY", false, Levels::MUSEUM}, // From East
        // Second-floor puzzle gates
        {6, 9, "Infinite Archive", LockType::CODE, "1911", false, Levels::MUSEUM_UPPER},
        {10, 6, "Taxidermy Studio", LockType::CODE, "0402", false, Levels::MUSEUM_UPPER}
    };

    g_playerKeys.clear();
    g_foundNotes.clear();
    g_accessPopup.clear();
    g_accessPopupUntil = 0;
    g_codeEntryActive = false;
    g_codeEntryLockIndex = -1;
    g_safeEntryIndex = -1;
    g_symbolEntryIndex = -1;
    g_codeEntryBuffer.clear();
    g_notesOpen = false;
    g_caveFinalNoteCollected = false;
    g_caveTimerActive = false;
    g_caveQuizActive = false;
    g_caveQuizPassed = false;
    g_caveQuizQuestionIndex = 0;
    g_caveQuiz.clear();
    g_mindTrapActive = false;
    g_mindTrapTriggerConsumed = false;
    g_mindTrapPhaseIndex = 0;
    g_mindTrapShowingResult = false;
    g_mindTrapLastResult.clear();
    g_mindTrapResultTimer = 0.0f;
    g_mindTrapFlickerTimer = 0.0f;
    g_mindTrapWhiteFlashTimer = 0.0f;
    g_mindTrapAdvanceOnResult = false;
    g_mindTrapExitOnResult = false;
    g_mindTrapReadyToExit = false;
    g_mindTrapTerminalLog.clear();
    g_mindTrapTypeQueue.clear();
    g_mindTrapTypingLine.clear();
    g_mindTrapTypingChars = 0;
    g_mindTrapTypingAccumulator = 0.0f;
    g_mindTrapPostLinePause = 0.0f;
    g_mindTrapAwaitingChoice = false;
    g_mindTrapSelectedOption = 0;
    g_mindTrapAdvanceAfterResult = false;
    g_mindTrapFinalizeAfterResult = false;
    g_mindTrapTerminalLog.clear();
    g_mindTrapTypeQueue.clear();
    g_mindTrapTypingLine.clear();
    g_mindTrapTypingChars = 0;
    g_mindTrapTypingAccumulator = 0.0f;
    g_mindTrapPostLinePause = 0.0f;
    g_mindTrapAwaitingChoice = false;
    g_mindTrapSelectedOption = 0;
    g_mindTrapAdvanceAfterResult = false;
    g_mindTrapFinalizeAfterResult = false;
    g_restorationWingUnlocked = false;
    g_directorDeskUnlocked = false;
    g_combatState = {};
    g_showHeldWeapon = true;
    g_revolverPickup = {};
    g_heldRevolverModelIndex = -1;
    g_revolverAiming = false;
    g_revolverShotCooldown = 0.0f;
    g_revolverRecoilTimer = 0.0f;
    g_revolverInspectCutsceneActive = false;
    g_revolverInspectCutsceneTimer = 0.0f;
    g_revolverInspectModelIndex = -1;
    g_revolverInspectBaseYaw = 0.0f;
    g_activeWeaponSounds.clear();
    g_generatorStartSound.reset();
    g_generatorStartSoundTimer = 0.0f;
    g_pendingShellDropTimers.clear();
    resetWhisperAmbience();
    g_firstLockedDoorDialogueShown = false;
    g_generatorNeedsGasLinePlayed = false;
    g_gasCanCollected = false;
    g_generatorFueled = false;
    g_generatorModelIndex = -1;
    g_gasCanModelIndex = -1;
    g_powerRestoreFlickerActive = false;
    g_powerRestoreFlickerTimer = 0.0f;
    g_cutsceneController.reset();
    g_dialogue.clear();

    if (!g_stairWallOverlayReady)
    {
        buildStairWallOverlay(g_stairWallOverlay);
        g_stairWallOverlayReady = true;
    }

    g_keyPickups.clear();

    if (engineContext.currentLevel == Levels::MUSEUM) {
        // Bronze Key in main atrium start
        g_keyPickups.push_back(addKeyPickupModelProxy(engineContext, "BRONZE KEY", 15.f, 8.f, rgb(180, 120, 40), "Bronze Key.glb"));
        // Silver Key in North Wing
        g_keyPickups.push_back(addKeyPickupModelProxy(engineContext, "SILVER KEY", 10.5f, 3.5f, rgb(190, 190, 200), "Silver Key.glb"));
       
        // Upper-floor pigment hunt (one pigment per distinct room)
        g_keyPickups.push_back(addKeyPickupModelProxy(engineContext, "BLACK PIGMENT", 4.5f, 4.5f, rgb(70, 70, 85), "BlackPigment.glb", Levels::MUSEUM_UPPER));
        g_keyPickups.push_back(addKeyPickupModelProxy(engineContext, "BLUE PIGMENT", 18.5f, 4.5f, rgb(90, 140, 220), "BluePigment.glb", Levels::MUSEUM_UPPER));
        g_keyPickups.push_back(addKeyPickupModelProxy(engineContext, "RED PIGMENT", 18.5f, 15.5f, rgb(215, 75, 70), "RedPigment.glb", Levels::MUSEUM_UPPER));
        g_keyPickups.push_back(addKeyPickupModelProxy(engineContext, "DIRECTOR'S KEY", 4.5f, 15.5f, rgb(210, 170, 95), "Bronze Key.glb"));
        // Fallback Gold Key in North Wing so progression cannot dead-end
       // g_keyPickups.push_back( {"GOLD KEY", 12.5f, 2.5f, false, addKeyPickupSprite( engineContext, 12.5f, 2.5f, "GOLD KEY", rgb( 255, 215, 0 ) )} );

        g_safes.clear();
        g_safeBoxIndices.clear();
        // Safe in SE Office
        g_safes.push_back({ "Director's Safe", "2026", 18.7f, 16.7f, false, "GOLD KEY" });
        addSafe3D(engineContext, 18.7f, 16.7f);

        g_symbols.clear();
        g_pedestalBoxIndices.clear();
        // Pedestal in NW Archives
        g_symbols.push_back({ "Ancient Pedestal", {1, 3, 0}, 3.5f, 3.5f, false, "IRON KEY" }); // WOLF(1) SERPENT(3) OWL(0)
        addPedestal3D(engineContext, 3.5f, 3.5f);

        // Director room furnishing + decor models
        addWorldModelInstance(resolveAssetModelPath("Full Desk.glb"), 16.36f, 16.55f, 0.8f, rgb(170, 150, 130), 3.1415926f, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("Shelf.glb"), 18.6f, 14.2f, 0.8f, rgb(170, 160, 140), -1.5707963f, 0, 0, false, 0, -0.05f);
        //  addWorldModelInstance( resolveAssetModelPath( "Note.glb" ), 18.24f, 14.48f, 0.14f, rgb( 230, 218, 184 ), -1.5707963f, -1.5707963f );
        //  addWorldModelInstance( resolveAssetModelPath( "Note.glb" ), 18.38f, 14.52f, 0.13f, rgb( 228, 216, 180 ), -1.5707963f, -1.5707963f );
        addWorldModelInstance(resolveAssetModelPath("Couch.glb"), 17.5f, 16.7f, 0.8f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("Boxes.glb"), 16.2f, 15.3f, 0.8f, rgb(184, 130, 98), -1.5707963f, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("Whiteboard.glb"), 17.5f, 17.f, 0.8f, rgb(116, 101, 60), -1.5707963, 0, 1.5707963, false, 0, 0.45f);
        addWorldModelInstance(resolveAssetModelPath("Refrigerator.glb"), 17.4f, 14.2f, 0.8f, rgb(116, 101, 60), 2.3415926, -0.03, 0, false, 0, -0.08f);
        addWorldModelInstance(resolveAssetModelPath("FileCabinet.glb"), 16.2f, 16.0f, 0.4f, rgb(69, 41, 34), 1.5707963f, 0, 0, false, 0, -0.05f);

        g_revolverPickup.weaponName = "REVOLVER";
        g_revolverPickup.x = 17.95f;
        g_revolverPickup.y = 16.1f;
        g_revolverPickup.level = Levels::MUSEUM;
        g_revolverPickup.collected = true;
        g_revolverPickup.modelIndex = -1;


        addWorldModelInstance(resolveAssetModelPath("SimplePillar.glb"), 8.5f, 8.5f, 1.1f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("SimplePillar.glb"), 13.5f, 8.5f, 1.1f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("SimplePillar.glb"), 8.5, 10.5, 1.1f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("SimplePillar.glb"), 13.5, 10.5, 1.1f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);

        // Scattered floor paper props (1-3)
        addWorldModelInstance(resolveAssetModelPath("Scattered Paper.glb"), 16.4f, 14.9f, 0.22f, rgb(224, 214, 188), 0.45f, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("Scattered Paper.glb"), 17.1f, 14.4f, 0.20f, rgb(220, 210, 182), -0.20f, 0, 0, false, 0, -0.05f);
        addWorldModelInstance(resolveAssetModelPath("Scattered Paper.glb"), 17.8f, 15.0f, 0.18f, rgb(226, 216, 190), 0.95f, 0, 0, false, 0, -0.05f);
        g_clueNotes.clear();
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Missed Calls",
            "[PHONE] 12 missed calls. No signal. No contacts. Why is my battery dropping so fast?",
            10.0f, 10.3f));
        // Atrium note
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Janitor's Log",
            "Dropped the Bronze Key nearby. It unlocks the West Wing, NW Archives, and SE Office.",
            6.1f, 10.8f));
        // West Wing Note
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Archivist Notebook",
            "The NW Archives pedestal requires the predator, the deceiver, and the wise one.",
            3.5f, 8.5f));
        // West Wing progression note (guarantees early North Wing access)
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Security Log",
            "The North Wing lockdown code is the year of the four rulers. Do not forget it.",
            4.5f, 10.5f));
        // SW Crypt Note
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Director Memo",
            "The SE Office safe code is current year. It contains the Gold Key.",
            3.5f, 15.5f));
        // East Wing Note
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Final Code Clue",
            "The South Wing emergency code is 7391.",
            18.5f, 9.5f));
        // North Wing fallback note so South code is always obtainable
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Emergency Override Slip",
            "If wing routing fails, South Wing emergency code is 7391.",
            9.2f, 2.8f));
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Conservation Log A",
            "We preserve the beauty of the frozen moment. Time should stop before decay can argue.",
            7.8f, 6.8f));
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Conservation Log B",
            "A perfect exhibit is one breath held forever. Preservation is mercy, not violence.",
            14.7f, 6.2f));
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Conservation Log C",
            "Stillness is purity. If they move, they suffer. If they freeze, they become art.",
            11.4f, 13.8f));
        // NE Vault lore note so the room is still meaningful after progression rebalance
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Vault Ledger",
            "Iron access approved. Reserve artifacts moved to East Wing transfer corridor.",
            17.5f, 2.5f));
        // Restoration Wing lore notes
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Spilled Solvent",
            "The red stains won't come up with standard bleach. The Director says it's 'Special Oil.' It smells like a hospital. Wait, I haven't seen anyone in a while. Where are they? How did I get here?",
            7.3f, 15.6f,
            Levels::MUSEUM_UPPER));
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Entry #402",
            "I can still see the fear in his eyes. The color drained from her body. The texture abandoned his face. He went limp. The Director's requests are becoming too much. ",
            14.9f, 4.2f,
            Levels::MUSEUM_UPPER));
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Scientist Note",
            "It's alive. I don't know how it happened. The doors just locked. It's only a matter of time now...",
            3.9f, 10.6f,
            Levels::MUSEUM_UPPER));
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Archive Assistant Letter",
            "Look away. If you look it in the eye it will take you.",
            18.2f, 8.2f,
            Levels::MUSEUM_UPPER));
        g_clueNotes.push_back(makeClueNote(engineContext,
            "Special Exhibit Intake Receipt",
            "NEW ACQUISITION // SUBJECT: YOU\nCondition: Conscious, disoriented, highly expressive under stress.\nCurator notes: Frame after identity fracture. Keep still. Preserve the moment forever.",
            11.2f, 11.0f,
            Levels::MUSEUM_UPPER));
    }
    g_museumPuzzleInitialized = true;
}
static void initCaveQuiz() {
    g_caveQuiz.clear();

    // Q1 focuses on the Director's twisted philosophy (Conservation Log B)
    g_caveQuiz.push_back({
        "According to the Director, what makes the perfect exhibit?",
        {"One breath held forever", "A flawless recreation of history", "Abstract interpretation", "Pure, untouched marble"},
        0
        });

    // Q2 focuses on the conversion process (Conservation Log C)
    g_caveQuiz.push_back({
        "At what exact moment does the subject finally become art?",
        {"When the frame is sealed", "When the solvent is applied", "When they freeze", "When the public arrives"},
        2
        });

    // Q3 focuses on the player's own grim realization (Intake Receipt)
    g_caveQuiz.push_back({
        "What is the physical condition of the newest acquisition?",
        {"Mummified in ash", "Conscious and disoriented", "Preserved in formaldehyde", "Asleep and unfeeling"},
        1
        });
}

static void initCaveFinalObjective(Engine& engineContext) {
    g_clueNotes.clear();
    g_foundNotes.clear();

    g_clueNotes.push_back(makeClueNote(engineContext,
        "Scribbled Warning",
        "The Warden statue tests those who try to leave. You must understand the Director's madness to pass. Remember: he wants one breath held forever.",
        2.4f, 2.1f));

    g_clueNotes.push_back(makeClueNote(engineContext,
        "Assistant's Regret",
        "I couldn't watch them suffer anymore. But the Director insists... stillness is purity. They only truly become art when they freeze.",
        7.0f, 2.9f));

    g_clueNotes.push_back(makeClueNote(engineContext,
        "Torn Intake Log",
        "I found the paperwork for the newest acquisition. It's... it's you. The notes say you are 'conscious and disoriented'. Don't let them catch you.",
        3.6f, 6.6f));

    g_clueNotes.push_back(makeClueNote(engineContext,
        "Last Journal Fragment",
        "You aren't escaping the museum... you're descending into the slaughterhouse it was built upon. The exhibits upstairs aren't statues. They're the ones who stopped moving.",
        9.3f, 8.1f));

    g_caveFinalNoteCollected = false;
    g_caveQuizActive = false;
    g_caveQuizPassed = false;
    g_caveQuizQuestionIndex = 0;
    initCaveQuiz();
}

static void clearPuzzleState() {
g_roomLocks.clear();
g_keyPickups.clear();
g_clueNotes.clear();
g_safes.clear();
g_symbols.clear();
g_safeBoxIndices.clear();
g_pedestalBoxIndices.clear();
g_playerKeys.clear();
g_foundNotes.clear();
g_accessPopup.clear();
g_accessPopupUntil = 0;
g_codeEntryActive = false;
g_codeEntryLockIndex = -1;
g_safeEntryIndex = -1;
g_symbolEntryIndex = -1;
g_codeEntryBuffer.clear();
    g_notesOpen = false;
    g_caveQuizActive = false;
    g_caveQuizPassed = false;
    g_caveQuizQuestionIndex = 0;
    g_caveQuiz.clear();
    g_mindTrapActive = false;
    g_mindTrapTriggerConsumed = false;
    g_mindTrapPhaseIndex = 0;
    g_mindTrapShowingResult = false;
    g_mindTrapLastResult.clear();
    g_mindTrapResultTimer = 0.0f;
    g_mindTrapFlickerTimer = 0.0f;
    g_mindTrapWhiteFlashTimer = 0.0f;
    g_mindTrapAdvanceOnResult = false;
    g_mindTrapExitOnResult = false;
    g_mindTrapReadyToExit = false;
    g_mindTrapTerminalLog.clear();
    g_mindTrapTypeQueue.clear();
    g_mindTrapTypingLine.clear();
    g_mindTrapTypingChars = 0;
    g_mindTrapTypingAccumulator = 0.0f;
    g_mindTrapPostLinePause = 0.0f;
    g_mindTrapAwaitingChoice = false;
    g_mindTrapSelectedOption = 0;
    g_mindTrapAdvanceAfterResult = false;
    g_mindTrapFinalizeAfterResult = false;
    g_museumPuzzleInitialized = false;
    g_caveTimerActive = false;
    g_restorationWingUnlocked = false;
    g_directorDeskUnlocked = false;
    g_revolverPickup = {};
    g_combatState = {};
    g_showHeldWeapon = true;
    g_heldRevolverModelIndex = -1;
    g_revolverAiming = false;
    g_revolverShotCooldown = 0.0f;
    g_revolverRecoilTimer = 0.0f;
    g_revolverInspectCutsceneActive = false;
    g_revolverInspectCutsceneTimer = 0.0f;
    g_revolverInspectModelIndex = -1;
    g_revolverInspectBaseYaw = 0.0f;
    g_activeWeaponSounds.clear();
    g_generatorStartSound.reset();
    g_generatorStartSoundTimer = 0.0f;
    g_pendingShellDropTimers.clear();
    resetWhisperAmbience();
    g_firstLockedDoorDialogueShown = false;
    g_generatorNeedsGasLinePlayed = false;
    g_gasCanCollected = false;
    g_generatorFueled = false;
    g_generatorModelIndex = -1;
    g_gasCanModelIndex = -1;
    g_powerRestoreFlickerActive = false;
    g_powerRestoreFlickerTimer = 0.0f;
    g_dialogue.clear();
    g_cutsceneController.reset();
}

static int findDoorLockIndex( Levels level, int tx, int ty ) {
    for (int i = 0; i < (int)g_roomLocks.size(); ++i)
    {
        if (g_roomLocks[ i ].level == level && g_roomLocks[ i ].tx == tx && g_roomLocks[ i ].ty == ty) return i;
    }
    return -1;
}

static Uint32 keyColorForName( const std::string &keyName ) {
    if (keyName == "BRONZE KEY") return rgb( 180, 120, 40 );
    if (keyName == "SILVER KEY") return rgb( 190, 190, 200 );
    if (keyName == "GOLD KEY") return rgb( 255, 215, 0 );
    if (keyName == "IRON KEY") return rgb( 135, 145, 155 );
    if (keyName == "BLACK PIGMENT") return rgb( 70, 70, 85 );
    if (keyName == "BLUE PIGMENT") return rgb( 90, 140, 220 );
    if (keyName == "RED PIGMENT") return rgb( 215, 75, 70 );
    if (keyName == "DIRECTOR'S KEY") return rgb( 210, 170, 95 );
    return rgb( 220, 210, 180 );
}

static std::string keyModelForName( const std::string &keyName ) {
    if (keyName == "BRONZE KEY") return "Bronze Key.glb";
    if (keyName == "SILVER KEY") return "Silver Key.glb";
    if (keyName == "GOLD KEY") return "Gold Key.glb";
    if (keyName == "IRON KEY") return "Iron Key.glb";
    if (keyName == "BLACK PIGMENT") return "BlackPigment.glb";
    if (keyName == "BLUE PIGMENT") return "BluePigment.glb";
    if (keyName == "RED PIGMENT") return "RedPigment.glb";
    if (keyName == "DIRECTOR'S KEY") return "Bronze Key.glb";
    return "Note.glb";
}

static bool isPlayerNearGenerator( Engine const &engineContext, float radius = 1.25f ) {
    if (engineContext.currentLevel != Levels::MUSEUM) return false;
    return isPlayerNearPoint( engineContext, kMuseumGeneratorX, kMuseumGeneratorY, radius );
}

static bool isPlayerNearGasCan( Engine const &engineContext, float radius = 0.95f ) {
    if (engineContext.currentLevel != Levels::MUSEUM) return false;
    if (g_gasCanCollected || g_generatorFueled) return false;
    return isPlayerNearPoint( engineContext, kMuseumGasCanX, kMuseumGasCanY, radius );
}

static void rebuildMuseumPowerInteractablesForLevel( Engine &engineContext, Levels level ) {
    g_generatorModelIndex = -1;
    g_gasCanModelIndex = -1;

    if (level != Levels::MUSEUM) return;

    g_generatorModelIndex = addWorldModelInstance(
        resolveFirstExistingAsset( { "Generator.glb", "generator.glb", "PowerGenerator.glb", "AirConditioner.glb" } ),
        kMuseumGeneratorX,
        kMuseumGeneratorY,
        0.70f,
        g_generatorFueled ? rgb( 225, 225, 205 ) : rgb( 130, 130, 130 ),
        -3.13,
        0.0f,
        0.0f,
        false,
        0.0f,
        -0.10f );

    if (!g_generatorFueled && !g_gasCanCollected)
    {
        g_gasCanModelIndex = addWorldModelInstance(
            resolveFirstExistingAsset( { "GasCan.glb", "Gas Can.glb", "gascan.glb", "MopBucket.glb" } ),
            kMuseumGasCanX,
            kMuseumGasCanY,
            0.3f,
            rgb( 208, 58, 52 ),
            1.5f,
            0.0f,
            0.0f,
            false,
            0.9f,
            -0.05f );
    }
}

static void rebuildMuseumInteractableVisualsForLevel( Engine &engineContext, Levels level ) {
    for (auto &k : g_keyPickups)
    {
        k.propIndex = -1;
        k.modelIndex = -1;
        if (k.collected || k.level != level) continue;

        KeyPickup vis = addKeyPickupModelProxy(
            engineContext,
            k.keyName,
            k.x,
            k.y,
            keyColorForName( k.keyName ),
            keyModelForName( k.keyName ),
            k.level );
        k.propIndex = vis.propIndex;
        k.modelIndex = vis.modelIndex;
    }

    for (auto &n : g_clueNotes)
    {
        n.propIndex = -1;
        n.modelIndex = -1;
        if (n.collected || n.level != level) continue;

        NotePickupVisual vis = addNotePickupModel( engineContext, n.x, n.y, n.title );
        n.propIndex = vis.propIndex;
        n.modelIndex = vis.modelIndex;
    }

    if (g_revolverPickup.collected)
    {
        g_revolverPickup.modelIndex = -1;
    }
    else if (g_revolverPickup.level == level)
    {
        bool needsSpawn = true;
        if (g_revolverPickup.modelIndex >= 0 && g_revolverPickup.modelIndex < (int)g_worldModels.size())
        {
            needsSpawn = false;
        }

        if (needsSpawn)
        {
            g_revolverPickup.modelIndex = addWorldModelInstance(
                resolveFirstExistingAsset( { "Revolver.glb", "revolver.glb", "SurgicalKnife.glb" } ),
                g_revolverPickup.x,
                g_revolverPickup.y,
                0.18f,
                rgb( 165, 170, 180 ),
                0.0f,
                0.0f,
                -1.5707963f,
                true,
                1.0f,
                0.20f );
        }
    }
    else
    {
        g_revolverPickup.modelIndex = -1;
    }

    rebuildMuseumPowerInteractablesForLevel( engineContext, level );
}

static bool getDoorAheadTile( Engine const &engineContext, int &tx, int &ty ) {
    float reach = 1.5f;
    tx = int( engineContext.positionX + engineContext.directionX * reach );
    ty = int( engineContext.positionY + engineContext.directionY * reach );
    if (tx < 0 || ty < 0 || tx >= engineContext.map.width || ty >= engineContext.map.height) return false;
    return engineContext.map.tiles[ ty * engineContext.map.width + tx ] == 2;
}

static int getNearbyKeyPickup( Engine const &engineContext, float radius = 0.9f ) {
    float radiusSq = radius * radius;
    for (int i = 0; i < (int)g_keyPickups.size(); ++i)
    {
        const auto &k = g_keyPickups[ i ];
        if (k.collected) continue;
        if (k.level != engineContext.currentLevel) continue;
        float dx = engineContext.positionX - k.x;
        float dy = engineContext.positionY - k.y;
        if (dx * dx + dy * dy <= radiusSq) return i;
    }
    return -1;
}

static int getNearbyClueNote( Engine const &engineContext, float radius = 0.9f ) {
    float radiusSq = radius * radius;
    for (int i = 0; i < (int)g_clueNotes.size(); ++i)
    {
        const auto &n = g_clueNotes[ i ];
        if (n.collected) continue;
        if (n.level != engineContext.currentLevel) continue;
        float dx = engineContext.positionX - n.x;
        float dy = engineContext.positionY - n.y;
        if (dx * dx + dy * dy <= radiusSq) return i;
    }
    return -1;
}

static int getNearbySafe( Engine const &engineContext, float radius = 1.6f ) {
    float radiusSq = radius * radius;
    for (int i = 0; i < (int)g_safes.size(); ++i)
    {
        const auto &s = g_safes[ i ];
        if (s.solved) continue;
        float dx = engineContext.positionX - s.x;
        float dy = engineContext.positionY - s.y;
        if (dx * dx + dy * dy <= radiusSq) return i;
    }
    return -1;
}

static int getNearbySymbol( Engine const &engineContext, float radius = 1.6f ) {
    float radiusSq = radius * radius;
    for (int i = 0; i < (int)g_symbols.size(); ++i)
    {
        const auto &s = g_symbols[ i ];
        if (s.solved) continue;
        float dx = engineContext.positionX - s.x;
        float dy = engineContext.positionY - s.y;
        if (dx * dx + dy * dy <= radiusSq) return i;
    }
    return -1;
}

static void hideWorldModelsNear( float x, float y, float radius = 0.50f ) {
    const float radiusSq = radius * radius;
    for (auto &m : g_worldModels)
    {
        if (!m.visible) continue;
        float dx = m.x - x;
        float dy = m.y - y;
        if ((dx * dx + dy * dy) <= radiusSq)
        {
            m.visible = false;
        }
    }
}

static bool loadLevel( Engine &engineContext, const LevelDef &level ) {
    namespace fs = std::filesystem;

    // Clear per-level state
    engineContext.artworks.clear();
    engineContext.artImages.clear();
    engineContext.props.clear();
    engineContext.propImages.clear();
    engineContext.quads.clear();
    engineContext.benches3D.clear();
    g_worldModels.clear();
    g_heldRevolverModelIndex = -1;
    g_revolverInspectModelIndex = -1;
    g_revolverInspectBaseYaw = 0.0f;
    g_revolverAiming = false;
    g_revolverRecoilTimer = 0.0f;
    g_activeWeaponSounds.clear();
    g_generatorStartSound.reset();
    g_generatorStartSoundTimer = 0.0f;
    g_pendingShellDropTimers.clear();
    g_editorSelectedModel = -1;
	engineContext.hasWallCracks = false;
    engineContext.hasFloorCracks = false;
    engineContext.caveMode = false;
    engineContext.hasFloorPuddles = false;
    engineContext.hasFloorStains = false;
    engineContext.hasWallStains = false;
    engineContext.hasWallOverlay = false;

    fs::path folder = level.folder;
    g_currentLevelFolder = folder.string();
    g_currentEditorModelsFile = makeEditorModelsFileNameForLevel( level.levelId, level.mapFile );
    refreshEditorAssetCatalog( true );
    /*
    {
        BoxProp box;
        box.centerX = 7.4f; box.centerY = 4.6f;
        box.halfLength = 0.5f;   // 2.0m long
        box.halfDepth = 0.5f;  // 0.5m deep
        box.height = 0.15f; // 55cm tall
        box.angle = 3.14159265f;

        // Load textures (or reuse existing images)
        if (!box.sideTexure.loadBMP( (folder / "bench.bmp").string() ))
        {
            box.sideTexure.width = 64; box.sideTexure.height = 64; box.sideTexure.pixels.assign( 64 * 64, rgb( 139, 90, 43 ) );
        }

        box.legTexure = box.sideTexure; // fallback


        box.legHalf = 0.05f;
        box.legInsetLength = 0.05f;   // pull legs inward along length
        box.legInsetDepth = 0.05f;   // pull legs inward along depth

        engineContext.benches3D.push_back( std::move( box ) );

    }
    */
  

    engineContext.ambianceTint = level.ambianceTint;
    engineContext.ambianceMul = level.ambianceMul;
    engineContext.indoorShadeLinear = level.isMuseumFloor ? 0.12f : 0.14f;
    engineContext.indoorShadeQuadratic = level.isMuseumFloor ? 0.035f : 0.050f;
    engineContext.indoorShadeMin = level.isMuseumFloor ? 0.01f : 0.015f;

    fs::path mapPath = level.mapFile.empty() ? (folder / "map.txt") : (folder / level.mapFile);
    // Map (1=wall, D=door)
    if (!loadMap( mapPath.string(), engineContext.map )) return false;

    auto loadOrFallback = [&]( const fs::path &path, Image &img, Uint32 fill ) {
        if (!img.loadBMP( path.string() ))
        {
            img.width = 64; img.height = 64;
            img.pixels.assign( 64 * 64, fill );
        }
        };

    loadOrFallback( folder / "wall.bmp", engineContext.wallTex, rgb( 80, 80, 100 ) );
    engineContext.hasFloor = engineContext.floorTex.loadBMP( (folder / "floor.bmp").string() );
    engineContext.hasCeiling = engineContext.ceilTex.loadBMP( (folder / "ceiling.bmp").string() );
    (void)engineContext.doorTexture.loadBMP( (folder / "door.bmp").string() );

    // Props
    loadProps( (folder / "props.txt").string(), engineContext.props, engineContext.propImages, engineContext.quads );

    auto modelTintForKind = [&]( const std::string &kind )->Uint32 {
        if (kind == "PLANT") return rgb( 255, 255, 255 );
        if (kind == "TRASHCAN") return rgb( 122, 132, 142 );
        if (kind == "VASE1" || kind == "VASE2" || kind == "VASE3") return rgb( 182, 158, 120 );
        if (kind == "BENCH") return rgb( 126, 96, 64 );
        if (kind == "SAFE") return rgb( 132, 144, 156 );
        if (kind == "PEDESTAL") return rgb( 164, 156, 142 );
        return rgb( 168, 168, 172 );
    };

    auto targetHeightForKind = [&]( const std::string &kind, float sourceScale )->float {
        const float s = std::max( 0.2f, sourceScale );
        if (kind == "PLANT") return 0.96f * s;
        if (kind == "TRASHCAN") return 0.88f * s;
        if (kind == "VASE1" || kind == "VASE2" || kind == "VASE3") return 0.74f * s;
        if (kind == "BENCH") return 0.56f * s;
        return 0.70f * s;
    };

    for (auto &prop : engineContext.props)
    {
        if (!prop.prefersModel || prop.modelAssetPath.empty()) continue;
        addWorldModelInstance( prop.modelAssetPath, prop.x, prop.y, targetHeightForKind( prop.kind, prop.scale ), modelTintForKind( prop.kind ), 0, 0, 0, false, 0, -0.05f );
        prop.scale = 0.0f;
    }

    // Build spatial buckets for quads (by tile)
    engineContext.quadBuckets.assign( engineContext.map.width * engineContext.map.height, {} );
    for (int i = 0; i < (int)engineContext.quads.size(); ++i)
    {
        const auto &q = engineContext.quads[ i ];
        int tx = (int)std::floor( q.centerX );
        int ty = (int)std::floor( q.centerY );
        if ((unsigned)tx < (unsigned)engineContext.map.width && (unsigned)ty < (unsigned)engineContext.map.height)
        {
            engineContext.quadBuckets[ ty * engineContext.map.width + tx ].push_back( i );
        }
    }


    if (isMuseumLikeLevel( (Levels)level.levelId ))
    {
        bool museumFreshStart = !g_museumPuzzleInitialized;
        if (!g_museumPuzzleInitialized)
        {
            initMuseumPuzzle( engineContext );
        }

		loadColumns( (folder / "columns.txt").string(), engineContext );

        if (loadArtworks( (folder / "artworks.txt").string(), engineContext.artworks ))
        {
            attachArtworksToWalls( engineContext );
            engineContext.artImages.resize( engineContext.artworks.size() );
            for (size_t i = 0; i < engineContext.artworks.size(); ++i)
            {
                std::filesystem::path ip = engineContext.artworks[ i ].imagePath;
                if (!ip.is_absolute()) ip = folder / ip;   // resolve relative to level folder
                // Always ensure art valid texture to avoid crashes later
                loadImageOrFallback( ip.string(), engineContext.artImages[ i ], rgb( 220, 220, 220 ) );
            }
        }

        // Stair transition is now represented as a wall mural overlay near the upper entry point.
        /*
        {
            BoxProp box;
            box.centerX = 2.6f; box.centerY = 2.0f;
            box.halfLength = 0.5f;   // 2.0m long
            box.halfDepth = 0.35f;  // 0.5m deep
            box.height = 0.15f; // 55cm tall
            box.angle = 3.14159265f;

            
            // Load textures (or reuse existing images)
            if (!box.sideTexure.loadBMP( (folder / "bench.bmp").string() ))
            {
                box.sideTexure.width = 64; box.sideTexure.height = 64; box.sideTexure.pixels.assign( 64 * 64, rgb( 139, 90, 43 ) );
            }

            box.legTexure = box.sideTexure; // fallback


            box.legHalf = 0.05f;
            box.legInsetLength = 0.05f;   // pull legs inward along length
            box.legInsetDepth = 0.05f;   // pull legs inward along depth

            engineContext.benches3D.push_back( std::move( box ) );

        }
        */

        if (museumFreshStart)
        {
            mesuemObjectives.viewedArtworks.clear();
            mesuemObjectives.totalArtworksToFind = (int)engineContext.artworks.size();
        }

        rebuildMuseumInteractableVisualsForLevel( engineContext, (Levels)level.levelId );
    }
    else
    {
        clearPuzzleState();
        if (level.levelId == Levels::CAVE)
        {
            initCaveFinalObjective( engineContext );
            g_caveTimerActive = true;
            g_caveTimerSeconds = 120.0f;
        }
    }

    engineContext.caveMode = (level.levelId == Levels::CAVE) || (level.levelId == Levels::TRANSITION);
    engineContext.hasWallOverlay = false;
    auto tryLoad = [&]( const std::filesystem::path &p, Image &dst, bool &flag ) {
        flag = dst.loadBMP( p.string() );
        };

    if (engineContext.caveMode)
    {
        // Optional overlay for rock variation
      //  std::filesystem::path overlay = (folder / "wall_overlay.bmp");
      //  if (engineContext.wallOverlay.loadBMP( overlay.string() ))
     //   {
     //       engineContext.hasWallOverlay = true;
     //   }
        // Defaults: tweak to taste
      

        engineContext.hasFloorCracks = engineContext.hasFloorStains = engineContext.hasFloorPuddles = false;
        engineContext.hasWallCracks = engineContext.hasWallStains = false;

        if (level.levelId == Levels::CAVE)
        {
            engineContext.lightRadius = 1.5f;
            engineContext.lightFalloff = 2.0f;
            engineContext.caveAmbient = 0.03f;

            tryLoad( folder / "floor_cracks.bmp", engineContext.floorOverlayCracks, engineContext.hasFloorCracks );
            //tryLoad( folder / "floor_stains.bmp", engineContext.floorOverlayStains, engineContext.hasFloorStains );
            tryLoad( folder / "floor_puddles.bmp", engineContext.floorOverlayPuddles, engineContext.hasFloorPuddles );

            tryLoad( folder / "wall_cracks.bmp", engineContext.wallOverlayCracks, engineContext.hasWallCracks );
            //tryLoad( folder / "wall_stain.bmp", engineContext.wallOverlayStains, engineContext.hasWallStains )
        }

        if (level.levelId == Levels::TRANSITION)
        {
            engineContext.lightRadius = 0.9f;
            engineContext.lightFalloff = 1.5f;
            engineContext.caveAmbient = 0.02f;
        }
    }

    if (g_unlockAllDoorsOverride)
    {
        applyUnlockAllDoorsOverride( engineContext );
    }

    loadEditorModelsForLevel();

    // Spawn & camera
    engineContext.positionX = level.spawnX;
    engineContext.positionY = level.spawnY;
    float art = level.spawnDirDeg * 3.14159265f / 180.f;
    engineContext.directionX = std::cos( art );
    engineContext.directionY = std::sin( art );
    engineContext.planeX = -engineContext.directionY * FOV_TAN;
    engineContext.planeY = engineContext.directionX * FOV_TAN;
    engineContext.yaw = level.spawnDirDeg;


    if (!level.objectiveLabel.empty())
    {
        mesuemObjectives.setMainObjective( level.objectiveLabel );
    }

    if (isMuseumLikeLevel( (Levels)level.levelId ))
    {
        if (mesuemObjectives.totalArtworksToFind <= 0)
        {
            mesuemObjectives.totalArtworksToFind = (int)engineContext.artworks.size();
        }
    }

    // Load the current levels' music track
    playMusicTrack( folder.string(), engineContext.currentLevel);

    return true;
}


static int pickArtworkUnderCrosshair( Engine const &engineContext ) {
    // Cast the same ray as the center column (x = RENDER_W / 2)
    int centerX = RENDER_W / 2;
    float camX = 2.0f * centerX / float( RENDER_W ) - 1.0f;
    float rayDirX = engineContext.directionX + engineContext.planeX * camX;
    float rayDirY = engineContext.directionY + engineContext.planeY * camX;

    int mapX = int( engineContext.positionX );
    int mapY = int( engineContext.positionY );
    float sideDistX, sideDistY;
    float deltaDistX = (rayDirX == 0 ? 1e30f : std::fabs( 1.0f / rayDirX ));
    float deltaDistY = (rayDirY == 0 ? 1e30f : std::fabs( 1.0f / rayDirY ));
    int stepX = 0;
    int stepY = 0;
    int side = 0;

    if (rayDirX < 0)
    {
        stepX = -1;
        sideDistX = (engineContext.positionX - mapX) * deltaDistX;
    }
    else
    {
        stepX = 1;
        sideDistX = (mapX + 1.0f - engineContext.positionX) * deltaDistX;
    }
    if (rayDirY < 0)
    {
        stepY = -1;
        sideDistY = (engineContext.positionY - mapY) * deltaDistY;
    }
    else
    {
        stepY = 1;
        sideDistY = (mapY + 1.0f - engineContext.positionY) * deltaDistY;
    }

    int hitTile = 0;
    while (!hitTile)
    {
        if (sideDistX < sideDistY)
        {
            sideDistX += deltaDistX; mapX += stepX; side = 0;
        }
        else
        {
            sideDistY += deltaDistY; mapY += stepY; side = 1;
        }
        if (mapX < 0 || mapY < 0 || mapX >= engineContext.map.width || mapY >= engineContext.map.height) return -1;
        hitTile = engineContext.map.tiles[ mapY * engineContext.map.width + mapX ];
    }
    if (hitTile != 1) return -1; // only real walls host framed art

    float perpWallDist = (side == 0)
        ? ((mapX - engineContext.positionX) + (1 - stepX) * 0.5f) / (rayDirX == 0 ? 1e-6f : rayDirX)
        : ((mapY - engineContext.positionY) + (1 - stepY) * 0.5f) / (rayDirY == 0 ? 1e-6f : rayDirY);
    perpWallDist = std::max( std::fabs( perpWallDist ), 0.05f );
   
    float wallX = (side == 0) ? (engineContext.positionY + perpWallDist * rayDirY)
        : (engineContext.positionX + perpWallDist * rayDirX);
    wallX -= std::floor( wallX );

    if (perpWallDist > 20.0f) return -1;

    int lineH = int( RENDER_H / std::max( perpWallDist, 1e-3f ) );
    int yCenter = RENDER_H / 2;

    for (size_t artIndex = 0; artIndex < engineContext.artworks.size(); ++artIndex)
    {
        const auto &art = engineContext.artworks[ artIndex ];
        if (!art.onWall) continue;
        if (art.wx != mapX || art.wy != mapY || art.side != side) continue;

        float u0 = std::clamp( art.uCenter - 0.5f * art.uWidth, 0.0f, 1.0f );
        float u1 = std::clamp( art.uCenter + 0.5f * art.uWidth, 0.0f, 1.0f );
        if (wallX < u0 || wallX > u1) continue;

        int bandH = std::max( 1, int( lineH * art.vHeight ) );
        int bandCenter = RENDER_H / 2 + int( (art.vCenter - 0.5f) * lineH );
        int bandStart = std::clamp( bandCenter - bandH / 2, 0, RENDER_H - 1 );
        int bandEnd = std::clamp( bandStart + bandH - 1, 0, RENDER_H - 1 );

        if (yCenter >= bandStart && yCenter <= bandEnd)
        {
            return art.id; // This is the one under the crosshair
        }
    }
    return -1;
}

void handleLevelChange( Engine &engineContext, std::vector<LevelDef> levels, Levels desiredLevel ) {
    engineContext.currentLevel = desiredLevel;
    loadLevel( engineContext, levels[ desiredLevel ] );
    if (desiredLevel == Levels::MUSEUM_UPPER)
    {
        g_cutsceneController.triggerUpstairsGalleryCutscene( engineContext, g_dialogue );
    }
}

static void startWakeCutscene( Engine &engineContext ) {
    g_wakeCutsceneActive = true;
    g_wakeCutsceneTimer = 0.0f;
    g_revolverAiming = false;
    engineContext.pitchOffset = 78.0f;
}

static void startNewMuseumRun( Engine &engineContext, std::vector<LevelDef> levels ) {
    clearPuzzleState();
    g_notesCollectedRun = 0;
    g_runElapsedSeconds = 0.0f;
    handleLevelChange( engineContext, levels, Levels::MUSEUM );
    startWakeCutscene( engineContext );
}

static std::string normalizeMindTrapTerminalText( std::string s );

static void pushMindTrapTerminalLine( const std::string &line ) {
    g_mindTrapTerminalLog.push_back( normalizeMindTrapTerminalText( line ) );
    constexpr size_t kMaxTerminalLines = 120;
    if (g_mindTrapTerminalLog.size() > kMaxTerminalLines)
    {
        g_mindTrapTerminalLog.erase( g_mindTrapTerminalLog.begin(), g_mindTrapTerminalLog.begin() + (g_mindTrapTerminalLog.size() - kMaxTerminalLines) );
    }
}

static std::string normalizeMindTrapTerminalText( std::string s ) {
    for (char &c : s)
    {
        c = char( std::toupper( unsigned char( c ) ) );
        const bool ok =
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == ' ' || c == '.' || c == ',' || c == '-' || c == '_' || c == '/' ||
            c == '>' || c == '<' || c == '[' || c == ']' || c == '(' || c == ')';
        if (!ok) c = ' ';
    }
    return s;
}

static void queueMindTrapTerminalLine( const std::string &line ) {
    g_mindTrapTypeQueue.push_back( normalizeMindTrapTerminalText( line ) );
}

static void queueMindTrapPhasePrompt() {
    if (g_mindTrapPhaseIndex < 0 || g_mindTrapPhaseIndex >= (int)g_mindTrapPhases.size()) return;

    const MindTrapPhase &phase = g_mindTrapPhases[ g_mindTrapPhaseIndex ];
    queueMindTrapTerminalLine( "------------------------------------------------------------" );
    queueMindTrapTerminalLine( phase.prompt );
    queueMindTrapTerminalLine( phase.commands[ 0 ] + "   :: " + phase.options[ 0 ] );
    queueMindTrapTerminalLine( phase.commands[ 1 ] + "   :: " + phase.options[ 1 ] );
    queueMindTrapTerminalLine( phase.commands[ 2 ] + "   :: " + phase.options[ 2 ] );

    g_mindTrapAwaitingChoice = false;
    g_mindTrapSelectedOption = std::clamp( g_mindTrapSelectedOption, 0, 2 );
}

static bool mindTrapTypewriterIdle() {
    return g_mindTrapTypeQueue.empty() && g_mindTrapTypingLine.empty();
}

static void updateMindTrapTypewriter( float dt ) {
    if (g_mindTrapPostLinePause > 0.0f)
    {
        g_mindTrapPostLinePause = std::max( 0.0f, g_mindTrapPostLinePause - dt );
        return;
    }

    if (g_mindTrapTypingLine.empty())
    {
        if (g_mindTrapTypeQueue.empty()) return;
        g_mindTrapTypingLine = g_mindTrapTypeQueue.front();
        g_mindTrapTypeQueue.pop_front();
        g_mindTrapTypingChars = 0;
        g_mindTrapTypingAccumulator = 0.0f;
    }

    g_mindTrapTypingAccumulator += dt * g_mindTrapTypingCharsPerSecond;
    while (g_mindTrapTypingAccumulator >= 1.0f && g_mindTrapTypingChars < g_mindTrapTypingLine.size())
    {
        ++g_mindTrapTypingChars;
        g_mindTrapTypingAccumulator -= 1.0f;
    }

    if (g_mindTrapTypingChars >= g_mindTrapTypingLine.size())
    {
        pushMindTrapTerminalLine( g_mindTrapTypingLine );
        g_mindTrapTypingLine.clear();
        g_mindTrapTypingChars = 0;
        g_mindTrapTypingAccumulator = 0.0f;
        g_mindTrapPostLinePause = 0.055f;
    }
}

static void initMindTrapPhases() {
    if (!g_mindTrapPhases.empty()) return;

    g_mindTrapPhases = {
        {
            "PHASE 1 // PANIC // MOTOR LOCK CONFIRMED. INPUT SIGNAL?",
            {
                "RUN PANIC.MOTOR",
                "RUN PANIC.VOICE",
                "RUN PANIC.MEMORY"
            },
            {
                "Force limbs against the stillness",
                "Attempt a vocal distress call",
                "Trace the last safe memory"
            },
            {
                "MOTION REQUEST DENIED. JOINTS RETURN 0 RESPONSE.",
                "VOCAL CHANNEL MUTED. NO AIR VOLUME REGISTERED.",
                "MEMORY THREAD FRAGMENTED. LOCATION TAG: MUSEUM // UNKNOWN."
            },
            -1
        },
        {
            "PHASE 2 // AUDIT // CORE TEMP FALLING. PRESERVATION CYCLE ACTIVE.",
            {
                "RUN AUDIT.REJECT",
                "RUN AUDIT.INTERNAL",
                "RUN AUDIT.ACCEPT"
            },
            {
                "Reject the scan as invalid",
                "Run an internal body check",
                "Accept diagnostic output"
            },
            {
                "ERROR: DENIAL FLAGGED AS NON-CLINICAL RESPONSE.",
                "NERVE MAP UPDATED: COLD STABLE // PAIN SUBSIDED.",
                "PRESERVATION STATUS: OPTIMAL. HUMAN VARIANCE MINIMAL."
            },
            -1
        },
        {
            "PHASE 3 // DISSOCIATION // SUBJECT REFERENCE SHIFTING.",
            {
                "SET IDENTITY.FIRST_PERSON",
                "SET IDENTITY.OBJECT_VIEW",
                "SET IDENTITY.ACQUISITION"
            },
            {
                "Reassert ownership with 'I'",
                "Observe body as an object",
                "Adopt curator catalog label"
            },
            {
                "IDENTITY TOKEN UNSTABLE. PRONOUN LOCK FAILED.",
                "OBSERVER MODE ENABLED. SUBJECT DISTANCE INCREASED.",
                "CATALOG INDEX WRITTEN: ACQUISITION // DISPLAY READY."
            },
            -1
        },
        {
            "PHASE 4 // THE SYNC // FINAL DIRECTIVE: ENTER STILLNESS.",
            {
                "EXEC SYNC.RESIST",
                "EXEC SYNC.HOLD_FEAR",
                "EXEC SYNC.SURRENDER"
            },
            {
                "Fight the frame and refuse",
                "Hold fear and wait for rescue",
                "Surrender to the stillness"
            },
            {
                "RESISTANCE LOOP DETECTED. CYCLE RESTARTED.",
                "HOPE SIGNAL EXPIRED. NO EXTERNAL HANDSHAKE FOUND.",
                "SYNC ACCEPTED. STILLNESS IS NOW PRIMARY PROCESS."
            },
            2
        }
    };
}

static bool isPlayerNearMindTrapTrigger( Engine const &engineContext ) {
    if (engineContext.currentLevel != Levels::MUSEUM_UPPER) return false;

    const bool nearSealedDoor = isPlayerNearPoint( engineContext, 16.4f, 9.4f, 1.25f );
    const bool nearSolventVault = isPlayerNearPoint( engineContext, 11.2f, 14.4f, 1.15f );
    return nearSealedDoor || nearSolventVault;
}

static void startMindTrapSequence( Engine &engineContext ) {
    initMindTrapPhases();

    g_mindTrapActive = true;
    g_mindTrapTriggerConsumed = true;
    g_mindTrapPhaseIndex = 0;
    g_mindTrapShowingResult = false;
    g_mindTrapLastResult.clear();
    g_mindTrapResultTimer = 0.0f;
    g_mindTrapFlickerTimer = 0.0f;
    g_mindTrapWhiteFlashTimer = 0.0f;
    g_mindTrapAdvanceOnResult = false;
    g_mindTrapExitOnResult = false;
    g_mindTrapReadyToExit = false;

    g_notesOpen = false;
    g_caveQuizActive = false;
    g_codeEntryActive = false;
    g_interactionAnim.active = false;
    g_levelTransition.active = false;
    g_revolverAiming = false;
    g_dialogue.clear();

    engineContext.placardOpen = false;
    engineContext.openArtId = -1;
    engineContext.statueChatActive = false;

    queueMindTrapTerminalLine( "[BOOT] INTERNAL DIAGNOSTIC CONSOLE" );
    queueMindTrapTerminalLine( "[CHECK] SENSORY FEED ............. OFFLINE" );
    queueMindTrapTerminalLine( "[CHECK] MOTOR CONTROL ............ LOCKED" );
    queueMindTrapTerminalLine( "[CHECK] PRESERVATION PROCESS ..... ACTIVE" );
    queueMindTrapTerminalLine( "" );
    queueMindTrapPhasePrompt();
}

static void commitMindTrapChoice( int choiceIndex ) {
    if (!g_mindTrapActive || g_mindTrapShowingResult || g_mindTrapReadyToExit || !g_mindTrapAwaitingChoice) return;
    if (g_mindTrapPhaseIndex < 0 || g_mindTrapPhaseIndex >= (int)g_mindTrapPhases.size()) return;
    if (choiceIndex < 0 || choiceIndex >= 3) return;

    const MindTrapPhase &phase = g_mindTrapPhases[ g_mindTrapPhaseIndex ];
    g_mindTrapAwaitingChoice = false;
    g_mindTrapSelectedOption = choiceIndex;

    pushMindTrapTerminalLine( normalizeMindTrapTerminalText( "> " + phase.commands[ choiceIndex ] ) );
    pushMindTrapTerminalLine( "[ENTER]" );

    g_mindTrapLastResult = phase.results[ choiceIndex ];
    queueMindTrapTerminalLine( g_mindTrapLastResult );
    g_mindTrapResultTimer = 1.25f;
    g_mindTrapShowingResult = true;
    g_mindTrapAdvanceAfterResult = false;
    g_mindTrapFinalizeAfterResult = false;

    const bool finalPhase = (g_mindTrapPhaseIndex == (int)g_mindTrapPhases.size() - 1);
    if (finalPhase)
    {
        if (choiceIndex == phase.surrenderOption)
        {
            queueMindTrapTerminalLine( "SYNC TOKEN ACCEPTED." );
            g_mindTrapFinalizeAfterResult = true;
        }
        else
        {
            queueMindTrapTerminalLine( "SYNC REJECTED. REQUIRED COMMAND: SURRENDER." );
        }
    }
    else
    {
        g_mindTrapAdvanceAfterResult = true;
    }
}

static void updateMindTrapSequence( Engine &engineContext, std::vector<LevelDef> &levels, GameState &currentState, float dt ) {
    if (!g_mindTrapActive) return;

    g_mindTrapFlickerTimer += dt;
    updateMindTrapTypewriter( dt );

    if (g_mindTrapShowingResult)
    {
        if (mindTrapTypewriterIdle())
        {
            g_mindTrapResultTimer -= dt;
        }

        if (g_mindTrapResultTimer <= 0.0f)
        {
            g_mindTrapShowingResult = false;

            if (g_mindTrapAdvanceAfterResult)
            {
                g_mindTrapPhaseIndex = std::min( g_mindTrapPhaseIndex + 1, (int)g_mindTrapPhases.size() - 1 );
                queueMindTrapTerminalLine( "" );
                queueMindTrapPhasePrompt();
            }
            else if (g_mindTrapFinalizeAfterResult)
            {
                g_mindTrapReadyToExit = true;
                g_mindTrapWhiteFlashTimer = 0.01f;
            }
            else
            {
                queueMindTrapTerminalLine( "" );
                queueMindTrapPhasePrompt();
            }

            g_mindTrapAdvanceAfterResult = false;
            g_mindTrapFinalizeAfterResult = false;
        }
    }

    if (!g_mindTrapShowingResult && !g_mindTrapReadyToExit && mindTrapTypewriterIdle())
    {
        g_mindTrapAwaitingChoice = true;
    }

    if (g_mindTrapReadyToExit)
    {
        g_mindTrapWhiteFlashTimer += dt;
        if (g_mindTrapWhiteFlashTimer >= 0.68f)
        {
            g_mindTrapActive = false;
            g_mindTrapReadyToExit = false;
            g_mindTrapShowingResult = false;
            g_mindTrapWhiteFlashTimer = 0.0f;

            handleLevelChange( engineContext, levels, Levels::TRANSITION );
            currentState = STATE_GAME;
        }
    }
}

static bool isPlayerNearStatue( Engine const &engineContext ) {
    if (engineContext.currentLevel != Levels::MUSEUM) return false;

    float currentX = engineContext.positionX;
    float currentY = engineContext.positionY;

    // Location of statue
    float statueX = 11.1;
    float statueY = 9.5;

    float tolerance = 1.0f; // 1 meter
    float distSq = (currentX - statueX) * (currentX - statueX) + (currentY - statueY) * (currentY - statueY);
    return (distSq <= tolerance * tolerance);
}

static bool isPlayerNearCaveStatue( Engine const &engineContext ) {
    if (engineContext.currentLevel != Levels::CAVE) return false;

    constexpr float statueX = 11.1f;
    constexpr float statueY = 9.5f;
    constexpr float tolerance = 1.2f;

    float dx = engineContext.positionX - statueX;
    float dy = engineContext.positionY - statueY;
    return (dx * dx + dy * dy) <= (tolerance * tolerance);
}

void renderStatueChatbox( Engine &engineContext ) {
    const int fontW = 11;
    const int fontH = 16;
    const int letterSpacing = 1; // Used for header/main text
    const int lineSpacing = 2;   // Used for header/main text
    const int advY = fontH + lineSpacing; // Vertical advance for regular text

    int width = RENDER_W - 30, height = (RENDER_H / 4) - 40;
    int x = 7, y = RENDER_H / 2;

    drawTextBox( engineContext, x, y, width, height, rgb( 18, 18, 24 ), rgb( 90, 90, 120 ) );

    int textX = x + 8;
    int textY = y + 8;
    int textWidth = width - 16;

    std::string header = "ChatGPT Statue | OpenAI | Current | Relief Sculpture | MicroMuseum \n";

    drawString16x16( engineContext, textX, textY, header, rgb( 255, 255, 0 ), textWidth, letterSpacing, lineSpacing, true );

    textY += 2 * advY;

    std::string actualText = "";
    if (mesuemObjectives.allCompleted() == false)
    {
        actualText = "Thank you for exploring the museum! Please view all works, then come back!";
    }
    else
    {
        actualText = "The next level is loading... please be patient";
    }

    drawString16x16( engineContext, textX, textY, actualText, rgb( 210, 210, 210 ), textWidth, letterSpacing, lineSpacing, true );


    const int hintLetterSpacing = 0;
    const int hintLineSpacing = 5;
    const int hintAdvX = fontW + hintLetterSpacing; 

    std::string hint = "Wait a few seconds...";

    int hintX = x + width - (hint.length() * hintAdvX) - 40;

    int hintY = y + height - fontH - 4; // fontH = 16 (height of the text)

    drawString16x16( engineContext, hintX, hintY, hint, rgb( 150, 200, 255 ), textWidth, hintLetterSpacing, hintLineSpacing, true, rgb( 20, 20, 50 ) );
}


void updateMusicStream() {
    // Don't do anything if music was never started
    if (!g_musicInitialized)
    {
        return;
    }
    if (config::useMusic == false)
    {
        if (music.getStatus() != sf::SoundStream::Status::Stopped)
        {
            music.stop();
        }
        return;
    }

    // Check if the song has finished playing
    if (music.getStatus() == sf::SoundStream::Status::Stopped)
    {
        // The song finished! Play the next one.
        playNextTrack();
    }
}


void renderPolishedPlacard( Engine &engineContext ) {
    if (!engineContext.placardOpen || engineContext.openArtId < 0) return;
    if (engineContext.openArtId >= (int)engineContext.artworks.size()) return;

    int artIndex = -1;
    for (size_t i = 0; i < engineContext.artworks.size(); ++i)
    {
        if (engineContext.artworks[ i ].id == engineContext.openArtId)
        {
            artIndex = (int)i;
            break;
        }
    }

    // If we somehow didn't find it, bail out
    if (artIndex < 0) return;

    const auto &art = engineContext.artworks[ artIndex ];

    int panelW = (int)(RENDER_W * 0.40f);
    int textMargin = 25;
    int maxTextW = panelW - (textMargin * 2);

    int estimatedHeight = 40; // Top padding
    estimatedHeight += 30;    // Title space
    estimatedHeight += 40;    // Meta/Location space
    estimatedHeight += 20;    // Divider

    auto calcH = [&]( const std::string &t ) {
        int lines = ((int)t.length() * 5 / maxTextW) + 1; // 5px per char approx
        return lines * 12;
        };

    estimatedHeight += calcH( art.placard );
    estimatedHeight += calcH( art.rationale );
    estimatedHeight += calcH( art.reflection );

    int panelH = std::min( estimatedHeight, RENDER_H - 40 );
    int panelX = 20; // 20px gap from left edge
    int panelY = (RENDER_H - panelH) / 2; 

    Uint32 bgCol = rgb( 12, 12, 15 );
    Uint32 borderCol = rgb( 190, 160, 60 ); 

    drawTranslucentBox( engineContext, panelX, panelY, panelW, panelH, bgCol, 0.90f );

    for (int x = panelX; x < panelX + panelW; ++x)
    {
        putPix( engineContext, x, panelY, borderCol );
        putPix( engineContext, x, panelY + panelH - 1, borderCol );
    }
    for (int y = panelY; y < panelY + panelH; ++y)
    {
        putPix( engineContext, panelX, y, borderCol );
        putPix( engineContext, panelX + panelW - 1, y, borderCol );
    }

    int currentY = panelY + textMargin;
    int textX = panelX + textMargin;

    drawString16x16( engineContext, textX, currentY, art.title, rgb( 255, 255, 255 ), maxTextW, 1, 1, false );
    currentY += 25;

    std::string meta = art.artist + ", " + art.date;
    currentY = drawWrappedText( engineContext, textX, currentY, meta, borderCol, maxTextW );
    currentY += 3;

    currentY = drawWrappedText( engineContext, textX, currentY, art.location, rgb( 150, 150, 150 ), maxTextW );
    currentY += 10;

    for (int x = textX; x < textX + maxTextW; ++x) putPix( engineContext, x, currentY, rgb( 70, 70, 70 ) );
    currentY += 15;

    currentY = drawWrappedText( engineContext, textX, currentY, art.placard, rgb( 220, 220, 220 ), maxTextW );
    currentY += 20;

    drawStringTinyScaled( engineContext, textX, currentY, "HISTORICAL CONTEXT", borderCol, 1 );
    currentY += 12;
    currentY = drawWrappedText( engineContext, textX, currentY, art.rationale, rgb( 200, 200, 200 ), maxTextW );
    currentY += 20;

    drawStringTinyScaled( engineContext, textX, currentY, "ANALYSIS", borderCol, 1 );
    currentY += 12;
    currentY = drawWrappedText( engineContext, textX, currentY, art.reflection, rgb( 170, 190, 220 ), maxTextW );

    const Image &artImg = engineContext.artImages[ artIndex ];
    if (artImg.width > 0 && artImg.height > 0)
    {
        int availX = panelX + panelW + 20;   
        int availW = RENDER_W - availX - 20; 
        int availH = RENDER_H - 40;       
        int availY = 20;

        float imgAspect = (float)artImg.width / (float)artImg.height;
        int drawW = availW;
        int drawH = (int)(drawW / imgAspect);

        if (drawH > availH)
        {
            drawH = availH;
            drawW = (int)(drawH * imgAspect);
        }

        int drawX = availX + (availW - drawW) / 2;
        int drawY = availY + (availH - drawH) / 2;

        for (int x = drawX - 1; x <= drawX + drawW; ++x)
        {
            putPix( engineContext, x, drawY - 1, borderCol );
            putPix( engineContext, x, drawY + drawH, borderCol );
        }
        for (int y = drawY - 1; y <= drawY + drawH; ++y)
        {
            putPix( engineContext, drawX - 1, y, borderCol );
            putPix( engineContext, drawX + drawW, y, borderCol );
        }

        for (int y = 0; y < drawH; ++y)
        {
            float v = (float)y / std::max( 1.0f, (float)(drawH - 1) );
            int texY = std::clamp( (int)(v * artImg.height), 0, artImg.height - 1 );

            for (int x = 0; x < drawW; ++x)
            {
                float u = (float)x / std::max( 1.0f, (float)(drawW - 1) );
                int texX = std::clamp( (int)(u * artImg.width), 0, artImg.width - 1 );

                Uint32 color = artImg.sample( texX, texY );

                if (((color >> 16) & 255) == 255 && ((color >> 8) & 255) == 0 && (color & 255) == 255) continue;

                putPix( engineContext, drawX + x, drawY + y, color );
            }
        }
    }

}


void renderGalleryCard( Engine &engineContext ) {
    float px = engineContext.positionX;
    float py = engineContext.positionY;

    std::string wingName;
    std::string wingDesc;

    if (engineContext.currentLevel == Levels::MUSEUM_UPPER)
    {
        wingName = "Restoration Hub";
        wingDesc = "Research Facility";

        if (py < 7.0f && px >= 7.0f && px <= 15.0f)
        {
            wingName = "Studio";
            wingDesc = "Specimen Processing";
        }
        else if (py > 12.0f && px >= 7.0f && px <= 15.0f)
        {
            wingName = "Solvent Vault";
            wingDesc = "Storage";
        }
        else if (px < 7.0f && py >= 7.0f && py <= 12.0f)
        {
            wingName = "Archive";
            wingDesc = "Corridor";
        }
        else if (px > 15.0f && py >= 7.0f && py <= 12.0f)
        {
            wingName = "Masterpiece Skylight";
            wingDesc = "Observations";
        }
    }
    else
    {
        wingName = "Central Atrium";
        wingDesc = "Public Hub";

        if (py < 7.0f && px >= 7.0f && px <= 15.0f)
        {
            wingName = "North Wing";
            wingDesc = "Baroque & Dutch Golden Age";
        }
        else if (py > 12.0f && px >= 7.0f && px <= 15.0f)
        {
            wingName = "South Wing";
            wingDesc = "Prehistoric & Egyptian";
        }
        else if (px < 7.0f && py >= 7.0f && py <= 12.0f)
        {
            wingName = "West Wing";
            wingDesc = "Antiquity & Roman Empire";
        }
        else if (px > 15.0f && py >= 7.0f && py <= 12.0f)
        {
            wingName = "East Wing";
            wingDesc = "Northern Renaissance";
        }
    }

    int titleW = (int)wingName.length() * 11;
    int titleX = (RENDER_W - titleW) / 2;

    int descW = (int)wingDesc.length() * 4;
    int descX = (RENDER_W - descW) / 2;

    int boxW = std::max( titleW + 40, descW + 40 );
    int boxX = (RENDER_W - boxW) / 2;

    // Draw the card at the top center
    drawTextBox( engineContext, boxX, 10, boxW, 40, rgb( 15, 15, 18 ), rgb( 180, 150, 50 ) );
    drawString16x16( engineContext, titleX, 15, wingName, rgb( 255, 230, 100 ), RENDER_W, 1, 1, true, rgb( 0, 0, 0 ) );
    drawStringTinyScaled( engineContext, descX, 35, wingDesc, rgb( 200, 200, 200 ), 1, 1, 1, false );
}


void renderObjectives( Engine &engineContext ) {
    int width = (RENDER_W / 3) + 20;
    int height = 55;
    int x = RENDER_W - width - 10; // Anchor to top right
    int y = 10;

    Uint32 colBg = rgb( 15, 15, 18 );
    Uint32 colBorder = rgb( 180, 150, 50 ); // Museum Gold
    Uint32 colText = rgb( 220, 220, 230 );

    // Draw main box
    drawTextBox( engineContext, x, y, width, height, colBg, colBorder );

    std::string header = "Progress";
    drawString16x16( engineContext, x + 10, y + 8, header, colBorder, width, 1, 1, false );

    // Draw Progress Bar outline
    int barX = x + 10;
    int barY = y + 30;
    int barWidth = width - 20;
    int barHeight = 12;
    drawTextBox( engineContext, barX, barY, barWidth, barHeight, rgb( 10, 10, 10 ), rgb( 100, 100, 100 ) );

    // Fill Progress Bar
    float progress = mesuemObjectives.getProgress();
    int fillWidth = (int)((barWidth - 2) * progress);
    for (int by = barY + 1; by < barY + barHeight - 1; ++by)
    {
        for (int bx = barX + 1; bx < barX + 1 + fillWidth; ++bx)
        {
            putPix( engineContext, bx, by, rgb( 180, 150, 50 ) ); // Gold fill
        }
    }

    // Progress Text
    std::string progText = std::to_string( mesuemObjectives.viewedArtworks.size() ) + "/" + std::to_string( mesuemObjectives.totalArtworksToFind );
    drawStringTinyScaled( engineContext, barX + barWidth - 30, barY - 12, progText, colText, 1 );

    if (!mesuemObjectives.mainObjective.empty())
    {
        drawStringTinyScaled( engineContext, x + 10, y + height - 8, mesuemObjectives.mainObjective, rgb( 190, 190, 205 ), 1, 1, 1, false );
    }
}

static void renderCaveHUD( Engine &engineContext ) {
    if (!g_caveTimerActive || engineContext.currentLevel != Levels::CAVE || g_caveQuizPassed) return;

    int w = 180;
    int h = 40;
    int x = RENDER_W - w - 10;
    int y = 10;
    drawTextBox( engineContext, x, y, w, h, rgb( 20, 10, 10 ), rgb( 180, 50, 50 ) );

    int mins = (int)g_caveTimerSeconds / 60;
    int secs = (int)g_caveTimerSeconds % 60;
    char buf[ 32 ];
    snprintf( buf, sizeof( buf ), "Oxygen Time Left %02d:%02d", mins, secs );
    drawString16x16( engineContext, x + 12, y + 12, buf, rgb( 255, 100, 100 ), w, 1, 1, false );
}

static void renderDialogueSubtitle( Engine &engineContext ) {
    if (!g_dialogue.isActive()) return;

    const std::string text = g_dialogue.currentText();
    if (text.empty()) return;

    int w = 760;
    int h = 50;
    int x = (RENDER_W - w) / 2;
    int y = RENDER_H - h - 30;

    drawTextBox( engineContext, x, y, w, h, rgb( 8, 8, 12 ), rgb( 130, 130, 150 ) );
    drawWrappedText( engineContext, x + 14, y + 18, text, rgb( 235, 235, 235 ), w - 28, 2);
}

static void renderCombatHUD( Engine &engineContext ) {
    if (!g_combatState.active || !g_combatState.hasRevolver) return;

    int w = 200;
    int h = 50;
    int x = 12;
    int y = RENDER_H - h - 12;

    drawTextBox( engineContext, x, y, w, h, rgb( 10, 10, 14 ), rgb( 150, 150, 165 ) );
    drawStringTinyScaled( engineContext, x + 12, y + 12, "Revolver", rgb( 220, 220, 225 ), 2, 1, 1, false );
    std::string ammo = std::to_string( g_combatState.loadedAmmo ) + " / " + std::to_string( g_combatState.reserveAmmo );
    drawString16x16( engineContext, x + 98, y + 20, ammo, rgb( 255, 215, 120 ), w - 104, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 12, y + 34, "H To Holster", rgb( 170, 170, 185 ), 1, 1, 1, false );
}

static void renderHeldRevolver( Engine &engineContext ) {
    (void)engineContext;
}

static void renderRevolverShotEffects( Engine &engineContext, float intensity ) {
    const float shotFx = std::clamp( intensity, 0.0f, 1.0f );
    if (shotFx <= 0.001f) return;

    const float flash = std::pow( shotFx, 0.42f );
    drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 255, 245, 230 ), 0.34f * flash );
    drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 255, 210, 170 ), 0.12f * flash );

    const int cx = g_revolverAiming ? int( RENDER_W * 0.55f ) : int( RENDER_W * 0.66f );
    const int cy = g_revolverAiming ? int( RENDER_H * 0.58f ) : int( RENDER_H * 0.66f );
    const float radius = std::max( 38.0f, 185.0f * flash );

    for (int y = std::max( 0, cy - int( radius ) ); y <= std::min( RENDER_H - 1, cy + int( radius ) ); ++y)
    {
        for (int x = std::max( 0, cx - int( radius ) ); x <= std::min( RENDER_W - 1, cx + int( radius ) ); ++x)
        {
            const float dx = float( x - cx );
            const float dy = float( y - cy );
            const float d = std::sqrt( dx * dx + dy * dy );
            if (d > radius) continue;

            const float glow = std::clamp( 1.0f - (d / radius), 0.0f, 1.0f ) * flash;
            if (glow < 0.22f) continue;

            const Uint8 r = Uint8( std::clamp( 180.0f + 58.0f * glow, 0.0f, 255.0f ) );
            const Uint8 g = Uint8( std::clamp( 145.0f + 62.0f * glow, 0.0f, 255.0f ) );
            const Uint8 b = Uint8( std::clamp( 116.0f + 48.0f * glow, 0.0f, 255.0f ) );
            putPix( engineContext, x, y, rgb( r, g, b ) );
        }
    }
}

static void renderSchoolSafeWeaponBlur( Engine &engineContext ) {
    if (!config::schoolMode) return;
    if (!g_combatState.active || !g_combatState.hasRevolver) return;
    if (!g_showHeldWeapon && !g_revolverInspectCutsceneActive) return;

    int x = int( RENDER_W * 0.56f );
    int y = int( RENDER_H * 0.54f );
    int w = int( RENDER_W * 0.36f );
    int h = int( RENDER_H * 0.30f );

    if (g_revolverAiming)
    {
        x = int( RENDER_W * 0.45f );
        y = int( RENDER_H * 0.48f );
        w = int( RENDER_W * 0.26f );
        h = int( RENDER_H * 0.26f );
    }
    else if (g_revolverInspectCutsceneActive)
    {
        x = int( RENDER_W * 0.40f );
        y = int( RENDER_H * 0.35f );
        w = int( RENDER_W * 0.30f );
        h = int( RENDER_H * 0.35f );
    }

    x = std::clamp( x, 0, RENDER_W - 1 );
    y = std::clamp( y, 0, RENDER_H - 1 );
    w = std::clamp( w, 1, RENDER_W - x );
    h = std::clamp( h, 1, RENDER_H - y );

    const int block = 6;
    for (int by = y; by < y + h; by += block)
    {
        for (int bx = x; bx < x + w; bx += block)
        {
            const int ex = std::min( bx + block, x + w );
            const int ey = std::min( by + block, y + h );

            uint32_t sumR = 0, sumG = 0, sumB = 0, count = 0;
            for (int py = by; py < ey; ++py)
            {
                for (int px = bx; px < ex; ++px)
                {
                    Uint32 c = engineContext.backbuffer[ py * RENDER_W + px ];
                    sumR += (c >> 16) & 255;
                    sumG += (c >> 8) & 255;
                    sumB += c & 255;
                    ++count;
                }
            }

            if (count == 0) continue;
            const Uint32 avg = rgb( Uint8( sumR / count ), Uint8( sumG / count ), Uint8( sumB / count ) );

            for (int py = by; py < ey; ++py)
            {
                for (int px = bx; px < ex; ++px)
                {
                    putPix( engineContext, px, py, avg );
                }
            }
        }
    }

    drawTextBox( engineContext, x, y, w, h, rgb( 0, 0, 0 ), rgb( 120, 120, 140 ) );
}

static void renderAccessPopup( Engine &engineContext ) {
    if (g_accessPopup.empty() || SDL_GetTicks() > g_accessPopupUntil) return;

    int w = 500;
    int h = 70;
    int x = (RENDER_W - w) / 2;
    int y = RENDER_H - h - 20;
    bool denied = (g_accessPopup.find( "denied" ) != std::string::npos) ||
        (g_accessPopup.find( "Denied" ) != std::string::npos) ||
        (g_accessPopup.find( "required" ) != std::string::npos);
    Uint32 border = denied ? rgb( 200, 40, 40 ) : rgb( 120, 170, 70 );
    Uint32 head = denied ? rgb( 255, 80, 80 ) : rgb( 180, 230, 120 );
    std::string title = denied ? "The Door Is Locked" : "LOG UPDATED";
    drawTextBox( engineContext, x, y, w, h, rgb( 12, 12, 16 ), border );
    drawString16x16( engineContext, x + 12, y + 10, title, head, w - 24, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 12, y + 38, g_accessPopup, rgb( 230, 230, 230 ), 2, 1, 1, false );
 
}

static void renderCodeEntry( Engine &engineContext ) {
    if (!g_codeEntryActive || g_codeEntryLockIndex < 0 || g_codeEntryLockIndex >= (int)g_roomLocks.size()) return;

    const RoomLock &lock = g_roomLocks[ g_codeEntryLockIndex ];
    int w = 420;
    int h = 150;
    int x = (RENDER_W - w) / 2;
    int y = (RENDER_H - h) / 2;
    drawTextBox( engineContext, x, y, w, h, rgb( 10, 10, 14 ), rgb( 180, 150, 50 ) );

    drawString16x16( engineContext, x + 16, y + 14, "ENTER ACCESS CODE", rgb( 255, 220, 120 ), w - 32, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 16, y + 42, lock.roomName, rgb( 170, 170, 185 ), 2, 1, 1, false );
    drawTextBox( engineContext, x + 16, y + 65, w - 32, 34, rgb( 0, 0, 0 ), rgb( 90, 90, 110 ) );
    drawString16x16( engineContext, x + 30, y + 74, g_codeEntryBuffer.empty() ? "----" : g_codeEntryBuffer, rgb( 220, 220, 230 ), w - 60, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 16, y + 112, "TYPE 4 DIGITS, ENTER TO CONFIRM, ESC TO CANCEL", rgb( 120, 120, 140 ), 1, 1, 1, false );
}

static void renderSafeEntry( Engine &engineContext ) {
    if (!g_codeEntryActive || g_safeEntryIndex < 0 || g_safeEntryIndex >= (int)g_safes.size()) return;

    const SafePuzzle &safe = g_safes[ g_safeEntryIndex ];
    int w = 380;
    int h = 160;
    int x = (RENDER_W - w) / 2;
    int y = (RENDER_H - h) / 2;
    drawTextBox( engineContext, x, y, w, h, rgb( 15, 10, 10 ), rgb( 150, 150, 150 ) );

    drawString16x16( engineContext, x + 16, y + 14, "SAFE CODE", rgb( 210, 210, 210 ), w - 32, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 16, y + 42, safe.safeName, rgb( 170, 170, 185 ), 2, 1, 1, false );
    drawTextBox( engineContext, x + 16, y + 65, w - 32, 34, rgb( 0, 0, 0 ), rgb( 70, 70, 70 ) );
    drawString16x16( engineContext, x + 30, y + 74, g_codeEntryBuffer.empty() ? "----" : g_codeEntryBuffer, rgb( 220, 220, 230 ), w - 60, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 16, y + 120, "INPUT 4 DIGITS, ENTER CONFIRM, ESC CANCEL", rgb( 120, 120, 140 ), 1, 1, 1, false );
}

static void renderSymbolEntry( Engine &engineContext ) {
    if (!g_codeEntryActive || g_symbolEntryIndex < 0 || g_symbolEntryIndex >= (int)g_symbols.size()) return;

    const SymbolPuzzle &sym = g_symbols[ g_symbolEntryIndex ];
    int w = 460;
    int h = 200;
    int x = (RENDER_W - w) / 2;
    int y = (RENDER_H - h) / 2;
    drawTextBox( engineContext, x, y, w, h, rgb( 10, 15, 10 ), rgb( 100, 150, 100 ) );

    drawString16x16( engineContext, x + 16, y + 14, "PEDESTAL", rgb( 150, 220, 150 ), w - 32, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 16, y + 42, sym.name, rgb( 170, 170, 185 ), 2, 1, 1, false );

    const char* symbolNames[] = { "OWL", "WOLF", "STAG", "SERPENT" };

    for(int i = 0; i < 3; ++i) {
        int bx = x + 30 + (i * 140);
        int by = y + 70;
        bool focus = (g_symbolFocus == i);
        drawTextBox( engineContext, bx, by, 120, 50, rgb( 5, 5, 5 ), focus ? rgb(200, 200, 200) : rgb( 60, 60, 60 ) );
        drawString16x16( engineContext, bx + 10, by + 16, symbolNames[g_symbolState[i]], focus ? rgb( 255, 255, 255 ) : rgb( 160, 160, 160 ), 100, 1, 1, false );
    }

    drawStringTinyScaled( engineContext, x + 16, y + 140, "LEFT/RIGHT SELECT SLOT", rgb( 120, 120, 140 ), 1, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 16, y + 155, "UP/DOWN CHANGE SYMBOL", rgb( 120, 120, 140 ), 1, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 16, y + 170, "ENTER TO SUBMIT, ESC TO EXIT", rgb( 120, 120, 140 ), 1, 1, 1, false );
}

static std::vector<std::string> wrapNoteTextLines( const std::string &text, int maxCharsPerLine ) {
    std::vector<std::string> out;
    std::istringstream paragraphs( text );
    std::string paragraph;

    while (std::getline( paragraphs, paragraph, '\n' ))
    {
        if (paragraph.empty())
        {
            out.push_back( "" );
            continue;
        }

        std::istringstream words( paragraph );
        std::string word;
        std::string line;
        while (words >> word)
        {
            if (line.empty())
            {
                line = word;
            }
            else if ((int)(line.size() + 1 + word.size()) <= maxCharsPerLine)
            {
                line += " " + word;
            }
            else
            {
                out.push_back( line );
                line = word;
            }
        }
        if (!line.empty()) out.push_back( line );
    }

    if (out.empty()) out.push_back( "" );
    return out;
}

static void renderNotesScreen( Engine &engineContext ) {
    if (!g_notesOpen) return;

    drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), 180.0f / 255.0f );

    int panelW = RENDER_W - 140;
    int panelH = RENDER_H - 110;
    int x = (RENDER_W - panelW) / 2;
    int y = (RENDER_H - panelH) / 2;

    drawTranslucentBox( engineContext, x + 8, y + 8, panelW, panelH, rgb( 0, 0, 0 ), 100.0f / 255.0f );
    drawTextBox( engineContext, x, y, panelW, panelH, rgb( 240, 240, 235 ), rgb( 212, 212, 206 ) );
    drawString16x16( engineContext, x + 18, y + 16, "FIELD NOTES", rgb( 30, 30, 30 ), panelW - 36, 1, 1, false );
    drawStringTinyScaled( engineContext, x + panelW - 290, y + 22, "UP/DOWN SELECT  PGUP/PGDN SCROLL  N/ESC CLOSE", rgb( 70, 70, 70 ), 1, 1, 1, false );

    int listX = x + 16;
    int listY = y + 52;
    int listW = 290;
    int listH = panelH - 66;

    int bodyX = listX + listW + 12;
    int bodyY = listY;
    int bodyW = panelW - (bodyX - x) - 16;
    int bodyH = listH;

    drawTextBox( engineContext, listX, listY, listW, listH, rgb( 232, 232, 227 ), rgb( 205, 205, 198 ) );
    drawTextBox( engineContext, bodyX, bodyY, bodyW, bodyH, rgb( 235, 235, 230 ), rgb( 205, 205, 198 ) );

    if (g_foundNotes.empty())
    {
        drawString16x16( engineContext, listX + 12, listY + 14, "No clues collected yet.", rgb( 30, 30, 30 ), listW - 24, 1, 1, false );
        return;
    }

    g_notesSelected = std::clamp( g_notesSelected, 0, (int)g_foundNotes.size() - 1 );
    int listLineY = listY + 10;
    const int listLineStep = 18;
    for (int i = 0; i < (int)g_foundNotes.size(); ++i)
    {
        int noteIdx = g_foundNotes[ i ];
        if (noteIdx < 0 || noteIdx >= (int)g_clueNotes.size()) continue;
        const auto &note = g_clueNotes[ noteIdx ];

        bool selected = (i == g_notesSelected);
        if (selected)
        {
            drawTextBox( engineContext, listX + 6, listLineY - 2, listW - 12, 16, rgb( 225, 225, 220 ), rgb( 30, 30, 30 ) );
        }

        drawStringTinyScaled( engineContext, listX + 10, listLineY, note.title, selected ? rgb( 30, 30, 30 ) : rgb( 55, 55, 55 ), 1, 1, 1, false );
        listLineY += listLineStep;
        if (listLineY > listY + listH - 14) break;
    }

    int selectedNoteIdx = g_foundNotes[ g_notesSelected ];
    if (selectedNoteIdx < 0 || selectedNoteIdx >= (int)g_clueNotes.size()) return;

    const auto &selected = g_clueNotes[ selectedNoteIdx ];
    drawString16x16( engineContext, bodyX + 12, bodyY + 10, selected.title, rgb( 30, 30, 30 ), bodyW - 24, 1, 1, false );

    const int maxChars = std::max( 20, (bodyW - 24) / 6 );
    std::vector<std::string> wrapped = wrapNoteTextLines( selected.body, maxChars );
    const int visibleLines = std::max( 1, (bodyH - 50) / 12 );
    const int maxScroll = std::max( 0, (int)wrapped.size() - visibleLines );
    g_notesBodyScroll = std::clamp( g_notesBodyScroll, 0, maxScroll );

    int textY = bodyY + 34;
    for (int i = g_notesBodyScroll; i < (int)wrapped.size() && i < g_notesBodyScroll + visibleLines; ++i)
    {
        drawStringTinyScaled( engineContext, bodyX + 12, textY, wrapped[ i ], rgb( 30, 30, 30 ), 1, 1, 1, false );
        textY += 12;
    }

    std::string scroll = "LINE " + std::to_string( std::min( (int)wrapped.size(), g_notesBodyScroll + 1 ) ) + "/" + std::to_string( std::max( 1, (int)wrapped.size() ) );
    drawStringTinyScaled( engineContext, bodyX + bodyW - 95, bodyY + bodyH - 14, scroll, rgb( 90, 90, 90 ), 1, 1, 1, false );
}

static void renderCompass( Engine &engineContext ) {
    const int boxX = 10;
    const int boxY = 10;
    const int boxW = 120;
    const int boxH = 120;
    const int cx = boxX + boxW / 2;
    const int cy = boxY + boxH / 2;
    const int r = 42;

    drawTextBox( engineContext, boxX, boxY, boxW, boxH, rgb( 14, 14, 18 ), rgb( 160, 140, 80 ) );
    for (int y = -r - 1; y <= r + 1; ++y)
    {
        for (int x = -r - 1; x <= r + 1; ++x)
        {
            int d2 = x * x + y * y;
            if (d2 >= (r - 1) * (r - 1) && d2 <= (r + 1) * (r + 1)) putPix( engineContext, cx + x, cy + y, rgb( 170, 150, 90 ) );
        }
    }
    drawStringTinyScaled( engineContext, cx - 2, cy - r - 10, "N", rgb( 235, 220, 170 ), 1, 1, 1, false );
    drawStringTinyScaled( engineContext, cx - 2, cy + r + 4, "S", rgb( 180, 180, 180 ), 1, 1, 1, false );
    drawStringTinyScaled( engineContext, cx + r + 5, cy - 2, "E", rgb( 180, 180, 180 ), 1, 1, 1, false );
    drawStringTinyScaled( engineContext, cx - r - 8, cy - 2, "W", rgb( 180, 180, 180 ), 1, 1, 1, false );

    float ang = std::atan2( engineContext.directionY, engineContext.directionX );
    int nx = cx + int( std::cos( ang ) * (r - 6) );
    int ny = cy + int( std::sin( ang ) * (r - 6) );
    int x0 = cx, y0 = cy, x1 = nx, y1 = ny;
    int dx = std::abs( x1 - x0 ), sx = (x0 < x1) ? 1 : -1;
    int dy = -std::abs( y1 - y0 ), sy = (y0 < y1) ? 1 : -1;
    int err = dx + dy;
    while (true)
    {
        putPix( engineContext, x0, y0, rgb( 230, 60, 60 ) );
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void renderLevelEditorOverlay( Engine &engineContext ) {
    if (!g_levelEditorMode) return;

    refreshEditorAssetCatalog();

    const auto &catalog = editorAssetCatalog();
    if (catalog.empty()) return;

    int w = 560;
    int h = 166;
    int x = 14;
    int y = RENDER_H - h - 14;
    drawTextBox( engineContext, x, y, w, h, rgb( 10, 10, 14 ), rgb( 90, 170, 210 ) );

    const auto &asset = catalog[ g_editorAssetIndex % (int)catalog.size() ];
    drawString16x16( engineContext, x + 12, y + 10, "LEVEL EDITOR MODE", rgb( 140, 215, 255 ), w - 24, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 12, y + 34, "ASSET: " + asset.label + " (" + asset.assetName + ")", rgb( 220, 220, 220 ), 2, 1, 1, false );

    if (g_editorSelectedModel >= 0 && g_editorSelectedModel < (int)g_worldModels.size() && g_worldModels[ g_editorSelectedModel ].editorPlaced)
    {
        const auto &m = g_worldModels[ g_editorSelectedModel ];
        std::ostringstream tr;
        tr.setf( std::ios::fixed );
        tr.precision( 2 );
        tr << "SELECTED @ X " << m.x << "  Y " << m.y << "  Z " << m.heightOffset;
        drawStringTinyScaled( engineContext, x + 12, y + 50, tr.str(), rgb( 190, 220, 170 ), 2, 1, 1, false );

        std::ostringstream rot;
        rot.setf( std::ios::fixed );
        rot.precision( 2 );
        rot << "ROT Y/P/R " << m.yaw << " / " << m.pitch << " / " << m.roll << "  SIZE " << m.editorTargetScale;
        drawStringTinyScaled( engineContext, x + 12, y + 64, rot.str(), rgb( 190, 220, 170 ), 2, 1, 1, false );
    }
    else
    {
        drawStringTinyScaled( engineContext, x + 12, y + 50, "NO SELECTED MODEL", rgb( 200, 190, 150 ), 2, 1, 1, false );
    }

    drawStringTinyScaled( engineContext, x + 12, y + 88, "F2 TO EXIT [ ] CYCLE ASSET ENTER PLACE TAB SELECT DEL DELETE", rgb( 170, 170, 190 ), 1, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 12, y + 104, "MOUSE MOVE X/Y  WHEEL Z  |  WASD X/Y  R/F Z  Q/E YAW  Z/X PITCH  C/V ROLL  -/= SIZE", rgb( 170, 170, 190 ), 1, 1, 1, false );
    drawStringTinyScaled( engineContext, x + 12, y + 120, "CTRL+S SAVE TO " + g_currentEditorModelsFile + " (current level)", rgb( 170, 170, 190 ), 1, 1, 1, false );
}

static void renderEndingScreen( Engine &engineContext ) {
    int w = RENDER_W - 120;
    int h = RENDER_H - 80;
    int x = (RENDER_W - w) / 2;
    int y = (RENDER_H - h) / 2;
    drawTextBox( engineContext, x, y, w, h, rgb( 10, 10, 14 ), rgb( 190, 160, 80 ) );
    drawString16x16( engineContext, x + 20, y + 20, "THE EXHIBIT IS COMPLETE", rgb( 255, 230, 120 ), w - 40, 1, 1, false );
    int cy = y + 60;
    cy = drawWrappedText( engineContext, x + 20, cy, "Final note decoded: You are beneath the museum in buried foundation tunnels.", rgb( 220, 220, 220 ), w - 40 );
    cy += 14;
    std::string stats = "Art Viewed: " + std::to_string( mesuemObjectives.viewedArtworks.size() ) +
        "   Notes Found: " + std::to_string( g_notesCollectedRun ) +
        "   Time: " + std::to_string( int( g_runElapsedSeconds ) ) + "s";
    drawStringTinyScaled( engineContext, x + 20, cy, stats, rgb( 170, 190, 220 ), 2, 1, 1, false );
    drawString16x16( engineContext, x + 20, y + h - 34, "[R] Restart   [ESC] Menu", rgb( 210, 210, 210 ), w - 40, 1, 1, false );
}

static int wrapTextureCoord( int value, int size ) {
    if (size <= 0) return 0;
    value %= size;
    if (value < 0) value += size;
    return value;
}

static void renderWorldModelsRange(
    Engine &engineContext,
    std::vector<float> &meshInvDepthBuffer,
    const std::vector<float> &wallInvDepthBuffer,
    float pitchOffset,
    int yStartInclusive,
    int yEndExclusive ) {
    if (yStartInclusive >= yEndExclusive) return;

    const float museumPowerMul = museumPowerLightMultiplierForLevel( engineContext.currentLevel );
    const float projScaleY = (RENDER_W * 0.5f);
    const float horizon = (RENDER_H * 0.5f) + pitchOffset;
    const float camHeight = 0.52f;
    const float nearClip = 0.18f;
    const float invDet = 1.0f / (engineContext.planeX * engineContext.directionY - engineContext.directionX * engineContext.planeY);
    const glm::vec3 lightDir = glm::normalize( glm::vec3( -0.35f, 0.85f, -0.40f ) );
    const float kViewPreload = 0.28f;
    const float kCullingEpsilon = 0.015f;

    struct CpuProjVert {
        float sx = 0.0f;
        float sy = 0.0f;
        float z = -1.0f;
        glm::vec3 world{0.0f};
        glm::vec3 color{1.0f};
        glm::vec2 uv{0.0f};
        bool valid = false;
    };

    auto toFixed256 = []( float v )->int {
        return std::clamp( int( v * 256.0f + 0.5f ), 0, 256 );
    };

    for (const auto &inst : g_worldModels)
    {
        if (!inst.visible || !inst.model || inst.model->indices.size() < 3) continue;
        if (shouldGpuRenderModel( engineContext, inst )) continue;

        const glm::vec3 modelHalfExtents = glm::max( (inst.model->boundsMax - inst.model->boundsMin) * 0.5f, glm::vec3( 0.0001f ) ) * inst.scale;
        const float modelRadius = std::max( 0.05f, glm::length( modelHalfExtents ) );
        const float modelCenterY = inst.heightOffset + modelHalfExtents.y;

        const float centerDx = inst.x - engineContext.positionX;
        const float centerDy = inst.y - engineContext.positionY;
        const float centerTx = invDet * (engineContext.directionY * centerDx - engineContext.directionX * centerDy);
        const float centerTz = invDet * (-engineContext.planeY * centerDx + engineContext.planeX * centerDy);

        if ((centerTz + modelRadius) <= nearClip) continue;
        if ((centerTz - modelRadius) > g_worldModelRenderDistance) continue;

        const float txRadius = modelRadius / std::max( 0.001f, FOV_TAN );
        const float horizontalLimit = (1.0f + kViewPreload) * std::max( centerTz, nearClip );
        if ((centerTx - txRadius) > horizontalLimit || (centerTx + txRadius) < -horizontalLimit) continue;

        const float centerScreenY = horizon - ((modelCenterY - camHeight) * projScaleY / std::max( centerTz, nearClip ));
        const float verticalRadiusPx = (projScaleY * modelRadius) / std::max( centerTz, nearClip );
        if ((centerScreenY + verticalRadiusPx) < (-RENDER_H * kViewPreload) ||
            (centerScreenY - verticalRadiusPx) > (RENDER_H * (1.0f + kViewPreload)))
        {
            continue;
        }

        std::vector<CpuProjVert> projected( inst.model->vertices.size() );
        std::vector<glm::vec3> transformed( inst.model->vertices.size(), glm::vec3( 0.0f ) );

        const float timeSeconds = SDL_GetTicks() * 0.001f;
        const float yawNow = inst.yaw + (inst.spinYaw ? (inst.spinSpeed * timeSeconds) : 0.0f);
        const glm::quat qYaw = glm::angleAxis( yawNow, glm::vec3( 0.0f, 1.0f, 0.0f ) );
        const glm::quat qPitch = glm::angleAxis( inst.pitch, glm::vec3( 1.0f, 0.0f, 0.0f ) );
        const glm::quat qRoll = glm::angleAxis( inst.roll, glm::vec3( 0.0f, 0.0f, 1.0f ) );
        const glm::quat q = qYaw * qPitch * qRoll;
        const glm::vec3 pivot(
            (inst.model->boundsMin.x + inst.model->boundsMax.x) * 0.5f,
            inst.model->boundsMin.y,
            (inst.model->boundsMin.z + inst.model->boundsMax.z) * 0.5f );

        float modelMinY = std::numeric_limits<float>::max();
        for (size_t vi = 0; vi < inst.model->vertices.size(); ++vi)
        {
            const glm::vec3 local = (inst.model->vertices[ vi ] - pivot) * inst.scale;
            transformed[ vi ] = q * local;
            modelMinY = std::min( modelMinY, transformed[ vi ].y );
        }
        if (!std::isfinite( modelMinY )) modelMinY = 0.0f;

        bool hasProjectedVerts = false;
        float modelMinSx = std::numeric_limits<float>::max();
        float modelMaxSx = std::numeric_limits<float>::lowest();
        float modelMinSy = std::numeric_limits<float>::max();
        float modelMaxSy = std::numeric_limits<float>::lowest();
        float modelNearestZ = std::numeric_limits<float>::max();

        for (size_t vi = 0; vi < inst.model->vertices.size(); ++vi)
        {
            const glm::vec3 r = transformed[ vi ];

            const float wx = inst.x + r.x;
            const float wy = (r.y - modelMinY) + inst.heightOffset;
            const float wz = inst.y + r.z;

            const float dx = wx - engineContext.positionX;
            const float dy = wz - engineContext.positionY;
            const float tx = invDet * (engineContext.directionY * dx - engineContext.directionX * dy);
            const float tz = invDet * (-engineContext.planeY * dx + engineContext.planeX * dy);
            if (tz <= nearClip) continue;

            projected[ vi ].sx = (RENDER_W * 0.5f) * (1.0f + (tx / tz));
            projected[ vi ].sy = horizon - ((wy - camHeight) * projScaleY / tz);
            projected[ vi ].z = tz;
            projected[ vi ].world = glm::vec3( wx, wy, wz );
            if (vi < inst.model->colors.size()) projected[ vi ].color = inst.model->colors[ vi ];
            if (vi < inst.model->uvs.size()) projected[ vi ].uv = inst.model->uvs[ vi ];
            projected[ vi ].valid = true;

            hasProjectedVerts = true;
            modelMinSx = std::min( modelMinSx, projected[ vi ].sx );
            modelMaxSx = std::max( modelMaxSx, projected[ vi ].sx );
            modelMinSy = std::min( modelMinSy, projected[ vi ].sy );
            modelMaxSy = std::max( modelMaxSy, projected[ vi ].sy );
            modelNearestZ = std::min( modelNearestZ, tz );
        }

        if (!hasProjectedVerts) continue;

        const int modelScreenMinX = std::max( 0, (int)std::floor( modelMinSx ) );
        const int modelScreenMaxX = std::min( RENDER_W - 1, (int)std::ceil( modelMaxSx ) );
        const int modelScreenMinY = std::max( 0, (int)std::floor( modelMinSy ) );
        const int modelScreenMaxY = std::min( RENDER_H - 1, (int)std::ceil( modelMaxSy ) );
        if (modelScreenMinX > modelScreenMaxX || modelScreenMinY > modelScreenMaxY) continue;
        if (modelScreenMaxY < yStartInclusive || modelScreenMinY >= yEndExclusive) continue;

        const int modelScreenW = modelScreenMaxX - modelScreenMinX + 1;
        const int modelScreenH = modelScreenMaxY - modelScreenMinY + 1;
        if (g_perfLowMode && (modelScreenW * modelScreenH) <= 20) continue;

        bool occlusionRejected = true;
        int sampleStepX = std::max( 1, (modelScreenMaxX - modelScreenMinX + 1) / 24 );
        for (int sx = modelScreenMinX; sx <= modelScreenMaxX; sx += sampleStepX)
        {
            if (modelNearestZ < (engineContext.zbuffer[ sx ] - kCullingEpsilon))
            {
                occlusionRejected = false;
                break;
            }
        }
        if (occlusionRejected && (modelScreenMaxX != modelScreenMinX))
        {
            if (modelNearestZ < (engineContext.zbuffer[ modelScreenMaxX ] - kCullingEpsilon))
            {
                occlusionRejected = false;
            }
        }
        if (occlusionRejected) continue;

        int renderedTrianglesForModel = 0;
        const int triangleBudget = g_perfLowMode ? 520 : std::numeric_limits<int>::max();
        const int triStride = std::max( 1, g_meshTriangleStride );
        const int rasterStep = std::max( 1, g_meshRasterStep );

        for (size_t ii = 0; ii + 2 < inst.model->indices.size(); ii += 3)
        {
            const int triIdx = int( ii / 3 );
            if (triStride > 1 && (triIdx % triStride) != 0) continue;
            if (renderedTrianglesForModel >= triangleBudget) break;

            const uint32_t i0 = inst.model->indices[ ii + 0 ];
            const uint32_t i1 = inst.model->indices[ ii + 1 ];
            const uint32_t i2 = inst.model->indices[ ii + 2 ];
            if (i0 >= projected.size() || i1 >= projected.size() || i2 >= projected.size()) continue;

            const CpuProjVert &a = projected[ i0 ];
            const CpuProjVert &b = projected[ i1 ];
            const CpuProjVert &c = projected[ i2 ];
            if (!a.valid || !b.valid || !c.valid) continue;
            if (a.z <= nearClip || b.z <= nearClip || c.z <= nearClip) continue;

            const float area = (b.sx - a.sx) * (c.sy - a.sy) - (b.sy - a.sy) * (c.sx - a.sx);
            if (std::fabs( area ) < 1e-5f) continue;

            int minX = std::max( 0, (int)std::floor( std::min( { a.sx, b.sx, c.sx } ) ) );
            int maxX = std::min( RENDER_W - 1, (int)std::ceil( std::max( { a.sx, b.sx, c.sx } ) ) );
            int minY = std::max( 0, (int)std::floor( std::min( { a.sy, b.sy, c.sy } ) ) );
            int maxY = std::min( RENDER_H - 1, (int)std::ceil( std::max( { a.sy, b.sy, c.sy } ) ) );

            if (minX > maxX || minY > maxY) continue;
            if ((maxX - minX) > (RENDER_W - 8) || (maxY - minY) > (RENDER_H - 8)) continue;

            if (g_perfLowMode)
            {
                const int triW = maxX - minX + 1;
                const int triH = maxY - minY + 1;
                if ((triW * triH) <= 2) continue;

                const float triMidZ = (a.z + b.z + c.z) * (1.0f / 3.0f);
                if (triMidZ > (g_worldModelRenderDistance * 0.55f) && (triIdx & 1))
                {
                    continue;
                }
            }

            minY = std::max( minY, yStartInclusive );
            maxY = std::min( maxY, yEndExclusive - 1 );
            if (minY > maxY) continue;

            glm::vec3 nrm = glm::cross( b.world - a.world, c.world - a.world );
            const float nLen = glm::length( nrm );
            if (nLen <= 1e-6f) continue;
            nrm /= nLen;
            const float lambert = std::clamp( 0.35f + 0.65f * std::fabs( glm::dot( nrm, lightDir ) ), 0.20f, 1.0f );

            const float triDepth = (a.z + b.z + c.z) * (1.0f / 3.0f);
            float distanceShade;
            if (engineContext.caveMode)
            {
                const float R = engineContext.lightRadius;
                const float t = std::clamp( 1.0f - std::pow( triDepth / std::max( 0.001f, R ), engineContext.lightFalloff ), 0.0f, 1.0f );
                distanceShade = std::max( engineContext.caveAmbient, t );
            }
            else
            {
                distanceShade = 1.0f / (1.0f + engineContext.indoorShadeLinear * triDepth + engineContext.indoorShadeQuadratic * triDepth * triDepth);
                distanceShade = std::clamp( distanceShade, engineContext.indoorShadeMin, 1.0f );
            }

            glm::vec3 materialColor( 1.0f );
            if (triIdx >= 0 && triIdx < (int)inst.model->triangleBaseColorFactor.size())
            {
                const glm::vec4 f = inst.model->triangleBaseColorFactor[ triIdx ];
                materialColor = glm::vec3( f.r, f.g, f.b );
            }

            int texIdx = -1;
            if (triIdx >= 0 && triIdx < (int)inst.model->triangleTextureIndex.size())
            {
                texIdx = inst.model->triangleTextureIndex[ triIdx ];
            }

            const Image *tex = nullptr;
            if (texIdx >= 0 && texIdx < (int)inst.model->baseColorTextures.size())
            {
                const Image &candidate = inst.model->baseColorTextures[ texIdx ];
                if (candidate.width > 0 && candidate.height > 0)
                {
                    tex = &candidate;
                }
            }

            glm::vec3 vertexMul( 1.0f );
            if (!tex)
            {
                vertexMul = (a.color + b.color + c.color) * (1.0f / 3.0f);
            }

            const float lit = std::clamp( 0.16f + 0.90f * (lambert * distanceShade), 0.16f, 1.00f );
            const int shadeR256 = toFixed256( materialColor.r * vertexMul.r * lit * museumPowerMul );
            const int shadeG256 = toFixed256( materialColor.g * vertexMul.g * lit * museumPowerMul );
            const int shadeB256 = toFixed256( materialColor.b * vertexMul.b * lit * museumPowerMul );
            const Uint32 solidColor = rgb(
                Uint8( (255 * shadeR256) >> 8 ),
                Uint8( (255 * shadeG256) >> 8 ),
                Uint8( (255 * shadeB256) >> 8 ) );

            const float invZ0 = 1.0f / std::max( 0.0001f, a.z );
            const float invZ1 = 1.0f / std::max( 0.0001f, b.z );
            const float invZ2 = 1.0f / std::max( 0.0001f, c.z );

            const float invArea = 1.0f / area;

            // Edge stepping for E(p) = (v0.x - p.x)*(v1.y - p.y) - (v0.y - p.y)*(v1.x - p.x)
            const float w0dx = (b.sy - c.sy) * float( rasterStep );
            const float w0dy = (c.sx - b.sx) * float( rasterStep );
            const float w1dx = (c.sy - a.sy) * float( rasterStep );
            const float w1dy = (a.sx - c.sx) * float( rasterStep );
            const float w2dx = (a.sy - b.sy) * float( rasterStep );
            const float w2dy = (b.sx - a.sx) * float( rasterStep );

            const float invZdx = (w0dx * invZ0 + w1dx * invZ1 + w2dx * invZ2) * invArea;
            const float invZdy = (w0dy * invZ0 + w1dy * invZ1 + w2dy * invZ2) * invArea;

            const float sampleStartX = float( minX ) + 0.5f;
            const float sampleStartY = float( minY ) + 0.5f;

            const float w0RowInit =
                (b.sx - sampleStartX) * (c.sy - sampleStartY) -
                (b.sy - sampleStartY) * (c.sx - sampleStartX);
            const float w1RowInit =
                (c.sx - sampleStartX) * (a.sy - sampleStartY) -
                (c.sy - sampleStartY) * (a.sx - sampleStartX);
            const float w2RowInit =
                (a.sx - sampleStartX) * (b.sy - sampleStartY) -
                (a.sy - sampleStartY) * (b.sx - sampleStartX);

            float w0Row = w0RowInit;
            float w1Row = w1RowInit;
            float w2Row = w2RowInit;

            float invZRow = (w0Row * invZ0 + w1Row * invZ1 + w2Row * invZ2) * invArea;

            int uFixedRow = 0;
            int vFixedRow = 0;
            int duFixedX = 0;
            int dvFixedX = 0;
            int duFixedY = 0;
            int dvFixedY = 0;

            if (tex)
            {
                const float u0 = a.uv.x;
                const float u1 = b.uv.x;
                const float u2 = c.uv.x;
                const float v0 = a.uv.y;
                const float v1 = b.uv.y;
                const float v2 = c.uv.y;

                const float udx = (w0dx * u0 + w1dx * u1 + w2dx * u2) * invArea;
                const float udy = (w0dy * u0 + w1dy * u1 + w2dy * u2) * invArea;
                const float vdx = (w0dx * v0 + w1dx * v1 + w2dx * v2) * invArea;
                const float vdy = (w0dy * v0 + w1dy * v1 + w2dy * v2) * invArea;

                const float uStart = (w0Row * u0 + w1Row * u1 + w2Row * u2) * invArea;
                const float vStart = (w0Row * v0 + w1Row * v1 + w2Row * v2) * invArea;

                const float uScale = float( tex->width ) * 65536.0f;
                const float vScale = float( tex->height ) * 65536.0f;

                uFixedRow = int( uStart * uScale );
                vFixedRow = int( vStart * vScale );
                duFixedX = int( udx * uScale );
                dvFixedX = int( vdx * vScale );
                duFixedY = int( udy * uScale );
                dvFixedY = int( vdy * vScale );
            }

            const bool areaPositive = area > 0.0f;

            for (int y = minY; y <= maxY; y += rasterStep)
            {
                float w0 = w0Row;
                float w1 = w1Row;
                float w2 = w2Row;
                float invZ = invZRow;
                int uFixed = uFixedRow;
                int vFixed = vFixedRow;

                for (int x = minX; x <= maxX; x += rasterStep)
                {
                    const bool inside = areaPositive
                        ? (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f)
                        : (w0 <= 0.0f && w1 <= 0.0f && w2 <= 0.0f);

                    if (inside && invZ > wallInvDepthBuffer[ x ])
                    {
                        const int pix = y * RENDER_W + x;
                        if (invZ > meshInvDepthBuffer[ pix ])
                        {
                            if (tex)
                            {
                                const int tx = wrapTextureCoord( uFixed >> 16, tex->width );
                                const int ty = wrapTextureCoord( vFixed >> 16, tex->height );
                                const Uint32 tc = tex->sample( tx, ty );
                                const Uint8 r = Uint8( (int( (tc >> 16) & 255 ) * shadeR256) >> 8 );
                                const Uint8 g = Uint8( (int( (tc >> 8) & 255 ) * shadeG256) >> 8 );
                                const Uint8 bch = Uint8( (int( tc & 255 ) * shadeB256) >> 8 );
                                engineContext.backbuffer[ pix ] = rgb( r, g, bch );
                            }
                            else
                            {
                                engineContext.backbuffer[ pix ] = solidColor;
                            }

                            meshInvDepthBuffer[ pix ] = invZ;
                        }
                    }

                    w0 += w0dx;
                    w1 += w1dx;
                    w2 += w2dx;
                    invZ += invZdx;
                    uFixed += duFixedX;
                    vFixed += dvFixedX;
                }

                w0Row += w0dy;
                w1Row += w1dy;
                w2Row += w2dy;
                invZRow += invZdy;
                uFixedRow += duFixedY;
                vFixedRow += dvFixedY;
            }

            ++renderedTrianglesForModel;
        }
    }
}

static void rasterWorkerThreadMain( unsigned int workerIndex ) {
    uint64_t observedSerial = 0;

    while (true)
    {
        Engine *jobEngine = nullptr;
        std::vector<float> *jobMeshDepth = nullptr;
        const std::vector<float> *jobWallDepth = nullptr;
        float jobPitchOffset = 0.0f;
        unsigned int jobWorkerCount = 0;
        uint64_t localSerial = 0;

        {
            std::unique_lock<std::mutex> lock( g_rasterWorkMutex );
            g_rasterWorkCv.wait( lock, [&]() {
                return g_rasterPoolShutdown || (g_rasterJobAvailable && g_rasterJobSerial != observedSerial);
            } );

            if (g_rasterPoolShutdown)
            {
                return;
            }

            localSerial = g_rasterJobSerial;
            observedSerial = localSerial;
            jobEngine = g_rasterJobEngine;
            jobMeshDepth = g_rasterJobMeshDepth;
            jobWallDepth = g_rasterJobWallDepth;
            jobPitchOffset = g_rasterJobPitchOffset;
            jobWorkerCount = g_rasterJobWorkerCount;
        }

        if (workerIndex < jobWorkerCount && jobEngine && jobMeshDepth && jobWallDepth)
        {
            const int y0 = int( (workerIndex * RENDER_H) / jobWorkerCount );
            const int y1 = int( ((workerIndex + 1) * RENDER_H) / jobWorkerCount );
            renderWorldModelsRange( *jobEngine, *jobMeshDepth, *jobWallDepth, jobPitchOffset, y0, y1 );

            std::lock_guard<std::mutex> doneLock( g_rasterWorkMutex );
            if (g_rasterJobSerial == localSerial)
            {
                ++g_rasterJobCompleted;
                if (g_rasterJobCompleted >= g_rasterJobWorkerCount)
                {
                    g_rasterDoneCv.notify_one();
                }
            }
        }
    }
}

static void initRasterWorkerPool() {
    if (!g_rasterWorkers.empty()) return;
    if (g_detectedThreadCount <= 1u) return;

    g_rasterPoolShutdown = false;
    g_rasterJobAvailable = false;
    g_rasterJobSerial = 0;
    g_rasterJobWorkerCount = 0;
    g_rasterJobCompleted = 0;

    g_rasterWorkers.reserve( g_detectedThreadCount );
    for (unsigned int i = 0; i < g_detectedThreadCount; ++i)
    {
        g_rasterWorkers.emplace_back( rasterWorkerThreadMain, i );
    }
}

static void shutdownRasterWorkerPool() {
    {
        std::lock_guard<std::mutex> lock( g_rasterWorkMutex );
        g_rasterPoolShutdown = true;
    }
    g_rasterWorkCv.notify_all();

    for (auto &t : g_rasterWorkers)
    {
        if (t.joinable()) t.join();
    }
    g_rasterWorkers.clear();
}

static void dispatchRasterWorkers(
    Engine &engineContext,
    std::vector<float> &meshDepthBuffer,
    const std::vector<float> &wallInvDepthBuffer,
    float pitchOffset,
    unsigned int workerCount ) {
    workerCount = std::max( 1u, workerCount );

    {
        std::lock_guard<std::mutex> lock( g_rasterWorkMutex );
        g_rasterJobEngine = &engineContext;
        g_rasterJobMeshDepth = &meshDepthBuffer;
        g_rasterJobWallDepth = &wallInvDepthBuffer;
        g_rasterJobPitchOffset = pitchOffset;
        g_rasterJobWorkerCount = workerCount;
        g_rasterJobCompleted = 0;
        g_rasterJobAvailable = true;
        ++g_rasterJobSerial;
    }

    g_rasterWorkCv.notify_all();

    std::unique_lock<std::mutex> doneLock( g_rasterWorkMutex );
    g_rasterDoneCv.wait( doneLock, [&]() {
        return g_rasterJobCompleted >= g_rasterJobWorkerCount;
    } );
}

static void renderWorldModels( Engine &engineContext, std::vector<float> &meshDepthBuffer, float pitchOffset ) {
    if (g_worldModels.empty()) return;

    std::vector<float> wallInvDepthBuffer;
    wallInvDepthBuffer.resize( RENDER_W, 0.0f );
    for (int x = 0; x < RENDER_W; ++x)
    {
        const float wz = engineContext.zbuffer[ x ];
        wallInvDepthBuffer[ x ] = (wz > 0.0001f) ? (1.0f / wz) : std::numeric_limits<float>::infinity();
    }

    const int fullYStart = 0;
    const int fullYEnd = RENDER_H;
    const unsigned int threadCount = std::max( 1u, g_detectedThreadCount );
    const bool useMultithreading = g_multithreadingEnabled && threadCount > 1 && g_worldModels.size() >= 4;

    if (!useMultithreading)
    {
        renderWorldModelsRange( engineContext, meshDepthBuffer, wallInvDepthBuffer, pitchOffset, fullYStart, fullYEnd );
        return;
    }

    const unsigned int maxUsefulThreadsByRows = std::max( 1u, (unsigned int)(RENDER_H / 96) );
    const unsigned int workerCount = std::max( 1u, std::min( threadCount, maxUsefulThreadsByRows ) );

    if (workerCount <= 1u || g_rasterWorkers.empty())
    {
        renderWorldModelsRange( engineContext, meshDepthBuffer, wallInvDepthBuffer, pitchOffset, fullYStart, fullYEnd );
        return;
    }

    dispatchRasterWorkers( engineContext, meshDepthBuffer, wallInvDepthBuffer, pitchOffset, workerCount );
}

static void renderWorldModelsGpu( Engine &engineContext, float pitchOffset ) {
    if (!isGpuModelRenderingEnabled()) return;
    if (!engineContext.renderer || g_worldModels.empty()) return;

    const float museumPowerMul = museumPowerLightMultiplierForLevel( engineContext.currentLevel );
    const float projScaleY = (RENDER_W * 0.5f);
    const float horizon = (RENDER_H * 0.5f) + pitchOffset;
    const float camHeight = 0.52f;
    const float nearClip = 0.18f;
    const float invDet = 1.0f / (engineContext.planeX * engineContext.directionY - engineContext.directionX * engineContext.planeY);

    struct GpuVert { float sx = 0, sy = 0, z = -1; glm::vec3 world{0.0f}; glm::vec3 vcolor{1.0f}; glm::vec2 uv{0.0f}; bool valid = false; };
    struct DrawTri { SDL_Vertex a{}, b{}, c{}; float z = 0.0f; SDL_Texture* texture = nullptr; };

    std::vector<DrawTri> tris;
    tris.reserve( 4096 );

    for (const auto &inst : g_worldModels)
    {
        if (!inst.visible || !inst.model || inst.model->indices.size() < 3) continue;
        if (!shouldGpuRenderModel( engineContext, inst )) continue;

        if (inst.model->hwTextures.size() != inst.model->baseColorTextures.size()) {
            inst.model->hwTextures.resize(inst.model->baseColorTextures.size(), nullptr);
            for (size_t t = 0; t < inst.model->baseColorTextures.size(); ++t) {
                const auto& img = inst.model->baseColorTextures[t];
                if (img.width > 0 && img.height > 0) {
                    SDL_Texture* tex = SDL_CreateTexture(engineContext.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, img.width, img.height);
                    if (tex)
                    {
                        SDL_UpdateTexture(tex, nullptr, img.pixels.data(), img.width * 4);
                        SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);
                    }
                    inst.model->hwTextures[t] = tex;
                }
            }
        }

        const glm::vec3 modelHalfExtents = glm::max( (inst.model->boundsMax - inst.model->boundsMin) * 0.5f, glm::vec3( 0.0001f ) ) * inst.scale;
        const float modelRadius = std::max( 0.05f, glm::length( modelHalfExtents ) );

        const float centerDx = inst.x - engineContext.positionX;
        const float centerDy = inst.y - engineContext.positionY;
        const float centerTz = invDet * (-engineContext.planeY * centerDx + engineContext.planeX * centerDy);
        if ((centerTz + modelRadius) <= nearClip) continue;
        if ((centerTz - modelRadius) > g_worldModelRenderDistance) continue;

        std::vector<GpuVert> projected;
        projected.resize( inst.model->vertices.size() );
        std::vector<glm::vec3> transformed;
        transformed.resize( inst.model->vertices.size(), glm::vec3( 0.0f ) );

        const float timeSeconds = SDL_GetTicks() * 0.001f;
        const float yawNow = inst.yaw + (inst.spinYaw ? (inst.spinSpeed * timeSeconds) : 0.0f);
        const glm::quat qYaw = glm::angleAxis( yawNow, glm::vec3( 0.0f, 1.0f, 0.0f ) );
        const glm::quat qPitch = glm::angleAxis( inst.pitch, glm::vec3( 1.0f, 0.0f, 0.0f ) );
        const glm::quat qRoll = glm::angleAxis( inst.roll, glm::vec3( 0.0f, 0.0f, 1.0f ) );
        const glm::quat q = qYaw * qPitch * qRoll;
        const glm::vec3 pivot(
            (inst.model->boundsMin.x + inst.model->boundsMax.x) * 0.5f,
            inst.model->boundsMin.y,
            (inst.model->boundsMin.z + inst.model->boundsMax.z) * 0.5f );

        float modelMinY = std::numeric_limits<float>::max();
        for (size_t i = 0; i < inst.model->vertices.size(); ++i)
        {
            const glm::vec3 local = (inst.model->vertices[ i ] - pivot) * inst.scale;
            transformed[ i ] = q * local;
            modelMinY = std::min( modelMinY, transformed[ i ].y );
        }
        if (!std::isfinite( modelMinY )) modelMinY = 0.0f;

        for (size_t i = 0; i < inst.model->vertices.size(); ++i)
        {
            const glm::vec3 r = transformed[ i ];
            const float wx = inst.x + r.x;
            const float wy = (r.y - modelMinY) + inst.heightOffset;
            const float wz = inst.y + r.z;

            const float dx = wx - engineContext.positionX;
            const float dy = wz - engineContext.positionY;
            const float tx = invDet * (engineContext.directionY * dx - engineContext.directionX * dy);
            const float tz = invDet * (-engineContext.planeY * dx + engineContext.planeX * dy);
            if (tz <= nearClip) continue;

            projected[ i ].sx = (RENDER_W * 0.5f) * (1.0f + (tx / tz));
            projected[ i ].sy = horizon - ((wy - camHeight) * projScaleY / tz);
            projected[ i ].z = tz;
            projected[ i ].world = glm::vec3( wx, wy, wz );
            if (i < inst.model->colors.size()) projected[ i ].vcolor = inst.model->colors[ i ];
            if (i < inst.model->uvs.size()) projected[ i ].uv = inst.model->uvs[ i ];
            projected[ i ].valid = true;
        }

        const int triStride = std::max( 1, g_meshTriangleStride );
        int triBudget = g_perfLowMode ? 650 : std::numeric_limits<int>::max();

        for (size_t i = 0; i + 2 < inst.model->indices.size(); i += 3)
        {
            if (triBudget <= 0) break;
            const int triIdx = int( i / 3 );
            if (triStride > 1 && (triIdx % triStride) != 0) continue;

            const uint32_t i0 = inst.model->indices[ i + 0 ];
            const uint32_t i1 = inst.model->indices[ i + 1 ];
            const uint32_t i2 = inst.model->indices[ i + 2 ];
            if (i0 >= projected.size() || i1 >= projected.size() || i2 >= projected.size()) continue;

            const GpuVert &a = projected[ i0 ];
            const GpuVert &b = projected[ i1 ];
            const GpuVert &c = projected[ i2 ];
            if (!a.valid || !b.valid || !c.valid) continue;

            const float area = (b.sx - a.sx) * (c.sy - a.sy) - (b.sy - a.sy) * (c.sx - a.sx);
            if (std::fabs( area ) < 0.01f) continue;
            if (area >= -0.01f) continue;

            const int minX = std::max( 0, (int)std::floor( std::min( { a.sx, b.sx, c.sx } ) ) );
            const int maxX = std::min( RENDER_W - 1, (int)std::ceil( std::max( { a.sx, b.sx, c.sx } ) ) );
            const int minY = std::max( 0, (int)std::floor( std::min( { a.sy, b.sy, c.sy } ) ) );
            const int maxY = std::min( RENDER_H - 1, (int)std::ceil( std::max( { a.sy, b.sy, c.sy } ) ) );
            if (minX > maxX || minY > maxY) continue;

            const float triMidZ = (a.z + b.z + c.z) * (1.0f / 3.0f);
            const int sxA = std::clamp( (int)a.sx, 0, RENDER_W - 1 );
            const int sxB = std::clamp( (int)b.sx, 0, RENDER_W - 1 );
            const int sxC = std::clamp( (int)c.sx, 0, RENDER_W - 1 );
            if (a.z >= engineContext.zbuffer[ sxA ] && b.z >= engineContext.zbuffer[ sxB ] && c.z >= engineContext.zbuffer[ sxC ]) {
                continue;
            }

            const glm::vec3 nrm = glm::normalize( glm::cross( b.world - a.world, c.world - a.world ) );
            const float lambert = std::clamp( 0.25f + 0.75f * std::fabs( glm::dot( nrm, glm::normalize( glm::vec3( -0.35f, 0.85f, -0.40f ) ) ) ), 0.15f, 1.0f );
            float shade = 1.0f;
            if (engineContext.caveMode)
            {
                float R = engineContext.lightRadius;
                float t = std::clamp( 1.0f - std::pow( triMidZ / std::max( 0.001f, R ), engineContext.lightFalloff ), 0.0f, 1.0f );
                shade = std::max( engineContext.caveAmbient, t );
            }
            else
            {
                shade = 1.0f / (1.0f + engineContext.indoorShadeLinear * triMidZ + engineContext.indoorShadeQuadratic * triMidZ * triMidZ);
                shade = std::clamp( shade, engineContext.indoorShadeMin, 1.0f );
            }
            glm::vec3 baseColor( 1.0f );
            if (triIdx >= 0 && triIdx < (int)inst.model->triangleBaseColorFactor.size())
            {
                const glm::vec4 f = inst.model->triangleBaseColorFactor[ triIdx ];
                baseColor = glm::vec3( f.r, f.g, f.b );
            }

            int texIdx = -1;
            if (triIdx >= 0 && triIdx < (int)inst.model->triangleTextureIndex.size())
            {
                texIdx = inst.model->triangleTextureIndex[ triIdx ];
            }

            const glm::vec3 vAvg = (a.vcolor + b.vcolor + c.vcolor) * (1.0f / 3.0f);
            const float tr = float( (inst.tint >> 16) & 255 ) / 255.0f;
            const float tg = float( (inst.tint >> 8) & 255 ) / 255.0f;
            const float tb = float( inst.tint & 255 ) / 255.0f;

            const glm::vec3 rgb = glm::clamp(
                baseColor * vAvg * glm::vec3( tr, tg, tb ) *
                std::clamp( lambert * shade * engineContext.ambianceMul * g_horrorLightingMul * museumPowerMul, 0.10f, 1.0f ),
                glm::vec3( 0.0f ), glm::vec3( 1.0f ) );
            const SDL_FColor col{ rgb.r, rgb.g, rgb.b, 1.0f };

            DrawTri out{};
            out.z = triMidZ;
            out.a.position = SDL_FPoint{ a.sx * float( WIN_SCALE ), a.sy * float( WIN_SCALE ) };
            out.b.position = SDL_FPoint{ b.sx * float( WIN_SCALE ), b.sy * float( WIN_SCALE ) };
            out.c.position = SDL_FPoint{ c.sx * float( WIN_SCALE ), c.sy * float( WIN_SCALE ) };
            out.a.tex_coord = SDL_FPoint{ a.uv.x, a.uv.y };
            out.b.tex_coord = SDL_FPoint{ b.uv.x, b.uv.y };
            out.c.tex_coord = SDL_FPoint{ c.uv.x, c.uv.y };
            out.a.color = col; out.b.color = col; out.c.color = col;
            out.texture = (texIdx >= 0 && texIdx < (int)inst.model->hwTextures.size()) ? inst.model->hwTextures[ texIdx ] : nullptr;

            tris.push_back( out );

            --triBudget;
        }
    }

    if (!tris.empty())
    {
        std::sort( tris.begin(), tris.end(), []( const DrawTri &lhs, const DrawTri &rhs ) {
            return lhs.z > rhs.z; // draw far-to-near to emulate opaque depth ordering
            } );

        SDL_Texture* currentTex = tris[ 0 ].texture;
        std::vector<SDL_Vertex> batch;
        batch.reserve( 768 );

        auto flushBatch = [&]() {
            if (batch.empty()) return;
            SDL_RenderGeometry( engineContext.renderer, currentTex, batch.data(), (int)batch.size(), nullptr, 0 );
            batch.clear();
        };

        for (const DrawTri &t : tris)
        {
            if (t.texture != currentTex)
            {
                flushBatch();
                currentTex = t.texture;
            }

            batch.push_back( t.a );
            batch.push_back( t.b );
            batch.push_back( t.c );
        }

        flushBatch();
    }
}

static void render( Engine &engineContext, float dt ) {
    (void)dt;

    const float museumPowerMul = museumPowerLightMultiplierForLevel( engineContext.currentLevel );

    const float shotFx01 = std::clamp( g_revolverRecoilTimer / std::max( 0.001f, kRevolverRecoilDuration ), 0.0f, 1.0f );
    const float shotShakeWave = std::pow( shotFx01, 0.56f );
    const float shakePhase = SDL_GetTicks() * 0.001f;
    const float shotPitchShake =
        (std::sin( shakePhase * 92.0f ) * 0.78f + std::cos( shakePhase * 141.0f ) * 0.42f) *
        kRevolverScreenShakeY * shotShakeWave;
    const float recoilKickPitch = shotFx01 * 6.0f;
    const float effectivePitchOffset = engineContext.pitchOffset + shotPitchShake + recoilKickPitch;
    g_lastEffectivePitchOffset = effectivePitchOffset;

    auto luma = []( Uint32 c ) -> float {
        float r = float( (c >> 16) & 255 ), g = float( (c >> 8) & 255 ), b = float( c & 255 );
        return (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;
        };

  
    auto mulFromOverlay = [&]( Uint32 oc, float strength, float minMul, float maxMul, float gamma = 1.0f ) -> float {
        float L = std::pow( std::clamp( luma( oc ), 0.0f, 1.0f ), gamma );
        float m = 1.0f - strength * (1.0f - L);               // dark pixels -> lower multiplier
        return std::clamp( m, minMul, maxMul );
        };

    // Apply brightness multiplier to a packed ARGB8888 color (no hue shift)
    auto applyMul = [&]( Uint32 base, float m ) -> Uint32 {
        float rf = float( (base >> 16) & 255 ) * m;
        float gf = float( (base >> 8) & 255 ) * m;
        float bf = float( base & 255 ) * m;
        Uint8 r = Uint8( std::clamp( rf, 0.0f, 255.0f ) );
        Uint8 g = Uint8( std::clamp( gf, 0.0f, 255.0f ) );
        Uint8 b = Uint8( std::clamp( bf, 0.0f, 255.0f ) );
        return rgb( r, g, b );
        };

    auto shadeCol = []( Uint32 c, float s ) -> Uint32 {
        s = std::clamp( s, 0.0f, 1.0f );
        Uint8 r = Uint8( ((c >> 16) & 255) * s );
        Uint8 g = Uint8( ((c >> 8) & 255) * s );
        Uint8 b = Uint8( (c & 255) * s );
        return rgb( r, g, b );
        };

    auto applyAmbience = [&]( Uint32 c, float shade ) -> Uint32 {
        Uint32 shaded = shadeCol( c, shade );
        float tr = float( (engineContext.ambianceTint >> 16) & 255 ) / 255.0f;
        float tg = float( (engineContext.ambianceTint >> 8) & 255 ) / 255.0f;
        float tb = float( engineContext.ambianceTint & 255 ) / 255.0f;
        const float horrorMul = std::clamp( g_horrorLightingMul, 0.35f, 1.0f );
        Uint8 r = Uint8( std::clamp( float( (shaded >> 16) & 255 ) * tr * engineContext.ambianceMul * horrorMul * museumPowerMul, 0.0f, 255.0f ) );
        Uint8 g = Uint8( std::clamp( float( (shaded >> 8) & 255 ) * tg * engineContext.ambianceMul * horrorMul * museumPowerMul, 0.0f, 255.0f ) );
        Uint8 b = Uint8( std::clamp( float( shaded & 255 ) * tb * engineContext.ambianceMul * horrorMul * museumPowerMul, 0.0f, 255.0f ) );
        return rgb( r, g, b );
        };

    auto caveLight = [&]( float dist ) -> float {
        if (!engineContext.caveMode) return 1.0f;
        float R = engineContext.lightRadius;
        float t = std::clamp( 1.0f - std::pow( dist / std::max( 0.001f, R ), engineContext.lightFalloff ), 0.0f, 1.0f );
        return std::max( engineContext.caveAmbient, t );
        };

    updateHeldRevolverModel( engineContext );

    const int half = RENDER_H / 2;
    engineContext.zbuffer.assign( RENDER_W, 1e9f );

    static int clipTop[ RENDER_W ];
    static int clipBot[ RENDER_W ];
    for (int i = 0; i < RENDER_W; ++i)
    {
        clipTop[ i ] = RENDER_H;
        clipBot[ i ] = -1;
    }

	// Walls (raycasted)
    for (int x = 0; x < RENDER_W; ++x)
    {
        // Build ray
        float camX = 2.0f * x / float( RENDER_W ) - 1.0f;
        float rayDirX = engineContext.directionX + engineContext.planeX * camX;
        float rayDirY = engineContext.directionY + engineContext.planeY * camX;

        int mapX = int( engineContext.positionX );
        int mapY = int( engineContext.positionY );

        float sideDistX, sideDistY;
        float deltaDistX = (rayDirX == 0) ? 1e30f : std::fabs( 1.0f / rayDirX );
        float deltaDistY = (rayDirY == 0) ? 1e30f : std::fabs( 1.0f / rayDirY );
        int stepX = 0, stepY = 0, side = 0;

        if (rayDirX < 0)
        {
            stepX = -1; 
            sideDistX = (engineContext.positionX - mapX) * deltaDistX;
        }
        else
        {
            stepX = 1; 
            sideDistX = (mapX + 1.0f - engineContext.positionX) * deltaDistX;
        }
        if (rayDirY < 0)
        {
            stepY = -1; 
            sideDistY = (engineContext.positionY - mapY) * deltaDistY;
        }
        else
        {
            stepY = 1; 
            sideDistY = (mapY + 1.0f - engineContext.positionY) * deltaDistY;
        }

        // DDA
        int hitTile = 0;
        while (!hitTile)
        {
            if (sideDistX < sideDistY)
            {
                sideDistX += deltaDistX; 
                mapX += stepX; 
                side = 0;
            }
            else
            {
                sideDistY += deltaDistY; 
                mapY += stepY; 
                side = 1;
            }

            const float rayTravel = std::min( sideDistX, sideDistY );
            if (rayTravel > g_wallRenderDistance)
            {
                break;
            }

            if (mapX < 0 || mapY < 0 || mapX >= engineContext.map.width || mapY >= engineContext.map.height) break;
            int tile = engineContext.map.tiles[ mapY * engineContext.map.width + mapX ];
            if (tile > 0) hitTile = tile;
        }
        if (!hitTile) continue;

        // Perpendicular distance
        float perpWallDist = (side == 0)
            ? ((mapX - engineContext.positionX) + (1 - stepX) * 0.5f) / (rayDirX == 0 ? 1e-6f : rayDirX)
            : ((mapY - engineContext.positionY) + (1 - stepY) * 0.5f) / (rayDirY == 0 ? 1e-6f : rayDirY);
        perpWallDist = std::max( std::fabs( perpWallDist ), 0.05f );
        if (perpWallDist > g_wallRenderDistance) continue;

        // Column geometry
        int lineH = int( RENDER_H / std::max( perpWallDist, 1e-3f ) );

    int bob = half + (int)effectivePitchOffset;
        int drawStart = std::max( 0, -lineH / 2 + bob );
        int drawEnd = std::min( RENDER_H - 1, lineH / 2 + bob );
        clipTop[ x ] = std::min( clipTop[ x ], drawStart );
        clipBot[ x ] = std::max( clipBot[ x ], drawEnd );
        // Wall X coordinate (for texture)
        float wallX = (side == 0)
            ? (engineContext.positionY + perpWallDist * rayDirY)
            : (engineContext.positionX + perpWallDist * rayDirX);
        wallX -= std::floor( wallX );

        // Texture selection
        const Image &wallTexture = (hitTile == 2) ? engineContext.doorTexture : engineContext.wallTex;

        // Draw wall column (uses fixed-step in RendererHelpers)
        drawTexturedColumn( engineContext, wallTexture, x, drawStart, drawEnd, perpWallDist, wallX, side );

        if (hitTile == 1)
        {
            if ((engineContext.currentLevel == Levels::MUSEUM || engineContext.currentLevel == Levels::MUSEUM_UPPER) && !(g_perfLowMode && (x & 1))) {
                for (size_t artIndex = 0; artIndex < engineContext.artworks.size(); ++artIndex)
                {
                    const auto& art = engineContext.artworks[artIndex];
                    if (!art.onWall) continue;


                    if (art.wx != mapX || art.wy != mapY || art.side != side) continue;

                    
                    bool visible = true;

                    if (side == 0)
                    { // Vertical Wall (X-Axis)
                        float frac = art.x - std::floor( art.x ); // e.g., 3.1 -> 0.1

                        // Ray moving Right (>0) hits West Face. Ray moving Left (<0) hits East Face.
                        bool hittingWestFace = (rayDirX > 0);

                        if (frac < 0.45f && !hittingWestFace) visible = false; // Art is on West, but we hit East
                        if (frac > 0.55f && hittingWestFace)  visible = false; // Art is on East, but we hit West
                    }
                    else
                    { // Horizontal Wall (Y-Axis)
                        float frac = art.y - std::floor( art.y );

                        // Ray moving Down (>0) hits North Face. Ray moving Up (<0) hits South Face.
                        bool hittingNorthFace = (rayDirY > 0);

                        if (frac < 0.45f && !hittingNorthFace) visible = false; // Art is on North, but we hit South
                        if (frac > 0.55f && hittingNorthFace)  visible = false; // Art is on South, but we hit North
                    }

                    if (!visible) continue;
                    

                    float u0 = std::clamp(art.uCenter - art.uWidth * 0.5f, 0.0f, 1.0f);
                    float u1 = std::clamp(art.uCenter + art.uWidth * 0.5f, 0.0f, 1.0f);
                    if (wallX < u0 || wallX > u1) continue;

                    const Image& texture = engineContext.artImages[artIndex];

                    // Frame/mat proportions
                    const float FRAME_U = 0.08f, FRAME_V = 0.08f;
                    const float MAT_U = 0.03f, MAT_V = 0.04f;

                    const Uint32 goldLight = rgb(235, 200, 80);
                    const Uint32 goldMid = rgb(212, 175, 55);
                    const Uint32 goldDark = rgb(160, 130, 40);
                    const Uint32 matCol = rgb(235, 235, 220);

                    float uLocal = (wallX - u0) / std::max(0.0001f, (u1 - u0));

                    int bandH = std::max(1, int(lineH * art.vHeight));
                    int bandCenter = RENDER_H / 2 + int( (art.vCenter - 0.5f) * lineH );
                    int bandStart = std::clamp( bandCenter - bandH / 2, 0, RENDER_H - 1 );
                    int bandEnd = std::clamp( bandStart + bandH - 1, 0, RENDER_H - 1 );

                    float uLeftFrameEdge = FRAME_U;
                    float uRightFrameEdge = 1.0f - FRAME_U;
                    float uLeftMatEdge = FRAME_U + MAT_U;
                    float uRightMatEdge = 1.0f - (FRAME_U + MAT_U);

                    for (int y = bandStart; y <= bandEnd; ++y)
                    {
                        float vLocal = (y - bandStart) / float(std::max(1, bandH - 1));
                        float vTopFrameEdge = FRAME_V;
                        float vBottomFrameEdge = 1.0f - FRAME_V;
                        float vTopMatEdge = FRAME_V + MAT_V;
                        float vBottomMatEdge = 1.0f - (FRAME_V + MAT_V);

                        Uint32 color;

                        bool inFrame =
                            (uLocal < uLeftFrameEdge) || (uLocal > uRightFrameEdge) ||
                            (vLocal < vTopFrameEdge) || (vLocal > vBottomFrameEdge);

                        if (inFrame)
                        {
                            bool topOrLeft = (vLocal < vTopFrameEdge + 0.02f) || (uLocal < uLeftFrameEdge + 0.02f);
                            bool bottomOrRight = (vLocal > vBottomFrameEdge - 0.02f) || (uLocal > uRightFrameEdge - 0.02f);
                            color = goldMid;
                            if (topOrLeft)
                            {
                                color = goldLight;
                            }
                            else if (bottomOrRight)
                            {
                                color = goldDark;
                            }
                        }
                        else
                        {
                            bool inMat =
                                (uLocal < uLeftMatEdge) || (uLocal > uRightMatEdge) ||
                                (vLocal < vTopMatEdge) || (vLocal > vBottomMatEdge);

                            if (inMat)
                            {
                                color = matCol;
                            }
                            else
                            {
                                float innerU0 = uLeftMatEdge, innerU1 = uRightMatEdge;
                                float innerV0 = vTopMatEdge, innerV1 = vBottomMatEdge;
                                float un = (uLocal - innerU0) / std::max(0.0001f, (innerU1 - innerU0));
                                float vn = (vLocal - innerV0) / std::max(0.0001f, (innerV1 - innerV0));
                                int texX = std::clamp(int(un * (texture.width - 1)), 0, texture.width - 1);
                                int texY = std::clamp(int(vn * (texture.height - 1)), 0, texture.height - 1);
                                color = texture.sample(texX, texY);

                                // magenta transparent -> mat
                                if (((color >> 16) & 255) == 255 && ((color >> 8) & 255) == 0 && (color & 255) == 255)
                                    color = matCol;
                            }
                        }
                        putPix(engineContext, x, y, color);
                    }
                }
            }
            if (engineContext.currentLevel == Levels::MUSEUM && g_stairWallOverlayReady)
            {
                bool stairWallTile = (side == 0) && (mapX == 22) && (mapY == 9);
                if (stairWallTile)
                {
                    float u0 = 0.0f;
                    float u1 = 1.0f;
                    if (wallX >= u0 && wallX <= u1)
                    {
                        int bandStart = drawStart;
                        int bandEnd = drawEnd;
                        int bandH = std::max( 1, bandEnd - bandStart + 1 );

                        float un = (wallX - u0) / std::max( 0.0001f, (u1 - u0) );
                        int texX = std::clamp( int( un * (g_stairWallOverlay.width - 1) ), 0, g_stairWallOverlay.width - 1 );

                        for (int y = bandStart; y <= bandEnd; ++y)
                        {
                            float vn = (y - bandStart) / float( std::max( 1, bandH - 1 ) );
                            int texY = std::clamp( int( vn * (g_stairWallOverlay.height - 1) ), 0, g_stairWallOverlay.height - 1 );
                            Uint32 c = g_stairWallOverlay.sample( texX, texY );
                            if (((c >> 16) & 255) == 255 && ((c >> 8) & 255) == 0 && (c & 255) == 255) continue;
                            putPix( engineContext, x, y, c );
                        }
                    }
                }
            }
            else if (engineContext.currentLevel == Levels::CAVE) {

            }
        }

        // Fill zbuffer for sprites/floor/ceiling occlusion
        engineContext.zbuffer[ x ] = perpWallDist;
    }

    // Floor and ceiling 
    float rayDirX0 = engineContext.directionX - engineContext.planeX;
    float rayDirY0 = engineContext.directionY - engineContext.planeY;
    float rayDirX1 = engineContext.directionX + engineContext.planeX;
    float rayDirY1 = engineContext.directionY + engineContext.planeY;

    const float posZ = 0.5f * RENDER_H;
    int bob = half + (int)effectivePitchOffset;

    for (int y = 0; y < RENDER_H; ++y)
    {
        const int prop = y - bob;
        if (prop == 0) continue;

        float rowDist = std::fabs( posZ / float( prop ) );

        // Step across row
        float stepX = rowDist * (rayDirX1 - rayDirX0) / float( RENDER_W );
        float stepY = rowDist * (rayDirY1 - rayDirY0) / float( RENDER_W );
        float worldX = engineContext.positionX + rowDist * rayDirX0;
        float worldY = engineContext.positionY + rowDist * rayDirY0;

        for (int x = 0; x < RENDER_W; ++x)
        {
            float fx = worldX - std::floor( worldX );
            float fy = worldY - std::floor( worldY );
            if (y >= clipTop[ x ] && y <= clipBot[ x ])
            {
                worldX += stepX;
                worldY += stepY;
                continue; // don't overwrite walls
            }

            if (y >= half)
            {
                if (engineContext.hasFloor)
                {
                    int tx = int( fx * engineContext.floorTex.width );
                    int ty = int( fy * engineContext.floorTex.height );
                    Uint32 color = engineContext.floorTex.sample( tx, ty );

                    float m = 1.0f;

                    if (engineContext.hasFloorStains && !g_perfLowMode)
                    {
                        int ox = int( fx * engineContext.floorOverlayStains.width ) % engineContext.floorOverlayStains.width;
                        int oy = int( fy * engineContext.floorOverlayStains.height ) % engineContext.floorOverlayStains.height;
                        Uint32 oc = engineContext.floorOverlayStains.sample( ox, oy );
                        m *= mulFromOverlay( oc, /*strength*/0.45f, /*min*/0.80f, /*max*/1.03f, /*gamma*/1.2f );
                    }
                    if (engineContext.hasFloorCracks && !g_perfLowMode)
                    {
                        int ox = int( fx * engineContext.floorOverlayCracks.width ) % engineContext.floorOverlayCracks.width;
                        int oy = int( fy * engineContext.floorOverlayCracks.height ) % engineContext.floorOverlayCracks.height;
                        Uint32 oc = engineContext.floorOverlayCracks.sample( ox, oy );
                        m *= mulFromOverlay( oc, /*strength*/0.85f, /*min*/0.55f, /*max*/1.00f, /*gamma*/1.6f );
                    }
                    if (engineContext.hasFloorPuddles && !g_perfLowMode)
                    {
                        int ox = int( fx * engineContext.floorOverlayPuddles.width ) % engineContext.floorOverlayPuddles.width;
                        int oy = int( fy * engineContext.floorOverlayPuddles.height ) % engineContext.floorOverlayPuddles.height;
                        Uint32 oc = engineContext.floorOverlayPuddles.sample( ox, oy );
                        m *= mulFromOverlay( oc, /*strength*/0.60f, /*min*/0.70f, /*max*/1.02f, /*gamma*/1.1f );
                    }

                    color = applyMul( color, m );

                    float shade = 1.0f;
                    if (engineContext.caveMode)
                    {
                        shade = std::clamp( 1.0f / (0.02f * rowDist), 0.30f, 1.0f );
                        shade *= caveLight( rowDist );
                    }
                    else
                    {
                        // Museum Mode: Match the wall formula for consistency
                        shade = 1.0f / (1.0f + engineContext.indoorShadeLinear * rowDist + engineContext.indoorShadeQuadratic * rowDist * rowDist);
                        shade = std::clamp( shade, engineContext.indoorShadeMin, 1.0f );
                    }
                    putPix( engineContext, x, y, applyAmbience( color, shade ) );

                    if (!g_perfLowMode && rowDist < engineContext.zbuffer[ x ] && !engineContext.quadBuckets.empty())
                    {
                        if (shade >= 0.06f) // skip work when very dark
                        {
                            int txTile = (int)std::floor( worldX );
                            int tyTile = (int)std::floor( worldY );
                            if ((unsigned)txTile < (unsigned)engineContext.map.width && (unsigned)tyTile < (unsigned)engineContext.map.height)
                            {
                                const auto &bucket = engineContext.quadBuckets[ tyTile * engineContext.map.width + txTile ];
                                for (int qi : bucket)
                                {
                                    const auto &q = engineContext.quads[ qi ];
                                    float u, v;
                                    if (!quadprop_local_uv( q, worldX, worldY, u, v )) continue;

                                    Uint32 dc = sample_bilinear_uv_keyed( q.texture, u, v );
                                    // magenta keyed; ignore transparent
                                    if (((dc >> 16) & 255) == 255 && ((dc >> 8) & 255) == 0 && (dc & 255) == 255) continue;

                                    // Treat quad as neutral detail: compute multiplier from its luminance
                                    float mul = mulFromOverlay( dc, /*strength*/1.00f, /*min*/0.55f, /*max*/1.05f, /*gamma*/1.4f );
                                    // Incorporate decal AO & cave light (as darkening influence)
                                    float ao = std::clamp( q.AOMultiplier, 0.5f, 1.0f );
                                    float l = caveLight( rowDist );
                                    float finalMul = std::clamp( mul * (0.9f + 0.1f * ao) * l, 0.0f, 1.05f );

                                    // Multiply the pixel already written in backbuffer
                                    Uint32 under = engineContext.backbuffer[ y * RENDER_W + x ];
                                    putPix( engineContext, x, y, applyMul( under, finalMul ) );
                                }
                            }
                        }
                    }
                }
                else
                {
                    putPix( engineContext, x, y, rgb( 12, 12, 14 ) );
                }
            }
            else
            {
                if (y >= clipTop[ x ] && y <= clipBot[ x ])
                {
                    worldX += stepX;
                    worldY += stepY;
                    continue; // don't overwrite walls
                }

                // Ceiling
                if (engineContext.hasCeiling)
                {
                    int tx = int( fx * engineContext.ceilTex.width );
                    int ty = int( fy * engineContext.ceilTex.height );
                    Uint32 color = engineContext.ceilTex.sample( tx, ty );
                    float shade = 1.0f;
                    if (engineContext.caveMode)
                    {
                        shade = std::clamp( 1.0f / (0.02f * rowDist), 0.30f, 1.0f );
                        shade *= caveLight( rowDist );
                    }
                    else
                    {
                        // Museum Mode: Match the wall formula for consistency
                        shade = 1.0f / (1.0f + engineContext.indoorShadeLinear * rowDist + engineContext.indoorShadeQuadratic * rowDist * rowDist);
                        shade = std::clamp( shade, engineContext.indoorShadeMin, 1.0f );
                    }

                    putPix( engineContext, x, y, applyAmbience( color, shade ) );
                }
                else
                {
                    putPix( engineContext, x, y, rgb( 30, 30, 38 ) );
                }
            }

            worldX += stepX;
            worldY += stepY;
        }
    }

    if (engineContext.benches3D.size() > 0)
    {
        for (const auto &box : engineContext.benches3D)
        {
            render_box_top( engineContext, box, box.sideTexure );
            render_box( engineContext, box );
            //render_legs( engineContext, box );
        }
    }

    if (!isGpuModelRenderingEnabled())
    {
        static std::vector<float> meshDepthBuffer;
        meshDepthBuffer.assign( RENDER_W * RENDER_H, 0.0f );
        renderWorldModels( engineContext, meshDepthBuffer, effectivePitchOffset );
    }
    else
    {
        // In Smart mode the CPU pass still handles the subset not offloaded to GPU.
        if (config::gpuRenderMode == 1)
        {
            static std::vector<float> meshDepthBuffer;
            meshDepthBuffer.assign( RENDER_W * RENDER_H, 0.0f );
            renderWorldModels( engineContext, meshDepthBuffer, effectivePitchOffset );
        }
    }



	// Props (billboarded)
    for (size_t i = 0; i < engineContext.props.size(); ++i)
    {
        const auto &prop = engineContext.props[ i ];
        if (prop.scale <= 0.0f) continue;
        const auto &texture = engineContext.propImages[ prop.textureID ];

        // Camera space
        float dx = prop.x - engineContext.positionX, dy = prop.y - engineContext.positionY;
        float invDet = 1.0f / (engineContext.planeX * engineContext.directionY - engineContext.directionX * engineContext.planeY);
        float transX = invDet * (engineContext.directionY * dx - engineContext.directionX * dy);
        float transY = invDet * (-engineContext.planeY * dx + engineContext.planeX * dy);
        if (transY <= 0) continue;

        int spriteScreenX = int( (RENDER_W / 2) * (1 + transX / transY) );
        float baseH = (RENDER_H / transY);
        int spriteH = std::max( 1, int( std::fabs( baseH * prop.scale ) ) );
        int spriteW = spriteH;
        int bottomY = int( RENDER_H * 0.5f + baseH * 0.5f );

        int y0 = bottomY - spriteH;
        int y1 = bottomY - 1;
        int x0 = -spriteW / 2 + spriteScreenX;
        int x1 = spriteW / 2 + spriteScreenX - 1;

        int cy0 = std::max( 0, y0 );
        int cy1 = std::min( RENDER_H - 1, y1 );
        int cx0 = std::max( 0, x0 );
        int cx1 = std::min( RENDER_W - 1, x1 );
        if (cy0 > cy1 || cx0 > cx1) continue;

        float invSpriteH = 1.0f / std::max( 1, spriteH );
        float invSpriteW = 1.0f / std::max( 1, spriteW );

        for (int sx = cx0; sx <= cx1; ++sx)
        {
            if (!(transY > 0 && transY < engineContext.zbuffer[ sx ])) continue;

            float u = float( sx - x0 ) * invSpriteW;
            int texX = std::clamp( int( u * texture.width ), 0, texture.width - 1 );

            int yStep = g_perfLowMode ? 2 : 1;
            for (int sy = cy0; sy <= cy1; sy += yStep)
            {
                float v = float( sy - y0 ) * invSpriteH;
                int texY = std::clamp( int( v * texture.height ), 0, texture.height - 1 );

                Uint32 color = texture.sample( texX, texY );
                if (!isNearMagenta( color, 120 ))
                {
                    float shade = 1.0f;
                    if (engineContext.caveMode)
                    {
                        shade = caveLight( transY );
                    }
                    else
                    {
                        shade = 1.0f / (1.0f + engineContext.indoorShadeLinear * transY + engineContext.indoorShadeQuadratic * transY * transY);
                        shade = std::clamp( shade, 0.08f, 1.0f );
                    }
                    Uint32 shaded = applyAmbience( color, shade );
                    putPix( engineContext, sx, sy, shaded );
                    if (yStep == 2 && sy + 1 <= cy1)
                    {
                        putPix( engineContext, sx, sy + 1, shaded );
                    }
                }
            }
        }
    }

    if (!engineContext.columns.empty())
    {
        for (auto &col : engineContext.columns)
        {
            float dx = col.x - engineContext.positionX;
            float dy = col.y - engineContext.positionY;
            col.distance = dx * dx + dy * dy;
        }

        std::sort( engineContext.columns.begin(), engineContext.columns.end(), []( const ColumnProp &a, const ColumnProp &b ) {
            return a.distance > b.distance;
            } );

        for (const auto &col : engineContext.columns)
        {
            auto setIt = engineContext.columnSpriteSets.find( col.setName );
            if (setIt == engineContext.columnSpriteSets.end() || setIt->second.numViews == 0) continue;

            const SpriteSet &spriteSet = setIt->second;
            const int numViews = spriteSet.numViews;
            float toPlayerX = engineContext.positionX - col.x;
            float toPlayerY = engineContext.positionY - col.y;
            float rel = std::atan2( toPlayerY, toPlayerX );
            while (rel < 0.0f) rel += 2.0f * 3.14159265f;
            while (rel >= 2.0f * 3.14159265f) rel -= 2.0f * 3.14159265f;
            float slice = (2.0f * 3.14159265f) / numViews;
            int viewIndex = int( (rel + slice * 0.5f) / slice ) % numViews;
            const Image &texture = spriteSet.views[ viewIndex ];

            float dx = col.x - engineContext.positionX;
            float dy = col.y - engineContext.positionY;
            float invDet = 1.0f / (engineContext.planeX * engineContext.directionY - engineContext.directionX * engineContext.planeY);
            float transX = invDet * (engineContext.directionY * dx - engineContext.directionX * dy);
            float transY = invDet * (-engineContext.planeY * dx + engineContext.planeX * dy);
            if (transY <= 0) continue;

            int spriteScreenX = int( (RENDER_W / 2) * (1 + transX / transY) );
            float baseH = (RENDER_H / transY);
            int spriteH = std::max( 1, int( std::fabs( baseH * col.scale ) ) );
            int spriteW = (texture.height > 0) ? std::max( 1, int( spriteH * (float( texture.width ) / float( texture.height )) ) ) : spriteH;
            int bottomY = int( RENDER_H * 0.5f + baseH * 0.5f );

            int y0 = bottomY - spriteH;
            int y1 = bottomY - 1;
            int x0 = -spriteW / 2 + spriteScreenX;
            int x1 = spriteW / 2 + spriteScreenX - 1;

            int cy0 = std::max( 0, y0 );
            int cy1 = std::min( RENDER_H - 1, y1 );
            int cx0 = std::max( 0, x0 );
            int cx1 = std::min( RENDER_W - 1, x1 );
            if (cy0 > cy1 || cx0 > cx1) continue;

            float invSpriteH = 1.0f / std::max( 1, spriteH );
            float invSpriteW = 1.0f / std::max( 1, spriteW );
            for (int sx = cx0; sx <= cx1; ++sx)
            {
                if (!(transY > 0 && transY < engineContext.zbuffer[ sx ])) continue;
                float u = float( sx - x0 ) * invSpriteW;
                int texX = std::clamp( int( u * texture.width ), 0, texture.width - 1 );
                for (int sy = cy0; sy <= cy1; ++sy)
                {
                    float v = float( sy - y0 ) * invSpriteH;
                    int texY = std::clamp( int( v * texture.height ), 0, texture.height - 1 );
                    Uint32 color = texture.sample( texX, texY );
                    if (!boolIsNearBlack( color, 120 ))
                    {
                        float shade = 1.0f;
                        if (engineContext.caveMode)
                        {
                            shade = std::clamp( 1.0f / (0.35f * transY), 0.20f, 1.0f );
                            shade *= caveLight( transY );
                        }
                        else
                        {
                            shade = 1.0f / (1.0f + engineContext.indoorShadeLinear * transY + engineContext.indoorShadeQuadratic * transY * transY);
                            shade = std::clamp( shade, engineContext.indoorShadeMin, 1.0f );
                            shade *= 0.78f;
                        }
                        putPix( engineContext, sx, sy, applyAmbience( color, shade ) );
                    }
                }
            }
        }
    }

  

    /*
    for (auto &col : engineContext.columns)
    {
        float dx = col.x - engineContext.positionX;
        float dy = col.y - engineContext.positionY;
        col.distance = dx * dx + dy * dy; // Use squared distance
    }

    std::sort( engineContext.columns.begin(), engineContext.columns.end(), []( const ColumnProp &a, const ColumnProp &b ) {
        return a.distance > b.distance; // Farthest first
        } );

    for (const auto &col : engineContext.columns)
    {
        // Find the sprite set for this column
        auto setIt = engineContext.columnSpriteSets.find( col.setName );
        if (setIt == engineContext.columnSpriteSets.end() || setIt->second.numViews == 0)
        {
            continue; // This column has no valid sprite set, skip rendering
        }
        const SpriteSet &spriteSet = setIt->second;
        const int numViews = spriteSet.numViews;

        // Vector from player to column
        float vecX = col.x - engineContext.positionX;
        float vecY = col.y - engineContext.positionY;

        float vecX_to_Player = engineContext.positionX - col.x;
        float vecY_to_Player = engineContext.positionY - col.y;

        // Angle from column's center to the player
        float relativeAngle = std::atan2( vecY_to_Player, vecX_to_Player );

        // Normalize angle to [0, 2*PI]
        while (relativeAngle < 0) relativeAngle += 2.0f * 3.14159265f;
        while (relativeAngle >= 2.0f * 3.14159265f) relativeAngle -= 2.0f * 3.14159265f;

        // Map normalized angle to sprite index
        // We add slice*0.5 to offset the start, so 0 degrees is centered on the "front" sprite
        float slice = (2.0f * 3.14159265f) / numViews;
        int viewIndex = static_cast<int>( (relativeAngle + slice * 0.5f) / slice ) % numViews;

        const auto &texture = spriteSet.views[ viewIndex ];


        // Camera space
        float dx = col.x - engineContext.positionX;
        float dy = col.y - engineContext.positionY;
        float invDet = 1.0f / (engineContext.planeX * engineContext.directionY - engineContext.directionX * engineContext.planeY);
        float transX = invDet * (engineContext.directionY * dx - engineContext.directionX * dy);
        float transY = invDet * (-engineContext.planeY * dx + engineContext.planeX * dy);
        if (transY <= 0) continue; // Behind player

        int spriteScreenX = int( (RENDER_W / 2) * (1 + transX / transY) );
        float baseH = (RENDER_H / transY);
        int spriteH = std::max( 1, int( std::fabs( baseH * col.scale ) ) );

        // Calculate width based on texture's aspect ratio
        int spriteW = spriteH;
        if (texture.height > 0)
        {
            spriteW = std::max( 1, int( spriteH * (float( texture.width ) / float( texture.height )) ) );
        }

        int bottomY = int( RENDER_H * 0.5f + baseH * 0.5f ); // Aligns bottom with floor

        int y0 = bottomY - spriteH;
        int y1 = bottomY - 1;
        int x0 = -spriteW / 2 + spriteScreenX;
        int x1 = spriteW / 2 + spriteScreenX - 1;

        int cy0 = std::max( 0, y0 );
        int cy1 = std::min( RENDER_H - 1, y1 );
        int cx0 = std::max( 0, x0 );
        int cx1 = std::min( RENDER_W - 1, x1 );
        if (cy0 > cy1 || cx0 > cx1) continue;

        float invSpriteH = 1.0f / std::max( 1, spriteH );
        float invSpriteW = 1.0f / std::max( 1, spriteW );

        for (int sx = cx0; sx <= cx1; ++sx)
        {
            if (!(transY > 0 && transY < engineContext.zbuffer[ sx ])) continue;

            float u = float( sx - x0 ) * invSpriteW;
            int texX = std::clamp( int( u * texture.width ), 0, texture.width - 1 );

            for (int sy = cy0; sy <= cy1; ++sy)
            {
                float v = float( sy - y0 ) * invSpriteH;
                int texY = std::clamp( int( v * texture.height ), 0, texture.height - 1 );

                Uint32 color = texture.sample( texX, texY );
                if (!boolIsNearBlack( color, 120 )) // Use existing transparency check
                {
                    // Apply cave lighting / distance fog
                    float shade = caveLight( transY );
                    color = shadeCol( color, shade );
                    putPix( engineContext, sx, sy, color );
                }
            }
        }
    }
    */


}

static void renderGameplayUiPass( Engine &engineContext ) {
    const bool overlayBusy = g_interactionAnim.active || g_levelTransition.active || g_notesOpen || g_codeEntryActive || g_caveQuizActive || g_levelEditorMode || g_cutsceneController.isCameraLockActive() || g_revolverInspectCutsceneActive || g_wakeCutsceneActive;
    const bool cutsceneHudSuppressed = g_cutsceneController.isCameraLockActive() || g_revolverInspectCutsceneActive || g_wakeCutsceneActive;
    const float shotFx01 = std::clamp( g_revolverRecoilTimer / std::max( 0.001f, kRevolverRecoilDuration ), 0.0f, 1.0f );
    const float museumDarknessAdd = museumPowerDarknessAddForLevel( engineContext.currentLevel );

    int lookingAtArt = pickArtworkUnderCrosshair( engineContext );

    float distanceToArt = 0.0f;
    if (lookingAtArt != -1)
    {
        const Artwork *art = nullptr;
        for (const auto &artWork : engineContext.artworks)
        {
            if (artWork.id == lookingAtArt)
            {
                art = &artWork;
                break;
            }
        }
        if (art)
        {
            float dx = (art->wx + 0.5f) - engineContext.positionX;
            float dy = (art->wy + 0.5f) - engineContext.positionY;
            distanceToArt = std::sqrt( dx * dx + dy * dy );
        }
    }

    if (!overlayBusy && lookingAtArt != -1 && engineContext.placardOpen == false && engineContext.journalOpen == false && distanceToArt < 2.5f)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 50, (RENDER_H / 2) + 5, "[E] To View", rgb( 220, 220, 220 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.inRangeOfStatue && !engineContext.statueChatActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 70, (RENDER_H / 2) + 25, "[E] To Talk", rgb( 220, 220, 220 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.showHelp)
    {
        drawString16x16( engineContext, 10, RENDER_H - 40, "[N] Notes", rgb( 200, 200, 120 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.currentLevel == Levels::CAVE && !g_caveQuizPassed && !g_caveQuizActive)
    {
        drawStringTinyScaled( engineContext, 12, RENDER_H - 20, "Logs Hold Clues", rgb( 170, 180, 210 ), 1, 1, 1, false );
    }

    renderRevolverShotEffects( engineContext, shotFx01 );
    renderSchoolSafeWeaponBlur( engineContext );

    drawTranslucentBox(
        engineContext,
        0,
        0,
        RENDER_W,
        RENDER_H,
        rgb( 0, 0, 0 ),
        std::clamp( g_horrorDarknessOverlay + (engineContext.caveMode ? 0.05f : 0.0f) + museumDarknessAdd, 0.0f, 0.58f ) );

    if (g_wakeCutsceneActive)
    {
        const float p = std::clamp( g_wakeCutsceneTimer / std::max( 0.001f, kWakeCutsceneDuration ), 0.0f, 1.0f );
        const float eyeOpen = std::clamp( std::pow( p, 1.9f ), 0.0f, 1.0f );
        const int lidH = int( (RENDER_H * 0.5f) * (1.0f - eyeOpen) );

        if (lidH > 0)
        {
            drawTextBox( engineContext, 0, 0, RENDER_W, lidH, rgb( 0, 0, 0 ), rgb( 0, 0, 0 ) );
            drawTextBox( engineContext, 0, RENDER_H - lidH, RENDER_W, lidH, rgb( 0, 0, 0 ), rgb( 0, 0, 0 ) );
        }

        drawTranslucentBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), std::clamp( 0.55f - p * 0.55f, 0.0f, 0.55f ) );

        if (p > 0.38f && p < 0.92f)
        {
            drawStringTinyScaled( engineContext, (RENDER_W / 2) - 45, RENDER_H - 44, "...where am I?", rgb( 185, 185, 200 ), 2, 1, 1, false );
        }
    }

    drawStringTinyScaled( engineContext, 12, RENDER_H - 20, "X: " + to_string( engineContext.positionX ) + " " + "Y: " + to_string( engineContext.positionY ), rgb( 0, 0, 0 ), 1, 1, 1, false );
    {
        int fpsInt = (int)(engineContext.fps + 0.5f);
        drawStringTinyScaled( engineContext, 12, RENDER_H - 50, string( "FPS: " ) + to_string( fpsInt ), rgb( 200, 200, 200 ), 1, 1, 1, false );
    }

    int nearbyKey = getNearbyKeyPickup( engineContext );
    if (!overlayBusy && nearbyKey >= 0 && !g_codeEntryActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 95, (RENDER_H / 2) + 45, "[E] Interact", rgb( 255, 240, 140 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    int nearbyNote = getNearbyClueNote( engineContext );
    if (!overlayBusy && nearbyNote >= 0 && !g_codeEntryActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 95, (RENDER_H / 2) + 85, "[E] Collect", rgb( 220, 225, 180 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM && isPlayerNearGasCan( engineContext ))
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 122, (RENDER_H / 2) + 125, "[E] Pick Up Gas Can", rgb( 235, 215, 140 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM && isPlayerNearGenerator( engineContext ))
    {
        std::string prompt = "[E] Check Generator";
        if (g_generatorFueled)
        {
            prompt = "Generator Running";
        }
        else if (g_gasCanCollected)
        {
            prompt = "[E] Fill Generator";
        }
        drawString16x16( engineContext, (RENDER_W / 2) - 122, (RENDER_H / 2) + 145, prompt, rgb( 255, 240, 170 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    int nearbySafe = getNearbySafe( engineContext );
    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM && nearbySafe >= 0 && !g_codeEntryActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 105, (RENDER_H / 2) + 65, "[F] Open Safe", rgb( 180, 210, 255 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    int nearbySymbol = getNearbySymbol( engineContext );
    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM && nearbySymbol >= 0 && !g_codeEntryActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 105, (RENDER_H / 2) + 65, "[F] Examine Pedestal", rgb( 250, 180, 250 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && isPlayerNearDirectorDesk( engineContext ) && !g_directorDeskUnlocked)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 150, (RENDER_H / 2) + 125, "[F] Unlock Director's Desk", rgb( 230, 200, 150 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.currentLevel == Levels::CAVE && isPlayerNearCaveStatue( engineContext ) && !g_caveQuizActive)
    {
        std::string statuePrompt = g_caveQuizPassed ? "WARDEN: PATH OPEN" : "[E] ANSWER WARDEN QUESTIONS";
        drawString16x16( engineContext, (RENDER_W / 2) - 145, (RENDER_H / 2) + 105, statuePrompt, rgb( 210, 220, 255 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM && isPlayerNearPoint( engineContext, kUpperEntryX, kUpperEntryY, kUpperEntryRadius ))
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 140, (RENDER_H / 2) + 105, "[E] Go To Upper Gallery", rgb( 205, 220, 255 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }
    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM_UPPER && isPlayerNearPoint( engineContext, 3.5f, 9.3f, 1.1f ))
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 145, (RENDER_H / 2) + 105, "[E] Back To Ground Floor", rgb( 205, 220, 255 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }
    if (!overlayBusy && !g_mindTrapTriggerConsumed && isPlayerNearMindTrapTrigger( engineContext ))
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 190, (RENDER_H / 2) + 145, "[E] Enter Solvent Diagnostic", rgb( 120, 235, 140 ), RENDER_W, 1, 2, true, rgb( 10, 25, 10 ) );
    }

    int doorTx = 0, doorTy = 0;
    if (isMuseumLikeLevel( engineContext.currentLevel ) && getDoorAheadTile( engineContext, doorTx, doorTy ))
    {
        int lockIndex = findDoorLockIndex( engineContext.currentLevel, doorTx, doorTy );
        if (!overlayBusy && lockIndex >= 0 && !g_roomLocks[ lockIndex ].unlocked && !g_codeEntryActive)
        {
            const auto &lock = g_roomLocks[ lockIndex ];
            std::string req = (lock.type == LockType::KEY) ? ("[F] Use " + lock.requirement) : "[F] Enter 4-Digit Code";
            drawString16x16( engineContext, (RENDER_W / 2) - 110, (RENDER_H / 2) + 65, req, rgb( 255, 210, 100 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
        }
    }
    if (engineContext.currentLevel == Levels::MUSEUM_UPPER && getDoorAheadTile( engineContext, doorTx, doorTy ) && isRestorationGateDoorTile( doorTx, doorTy ) && !g_restorationWingUnlocked)
    {
        std::string req = hasRestorationPigments()
            ? "[F] Unseal Restoration Wing"
            : "[F] Need Black, Blue, Red Pigment";
        drawString16x16( engineContext, (RENDER_W / 2) - 155, (RENDER_H / 2) + 65, req, rgb( 255, 170, 170 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    const Artwork *art = nullptr;
    if (engineContext.openArtId >= 0)
    {
        for (const auto &artWork : engineContext.artworks)
        {
            if (artWork.id == engineContext.openArtId)
            {
                art = &artWork;
                break;
            }
        }
    }

    if (!cutsceneHudSuppressed)
    {
        renderCompass( engineContext );
    }

    if (art)
    {
        renderPolishedPlacard( engineContext );
    }
    if (engineContext.statueChatActive)
    {
        renderStatueChatbox( engineContext );
    }

    if (!cutsceneHudSuppressed && (engineContext.currentLevel == Levels::MUSEUM || engineContext.currentLevel == Levels::MUSEUM_UPPER))
    {
        renderObjectives( engineContext );
        renderGalleryCard( engineContext );
    }

    renderCaveHUD( engineContext );
    renderHeldRevolver( engineContext );

    renderLevelEditorOverlay( engineContext );

    if (!overlayBusy) renderAccessPopup( engineContext );
    if (!cutsceneHudSuppressed)
    {
        renderCombatHUD( engineContext );
    }
    renderNotesScreen( engineContext );
    renderCodeEntry( engineContext );
    renderSafeEntry( engineContext );
    renderSymbolEntry( engineContext );
    renderCaveQuiz( engineContext );
    renderInteractionAnimation( engineContext );
    renderLevelTransitionOverlay( engineContext );
    renderDialogueSubtitle( engineContext );
}
static void renderMenu(
    Engine &engineContext,
    int selection,
    float volume,
    bool musicOn,
    bool viewBob,
    int antiAliasingMode,
    int modelQualityPreset,
    int gpuRenderMode,
    bool multithreadingEnabled,
    unsigned int detectedThreadCount,
    bool schoolMode ) {
    // Dimensions
    int width = 460, height = 376;
    int x = (RENDER_W - width) / 2;
    int y = (RENDER_H - height) / 2;

    // Colors
    Uint32 bgCol = rgb( 25, 25, 30 );
    Uint32 borderCol = rgb( 180, 150, 50 );
    Uint32 textCol = rgb( 160, 160, 170 );
    Uint32 selCol = rgb( 255, 230, 100 );

    drawTextBox( engineContext, x, y, width, height, bgCol, borderCol );
    drawTextBox( engineContext, x + 4, y + 4, width - 8, height - 8, bgCol, borderCol );

    // Title Scaling
    std::string title = "MICRO MUSEUM";
    int scale = 3;
    // Tiny font is 3px wide * scale + 2px spacing
    int titleW = (int)title.length() * (3 * scale + 2);
    int titleX = x + (width - titleW) / 2;
    int titleY = y + 25;

    drawStringTinyScaled( engineContext, titleX, titleY, title, borderCol, scale, 2, 2, true );

    // Subtitle
    std::string sub = "INTERACTIVE GALLERY";
    int subX = x + (width - (int)sub.length() * 6) / 2;
    drawStringTinyScaled( engineContext, subX, titleY + 25, sub, textCol, 1, 3, 1, false );


    int optY = y + 86;
    int lineH = 24;

    bool showCursor = (SDL_GetTicks() / 350) % 2 == 0;

    auto drawItem = [&]( int index, std::string label ) {
        bool isSel = (selection == index);
        Uint32 col = isSel ? selCol : textCol;

        std::string prefix = (isSel && showCursor) ? "> " : "  ";
        std::string suffix = (isSel && showCursor) ? " <" : "  ";
        std::string fullText = prefix + label + suffix;

    
        int charAdv = 12;

        int textW = (int)fullText.length() * charAdv;
        int textX = x + (width - textW) / 2;

        drawString16x16( engineContext, textX, optY + (index * lineH), fullText, col, width, 1, 2, true, rgb( 10, 10, 10 ) );
        };

    drawItem( 0, "Play" );

    std::string musicState = musicOn ? "ON" : "OFF";
    drawItem( 1, "Enable Audio: " + musicState );

    std::string volStr;
	static bool resetToDetect = false;
    if ((int)volume > 0 && volume != config::calibratedVolume) {
        volStr = std::to_string((int)volume) + "%";
    }
    else {
		volStr = "DETECT";
    }


    drawItem( 2, "Music Volume: " + volStr );

    std::string viewBobEnabler = viewBob ? "ON" : "OFF";
    drawItem( 3, "View Bobbing: " + viewBobEnabler );

    std::string antiAliasingLabel = "LINEAR";
    if (antiAliasingMode == 0) antiAliasingLabel = "OFF";
    else if (antiAliasingMode == 2) antiAliasingLabel = "FXAA";
    drawItem( 4, "Anti-Aliasing: " + antiAliasingLabel );

    std::string quality = "BALANCED";
    if (modelQualityPreset == 0) quality = "HIGH";
    else if (modelQualityPreset == 2) quality = "PERFORMANCE";
    drawItem( 5, "Model Quality: " + quality );

    std::string gpuMode = "SMART";
    if (gpuRenderMode == 0) gpuMode = "NONE";
    else if (gpuRenderMode == 2) gpuMode = "FULL";
    drawItem( 6, "GPU Rendering: " + gpuMode );

    drawItem( 7, std::string( "Multithreading: " ) + (multithreadingEnabled ? "ON" : "OFF") + " (" + std::to_string( std::max( 1u, detectedThreadCount ) ) + " Threads)" );

    drawItem( 8, std::string( "School Mode: " ) + (schoolMode ? "ON" : "OFF") );

    drawItem( 9, "Quit" );

    std::string footer = "UP/DOWN Select    ENTER Confirm";
    int footW = (int)footer.length() * 4;
    drawStringTinyScaled( engineContext, x + (width - footW) / 2, y + height - 20, footer, rgb( 80, 80, 90 ), 1, 1, 1, false );
}

static void applyPresentationFilter( Engine &engineContext ) {
    if (!engineContext.backtexure) return;
    SDL_ScaleMode mode = (getAntiAliasingMode() == 0) ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR;
    (void)SDL_SetTextureScaleMode( engineContext.backtexure, mode );
}

static void applyPostAAMode( Engine &engineContext ) {
    if (getAntiAliasingMode() != 2) return;
    if (engineContext.backbuffer.size() != size_t( RENDER_W * RENDER_H )) return;

    static std::vector<Uint32> scratch;
    scratch.resize( engineContext.backbuffer.size() );
    scratch = engineContext.backbuffer;

    auto lum = []( Uint32 c ) -> float {
        const float r = float( (c >> 16) & 255 );
        const float g = float( (c >> 8) & 255 );
        const float b = float( c & 255 );
        return 0.299f * r + 0.587f * g + 0.114f * b;
    };

    for (int y = 1; y < RENDER_H - 1; ++y)
    {
        for (int x = 1; x < RENDER_W - 1; ++x)
        {
            const int i = y * RENDER_W + x;
            const Uint32 c = scratch[ i ];

            const float gx = std::fabs( lum( scratch[ i + 1 ] ) - lum( scratch[ i - 1 ] ) );
            const float gy = std::fabs( lum( scratch[ i + RENDER_W ] ) - lum( scratch[ i - RENDER_W ] ) );
            const float edge = gx + gy;
            if (edge < 26.0f) continue;

            float sumR = 0.0f, sumG = 0.0f, sumB = 0.0f;
            for (int oy = -1; oy <= 1; ++oy)
            {
                for (int ox = -1; ox <= 1; ++ox)
                {
                    const Uint32 s = scratch[ (y + oy) * RENDER_W + (x + ox) ];
                    sumR += float( (s >> 16) & 255 );
                    sumG += float( (s >> 8) & 255 );
                    sumB += float( s & 255 );
                }
            }

            const float avgR = sumR / 9.0f;
            const float avgG = sumG / 9.0f;
            const float avgB = sumB / 9.0f;

            const float blend = std::clamp( (edge - 26.0f) / 84.0f, 0.0f, 0.45f );
            const float srcR = float( (c >> 16) & 255 );
            const float srcG = float( (c >> 8) & 255 );
            const float srcB = float( c & 255 );

            const Uint8 outR = Uint8( std::clamp( srcR * (1.0f - blend) + avgR * blend, 0.0f, 255.0f ) );
            const Uint8 outG = Uint8( std::clamp( srcG * (1.0f - blend) + avgG * blend, 0.0f, 255.0f ) );
            const Uint8 outB = Uint8( std::clamp( srcB * (1.0f - blend) + avgB * blend, 0.0f, 255.0f ) );
            engineContext.backbuffer[ i ] = rgb( outR, outG, outB );
        }
    }
}

static void renderModernCrosshairOverlay( Engine &engineContext ) {
    if (!engineContext.renderer) return;

    const int screenW = RENDER_W * WIN_SCALE;
    const int screenH = RENDER_H * WIN_SCALE;
    const int cx = screenW / 2;
    const int cy = screenH / 2;
    const int dotSize = 4;
    const int dotHalf = dotSize / 2;

    SDL_SetRenderDrawBlendMode( engineContext.renderer, SDL_BLENDMODE_BLEND );

    SDL_SetRenderDrawColor( engineContext.renderer, 0, 0, 0, 150 );
    SDL_FRect borderRect{
        float( cx - dotHalf - 1 ),
        float( cy - dotHalf - 1 ),
        float( dotSize + 2 ),
        float( dotSize + 2 )
    };
    SDL_RenderFillRect( engineContext.renderer, &borderRect );

    SDL_SetRenderDrawColor( engineContext.renderer, 255, 255, 255, 200 );
    SDL_FRect dotRect{
        float( cx - dotHalf ),
        float( cy - dotHalf ),
        float( dotSize ),
        float( dotSize )
    };
    SDL_RenderFillRect( engineContext.renderer, &dotRect );
}

static void renderModernRevolverHudOverlay( Engine &engineContext ) {
    if (!engineContext.renderer || !g_combatState.active || !g_combatState.hasRevolver) return;

    SDL_SetRenderDrawBlendMode( engineContext.renderer, SDL_BLENDMODE_BLEND );

    const float centerX = float( RENDER_W * WIN_SCALE - 60 );
    const float centerY = float( RENDER_H * WIN_SCALE - 60 );
    const float ringRadius = 18.0f;
    const float chamberSize = 7.0f;
    const int loaded = std::clamp( g_combatState.loadedAmmo, 0, 6 );
    constexpr float kTau = 6.28318530718f;

    for (int i = 0; i < 6; ++i)
    {
        const float angle = (-kTau * 0.25f) + (kTau * (float)i / 6.0f);
        const float px = centerX + std::cos( angle ) * ringRadius;
        const float py = centerY + std::sin( angle ) * ringRadius;

        SDL_FRect chamberRect{
            px - (chamberSize * 0.5f),
            py - (chamberSize * 0.5f),
            chamberSize,
            chamberSize
        };

        if (i < loaded)
        {
            SDL_SetRenderDrawColor( engineContext.renderer, 255, 220, 100, 255 );
            SDL_RenderFillRect( engineContext.renderer, &chamberRect );
        }
        else
        {
            SDL_SetRenderDrawColor( engineContext.renderer, 50, 50, 50, 100 );
            SDL_RenderFillRect( engineContext.renderer, &chamberRect );
        }

        SDL_SetRenderDrawColor( engineContext.renderer, 25, 25, 25, i < loaded ? 190 : 135 );
        SDL_RenderRect( engineContext.renderer, &chamberRect );
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
    engineContext.window = SDL_CreateWindow( "Micro Museum", RENDER_W * WIN_SCALE, RENDER_H * WIN_SCALE, 0 );
    SDL_SetWindowPosition( engineContext.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED );

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
    {"Museum Ground", (assetRoot / "levels" / "museum").string(), "map.txt", 10.0f, 9.0f, 90.f, Levels::MUSEUM, rgb( 255, 242, 220 ), 0.72f, true, "Explore both floors and report to the statue"},
    {"Museum Upper", (assetRoot / "levels" / "museum_upper").string(), "map.txt", 3.8f, 9.3f, 0.f, Levels::MUSEUM_UPPER, rgb( 205, 225, 255 ), 0.66f, true, "Explore both floors and report to the statue"},
    {"Transition", (assetRoot / "levels" / "transition").string(), "map.txt", 1.5f, 4.5f, 270.f, Levels::TRANSITION, rgb( 235, 235, 235 ), 0.62f, false, "Proceed through the tunnels"},
    {"Cave", (assetRoot / "levels" / "cave").string(), "map.txt", 2.5f, 2.5f, 90.0f, Levels::CAVE, rgb( 200, 215, 255 ), 0.58f, false, "Find the final journal fragment"}


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
        if (currentState == STATE_GAME) 
        {
            g_runElapsedSeconds += dt;
            if (g_caveTimerActive && !g_caveQuizPassed)
            {
                g_caveTimerSeconds -= dt;
                if (g_caveTimerSeconds <= 0.0f)
                {
                    g_caveTimerActive = false;
                    showAccessPopup( "Oxygen depleted. Restarting from the museum.", 4000 );
                    startNewMuseumRun( engineContext, levels );
                }
            }
        }
        // Input
        SDL_Event ev;
        float actualSpeed = MOVE_SPEED;

        static float walkTime = 0.f;
        bool stepTriggered = false;
        if (!engineContext.isMoving)
        {
            engineContext.pitchOffset = engineContext.pitchOffset * 0.9f;
            if (std::abs( engineContext.pitchOffset ) < 0.5f) engineContext.pitchOffset = 0.0f;
        }
        else
        {
            walkTime += dt * MOVE_SPEED * 5.0f;
            engineContext.isMoving = false; 

            float sinValue = std::sin( walkTime );

            if (config::viewBobbing)
            {
                engineContext.pitchOffset = sinValue * 3.0f;
            }

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
                    playWeaponBufferedSound( g_shellDropBuffer, 95.0f );
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
            const float p = std::clamp( g_wakeCutsceneTimer / std::max( 0.001f, kWakeCutsceneDuration ), 0.0f, 1.0f );
            const float openEase = 1.0f - std::pow( 1.0f - p, 2.2f );
            engineContext.pitchOffset = (1.0f - openEase) * 78.0f + std::sin( SDL_GetTicks() * 0.004f ) * 0.6f;
            if (g_wakeCutsceneTimer >= kWakeCutsceneDuration)
            {
                g_wakeCutsceneActive = false;
                g_wakeCutsceneTimer = 0.0f;
                g_cutsceneController.reset();
                engineContext.pitchOffset = 0.0f;
            }
        }

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

                if (g_levelEditorMode && ev.type == SDL_EVENT_MOUSE_WHEEL)
                {
                    if (g_editorSelectedModel >= 0 && g_editorSelectedModel < (int)g_worldModels.size() && g_worldModels[ g_editorSelectedModel ].editorPlaced)
                    {
                        constexpr float kMouseWheelZStep = 0.05f;
                        nudgeSelectedEditorModel( 0.0f, 0.0f, (float)ev.wheel.y * kMouseWheelZStep, 0.0f, 0.0f, 0.0f, 0.0f );
                    }
                    continue;
                }

                if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
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

                    if (ev.button.button == SDL_BUTTON_LEFT)
                    {
                        if (!inputBlocked && g_combatState.active && g_combatState.hasRevolver)
                        {
                            if (g_revolverShotCooldown <= 0.0f)
                            {
                                if (g_combatState.loadedAmmo > 0)
                                {
                                    g_combatState.loadedAmmo--;
                                    g_revolverShotCooldown = 0.28f;
                                    g_revolverRecoilTimer = kRevolverRecoilDuration;
                                    playRevolverShotSequence( levels[ engineContext.currentLevel ].folder );
                                }
                              
                            }
                        }
                        continue;
                    }
                }

                if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP)
                {
                    if (ev.button.button == SDL_BUTTON_RIGHT)
                    {
                        g_revolverAiming = false;
                        continue;
                    }
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
                        showAccessPopup( "Door override enabled. All doors unlocked.", 2200 );
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
                                        showAccessPopup( "Correct. Next question.", 1200 );
                                    }
                                }
                                else
                                {
                                    playFailedDoorOpen( levels[ engineContext.currentLevel ].folder );
                                    showAccessPopup( "Incorrect. Check cave notes in the camp area.", 2200 );
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
                                    showAccessPopup( sym.name + " solved! Obtained " + sym.rewardKey, 2800 );
                                    g_playerKeys.insert( sym.rewardKey );
                                    triggerInteractionAnim( InteractionAnimType::ITEM_PICKUP, "ACQUIRED " + sym.rewardKey, 0.8f );
                                    g_dialogue.start( {
                                        {"How do people get these keys normally?", 2.8f}
                                        } );
                                }
                                else
                                {
                                    showAccessPopup( "Pedestal mechanism is jammed." );
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
                                        showAccessPopup( "Wrong code. Access denied." );
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
                    else if (ev.key.key == SDLK_H)
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
                        if (!g_mindTrapTriggerConsumed && isPlayerNearMindTrapTrigger( engineContext ))
                        {
                            startMindTrapSequence( engineContext );
                            currentState = STATE_MIND_TRAP;
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
                                showAccessPopup( "Generator is humming.", 1200 );
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
                                    showAccessPopup( "Generator is empty. Need gas can.", 1800 );
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

                            showAccessPopup( "Generator fueled. Power restored.", 2500 );
                            triggerInteractionAnim( InteractionAnimType::KEY_USE, "RESTORING POWER", 0.85f );
                            g_dialogue.start( {
                                {"That should get the lights back on", 2.1f}
                                } );
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
                            hideWorldModelsNear( k.x, k.y );
                            showAccessPopup( "Acquired " + k.keyName + ".", 1800 );
                            playPickup(levels[engineContext.currentLevel].folder);
                            triggerInteractionAnim( InteractionAnimType::ITEM_PICKUP, "ACQUIRED " + k.keyName, 0.50f );
                            continue;
                        }

                        if (isRevolverNearby( engineContext ))
                        {
                            g_revolverPickup.collected = true;
                            if (g_revolverPickup.modelIndex >= 0 && g_revolverPickup.modelIndex < (int)g_worldModels.size())
                            {
                                g_worldModels[ g_revolverPickup.modelIndex ].visible = false;
                            }
                            hideWorldModelsNear( g_revolverPickup.x, g_revolverPickup.y, 0.9f );
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
                            hideWorldModelsNear( n.x, n.y );
                            showAccessPopup( "Collected note: " + n.title, 2200 );
							playPaperRustle( levels[ engineContext.currentLevel ].folder );
                            triggerInteractionAnim( InteractionAnimType::NOTE_COLLECT, "READING NOTE", 0.5f );

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
                                        {"Why would the director have this?", 2.5f}
                                        } );
                                    showAccessPopup( "Director's Desk unlocked. Revolver acquired.", 1900 );
                                    triggerInteractionAnim( InteractionAnimType::ITEM_PICKUP, "REVOLVER ACQUIRED", 0.6f );
                                }
                                else
                                {
                                    showAccessPopup( "Director's Desk already searched.", 1800 );
                                }
                            }
                            else
                            {
                                showAccessPopup( "The desk is locked. Director's Key required.", 2200 );
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
                                showAccessPopup( "Taxidermy seal broken. Restoration Wing unlocked.", 2400 );
                                triggerInteractionAnim( InteractionAnimType::KEY_USE, "RESTORATION WING UNSEALED", 1.0f );
                            }
                            else
                            {
                                showAccessPopup( "Seal active. Gather Black, Blue, and Red Pigment.", 2300 );
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

   
                        if (toggled && engineContext.currentLevel == Levels::TRANSITION)
                        {
                            handleLevelChange( engineContext, levels, Levels::CAVE );
                        }
                    }
                    else if (ev.key.scancode == SDL_SCANCODE_P)
                    {
                        float2 pos( engineContext.positionX, engineContext.positionY );
                        placePlant( engineContext, pos, levels[ curLevel ].folder + "/plant.bmp" );
                    }
                    else if (ev.key.scancode == SDL_SCANCODE_R)
                    {
                        float2 pos( engineContext.positionX, engineContext.positionY );
                        placeRope( engineContext, pos, levels[ curLevel ].folder + "/rope.bmp" );
                    }
                    else if (ev.key.scancode == SDL_SCANCODE_T)
                    {
                        float2 pos( engineContext.positionX, engineContext.positionY );
                        placeStatue( engineContext, pos, levels[ curLevel ].folder + "/statue.bmp" );
                    }
                    else if (ev.key.scancode == SDL_SCANCODE_V)
                    {
                        float2 pos( engineContext.positionX, engineContext.positionY );
                        placeVase( engineContext, pos, levels[ curLevel ].folder );
                    }
                    else if (ev.key.scancode == SDL_SCANCODE_C)
                    {
                        float2 pos( engineContext.positionX, engineContext.positionY );
                        placeCan( engineContext, pos, levels[ curLevel ].folder + "/trashcan.bmp" );
                    }
                    else if (ev.key.scancode == SDL_SCANCODE_O)
                    {
                        saveProps( (levels[ curLevel ].folder + "/props.txt"),
                            engineContext.props, engineContext.propImages, engineContext.quads );
                    }
                    else if (ev.key.scancode == SDL_SCANCODE_K)
                    {
                        handleLevelChange( engineContext, levels, Levels::MUSEUM );
                    }
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
                    if ((ev.key.key == SDLK_UP || ev.key.key == SDLK_W) && g_mindTrapAwaitingChoice)
                    {
                        g_mindTrapSelectedOption = (g_mindTrapSelectedOption + 2) % 3;
                    }
                    else if ((ev.key.key == SDLK_DOWN || ev.key.key == SDLK_S) && g_mindTrapAwaitingChoice)
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
                        queueMindTrapTerminalLine( "EXIT COMMAND DENIED. COMPLETE THE SYNC." );
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
            float ts = TURN_SPEED * dt;
            if (g_cutsceneController.isCameraLockActive() || g_revolverInspectCutsceneActive || g_wakeCutsceneActive) ts = 0.0f;
            if (ks[ SDL_SCANCODE_LEFT ])
            {
                float ang = -ts;
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
            }
            if (ks[ SDL_SCANCODE_RIGHT ])
            {
                float ang = ts;
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
                engineContext.planeY = engineContext.directionX * FOV_TAN;
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
                nx += engineContext.directionY * ms;
                ny += -engineContext.directionX * ms;
                engineContext.isMoving = true;
            }
            if (ks[ SDL_SCANCODE_D ])
            {
                nx += -engineContext.directionY * ms;
                ny += engineContext.directionX * ms;
                engineContext.isMoving = true;
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
                    if (mesuemObjectives.allCompleted() == true)
                    {
                        handleLevelChange( engineContext, levels, Levels::TRANSITION );
                    }
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
        if (currentState == STATE_MIND_TRAP)
        {
            drawTextBox( engineContext, 0, 0, RENDER_W, RENDER_H, rgb( 0, 0, 0 ), rgb( 0, 0, 0 ) );
        }
        else
        {
            render( engineContext, dt );
            applyPostAAMode( engineContext );
        }

        if (currentState == STATE_GAME)
        {
            renderGameplayUiPass( engineContext );
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
            renderEndingScreen( engineContext );
        }
        else if (currentState == STATE_MIND_TRAP)
        {
            renderMindTrapInterface( engineContext );
        }
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
