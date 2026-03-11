#include "GameEngine.h"
#include "RendererHelpers.h"
#include "PhysicsHelpers.h"
#include "MusicSystem.h"
#include <iostream>
#include <filesystem> 
#include <thread>

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
};

struct KeyPickup
{
    std::string keyName;
    float x = 0.f;
    float y = 0.f;
    bool collected = false;
    int propIndex = -1;
};

struct ClueNote
{
    std::string title;
    std::string body;
    float x = 0.f;
    float y = 0.f;
    bool collected = false;
    int propIndex = -1;
};

static std::vector<RoomLock> g_roomLocks;
static std::vector<KeyPickup> g_keyPickups;
static std::vector<ClueNote> g_clueNotes;
static std::unordered_set<std::string> g_playerKeys;
static std::vector<int> g_foundNotes;

static std::string g_accessPopup;
static Uint32 g_accessPopupUntil = 0;

static bool g_codeEntryActive = false;
static int g_codeEntryLockIndex = -1;
static std::string g_codeEntryBuffer;
static bool g_notesOpen = false;
static bool g_caveFinalNoteCollected = false;
static int g_notesCollectedRun = 0;
static float g_runElapsedSeconds = 0.0f;

struct LevelDef
{
    string name;
    string folder;
    float spawnX = 2.0f, spawnY = 9.5f, spawnDirDeg = 0.f;
    int levelId = 0;
};

enum GameState
{
    STATE_MENU,
    STATE_GAME,
    STATE_ENDING
};

static void showAccessPopup( const std::string &msg, Uint32 durationMs = 2200 ) {
    g_accessPopup = msg;
    g_accessPopupUntil = SDL_GetTicks() + durationMs;
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

static void initMuseumPuzzle( Engine &engineContext ) {
    g_roomLocks = {
        {6, 9, "West Wing", LockType::KEY, "BRONZE KEY", false},
        {10, 6, "North Wing", LockType::CODE, "0300", false},
        {16, 9, "East Wing", LockType::KEY, "SILVER KEY", false},
        {10, 12, "South Wing", LockType::CODE, "1642", false}
    };

    g_playerKeys.clear();
    g_foundNotes.clear();
    g_accessPopup.clear();
    g_accessPopupUntil = 0;
    g_codeEntryActive = false;
    g_codeEntryLockIndex = -1;
    g_codeEntryBuffer.clear();
    g_notesOpen = false;
    g_caveFinalNoteCollected = false;

    g_keyPickups.clear();
    g_keyPickups.push_back( {"BRONZE KEY", 12.6f, 9.6f, false, addKeyPickupSprite( engineContext, 12.6f, 9.6f, "BRONZE KEY", rgb( 180, 120, 40 ) )} );
    g_keyPickups.push_back( {"SILVER KEY", 10.3f, 3.2f, false, addKeyPickupSprite( engineContext, 10.3f, 3.2f, "SILVER KEY", rgb( 190, 190, 205 ) )} );

    g_clueNotes.clear();
    g_clueNotes.push_back( {
        "West Wing Curator Note",
        "The Roman gallery clue points to the North lock code. The rule of four guides the access",
        4.5f, 9.1f, false, addNotePickupSprite( engineContext, 4.5f, 9.1f, "West Wing Curator Note" )
        } );
    g_clueNotes.push_back( {
        "East Wing Archivist Note",
        "The final South lock code is the year of the militia",
        18.5f, 9.0f, false, addNotePickupSprite( engineContext, 18.5f, 9.0f, "East Wing Archivist Note" )
        } );
}

static void initCaveFinalObjective( Engine &engineContext ) {
    g_clueNotes.clear();
    g_foundNotes.clear();
    g_clueNotes.push_back( {
        "Last Journal Fragment",
        "You are beneath the museum in buried foundation tunnels.\nThe gallery was built over a much older site.",
        8.5f, 5.2f, false, addNotePickupSprite( engineContext, 8.5f, 5.2f, "Last Journal Fragment" )
        } );
    g_caveFinalNoteCollected = false;
}

static void clearPuzzleState() {
    g_roomLocks.clear();
    g_keyPickups.clear();
    g_clueNotes.clear();
    g_playerKeys.clear();
    g_foundNotes.clear();
    g_accessPopup.clear();
    g_accessPopupUntil = 0;
    g_codeEntryActive = false;
    g_codeEntryLockIndex = -1;
    g_codeEntryBuffer.clear();
    g_notesOpen = false;
}

static int findDoorLockIndex( int tx, int ty ) {
    for (int i = 0; i < (int)g_roomLocks.size(); ++i)
    {
        if (g_roomLocks[ i ].tx == tx && g_roomLocks[ i ].ty == ty) return i;
    }
    return -1;
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
        float dx = engineContext.positionX - n.x;
        float dy = engineContext.positionY - n.y;
        if (dx * dx + dy * dy <= radiusSq) return i;
    }
    return -1;
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


    fs::path folder = level.folder;
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
  

    // Map (1=wall, D=door)
    if (!loadMap( (folder / "map.txt").string(), engineContext.map )) return false;

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


    if (level.levelId == Levels::MUSEUM)
    {

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
        initMuseumPuzzle( engineContext );
    }
    else
    {
        clearPuzzleState();
        if (level.levelId == Levels::CAVE)
        {
            initCaveFinalObjective( engineContext );
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
            engineContext.lightRadius = 2.0f;
            engineContext.lightFalloff = 2.0f;
            engineContext.caveAmbient = 0.06f;

            tryLoad( folder / "floor_cracks.bmp", engineContext.floorOverlayCracks, engineContext.hasFloorCracks );
            //tryLoad( folder / "floor_stains.bmp", engineContext.floorOverlayStains, engineContext.hasFloorStains );
            tryLoad( folder / "floor_puddles.bmp", engineContext.floorOverlayPuddles, engineContext.hasFloorPuddles );

            tryLoad( folder / "wall_cracks.bmp", engineContext.wallOverlayCracks, engineContext.hasWallCracks );
            //tryLoad( folder / "wall_stain.bmp", engineContext.wallOverlayStains, engineContext.hasWallStains )
        }

        if (level.levelId == Levels::TRANSITION)
        {
            engineContext.lightRadius = 1.2f;
            engineContext.lightFalloff = 1.5f;
            engineContext.caveAmbient = 0.03f;
        }
    }

    // Spawn & camera
    engineContext.positionX = level.spawnX;
    engineContext.positionY = level.spawnY;
    float art = level.spawnDirDeg * 3.14159265f / 180.f;
    engineContext.directionX = std::cos( art );
    engineContext.directionY = std::sin( art );
    engineContext.planeX = -engineContext.directionY * FOV_TAN;
    engineContext.planeY = engineContext.directionX * FOV_TAN;
    engineContext.yaw = level.spawnDirDeg;


    // Init objectives
    mesuemObjectives.setMainObjective( "View All Artworks" );

    // Dynamically set the total to find based on the loaded artworks
    mesuemObjectives.totalArtworksToFind = engineContext.artworks.size();

    mesuemObjectives.viewedArtworks.clear();

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

    std::string wingName = "Central Atrium";
    std::string wingDesc = "Hub & Information";

    // Detect wings based on the 23x19 grid layout
    // North Wing: Y < 7, X between 7 and 15
    if (py < 7.0f && px >= 7.0f && px <= 15.0f)
    {
        wingName = "North Wing";
        wingDesc = "Baroque & Dutch Golden Age";
    }
    // South Wing: Y > 12, X between 7 and 15
    else if (py > 12.0f && px >= 7.0f && px <= 15.0f)
    {
        wingName = "South Wing";
        wingDesc = "Prehistoric & Egyptian";
    }
    // West Wing: X < 7, Y between 7 and 12
    else if (px < 7.0f && py >= 7.0f && py <= 12.0f)
    {
        wingName = "West Wing";
        wingDesc = "Antiquity & Roman Empire";
    }
    // East Wing: X > 15, Y between 7 and 12
    else if (px > 15.0f && py >= 7.0f && py <= 12.0f)
    {
        wingName = "East Wing";
        wingDesc = "Northern Renaissance";
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

    std::string header = "GALLERY TOUR";
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
    std::string title = denied ? "ACCESS DENIED" : "LOG UPDATED";
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

static void renderNotesScreen( Engine &engineContext ) {
    if (!g_notesOpen) return;

    int panelW = RENDER_W - 120;
    int panelH = RENDER_H - 100;
    int x = (RENDER_W - panelW) / 2;
    int y = (RENDER_H - panelH) / 2;

    drawTextBox( engineContext, x, y, panelW, panelH, rgb( 14, 12, 10 ), rgb( 170, 145, 95 ) );
    drawString16x16( engineContext, x + 18, y + 16, "FIELD NOTES", rgb( 240, 210, 140 ), panelW - 36, 1, 1, false );
    drawStringTinyScaled( engineContext, x + panelW - 170, y + 22, "N/ESC CLOSE", rgb( 145, 135, 110 ), 2, 1, 1, false );

    int cy = y + 52;
    if (g_foundNotes.empty())
    {
        drawString16x16( engineContext, x + 18, cy + 12, "No clues collected yet.", rgb( 210, 210, 210 ), panelW - 36, 1, 1, false );
        return;
    }

    for (int noteIdx : g_foundNotes)
    {
        if (noteIdx < 0 || noteIdx >= (int)g_clueNotes.size()) continue;
        const auto &note = g_clueNotes[ noteIdx ];

        drawString16x16( engineContext, x + 18, cy, note.title, rgb( 255, 232, 170 ), panelW - 36, 1, 1, false );
        cy += 20;
        cy = drawWrappedText( engineContext, x + 24, cy, note.body, rgb( 220, 220, 215 ), panelW - 48 );
        cy += 14;
        if (cy > y + panelH - 30) break;
    }
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

static void render( Engine &engineContext, float dt ) {
    (void)dt;

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

    auto caveLight = [&]( float dist ) -> float {
        if (!engineContext.caveMode) return 1.0f;
        float R = engineContext.lightRadius;
        float t = std::clamp( 1.0f - std::pow( dist / std::max( 0.001f, R ), engineContext.lightFalloff ), 0.0f, 1.0f );
        return std::max( engineContext.caveAmbient, t );
        };

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

        // Column geometry
        int lineH = int( RENDER_H / std::max( perpWallDist, 1e-3f ) );

        int bob = half + (int)engineContext.pitchOffset;
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
            if (engineContext.currentLevel == Levels::MUSEUM) {
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
    int bob = half + (int)engineContext.pitchOffset; 

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

                    if (engineContext.hasFloorStains)
                    {
                        int ox = int( fx * engineContext.floorOverlayStains.width ) % engineContext.floorOverlayStains.width;
                        int oy = int( fy * engineContext.floorOverlayStains.height ) % engineContext.floorOverlayStains.height;
                        Uint32 oc = engineContext.floorOverlayStains.sample( ox, oy );
                        m *= mulFromOverlay( oc, /*strength*/0.45f, /*min*/0.80f, /*max*/1.03f, /*gamma*/1.2f );
                    }
                    if (engineContext.hasFloorCracks)
                    {
                        int ox = int( fx * engineContext.floorOverlayCracks.width ) % engineContext.floorOverlayCracks.width;
                        int oy = int( fy * engineContext.floorOverlayCracks.height ) % engineContext.floorOverlayCracks.height;
                        Uint32 oc = engineContext.floorOverlayCracks.sample( ox, oy );
                        m *= mulFromOverlay( oc, /*strength*/0.85f, /*min*/0.55f, /*max*/1.00f, /*gamma*/1.6f );
                    }
                    if (engineContext.hasFloorPuddles)
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
                        shade = 1.0f / (1.0f + 0.08f * rowDist + 0.02f * rowDist * rowDist);
                        shade = std::clamp( shade, 0.02f, 1.0f );
                    }
                    putPix( engineContext, x, y, shadeCol( color, shade ) );

                    if (rowDist < engineContext.zbuffer[ x ] && !engineContext.quadBuckets.empty())
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
                        shade = 1.0f / (1.0f + 0.08f * rowDist + 0.02f * rowDist * rowDist);
                        shade = std::clamp( shade, 0.02f, 1.0f );
                    }

                    putPix( engineContext, x, y, shadeCol( color, shade ) );
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
            render_box( engineContext, box );
            //render_legs( engineContext, box );
        }
    }



	// Props (billboarded)
    for (size_t i = 0; i < engineContext.props.size(); ++i)
    {
        const auto &prop = engineContext.props[ i ];
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

            for (int sy = cy0; sy <= cy1; ++sy)
            {
                float v = float( sy - y0 ) * invSpriteH;
                int texY = std::clamp( int( v * texture.height ), 0, texture.height - 1 );

                Uint32 color = texture.sample( texX, texY );
                if (!isNearMagenta( color, 120 ))
                {
                    putPix( engineContext, sx, sy, color );
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


    int lookingAtArt = pickArtworkUnderCrosshair( engineContext );

    // Get distance to artwork that were looking at 

	float distanceToArt = 0.0f; 
    if (lookingAtArt != -1)
    {
        const Artwork* art = nullptr;
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



    if (lookingAtArt != -1 && engineContext.placardOpen == false && engineContext.journalOpen == false && distanceToArt < 2.5)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 50, (RENDER_H / 2) + 5, "[E] To View", rgb( 220, 220, 220 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (engineContext.inRangeOfStatue && !engineContext.statueChatActive)
    {
		drawString16x16( engineContext, (RENDER_W / 2) - 70, (RENDER_H / 2) + 25, "[E] To Talk", rgb( 220, 220, 220 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (engineContext.showHelp)
    {
        drawString16x16( engineContext, 10, RENDER_H - 20, "[F] Open Door", rgb( 220, 0, 0 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
        drawString16x16( engineContext, 10, RENDER_H - 40, "[N] Notes", rgb( 200, 200, 120 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    int nearbyKey = getNearbyKeyPickup( engineContext );
    if (nearbyKey >= 0 && !g_codeEntryActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 95, (RENDER_H / 2) + 45, "[E] PICK UP KEY ITEM", rgb( 255, 240, 140 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    int nearbyNote = getNearbyClueNote( engineContext );
    if (nearbyNote >= 0 && !g_codeEntryActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 95, (RENDER_H / 2) + 85, "[E] COLLECT NOTE", rgb( 220, 225, 180 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    int doorTx = 0, doorTy = 0;
    if (engineContext.currentLevel == Levels::MUSEUM && getDoorAheadTile( engineContext, doorTx, doorTy ))
    {
        int lockIndex = findDoorLockIndex( doorTx, doorTy );
        if (lockIndex >= 0 && !g_roomLocks[ lockIndex ].unlocked && !g_codeEntryActive)
        {
            const auto &lock = g_roomLocks[ lockIndex ];
            std::string req = (lock.type == LockType::KEY) ? ("[F] Use " + lock.requirement) : "[F] Enter 4-Digit Code";
            drawString16x16( engineContext, (RENDER_W / 2) - 110, (RENDER_H / 2) + 65, req, rgb( 255, 210, 100 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
        }
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

    renderCompass(engineContext);


    if (art) 
    {
        
        renderPolishedPlacard( engineContext );

    }
    if (engineContext.statueChatActive)
    {
        renderStatueChatbox( engineContext );
    }

    if (engineContext.currentLevel == Levels::MUSEUM)
    {
        renderObjectives( engineContext );
        renderGalleryCard( engineContext ); 
    }

    renderAccessPopup( engineContext );
    renderNotesScreen( engineContext );
    renderCodeEntry( engineContext );

    
}
static void renderMenu( Engine &engineContext, int selection, float volume, bool musicOn, bool viewBob ) {
    // Dimensions
    int width = 320, height = 225;
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


    int optY = y + 80;
    int lineH = 25;

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

    std::string volStr = std::to_string( (int)volume ) + "%";
    drawItem( 2, "Music Volume: " + volStr );

    std::string viewBobEnabler = viewBob ? "ON" : "OFF";
    drawItem( 3, "View Bobbing: " + viewBobEnabler );

    drawItem( 4, "Quit" );

    std::string footer = "UP/DOWN Select    ENTER Confirm";
    int footW = (int)footer.length() * 4;
    drawStringTinyScaled( engineContext, x + (width - footW) / 2, y + height - 20, footer, rgb( 80, 80, 90 ), 1, 1, 1, false );
}

int main( int argc, char **argv ) {
    (void)argc; (void)argv;
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
    engineContext.backtexure = SDL_CreateTexture( engineContext.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, RENDER_W, RENDER_H );



    engineContext.hasFloor = true;
    engineContext.hasCeiling = true;

    std::filesystem::path cwd = std::filesystem::current_path();

    std::vector<LevelDef> levels = {
    {"Museum", (cwd / "levels" / "museum").string(), 10.0f, 9.0f, 90.f, 0},
    {"Cave", (cwd / "levels" / "cave").string(), 2.5, 2.5, 90.0f, 1 },
    {"Transition", (cwd / "levels" / "transition").string(), 1.5, 4.5, 270.f, 2 }


    };

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

	int currentMenuSelection = 0; // 0=Play, 1=Music, 2=Volume, 3=viewbobbing, 4=quit
    const int numMenuOptions = 5;
    float musicVolume = getMusicVolume(); 
    g_notesCollectedRun = 0;
    g_runElapsedSeconds = 0.0f;
  


    // Main loop
    bool running = true; 
    Uint32 prev = SDL_GetTicks();
    while (running)
    {

        Uint32 now = SDL_GetTicks();
        float dt = (now - prev) / 1000.0f;
        prev = now;
        if (dt > 0.05f) dt = 0.05f;
        if (currentState == STATE_GAME) g_runElapsedSeconds += dt;
        // Input
        SDL_Event ev;
        float actualSpeed;

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
                        std::cout << "Footstep Triggered at peak/trough: " << sinValue << "\n";
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

        while (SDL_PollEvent( &ev ))
        {
            actualSpeed = MOVE_SPEED;
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
                            currentState = STATE_GAME;
                        }
						else if (currentMenuSelection == 4) // "Quit"
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
                        break;
                    }
                }
            }
            else if (currentState == STATE_GAME)
            {
                if (ev.type == SDL_EVENT_KEY_DOWN)
                {
                    if (g_notesOpen)
                    {
                        if (ev.key.key == SDLK_N || ev.key.key == SDLK_ESCAPE)
                        {
                            g_notesOpen = false;
                        }
                        continue;
                    }

                    if (g_codeEntryActive)
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
                        }
                        else if ((ev.key.key == SDLK_RETURN || ev.key.key == SDLK_KP_ENTER) && g_codeEntryLockIndex >= 0 && g_codeEntryLockIndex < (int)g_roomLocks.size())
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
                            }
                            else
                            {
                                showAccessPopup( "Wrong code. Access denied." );
                            }
                            g_codeEntryActive = false;
                            g_codeEntryBuffer.clear();
                            g_codeEntryLockIndex = -1;
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
                    }
                    else if (ev.key.key == SDLK_F1)
                    {
                        engineContext.showHelp = !engineContext.showHelp;
                    }
                    else if (ev.key.scancode == SDL_SCANCODE_E)
                    {
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
                            showAccessPopup( "Acquired " + k.keyName + ".", 1800 );
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
                            showAccessPopup( "Collected note: " + n.title, 2200 );
							playPaperRustle( levels[ engineContext.currentLevel ].folder );

                            if (engineContext.currentLevel == Levels::CAVE)
                            {
                                g_caveFinalNoteCollected = true;
                                currentState = STATE_ENDING;
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
                        int tx = 0;
                        int ty = 0;
                        if (engineContext.currentLevel == Levels::MUSEUM && getDoorAheadTile( engineContext, tx, ty ))
                        {
                            int lockIndex = findDoorLockIndex( tx, ty );
                            if (lockIndex >= 0 && !g_roomLocks[ lockIndex ].unlocked)
                            {
                                auto &lock = g_roomLocks[ lockIndex ];
                                if (lock.type == LockType::KEY)
                                {
                                    if (g_playerKeys.contains( lock.requirement ))
                                    {
                                        lock.unlocked = true;
                                        showAccessPopup( lock.requirement + " used.", 1600 );
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
                        }

   
                        if (toggled && engineContext.currentLevel == Levels::TRANSITION)
                        {
                            handleLevelChange( engineContext, levels, Levels::CAVE );
                        }
                    }
                    else if (ev.key.scancode == SDL_SCANCODE_LSHIFT)
                    {
                        actualSpeed += 0.8f;
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
                    else if (ev.key.scancode == SDL_SCANCODE_N)
                    {
                        handleLevelChange( engineContext, levels, Levels::TRANSITION );
                    }
                }
            }
            else if (currentState == STATE_ENDING)
            {
                if (ev.type == SDL_EVENT_KEY_DOWN)
                {
                    if (ev.key.key == SDLK_R)
                    {
                        handleLevelChange( engineContext, levels, Levels::MUSEUM );
                        g_notesCollectedRun = 0;
                        g_runElapsedSeconds = 0.0f;
                        currentState = STATE_GAME;
                    }
                    else if (ev.key.key == SDLK_ESCAPE)
                    {
                        currentState = STATE_MENU;
                    }
                }
            }
        }

        if (currentState == STATE_GAME)
        {
            const bool *ks = SDL_GetKeyboardState( nullptr );
            float ms = actualSpeed * dt;
            if (g_codeEntryActive || g_notesOpen) ms = 0.0f;
            float ts = TURN_SPEED * dt;
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
        }
        render( engineContext, dt );

        if (currentState == STATE_MENU)
        {
            renderMenu( engineContext, currentMenuSelection, musicVolume, config::useMusic, config::viewBobbing );
        }
        else if (currentState == STATE_ENDING)
        {
            renderEndingScreen( engineContext );
        }
        // Present to window (nearest-neighbor scale)
        SDL_UpdateTexture( engineContext.backtexure, nullptr, engineContext.backbuffer.data(), RENDER_W * 4  );
        SDL_RenderClear( engineContext.renderer );
        SDL_RenderTexture( engineContext.renderer, engineContext.backtexure, nullptr, nullptr );
        SDL_RenderPresent( engineContext.renderer );
    }

    SDL_DestroyTexture( engineContext.backtexure );
    SDL_DestroyRenderer( engineContext.renderer );
    SDL_DestroyWindow( engineContext.window );
    SDL_Quit();
    return 0;
}