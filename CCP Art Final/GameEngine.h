#pragma once
#include "Includes.h"
namespace fs = std::filesystem;

static inline Uint32 rgb( Uint8 r, Uint8 g, Uint8 b ) {
    return (0xFFu << 24) | (r << 16) | (g << 8) | b;
}


struct Image
{
    int width = 0;
    int height = 0;
    std::vector<Uint32> pixels; // ARGB8888
    int resolution = 1;
    bool loadBMP( const std::string &path ) {
        // Use the map to create a surface

        SDL_Surface *BMPSurface = SDL_LoadBMP( path.c_str() );
        if (!BMPSurface)
        {
            std::fprintf( stderr, "SDL_LoadBMP failed for %s: %s\n", path.c_str(), SDL_GetError() );
            return false;
        }
        // Get converted surface into a format for pixel colors 
        SDL_Surface *ColoredSurface = SDL_ConvertSurface( BMPSurface, SDL_PIXELFORMAT_ARGB8888 );
        SDL_DestroySurface( BMPSurface );

        if (!ColoredSurface)
        {
            std::fprintf( stderr, "SDL_ConvertSurface failed for %s: %s\n", path.c_str(), SDL_GetError() );
            return false;
        }
        // Set width, height, and copy pixel data
        width = ColoredSurface->w;
        height = ColoredSurface->h;
        resolution = width * height;
        pixels.resize( resolution );
        std::memcpy( pixels.data(), ColoredSurface->pixels, resolution * 4 );
        SDL_DestroySurface( ColoredSurface );
        return true;
    }

    // Gets pixel color data at (x,y) snapping to nearest valid pixel if needed
    Uint32 sample( int x, int y ) const {
        x = std::clamp( x, 0, width - 1 );
        y = std::clamp( y, 0, height - 1 );
        // Convert from 2D to 1D index with row-major order using offset of x
        return pixels[ y * width + x ];
    }
};

struct Map
{
    // Map width, height
    int width = 0;
    int height = 0;
    // Type of tile: door, wall, empty
    std::vector<int> tiles;
};

struct Prop
{
    float x, y;       // world position
    int textureID = -1;        // which texture to use
    std::string kind;
    std::string filename; // original bmp filename
    float scale = 1.0f;
};


struct QuadProp
{
    // Center position
    float centerX = 0.0f;
    float centerY = 0.0f;
    // Size along local axes 
    float width = 1.0f;
    float depth = 0.3f;
    // Orientation (rad)
    float angle = 0.0f;

    // Texture
    Image texture;
    std::string texturePath;

    // Direction vectors (unit length)
    float ux = 1.0f, uy = 0.0f;   // +U
    float vx = 0.0f, vy = 1.0f;   // +V
    // Ambient occlusion multiplier (1.0 = none) or shading factor
    float AOMultiplier = 0.8f;

    std::string kind;
    std::string filename;
};


struct Artwork
{
    // Wall x,y if on wall; -1,-1 if free-standing
    int wx = -1;
    int wy = -1;
    int side = -1;
    float uCenter = 0.5f;   // position along the wall face 
    float uWidth = 0.65f;   // width along the wall face
    float vCenter = 0.5f;   // vertical center
    float vHeight = 0.65f;   // height on the wall
    bool onWall = false;
    int id = 0;
    std::string title;
    std::string artist;
    std::string date;
    std::string period;
    std::string medium;
    std::string location;
    std::string placard;
    std::string rationale;
    std::string reflection;
    std::string imagePath;
    float x = 1.5f, y = 1.5f;
};

struct CaveArt : Artwork {
    float widthMultiplier;
    float heightMultiplier;
};



struct Sprite
{
    float x;
    float y;
    int textureID;
    int artworkID;
    float distance;
};


struct BoxProp
{

    float centerX = 0.f;
    float centerY = 0.f;

    float halfLength = 1.f;
    float halfDepth = 0.3f;

    // Height of the box (seat height)
    float height = 0.5f;

    // Orientation of the long axis in radians
    float angle = 0.f;

    Image sideTexure;

    Image legTexure;

    float legHalf = 0.05f;     // ~10 cm
    float legInsetLength = 0.12f;   // inset along length
    float legInsetDepth = 0.08f;   // inset along depth
};



struct Engine
{
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *backtexure = nullptr; // streaming texture
    std::vector<Uint32> backbuffer;

    Map map;
    Image wallTex;
    Image floorTex;
    Image ceilTex;
    bool hasFloor = false;
    bool hasCeiling = false;
    std::vector<Artwork> artworks;
    std::vector<Image> artImages;
    std::vector<Sprite> sprites;

    float positionX = 3.5f, positionY = 3.5f; // player pos
    float directionX = 1.0f, directionY = 0.0f;
    float planeX = -directionY * FOV_TAN;
    float planeY = directionX * FOV_TAN;
    std::vector<float> zbuffer; // SSAO

    bool showHelp = true;
    int nearestArt = -1;
    Uint32 lastPlacardTick = 0;
    bool placardOpen = false;
    bool journalOpen = false;
    int openArtId = -1;


    Image doorTexture;

    std::vector<Prop> props;
    std::vector<Image> propImages;

    std::vector<QuadProp> quads;
    std::vector<std::vector<int>> quadBuckets;

    std::vector<BoxProp> benches3D;   // NEW: true 3D benches (box + legs)

    bool caveMode = false;
    bool hasWallOverlay = false;
	float lightRadius = 5.0f;
	float lightFalloff = 2.0f;
	float caveAmbient = 0.08f;

    Image wallOverlay;
    Image floorOverlayCracks, floorOverlayStains, floorOverlayPuddles;
    bool hasFloorCracks = false, hasFloorStains = false, hasFloorPuddles = false;

    Image wallOverlayCracks, wallOverlayStains;   // (file name: wall_stain.bmp)
    bool hasWallCracks = false, hasWallStains = false;

    struct GrayTex
    {
        int width = 0, height = 0;
        std::vector<Uint8> data;  // 0..255, treated as multiplier m = v/255
        bool valid() const {
            return width > 0 && height > 0 && (int)data.size() == width * height;
        }
    };


    // Combined, baked multipliers (precomposed from cracks/stains/puddles)
    GrayTex floorMul;   
    bool hasFloorMul = false;
    GrayTex wallMul;    
    bool hasWallMul = false;

    Levels currentLevel = Levels::MUSEUM;

    float yaw;

    bool inRangeOfStatue = false;     
    bool statueChatActive = false;    
    Uint32 statueChatStartTick = 0;   
};


static bool loadMap( const std::string &path, Map &mapToLoad ) {
    std::ifstream mapPathStream( path );
    if (!mapPathStream.is_open())
    {
        std::fprintf( stderr, "Couldn't open %s\n", path.c_str() ); return false;
    }


    std::vector<std::string> lines;
    std::string line;


    while (std::getline( mapPathStream, line ))
    {
        if (!line.empty())
        {
            lines.push_back( line );
        }
    }

    mapToLoad.height = (int)lines.size();
    mapToLoad.width = lines.empty() ? 0 : (int)lines[ 0 ].size();
    mapToLoad.tiles.assign( mapToLoad.width * mapToLoad.height, 0 );
    // Parse map and push proper types to the array
    for (int y = 0; y < mapToLoad.height; ++y)
    {
        for (int x = 0; x < mapToLoad.width; ++x)
        {
            char c = lines[ y ][ x ];
            int v = 0;
            if (c == '1') v = 1;          // wall
            else if (c == 'D') v = 2;     // door (closed)
            else v = 0;                   // empty
            mapToLoad.tiles[ y * mapToLoad.width + x ] = v;
        }
    }
    return true;
}



inline void makeDirectionalQuad( QuadProp &prop, float centerX, float centerY, float length, float depth, float angleRadian ) {
    prop.centerX = centerX;
    prop.centerY = centerY;

    const float cos = std::cos( angleRadian );
    const float sin = std::sin( angleRadian );

    // For both multiply by sin/cos to get direction, then by 0.5*length/depth to get depth

    // half extent vectors
    prop.ux = 0.5f * length * cos;
    prop.uy = 0.5f * length * sin;

    // perpendicular for depth
    // -sin give 90 roation
    prop.vx = -0.5f * depth * sin;
    prop.vy = 0.5f * depth * cos;
}

static bool splitLine( const std::string &s, char sep, std::vector<std::string> &out ) {
    out.clear();
    std::string cur; cur.reserve( s.size() );
    for (char c : s)
    {
        if (c == sep)
        {
            out.push_back( cur ); cur.clear();
        }
        else
        {
            cur.push_back( c );
        }
    } out.push_back( cur );
    return true;
}

// id|title|artist|date|period|medium|location|placard|rationale|reflection|imagePath|x|y
static bool loadArtworks( const std::string &path, std::vector<Artwork> &works ) {
    std::ifstream artFileStream( path );

    if (!artFileStream.is_open())
    {
        std::fprintf( stderr, "Couldn't open %s\n", path.c_str() ); return false;
    }

    std::string line;
    int lineTrack = 0;
    works.clear();

    while (std::getline( artFileStream, line ))
    {
        ++lineTrack;

        if (line.empty() || line[ 0 ] == '#') continue;

        std::vector<std::string> v; 
        splitLine( line, '|', v );
        if (v.size() < 13)
        {
            std::fprintf( stderr, "Bad line %d in %s (got %zu fields)\n", lineTrack, path.c_str(), v.size() ); continue;
        }

        Artwork art;
        art.id = std::stoi( v[ 0 ] );
        art.title = v[ 1 ];
        art.artist = v[ 2 ];
        art.date = v[ 3 ];
        art.period = v[ 4 ];
        art.medium = v[ 5 ];
        art.location = v[ 6 ];
        art.placard = v[ 7 ];
        art.rationale = v[ 8 ];
        art.reflection = v[ 9 ];
        art.imagePath = v[ 10 ];
        art.x = std::stof( v[ 11 ] );
        art.y = std::stof( v[ 12 ] );
        works.push_back( art );
    }
    return true;
}

static bool loadImageOrFallback( const std::string &path, Image &out, Uint32 fillRgb = 0 ) {
    if (out.loadBMP( path )) return true;
    // simple 64x64 fallback if missing
    out.width = 64;
    out.height = 64;
    out.pixels.assign( 64 * 64, fillRgb ? fillRgb : rgb( 255, 0, 255 ) );
    return false;
}



static bool loadProps( const std::string &path, std::vector<Prop> &outProps, std::vector<Image> &outPropImages, std::vector<QuadProp> &outQuads ) {
    std::ifstream propsFileStream( path );
    if (!propsFileStream.is_open())
    {
        std::fprintf( stderr, "Couldn't open %s\n", path.c_str() );
        return false;
    }

    outProps.clear();
    outQuads.clear();
    outPropImages.clear();

    const fs::path base = fs::path( path ).parent_path();

    auto resolve = [&]( const std::string &p )->std::string {
        fs::path q = p;
        return q.is_absolute() ? q.string() : (base / q).string();
        };

    std::unordered_map<std::string, int> textureIndex;


    auto getBillboardTextureIndex = [&]( const std::string &p )->int {
        const std::string full = resolve( p );
        auto it = textureIndex.find( full );
        if (it != textureIndex.end()) return it->second;
        Image img; 
        loadImageOrFallback( full, img, rgb( 255, 0, 255 ) );
        int idx = (int)outPropImages.size();
        outPropImages.push_back( std::move( img ) );
        textureIndex[ full ] = idx;
        return idx;
        };

    std::string line; int lineTrack = 0;
    while (std::getline( propsFileStream, line ))
    {
        ++lineTrack;
        if (line.empty() || line[ 0 ] == '#') continue;

        std::istringstream ss( line );
        std::string kind; ss >> kind;

        if (kind.empty()) continue;

        // Normalize to uppercase
        for (auto &c : kind)
        {
            c = char( std::toupper( unsigned char( c ) ) );
        }

        auto isNumStart = []( int c ) { return std::isdigit( c ) || c == '+' || c == '-' || c == '.'; };

        int next = ss.peek();

        if (next != EOF && !isNumStart( next ) && kind != "ROPE" && kind != "BENCH" && kind != "PLANT" && kind != "STATUE" && kind != "VASE1" && kind != "VASE2" && kind != "VASE3" && kind != "TRASHCAN")
            continue; // skip descriptive line

        if (kind == "PLANT" || kind == "STATUE" || kind == "VASE1" || kind == "VASE2" || kind == "VASE3" || kind == "TRASHCAN")
        {
            float x, y;
            std::string bmp;
            float scale = 1.0f;
            if (!(ss >> x >> y >> bmp))
            {
                std::fprintf( stderr, "Bad %s line %d\n", kind.c_str(), lineTrack ); continue;
            }

            if (!(ss >> scale)) scale = 1.0f;
            Prop prop;
            prop.x = x;
            prop.y = y;
            prop.textureID = getBillboardTextureIndex( bmp );
            prop.kind = kind;
            prop.filename = bmp;
            prop.scale = scale;
            outProps.push_back( prop );
        }
        else if (kind == "ROPE")
        {
            float x, y; std::string bmp;
            if (!(ss >> x >> y >> bmp))
            {
                std::fprintf( stderr, "Bad %s line %d\n", kind.c_str(), lineTrack ); continue;
            }
            Prop prop;
            prop.x = x;
            prop.y = y;
            prop.textureID = getBillboardTextureIndex( bmp );
            prop.kind = kind;
            prop.filename = bmp;
            outProps.push_back( prop );
        }
    }
    return true;
}


static inline void quadprop_recalc_axes( QuadProp &quad ) {
    float cos = std::cos( quad.angle );
    float sin = std::sin( quad.angle );
    quad.ux = cos;
    quad.uy = sin;
    quad.vx = -sin;
    quad.vy = cos;
}



inline bool quadprop_local_uv( const QuadProp &quad, float wx, float wy,
    float &u, float &v ) {
    // Convert world point to quad local (centered) coords, projected on (U,V)
    const float dx = wx - quad.centerX;
    const float dy = wy - quad.centerY;

    const float UdotU = quad.ux * quad.ux + quad.uy * quad.uy;
    const float VdotV = quad.vx * quad.vx + quad.vy * quad.vy;
    // Degenerate test
    if (UdotU < 1e-8f || VdotV < 1e-8f) return false;

    const float dU = dx * quad.ux + dy * quad.uy;
    const float dV = dx * quad.vx + dy * quad.vy;

    // Map to normalized UV where -1 -> +1 corresponds 0 -> 1
    u = 0.5f + dU / (2.0f * UdotU);
    v = 0.5f + dV / (2.0f * VdotV);

    return (u >= 0.f && u <= 1.f && v >= 0.f && v <= 1.f);
}



static bool addQuadProp( Engine &engineContext, float centerX, float centerY, float width, float depth, float angle, const char *texturePath, float AO = 1.0f ) {
    QuadProp quad;
    quad.centerX = centerX;
    quad.centerY = centerY;
    quad.width = width;
    quad.depth = depth;
    quad.angle = angle;
    quad.texturePath = texturePath;
    quad.AOMultiplier = AO;

    // Compute coords for local axes and vectors
    quadprop_recalc_axes( quad );

    if (!quad.texture.loadBMP( quad.texturePath ))
    {
        // Throw error
        std::fprintf( stderr, "Couldn't load quad texture %s\n", texturePath );
        return false;
    }
    engineContext.quads.push_back( std::move( quad ) );
    return true;
}

void placePlant( Engine &engineContext, const float2 &position, const std::string &bmp ) {
    Prop plant;
    plant.x = position.x;
    plant.y = position.y;
    plant.filename = "plant.bmp";
    plant.kind = "PLANT";
    // Load texture once (if not already loaded)
    Image img;
    loadImageOrFallback( bmp, img, rgb( 255, 0, 255 ) );
    int textureID = (int)engineContext.propImages.size();
    engineContext.propImages.push_back( std::move( img ) );
    plant.textureID = textureID;
    //  p.scale = 0.8f;
    engineContext.props.push_back( plant );
}

void placeRope( Engine &engineContext, const float2 &position, const std::string &bmp ) {
    Prop rope;
    rope.x = position.x;
    rope.y = position.y;
    rope.filename = "rope.bmp";
    rope.kind = "ROPE";
    // Load texture once (if not already loaded)
    Image img;
    loadImageOrFallback( bmp, img, rgb( 255, 0, 255 ) );
    int textureID = (int)engineContext.propImages.size();
    engineContext.propImages.push_back( std::move( img ) );
    rope.textureID = textureID;
    //  p.scale = 0.8f;
    engineContext.props.push_back( rope );
}


void placeStatue( Engine &engineContext, const float2 &position, const std::string &bmp ) {
    Prop statue;
    statue.x = position.x;
    statue.y = position.y;
    statue.filename = "statue.bmp";
    statue.kind = "STATUE";
    // Load texture once (if not already loaded)
    Image img;
    loadImageOrFallback( bmp, img, rgb( 255, 0, 255 ) );
    int textureID = (int)engineContext.propImages.size();
    engineContext.propImages.push_back( std::move( img ) );
    statue.textureID = textureID;
    //  p.scale = 0.8f;
    engineContext.props.push_back( statue );
}

void placeVase( Engine &engineContext, const float2 &position, const std::string &bmp ) {
    std::vector<std::string> possibleVases = { "VASE1", "VASE2", "VASE3" };

    std::string actualVase = possibleVases[ rand() % possibleVases.size() ];
    Prop vase;

    if (actualVase == "VASE1")
    {
        vase.filename = "vase1.bmp";
    }
    else if (actualVase == "VASE2")
    {
        vase.filename = "vase2.bmp";
    }
    else if (actualVase == "VASE3")
    {
        vase.filename = "vase3.bmp";
    }
    vase.x = position.x;
    vase.y = position.y;
    vase.kind = actualVase;
    vase.scale = 0.5f;



    // Load texture once (if not already loaded)
    Image img;
    loadImageOrFallback( bmp + "/" + vase.filename, img, rgb( 255, 0, 255 ) );
    int texId = (int)engineContext.propImages.size();
    engineContext.propImages.push_back( std::move( img ) );
    vase.textureID = texId;

    engineContext.props.push_back( vase );
}

void placeCan( Engine &engineContext, const float2 &position, const std::string &bmp ) {

    Prop can;
    can.x = position.x;
    can.y = position.y;
    can.filename = "trashcan.bmp";
    can.kind = "TRASHCAN";
    can.scale = 0.5f;

    // Load texture once (if not already loaded)
    Image img;
    loadImageOrFallback( bmp, img, rgb( 255, 0, 255 ) );
    int texId = (int)engineContext.propImages.size();
    engineContext.propImages.push_back( std::move( img ) );
    can.textureID = texId;

    engineContext.props.push_back( can );
}





static bool saveProps( const std::string &path, const std::vector<Prop> &props, const std::vector<Image> &propImages, const std::vector<QuadProp> &quads ) {
    std::ofstream propsFileStream( path );
    if (!propsFileStream.is_open())
    {
        std::fprintf( stderr, "Couldn't write %s\n", path.c_str() );
        return false;
    }

    // Billboards (plants, statues, etc.)
    for (size_t index = 0; index < props.size(); index++)
    {
        const Prop &prop = props[ index ];

        propsFileStream << prop.kind << " " << prop.x << " " << prop.y << " " << prop.filename << " " << prop.scale << "\n";
    }

    /*
    // Floor quads (benches, ropes, etc.)
    for (size_t i = 0; i < quads.size(); i++)
    {
        const QuadProp &q = quads[ i ];
        // You?ll need to store back the params you used for make_oriented_quad.
        // For now, dump as BENCH placeholder.
        f << "BENCH " << q.cx << " " << q.cy
            << " " << q.len << " " << q.wid
            << " " << q.angle * 180.0f / 3.14159f
            << " quad_" << i << ".bmp\n";
    }
    */

    return true;
}



