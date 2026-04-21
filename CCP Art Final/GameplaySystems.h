#pragma once

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

static std::shared_ptr<sf::Sound> playDirectionalBufferedSound(
    const sf::SoundBuffer &buffer,
    float worldX,
    float worldY,
    float volume,
    float minDistance,
    float attenuation,
    bool looped = false ) {
    auto snd = std::make_shared<sf::Sound>( buffer );
    snd->setSpatializationEnabled( true );
    snd->setRelativeToListener( false );
    snd->setPosition( sf::Vector3f( worldX, worldY, 0.0f ) );
    snd->setMinDistance( std::max( 0.1f, minDistance ) );
    snd->setAttenuation( std::max( 0.0f, attenuation ) );
    snd->setLooping( looped );
    snd->setVolume( std::clamp( volume, 0.0f, 100.0f ) );
    snd->play();
    g_activeWeaponSounds.push_back( snd );
    return snd;
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

#include "EditorModelTools.h"

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

#include "ModelLoadingTools.h"

static float g_caveTimerSeconds = 120.0f;
static bool g_caveTimerActive = false;
static constexpr float kUpperEntryX = 20.6f;
static constexpr float kUpperEntryY = 9.3f;
static constexpr float kUpperEntryRadius = 0.85f;

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
        g_meshTriangleStride = 2;
        g_meshRasterStep = 3;
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

using Player = Engine;

static void updateAudioOcclusion( Player &p, float sourceX, float sourceY ) {
    static Uint32 lastTick = SDL_GetTicks();
    const Uint32 nowTick = SDL_GetTicks();
    float dt = (nowTick - lastTick) * (1.0f / 1000.0f);
    lastTick = nowTick;
    dt = std::clamp( dt, 0.0f, 0.05f );

    if (p.map.width <= 0 || p.map.height <= 0 || p.map.tiles.empty())
    {
        updateMusicOcclusion( false, dt );
        return;
    }

    const float dx = sourceX - p.positionX;
    const float dy = sourceY - p.positionY;
    const float sourceDistance = std::sqrt( dx * dx + dy * dy );
    if (sourceDistance <= 0.0001f)
    {
        updateMusicOcclusion( false, dt );
        return;
    }

    const float rayDirX = dx / sourceDistance;
    const float rayDirY = dy / sourceDistance;

    int mapX = int( p.positionX );
    int mapY = int( p.positionY );

    const float deltaDistX = (std::fabs( rayDirX ) < 1e-6f) ? 1e30f : std::fabs( 1.0f / rayDirX );
    const float deltaDistY = (std::fabs( rayDirY ) < 1e-6f) ? 1e30f : std::fabs( 1.0f / rayDirY );

    int stepX = 0;
    int stepY = 0;
    float sideDistX = 0.0f;
    float sideDistY = 0.0f;

    if (rayDirX < 0.0f)
    {
        stepX = -1;
        sideDistX = (p.positionX - float( mapX )) * deltaDistX;
    }
    else
    {
        stepX = 1;
        sideDistX = (float( mapX ) + 1.0f - p.positionX) * deltaDistX;
    }

    if (rayDirY < 0.0f)
    {
        stepY = -1;
        sideDistY = (p.positionY - float( mapY )) * deltaDistY;
    }
    else
    {
        stepY = 1;
        sideDistY = (float( mapY ) + 1.0f - p.positionY) * deltaDistY;
    }

    bool blocked = false;
    const int maxSteps = std::max( 8, p.map.width * p.map.height );
    for (int step = 0; step < maxSteps; ++step)
    {
        float travelDist = 0.0f;

        if (sideDistX < sideDistY)
        {
            travelDist = sideDistX;
            sideDistX += deltaDistX;
            mapX += stepX;
        }
        else
        {
            travelDist = sideDistY;
            sideDistY += deltaDistY;
            mapY += stepY;
        }

        if (travelDist >= sourceDistance)
        {
            break;
        }

        if (mapX < 0 || mapY < 0 || mapX >= p.map.width || mapY >= p.map.height)
        {
            break;
        }

        const int tile = p.map.tiles[ mapY * p.map.width + mapX ];
        if (tile == 1 || tile == 2)
        {
            blocked = true;
            break;
        }
    }

    updateMusicOcclusion( blocked, dt );
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

static bool isPlayerNearSolventCooler( Engine const &engineContext ) {
    if (engineContext.currentLevel != Levels::MUSEUM_UPPER) return false;
    return isPlayerNearPoint( engineContext, 11.3f, 16.05f, 1.15f );
}

static std::string normalizePhraseInput( const std::string &input ) {
    std::string out;
    out.reserve( input.size() );

    bool lastWasSpace = true;
    for (char ch : input)
    {
        char c = char( std::toupper( unsigned char( ch ) ) );
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '\'')
        {
            out.push_back( c );
            lastWasSpace = false;
        }
        else if (!lastWasSpace)
        {
            out.push_back( ' ' );
            lastWasSpace = true;
        }
    }

    while (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

static bool isSolventCoolerPhraseCorrect( const std::string &input ) {
    return normalizePhraseInput( input ) == "ONE BREATH HELD FOREVER";
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

