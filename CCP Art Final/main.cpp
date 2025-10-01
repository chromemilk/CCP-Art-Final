#include "GameEngine.h"
#include "RendererHelpers.h"
#include "PhysicsHelpers.h"
#include <iostream>

using namespace std;

struct LevelDef
{
    string name;
    string folder;
    float spawnX = 2.0f, spawnY = 9.5f, spawnDirDeg = 0.f;
};

static bool loadLevel( Engine &engineContext, const LevelDef &L ) {
    namespace fs = std::filesystem;

    // Clear per-level state
    engineContext.artworks.clear(); 
    engineContext.artImages.clear();
    engineContext.props.clear();
    engineContext.propImages.clear();
    engineContext.quads.clear();  
    engineContext.benches3D.clear();


    fs::path folder = L.folder;

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

    // Spawn & camera
    engineContext.positionX = L.spawnX; engineContext.positionY = L.spawnY;
    float art = L.spawnDirDeg * 3.14159265f / 180.f;
    engineContext.directionX = std::cos( art ); engineContext.directionY = std::sin( art );
    engineContext.planeX = -engineContext.directionY * FOV_TAN; engineContext.planeY = engineContext.directionX * FOV_TAN;

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


static void render( Engine &engineContext, float dt ) {
    (void)dt;

    auto shadeCol = []( Uint32 c, float s ) -> Uint32 {
        s = std::clamp( s, 0.0f, 1.0f );
        Uint8 r = Uint8( ((c >> 16) & 255) * s );
        Uint8 g = Uint8( ((c >> 8) & 255) * s );
        Uint8 box = Uint8( (c & 255) * s );
        return rgb( r, g, box );
        };

    const int half = RENDER_H / 2;

    engineContext.zbuffer.assign( RENDER_W, 1e9f );

    for (int x = 0; x < RENDER_W; ++x)
    {
        float camX = 2.0f * x / float( RENDER_W ) - 1.0f;
        float rayDirX = engineContext.directionX + engineContext.planeX * camX;
        float rayDirY = engineContext.directionY + engineContext.planeY * camX;

        int mapX = int( engineContext.positionX );
        int mapY = int( engineContext.positionY );
        float sideDistX, sideDistY;
        float deltaDistX = (rayDirX == 0) ? 1e30f : std::fabs( 1.0f / rayDirX );
        float deltaDistY = (rayDirY == 0) ? 1e30f : std::fabs( 1.0f / rayDirY );
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
            if (tile > 0)
            {
                hitTile = tile;
            }
        }
        if (!hitTile) continue;

        float perpWallDist;
        if (side == 0)
        {
            perpWallDist = ((mapX - engineContext.positionX) + (1 - stepX) * 0.5f) / (rayDirX == 0 ? 1e-6f : rayDirX);
        }
        else
        {
            perpWallDist = ((mapY - engineContext.positionY) + (1 - stepY) * 0.5f) / (rayDirY == 0 ? 1e-6f : rayDirY);
        }

        engineContext.zbuffer[ x ] = std::max( std::fabs( perpWallDist ), 0.05f );

      
    }


    float rayDirX0 = engineContext.directionX - engineContext.planeX;
    float rayDirY0 = engineContext.directionY - engineContext.planeY;
    float rayDirX1 = engineContext.directionX + engineContext.planeX;
    float rayDirY1 = engineContext.directionY + engineContext.planeY;

    const float posZ = 0.5f * RENDER_H;

    for (int y = 0; y < RENDER_H; ++y)
    {
        const int prop = y - half;
        if (prop == 0) continue; 

        float rowDist = std::fabs( posZ / float( prop ) ); // distance to this row in world units

        // Step across the row
        float stepX = rowDist * (rayDirX1 - rayDirX0) / float( RENDER_W );
        float stepY = rowDist * (rayDirY1 - rayDirY0) / float( RENDER_W );

        // World position at the left edge for this row
        float worldX = engineContext.positionX + rowDist * rayDirX0;
        float worldY = engineContext.positionY + rowDist * rayDirY0;

        for (int x = 0; x < RENDER_W; ++x)
        {
            float fx = worldX - std::floor( worldX );
            float fy = worldY - std::floor( worldY );

            if (y >= half)
            {
                // Floor
                if (engineContext.hasFloor)
                {
                    int tx = int( fx * engineContext.floorTex.width );
                    int ty = int( fy * engineContext.floorTex.height );
                    Uint32 color = engineContext.floorTex.sample( tx, ty );
                    float shade = std::clamp( 1.0f / (0.02f * rowDist), 0.30f, 1.0f );
                    putPix( engineContext, x, y, shadeCol( color, shade) );
                }
                else
                {
                    putPix( engineContext, x, y, rgb( 12, 12, 14 ) );
                }
            }
            else
            {
                // Ceiling
                if (engineContext.hasCeiling)
                {
                    int tx = int( fx * engineContext.ceilTex.width );
                    int ty = int( fy * engineContext.ceilTex.height );
                    Uint32 color = engineContext.ceilTex.sample( tx, ty );
                    float shade = std::clamp( 1.0f / (0.02f * rowDist), 0.35f, 1.0f );
                    putPix( engineContext, x, y, shadeCol( color, shade ) );
                }
                else
                {
                    putPix( engineContext, x, y, rgb( 30, 30, 38 ) );
                }
            }

       
            if (y >= half && rowDist < engineContext.zbuffer[ x ])
            {
                if (!engineContext.quads.empty())
                {
                    for (const auto &q : engineContext.quads)
                    {
                        float u, v;
                        if (!quadprop_local_uv( q, worldX, worldY, u, v )) continue;

                        Uint32 color = sample_bilinear_uv_keyed( q.texture, u, v );

                        // magenta = transparent
                        if (((color >> 16) & 255) == 255 && ((color >> 8) & 255) == 0 && (color & 255) == 255)
                        {
                            // skip
                        }
                        else
                        {
                            float shade = std::clamp( (1.0f / (0.02f * rowDist)) * q.AOMultiplier, 0.25f, 1.0f );
                            Uint8 r = Uint8( ((color >> 16) & 255) * shade );
                            Uint8 g = Uint8( ((color >> 8) & 255) * shade );
                            Uint8 box = Uint8( (color & 255) * shade );
                            putPix( engineContext, x, y, rgb( r, g, box ) );
                        }
                    }
                }
            }

            worldX += stepX;
            worldY += stepY;
        }
    }


    for (int x = 0; x < RENDER_W; ++x)
    {
        // Rebuild ray for this column
        float camX = 2.0f * x / float( RENDER_W ) - 1.0f;
        float rayDirX = engineContext.directionX + engineContext.planeX * camX;
        float rayDirY = engineContext.directionY + engineContext.planeY * camX;

        int mapX = int( engineContext.positionX ), mapY = int( engineContext.positionY );
        float sideDistX, sideDistY;
        float deltaDistX = (rayDirX == 0) ? 1e30f : std::fabs( 1.0f / rayDirX );
        float deltaDistY = (rayDirY == 0) ? 1e30f : std::fabs( 1.0f / rayDirY );
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

            if (mapX < 0 || mapY < 0 || mapX >= engineContext.map.width || mapY >= engineContext.map.height) break;

            int tile = engineContext.map.tiles[ mapY * engineContext.map.width + mapX ];
            if (tile > 0) hitTile = tile; // 1 wall, 2 door
        }
        if (!hitTile) continue;

        float perpWallDist;
        if (side == 0)
        {
            perpWallDist = ((mapX - engineContext.positionX) + (1 - stepX) * 0.5f) / (rayDirX == 0 ? 1e-6f : rayDirX);
        }
        else
        {
            perpWallDist = ((mapY - engineContext.positionY) + (1 - stepY) * 0.5f) / (rayDirY == 0 ? 1e-6f : rayDirY);
        }

        perpWallDist = std::max( std::fabs( perpWallDist ), 0.05f );

        int lineH = int( RENDER_H / std::max( perpWallDist, 1e-3f ) );
        int drawStart = std::max( 0, -lineH / 2 + RENDER_H / 2 );
        int drawEnd = std::min( RENDER_H - 1, lineH / 2 + RENDER_H / 2 );

        float wallX;
        if (side == 0)
        {
            wallX = engineContext.positionY + perpWallDist * rayDirY;
        }
        else
        {
            wallX = engineContext.positionX + perpWallDist * rayDirX;
        }

        wallX -= std::floor( wallX );

        const Image &wallTexture = (hitTile == 2) ? engineContext.doorTexture : engineContext.wallTex;
        drawTexturedColumn( engineContext, wallTexture, x, drawStart, drawEnd, perpWallDist, wallX );

        // Framed artwork overlay (only on real walls)
        if (hitTile == 1)
        {
            for (size_t artIndex = 0; artIndex < engineContext.artworks.size(); ++artIndex)
            {
                const auto &art = engineContext.artworks[ artIndex ];
                if (!art.onWall) continue;
                if (art.wx != mapX || art.wy != mapY || art.side != side) continue;

                float u0 = std::clamp( art.uCenter - art.uWidth * 0.5f, 0.0f, 1.0f );
                float u1 = std::clamp( art.uCenter + art.uWidth * 0.5f, 0.0f, 1.0f );
                if (wallX < u0 || wallX > u1) continue;

                const Image &texture = engineContext.artImages[ artIndex ];

                // Frame/mat proportions
                const float FRAME_U = 0.08f, FRAME_V = 0.08f;
                const float MAT_U = 0.03f, MAT_V = 0.04f;

                const Uint32 goldLight = rgb( 235, 200, 80 );
                const Uint32 goldMid = rgb( 212, 175, 55 );
                const Uint32 goldDark = rgb( 160, 130, 40 );
                const Uint32 matCol = rgb( 235, 235, 220 );

                float uLocal = (wallX - u0) / std::max( 0.0001f, (u1 - u0) );

                int bandH = std::max( 1, int( lineH * art.vHeight ) );
                int bandCenter = RENDER_H / 2 + int( (art.vCenter - 0.5f) * lineH );
                int bandStart = std::clamp( bandCenter - bandH / 2, 0, RENDER_H - 1 );
                int bandEnd = std::clamp( bandStart + bandH - 1, 0, RENDER_H - 1 );

                float uLeftFrameEdge = FRAME_U;
                float uRightFrameEdge = 1.0f - FRAME_U;
                float uLeftMatEdge = FRAME_U + MAT_U;
                float uRightMatEdge = 1.0f - (FRAME_U + MAT_U);

                for (int y = bandStart; y <= bandEnd; ++y)
                {
                    float vLocal = (y - bandStart) / float( std::max( 1, bandH - 1 ) );
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
                            float un = (uLocal - innerU0) / std::max( 0.0001f, (innerU1 - innerU0) );
                            float vn = (vLocal - innerV0) / std::max( 0.0001f, (innerV1 - innerV0) );
                            int texX = std::clamp( int( un * (texture.width - 1) ), 0, texture.width - 1 );
                            int texY = std::clamp( int( vn * (texture.height - 1) ), 0, texture.height - 1 );
                            color = texture.sample( texX, texY );

                            // magenta transparent -> mat
                            if (((color >> 16) & 255) == 255 && ((color >> 8) & 255) == 0 && (color & 255) == 255)
                                color = matCol;
                        }
                    }
                    putPix( engineContext, x, y, color );
                }
            }
        }
    }
    
    // Props 
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

        // Unscaled height at this distance
        float baseH = (RENDER_H / transY);

        // Scaled screen size
        int spriteH = std::max( 1, int( std::fabs( baseH * prop.scale ) ) );
        int spriteW = spriteH; // square billboard

        // Floor-contact Y in screen space (stable regardless of scale)
        int bottomY = int( RENDER_H * 0.5f + baseH * 0.5f );

        // Sprite rect in screen space BEFORE clipping
        int y0 = bottomY - spriteH;   // top
        int y1 = bottomY - 1;         // bottom inclusive
        int x0 = -spriteW / 2 + spriteScreenX;
        int x1 = spriteW / 2 + spriteScreenX - 1;

        // Clip to screen, but remember unclipped origins for texture mapping
        int cy0 = std::max( 0, y0 );
        int cy1 = std::min( RENDER_H - 1, y1 );
        int cx0 = std::max( 0, x0 );
        int cx1 = std::min( RENDER_W - 1, x1 );
        if (cy0 > cy1 || cx0 > cx1) continue;

        // Precompute reciprocal for mapping
        float invSpriteH = 1.0f / std::max( 1, spriteH );
        float invSpriteW = 1.0f / std::max( 1, spriteW );

        for (int sx = cx0; sx <= cx1; ++sx)
        {
            if (!(transY > 0 && transY < engineContext.zbuffer[ sx ])) continue;

            // Horizontal texture coord: use unclipped sprite left x0
            float u = float( sx - x0 ) * invSpriteW;     // 0..1 across sprite width
            int texX = std::clamp( int( u * texture.width ), 0, texture.width - 1 );

            for (int sy = cy0; sy <= cy1; ++sy)
            {
                // Vertical texture coord: use unclipped top y0
                float v = float( sy - y0 ) * invSpriteH; // 0..1 down sprite height
                int texY = std::clamp( int( v * texture.height ), 0, texture.height - 1 );

                Uint32 color = texture.sample( texX, texY );
                if (!isNearMagenta( color, 120 ))
                {
                    putPix( engineContext, sx, sy, color );
                }
            }
        }
    }


       for (const auto &box : engineContext.benches3D)
        {
            render_box( engineContext, box );
            render_legs( engineContext, box );

           // render_box_top( engineContext, box, (box.sideTexure.width > 0 ? box.sideTexure : engineContext.floorTex) );

        }

    
       // UI
    if (engineContext.showHelp)
    {

        drawTextBox( engineContext, 4, 4, 110, 12, rgb( 10, 10, 16 ), rgb( 90, 90, 120 ) );
        drawStringTiny( engineContext, 8, 8, "E: Interact, F: Open Door", rgb( 220, 220, 220 ) );
    }

    if (engineContext.placardOpen && engineContext.openArtId >= 0)
    {
        const Artwork *art = nullptr;
        for (const auto &artWork : engineContext.artworks)
        {
            if (artWork.id == engineContext.openArtId)
            {
                art = &artWork;
                break;
            }
        }
        if (art)
        {
            int width = RENDER_W - 16, height = RENDER_H / 3;
            int x = 8, y = RENDER_H - 130;
            drawTextBox( engineContext, x, y, width, height, rgb( 18, 18, 24 ), rgb( 90, 90, 120 ) );
            std::string header = art->title + " (" + art->date + ")\n" +
                art->artist + " — " + art->period + "\n" +
                art->medium + ", " + art->location + "\n";
            drawStringTiny( engineContext, x + 8, y + 8, header, rgb( 230, 230, 240 ) );
            drawStringTiny( engineContext, x + 8, y + 26, "Why it matters:\n" + art->rationale, rgb( 210, 210, 210 ) );
            drawStringTiny( engineContext, x + 8, y + 44, "My take:\n" + art->reflection, rgb( 200, 200, 200 ) );
        }
    }

    putPix( engineContext, RENDER_W / 2, RENDER_H / 2, rgb( 0, 0, 0 ) );
    putPix( engineContext, RENDER_W / 2 + 1, RENDER_H / 2, rgb( 0, 0, 0 ) );
    putPix( engineContext, RENDER_W / 2 - 1, RENDER_H / 2, rgb( 0, 0, 0 ) );
    putPix( engineContext, RENDER_W / 2, RENDER_H / 2 + 1, rgb( 0, 0, 0 ) );
    putPix( engineContext, RENDER_W / 2, RENDER_H / 2 - 1, rgb( 0, 0, 0 ) );


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
    {"Museum", (cwd / "levels" / "museum").string(), 5.5f, 1.5f, 90.f }
    };

    int curLevel = 0;
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


    // Load artwork images (ensure magenta background for transparency)

    /*
    engineContext.artImages.resize( engineContext.artworks.size() );
    for (size_t i = 0; i < engineContext.artworks.size(); ++i)
    {
        if (!engineContext.artImages[ i ].loadBMP( engineContext.artworks[ i ].imagePath ))
        {
            // create placeholder (64x64 magenta frame)
            engineContext.artImages[ i ].width = 64; engineContext.artImages[ i ].height = 64; engineContext.artImages[ i ].pixels.assign( 64 * 64, rgb( 255, 0, 255 ) );
            for (int y = 8; y < 56; ++y)for (int x = 8; x < 56; ++x) engineContext.artImages[ i ].pixels[ y * 64 + x ] = rgb( 220, 220, 220 );
        }
    }
    */
   

    // Main loop
    bool running = true; Uint32 prev = SDL_GetTicks();
    while (running)
    {

        Uint32 now = SDL_GetTicks(); 
        float dt = (now - prev) / 1000.0f; 
        prev = now; 
        if (dt > 0.05f) dt = 0.05f;
        // Input
        SDL_Event ev;
        float actualSpeed;

        while (SDL_PollEvent( &ev ))
        {
            actualSpeed = MOVE_SPEED;
            if (ev.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
            else if (ev.type == SDL_EVENT_KEY_DOWN)
            {
                if (ev.key.key == SDLK_ESCAPE)
                {
                    running = false;
                }
                else if (ev.key.key == SDLK_F1)
                {
                    engineContext.showHelp = !engineContext.showHelp;
                }
                else if (ev.key.scancode == SDL_SCANCODE_E)
                {
                    int id = pickArtworkUnderCrosshair( engineContext );
                    if (id < 0) id = findNearestArtwork( engineContext ); // optional fallback

                    if (id >= 0)
                    {
                        if (engineContext.placardOpen && engineContext.openArtId == id)
                        {
                            engineContext.placardOpen = false;
                            engineContext.openArtId = -1;
                        }
                        else
                        {
                            engineContext.openArtId = id;
                            engineContext.placardOpen = true;
                            engineContext.lastPlacardTick = SDL_GetTicks();
                        }
                    }
                }
                else if (ev.key.scancode == SDL_SCANCODE_F)
                {
                    (void)toggleDoorAhead( engineContext );
                }
                else if (ev.key.scancode == SDL_SCANCODE_LSHIFT)
                {
                    actualSpeed += 5.f;
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
                    placeCan( engineContext, pos, levels[ curLevel ].folder + "/trashcan.bmp");
                }
                else if (ev.key.scancode == SDL_SCANCODE_O)
                {
                    saveProps( (levels[ curLevel ].folder + "/props.txt"),
                        engineContext.props, engineContext.propImages, engineContext.quads );
                }
            }
        }
        const bool *ks = SDL_GetKeyboardState( nullptr );
        float ms = actualSpeed * dt; float ts = TURN_SPEED * dt;
        if (ks[ SDL_SCANCODE_LEFT ])
        {
            float ang = -ts;
            float ndx = engineContext.directionX * std::cos( ang ) - engineContext.directionY * std::sin( ang );
            float ndy = engineContext.directionX * std::sin( ang ) + engineContext.directionY * std::cos( ang );
            engineContext.directionX = ndx; engineContext.directionY = ndy;
            // re-derive plane to stay perfectly perpendicular and correct FOV
            engineContext.planeX = -engineContext.directionY * FOV_TAN;
            engineContext.planeY = engineContext.directionX * FOV_TAN;
        }
        if (ks[ SDL_SCANCODE_RIGHT ])
        {
            float ang = ts;
            float ndx = engineContext.directionX * std::cos( ang ) - engineContext.directionY * std::sin( ang );
            float ndy = engineContext.directionX * std::sin( ang ) + engineContext.directionY * std::cos( ang );
            engineContext.directionX = ndx; engineContext.directionY = ndy;
            engineContext.planeX = -engineContext.directionY * FOV_TAN;
            engineContext.planeY = engineContext.directionX * FOV_TAN;
        }
        // move: W/S
        float nx = engineContext.positionX, ny = engineContext.positionY;
        if (ks[ SDL_SCANCODE_W ])
        {
            nx += engineContext.directionX * ms;
            ny += engineContext.directionY * ms;
        }
        if (ks[ SDL_SCANCODE_S ])
        {
            nx -= engineContext.directionX * ms; 
            ny -= engineContext.directionY * ms;
        }
        // strafe: A/D
        if (ks[ SDL_SCANCODE_A ])
        {
            nx += engineContext.directionY * ms;
            ny += -engineContext.directionX * ms;
        }
        if (ks[ SDL_SCANCODE_D ])
        {
            nx += -engineContext.directionY * ms;
            ny += engineContext.directionX * ms;
        }
        auto pass = [&]( float x, float y ) {
            int mx = int( x ), my = int( y );
            if (mx < 0 || my < 0 || mx >= engineContext.map.width || my >= engineContext.map.height) return false;
            int t = engineContext.map.tiles[ my * engineContext.map.width + mx ];
            if (t != 0) return false;

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

            const float pad = 0.02f;
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

        {
            // Keep open while you keep looking at it; close after ~600ms of looking away
            static const Uint32 KEEP_MS = 600;

            int under = pickArtworkUnderCrosshair( engineContext );
            Uint32 now = SDL_GetTicks();

            if (engineContext.placardOpen)
            {
                if (under == engineContext.openArtId)
                {
                    engineContext.lastPlacardTick = now; // still looking at it ? refresh timer
                }
                else if (now - engineContext.lastPlacardTick > KEEP_MS)
                {
                    engineContext.placardOpen = false;
                    engineContext.openArtId = -1;
                }
            }
        }
        render( engineContext, dt );

        // Present to window (nearest-neighbor scale)
        SDL_UpdateTexture( engineContext.backtexure, nullptr, engineContext.backbuffer.data(), RENDER_W * 4 );
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