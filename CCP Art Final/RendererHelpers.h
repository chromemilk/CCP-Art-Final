#include "GameEngine.h"

static void putPix( Engine &engineContext, int x, int y, Uint32 c ) {
    if ((unsigned)x < (unsigned)RENDER_W && (unsigned)y < (unsigned)RENDER_H) engineContext.backbuffer[ y * RENDER_W + x ] = c;
}

static void clear( Engine &engineContext, Uint32 top, Uint32 bottom ) {
    int mid = RENDER_H / 2;
    for (int y = 0; y < RENDER_H; ++y)
    {
        Uint32 choice = (y < mid) ? top : bottom;
        std::fill_n( &engineContext.backbuffer[ y * RENDER_W ], RENDER_W, choice );
    }
}
static void drawTexturedColumn( Engine &engineContext, const Image &texture, int x, int drawStart, int drawEnd, float perpDist, float wallX ) {
    int textureW = texture.width;
    int textureH = texture.height;
    int textureX = int( wallX * float( textureW ) );
    textureX = std::clamp( textureX, 0, textureW - 1 );

    const int lineH = int( RENDER_H / std::max( perpDist, 1e-3f ) );

    const int wallTopY = -lineH / 2 + RENDER_H / 2;

    

    for (int y = drawStart; y <= drawEnd; ++y)
    {
   
        int y_relative = y - wallTopY;
        int textureY = int( (y_relative * (float)textureH) / (float)std::max( 1, lineH ) );
        textureY = std::clamp( textureY, 0, textureH - 1 ); // Clamp to be safe

     
        Uint32 color = texture.sample( textureX, textureY );
        float mul = 1.0f;

        // Use texture-space tiling so overlays repeat seamlessly regardless of texture size
        if (engineContext.hasWallStains)
        {
            int ow = engineContext.wallOverlayStains.width, oh = engineContext.wallOverlayStains.height;
            if (ow > 0 && oh > 0)
            {
                int ox = (int)((textureX / float( textureW )) * ow) % ow;
                int oy = (int)((textureY / float( textureH )) * oh) % oh;
                if (ox < 0) ox += ow; if (oy < 0) oy += oh;
                Uint32 oc = engineContext.wallOverlayStains.sample( ox, oy );
                // Subtle, broad discoloration
                // strength, min..max, gamma tuned to keep color natural
                float m = 1.0f - 0.40f * (1.0f - std::pow(
                    std::clamp( (0.299f * ((oc >> 16) & 255) + 0.587f * ((oc >> 8) & 255) + 0.114f * (oc & 255)) / 255.0f, 0.0f, 1.0f ), 1.2f ));
                mul *= std::clamp( m, 0.85f, 1.03f );
            }
        }
        if (engineContext.hasWallCracks)
        {
            int ow = engineContext.wallOverlayCracks.width, oh = engineContext.wallOverlayCracks.height;
            if (ow > 0 && oh > 0)
            {
                int ox = (int)((textureX / float( textureW )) * ow) % ow;
                int oy = (int)((textureY / float( textureH )) * oh) % oh;
                if (ox < 0) ox += ow; if (oy < 0) oy += oh;
                Uint32 oc = engineContext.wallOverlayCracks.sample( ox, oy );
                // Stronger dark filaments, no color shift
                float L = (0.299f * ((oc >> 16) & 255) + 0.587f * ((oc >> 8) & 255) + 0.114f * (oc & 255)) / 255.0f;
                float m = 1.0f - 0.90f * (1.0f - std::pow( std::clamp( L, 0.0f, 1.0f ), 1.6f ));
                mul *= std::clamp( m, 0.55f, 1.00f );
            }
        }

        // Apply brightness multiplier 
        {
            float rf = float( (color >> 16) & 255 ) * mul;
            float gf = float( (color >> 8) & 255 ) * mul;
            float bf = float( color & 255 ) * mul;
            color = rgb(
                Uint8( std::clamp( rf, 0.0f, 255.0f ) ),
                Uint8( std::clamp( gf, 0.0f, 255.0f ) ),
                Uint8( std::clamp( bf, 0.0f, 255.0f ) )
            );
        }


        if (engineContext.caveMode && engineContext.hasWallOverlay)
        {
            int ox = textureX % engineContext.wallOverlay.width;
            int oy = textureY % engineContext.wallOverlay.height;
            Uint32 o = engineContext.wallOverlay.sample( ox, oy );
            float mr = (((o >> 16) & 255) / 255.0f) * 0.20f + 0.85f;
            float mg = (((o >> 8) & 255) / 255.0f) * 0.20f + 0.85f;
            float mb = ((o & 255) / 255.0f) * 0.20f + 0.85f;
            Uint8 r = Uint8( ((color >> 16) & 255) * mr );
            Uint8 g = Uint8( ((color >> 8) & 255) * mg );
            Uint8 b = Uint8( (color & 255) * mb );
            color = rgb( r, g, b );
        }


        float shade = std::clamp( 1.0f / (0.4f * perpDist), 0.15f, 1.0f );

        if (engineContext.caveMode)
        {
            float R = engineContext.lightRadius;
            float t = std::clamp( 1.0f - std::pow( perpDist / std::max( 0.001f, R ), engineContext.lightFalloff ), 0.0f, 1.0f );
            float l = std::max( engineContext.caveAmbient, t );
            shade *= l;
        }

        Uint8 r = (color >> 16) & 255, g = (color >> 8) & 255, box = color & 255;
        r = Uint8( r * shade ); g = Uint8( g * shade ); box = Uint8( box * shade );
        putPix( engineContext, x, y, rgb( r, g, box ) );
    }
}


static void drawTextBox( Engine &engineContext, int x, int y, int width, int height, Uint32 bg, Uint32 fg ) {
    // solid rect with 1px border
    for (int yy = y; yy < y + height; ++yy)
    {
        for (int xx = x; xx < x + width; ++xx)
        {
            bool border = (yy == y || yy == y + height - 1 || xx == x || xx == x + width - 1);
            putPix( engineContext, xx, yy, border ? fg : bg );
        }
    }
}

using Glyph = std::array<uint8_t, 5>;

static const std::unordered_map<char, Glyph> TINY_GLYPHS = {
    // space & basics
    {' ', {0b000,0b000,0b000,0b000,0b000}},
    {'!', {0b010,0b010,0b010,0b000,0b010}},
    {'"', {0b101,0b101,0b000,0b000,0b000}},
    {'#', {0b101,0b111,0b101,0b111,0b101}},
    {'$', {0b111,0b100,0b111,0b001,0b111}},
    {'%', {0b101,0b001,0b010,0b100,0b101}},
    {'&', {0b010,0b101,0b010,0b101,0b010}},
    {'\'',{0b010,0b010,0b000,0b000,0b000}},
    {'(', {0b001,0b010,0b010,0b010,0b001}},
    {')', {0b100,0b010,0b010,0b010,0b100}},
    {'*', {0b101,0b010,0b111,0b010,0b101}},
    {'+', {0b000,0b010,0b111,0b010,0b000}},
    {',', {0b000,0b000,0b000,0b010,0b100}},
    {'-', {0b000,0b000,0b111,0b000,0b000}},
    {'.', {0b000,0b000,0b000,0b000,0b010}},
    {'/', {0b001,0b001,0b010,0b100,0b100}},
    {':', {0b000,0b010,0b000,0b010,0b000}},
    {';', {0b000,0b010,0b000,0b010,0b100}},
    {'?', {0b111,0b001,0b011,0b000,0b010}},

    // digits
    {'0', {0b111,0b101,0b101,0b101,0b111}},
    {'1', {0b010,0b110,0b010,0b010,0b111}},
    {'2', {0b111,0b001,0b111,0b100,0b111}},
    {'3', {0b111,0b001,0b111,0b001,0b111}},
    {'4', {0b101,0b101,0b111,0b001,0b001}},
    {'5', {0b111,0b100,0b111,0b001,0b111}},
    {'6', {0b111,0b100,0b111,0b101,0b111}},
    {'7', {0b111,0b001,0b001,0b001,0b001}},
    {'8', {0b111,0b101,0b111,0b101,0b111}},
    {'9', {0b111,0b101,0b111,0b001,0b111}},

    // A–Z (uppercase)
    {'A', {0b010,0b101,0b111,0b101,0b101}},
    {'B', {0b110,0b101,0b110,0b101,0b110}},
    {'C', {0b011,0b100,0b100,0b100,0b011}},
    {'D', {0b110,0b101,0b101,0b101,0b110}},
    {'E', {0b111,0b100,0b110,0b100,0b111}},
    {'F', {0b111,0b100,0b110,0b100,0b100}},
    {'G', {0b011,0b100,0b101,0b101,0b011}},
    {'H', {0b101,0b101,0b111,0b101,0b101}},
    {'I', {0b111,0b010,0b010,0b010,0b111}},
    {'J', {0b111,0b001,0b001,0b101,0b111}},
    {'K', {0b101,0b101,0b110,0b101,0b101}},
    {'L', {0b100,0b100,0b100,0b100,0b111}},
    {'M', {0b101,0b111,0b111,0b101,0b101}},
    {'N', {0b101,0b111,0b111,0b111,0b101}},
    {'O', {0b111,0b101,0b101,0b101,0b111}},
    {'P', {0b111,0b101,0b111,0b100,0b100}},
    {'Q', {0b111,0b101,0b101,0b111,0b001}},
    {'R', {0b111,0b101,0b111,0b101,0b101}},
    {'S', {0b111,0b100,0b111,0b001,0b111}},
    {'T', {0b111,0b010,0b010,0b010,0b010}},
    {'U', {0b101,0b101,0b101,0b101,0b111}},
    {'V', {0b101,0b101,0b101,0b101,0b010}},
    {'W', {0b101,0b101,0b111,0b111,0b101}},
    {'X', {0b101,0b101,0b010,0b101,0b101}},
    {'Y', {0b101,0b101,0b111,0b010,0b010}},
    {'Z', {0b111,0b001,0b010,0b100,0b111}},
};



using Glyph8x8 = std::array<uint8_t, 8>;

static const std::unordered_map<char, Glyph8x8> FONT_8X8 = {
    {' ', {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
    {'!', {0x00,0x18,0x18,0x18,0x18,0x00,0x18,0x00}},
    {'"', {0x00,0x66,0x66,0x24,0x00,0x00,0x00,0x00}},
    {'#', {0x00,0x24,0x7E,0x24,0x24,0x7E,0x24,0x00}},
    {'$', {0x00,0x18,0x3E,0x50,0x3C,0x0A,0x7C,0x18}},
    {'%', {0x00,0x60,0x66,0x0C,0x18,0x30,0x66,0x06}},
    {'&', {0x00,0x3C,0x66,0x3C,0x6C,0x66,0x3C,0x00}},
    {'\'',{0x00,0x18,0x18,0x30,0x00,0x00,0x00,0x00}},
    {'(', {0x00,0x0C,0x18,0x30,0x30,0x18,0x0C,0x00}},
    {')', {0x00,0x30,0x18,0x0C,0x0C,0x18,0x30,0x00}},
    {'*', {0x00,0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00}},
    {'+', {0x00,0x00,0x18,0x18,0x7E,0x18,0x18,0x00}},
    {',', {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}},
    {'-', {0x00,0x00,0x00,0x00,0x7E,0x00,0x00,0x00}},
    {'.', {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}},
    {'/', {0x00,0x06,0x0C,0x18,0x30,0x60,0xC0,0x00}},
    {'0', {0x00,0x3C,0x66,0x6E,0x76,0x66,0x3C,0x00}},
    {'1', {0x00,0x18,0x38,0x18,0x18,0x18,0x7E,0x00}},
    {'2', {0x00,0x3C,0x66,0x06,0x1C,0x30,0x7E,0x00}},
    {'3', {0x00,0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C}},
    {'4', {0x00,0x0C,0x1C,0x3C,0x6C,0x7E,0x0C,0x0C}},
    {'5', {0x00,0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C}},
    {'6', {0x00,0x3C,0x60,0x60,0x7C,0x66,0x66,0x3C}},
    {'7', {0x00,0x7E,0x66,0x0C,0x18,0x30,0x30,0x00}},
    {'8', {0x00,0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C}},
    {'9', {0x00,0x3C,0x66,0x66,0x3E,0x06,0x0C,0x3C}},
    {':', {0x00,0x00,0x00,0x18,0x00,0x18,0x00,0x00}},
    {';', {0x00,0x00,0x00,0x18,0x00,0x18,0x30,0x00}},
    {'<', {0x00,0x0C,0x18,0x30,0x60,0x30,0x18,0x0C}},
    {'=', {0x00,0x00,0x7E,0x00,0x00,0x7E,0x00,0x00}},
    {'>', {0x00,0x30,0x18,0x0C,0x06,0x0C,0x18,0x30}},
    {'?', {0x00,0x3C,0x66,0x0C,0x18,0x18,0x00,0x18}},
    {'@', {0x00,0x3C,0x66,0x7E,0x7E,0x70,0x60,0x3C}},
    {'A', {0x00,0x18,0x3C,0x66,0x66,0x7E,0x66,0x66}},
    {'B', {0x00,0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C}},
    {'C', {0x00,0x3C,0x66,0x60,0x60,0x60,0x66,0x3C}},
    {'D', {0x00,0x78,0x6C,0x66,0x66,0x66,0x6C,0x78}},
    {'E', {0x00,0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E}},
    {'F', {0x00,0x7E,0x60,0x60,0x7C,0x60,0x60,0x60}},
    {'G', {0x00,0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C}},
    {'H', {0x00,0x66,0x66,0x66,0x7E,0x66,0x66,0x66}},
    {'I', {0x00,0x7E,0x18,0x18,0x18,0x18,0x18,0x7E}},
    {'J', {0x00,0x3E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38}},
    {'K', {0x00,0x66,0x6C,0x78,0x70,0x78,0x6C,0x66}},
    {'L', {0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x7E}},
    {'M', {0x00,0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6}},
    {'N', {0x00,0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6}},
    {'O', {0x00,0x3C,0x66,0x66,0x66,0x66,0x66,0x3C}},
    {'P', {0x00,0x7C,0x66,0x66,0x7C,0x60,0x60,0x60}},
    {'Q', {0x00,0x3C,0x66,0x66,0x66,0x6A,0x6C,0x3E}},
    {'R', {0x00,0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66}},
    {'S', {0x00,0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C}},
    {'T', {0x00,0x7E,0x7E,0x18,0x18,0x18,0x18,0x18}},
    {'U', {0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x3C}},
    {'V', {0x00,0x66,0x66,0x66,0x66,0x66,0x3C,0x18}},
    {'W', {0x00,0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6}},
    {'X', {0x00,0x66,0x66,0x3C,0x18,0x3C,0x66,0x66}},
    {'Y', {0x00,0x66,0x66,0x66,0x3C,0x18,0x18,0x18}},
    {'Z', {0x00,0x7E,0x0C,0x18,0x30,0x60,0x60,0x7E}},
    {'[', {0x00,0x3E,0x30,0x30,0x30,0x30,0x30,0x3E}},
    {'\\',{0x00,0xC0,0x60,0x30,0x18,0x0C,0x06,0x00}},
    {']', {0x00,0x3E,0x0C,0x0C,0x0C,0x0C,0x0C,0x3E}},
    {'^', {0x00,0x18,0x3C,0x66,0x00,0x00,0x00,0x00}},
    {'_', {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}},
    {'`', {0x00,0x30,0x18,0x0C,0x00,0x00,0x00,0x00}},
    {'a', {0x00,0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E}},
    {'b', {0x00,0x60,0x60,0x7C,0x66,0x66,0x66,0x7C}},
    {'c', {0x00,0x00,0x00,0x3C,0x60,0x60,0x60,0x3C}},
    {'d', {0x00,0x06,0x06,0x3E,0x66,0x66,0x66,0x3E}},
    {'e', {0x00,0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C}},
    {'f', {0x00,0x1C,0x36,0x30,0x7C,0x30,0x30,0x30}},
    {'g', {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C}}, 
    {'h', {0x00,0x60,0x60,0x7C,0x66,0x66,0x66,0x66}},
    {'i', {0x00,0x00,0x18,0x00,0x38,0x18,0x18,0x3C}},
    {'j', {0x00,0x0C,0x00,0x0C,0x0C,0x0C,0x6C,0x38}},
    {'k', {0x00,0x60,0x60,0x6C,0x78,0x70,0x78,0x6C}},
    {'l', {0x00,0x38,0x18,0x18,0x18,0x18,0x18,0x3C}},
    {'m', {0x00,0x00,0x00,0xEC,0xFE,0xD6,0xD6,0xC6}},
    {'n', {0x00,0x00,0x00,0x7C,0x66,0x66,0x66,0x66}},
    {'o', {0x00,0x00,0x00,0x3C,0x66,0x66,0x66,0x3C}},
    {'p', {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60}}, 
    {'q', {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x0E}},
    {'r', {0x00,0x00,0x00,0x7C,0x66,0x60,0x60,0x60}},
    {'s', {0x00,0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C}},
    {'t', {0x00,0x18,0x18,0x7E,0x18,0x18,0x1C,0x0C}},
    {'u', {0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x3E}},
    {'v', {0x00,0x00,0x00,0x66,0x66,0x66,0x3C,0x18}},
    {'w', {0x00,0x00,0x00,0xC6,0xC6,0xD6,0xFE,0xEE}},
    {'x', {0x00,0x00,0x00,0x66,0x3C,0x18,0x3C,0x66}},
    {'y', {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x3C}},
    {'z', {0x00,0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E}},
    {'{', {0x00,0x0E,0x18,0x18,0x70,0x18,0x18,0x0E}},
    {'|', {0x00,0x18,0x18,0x18,0x00,0x18,0x18,0x18}},
    {'}', {0x00,0x70,0x18,0x18,0x0E,0x18,0x18,0x70}},
    {'~', {0x00,0x00,0x31,0x6B,0x00,0x00,0x00,0x00}},
};



static std::string asciiize( const std::string &s ) {
    std::string out; out.reserve( s.size() );
    for (size_t i = 0; i < s.size(); )
    {
        unsigned char box = static_cast<unsigned char>( s[ i ] );

        if (box == 0xE2 && i + 2 < s.size())
        {
            unsigned char b1 = static_cast<unsigned char>(s[ i + 1 ]);
            unsigned char b2 = static_cast<unsigned char>(s[ i + 2 ]);
            // en dash (–) or em dash (—)
            if (b1 == 0x80 && (b2 == 0x93 || b2 == 0x94))
            {
                out.push_back( '-' ); i += 3; continue;
            }
            // left/right single quote
            if (b1 == 0x80 && (b2 == 0x98 || b2 == 0x99))
            {
                out.push_back( '\'' ); i += 3; continue;
            }
            // left/right double quote 
            if (b1 == 0x80 && (b2 == 0x9C || b2 == 0x9D))
            {
                out.push_back( '"' ); i += 3; continue;
            }
            // bullet 
            if (b1 == 0x80 && b2 == 0xA2)
            {
                out.push_back( '*' ); i += 3; continue;
            }
        }
        if (box == 0xC2 && i + 1 < s.size())
        {
            unsigned char b1 = static_cast<unsigned char>(s[ i + 1 ]);
            // degree symbol 
            if (b1 == 0xB0)
            {
                out.push_back( 'o' ); i += 2; continue;
            }
        }

        if (box >= 32 && box <= 126)
        {
            out.push_back( static_cast<char>(box) );
        }
        else
        {
            out.push_back( ' ' );
        }
        ++i;
    }
    return out;
}

static void drawChar8x8( Engine &engineContext, int x, int y, char c, Uint32 color, Uint32 bgColor = 0, bool transparentBg = true ) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 32); // Uppercase

    auto it = FONT_8X8.find( c );
    if (it == FONT_8X8.end()) it = FONT_8X8.find( '?' );
    const Glyph8x8 &g = (it != FONT_8X8.end()) ? it->second : FONT_8X8.at( ' ' );

    for (int row = 0; row < 8; ++row)
    {
        uint8_t mask = g[ row ];
        for (int bit = 0; bit < 8; ++bit)
        {
            if (mask & (1 << (7 - bit)))
            {
                putPix( engineContext, x + bit, y + row, color );
            }
            else if (!transparentBg)
            {
                putPix( engineContext, x + bit, y + row, bgColor );
            }
        }
    }
}


static void drawString8x8( Engine &engineContext,
    int x, int y,
    const std::string &text,
    Uint32 color,
    int wrapWidth = RENDER_W, // Max width in pixels
    int letterSpacing = 1,
    int lineSpacing = 2,
    bool dropShadow = false,
    Uint32 shadowColor = 0 ) {
    std::string t = asciiize( text );

    const int charW = 10;
    const int charH = 10;
    const int advX = charW + letterSpacing;  
    const int advY = charH + lineSpacing;    

    int cx = x, cy = y;
    int rightLimit = std::min( x + wrapWidth, RENDER_W ) - charW;

    for (size_t i = 0; i < t.length(); ++i)
    {
        char c = t[ i ];

        if (c == '\n')
        {
            cx = x;
            cy += advY;
            continue;
        }

        if (c != ' ' && cx > rightLimit) // We're over the edge
        {
            // Look backward to find the start of the current word
            size_t startOfWord = i;
            while (startOfWord > 0 && t[ startOfWord - 1 ] != ' ' && t[ startOfWord - 1 ] != '\n')
            {
                startOfWord--;
            }

            if (startOfWord > 0) // Check if we're not at the very beginning
            {
            
                size_t prevSpace = t.rfind( ' ', i );
                if (prevSpace != std::string::npos && prevSpace < i)
                {
                    int wordLen = (int)i - (int)prevSpace;
                    if (cx - (wordLen * advX) > x)
                    {
                        // This word fits on the next line
                        cx = x;
                        cy += advY;
                        i = prevSpace; 
                        continue;
                    }
                }
            }

            // Just force a wrap.
            cx = x;
            cy += advY;
        }

        if (c == ' ')
        {
            cx += advX;
            continue;
        }

        if (dropShadow)
        {
            drawChar8x8( engineContext, cx + 1, cy + 1, c, shadowColor, 0, true );
        }
        drawChar8x8( engineContext, cx, cy, c, color, 0, true );
        cx += advX;
    }
}


static void drawGlyphTinyScaled( Engine &engineContext, int x, int y, const Glyph &g, Uint32 color, int scale ) {
    if (scale <= 0) return;
    for (int row = 0; row < 5; ++row)
    {
        uint8_t mask = g[ row ];
        for (int bit = 0; bit < 3; ++bit)
        {
            if (mask & (1 << (2 - bit)))
            {
                // scale the single pixel into a scale x scale block
                int px = x + bit * scale;
                int py = y + row * scale;
                for (int dy = 0; dy < scale; ++dy)
                {
                    Uint32 *dst = &engineContext.backbuffer[ (py + dy) * RENDER_W + px ];
                    for (int dx = 0; dx < scale; ++dx) dst[ dx ] = color;
                }
            }
        }
    }
}



static void drawCharTinyScaled( Engine &engineContext, int x, int y, char c, Uint32 color, int scale ) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 32);
    auto it = TINY_GLYPHS.find( c );
    if (it == TINY_GLYPHS.end()) it = TINY_GLYPHS.find( '?' );
    const Glyph &g = (it != TINY_GLYPHS.end()) ? it->second : TINY_GLYPHS.at( ' ' );
    drawGlyphTinyScaled( engineContext, x, y, g, color, scale );
}

static void drawStringTinyScaled( Engine &engineContext,
    int x, int y,
    const std::string &text,
    Uint32 color,
    int scale,
    int letterSpacing = 1,
    int lineSpacing = 1,
    bool dropShadow = true ) {
    std::string t = asciiize( text );

    const int charW = 3 * scale;
    const int charH = 5 * scale;
    const int advX = charW + letterSpacing;  // original was 3+1 = 4
    const int advY = charH + lineSpacing;    // original was 5+1 = 6

    int cx = x, cy = y;

    for (char c : t)
    {
        if (c == '\n' || cx > RENDER_W - advX)
        {
            // newline or wrap
            if (c == '\n')
            { /* keep */
            }
            cx = x;
            cy += advY;
            if (c == '\n') continue;
        }

        if (dropShadow)
        {
            // subtle 1px*scale shadow for readability
            drawCharTinyScaled( engineContext, cx + scale, cy + scale, c, rgb( 0, 0, 0 ), scale );
        }
        drawCharTinyScaled( engineContext, cx, cy, c, color, scale );
        cx += advX;
    }
}


static void drawCharTiny( Engine &engineContext, int x, int y, char c, Uint32 color ) {
    // Uppercase a–z to A–Z
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 32);

    auto it = TINY_GLYPHS.find( c );
    if (it == TINY_GLYPHS.end())
    {
        it = TINY_GLYPHS.find( '?' );
    }

    const Glyph &g = (it != TINY_GLYPHS.end()) ? it->second : TINY_GLYPHS.at( ' ' );
    for (int row = 0; row < 5; ++row)
    {
        uint8_t mask = g[ row ];
        for (int bit = 0; bit < 3; ++bit)
        {
            if (mask & (1 << (2 - bit)))
            {
                putPix( engineContext, x + bit, y + row, color );
            }
        }
    }
}

static void drawStringTiny( Engine &engineContext, int x, int y, const std::string &text, Uint32 color ) {
    std::string t = asciiize( text );       // normalize to ASCII
    int centerX = x;
    int centerY = y;
    for (char c : t)
    {
        if (c == '\n')
        {
            centerY += 6; 
            centerX = x; 
            continue;
        }
        drawCharTiny( engineContext, centerX, centerY, c, color );
        centerX += 4;                       // advance
        if (centerX > RENDER_W - 4)
        {
            centerY += 6; 
            centerX = x;
        }
    }
}

static bool toggleDoorAhead( Engine &engineContext) {
    float reach = 1.5f; // about 0.8 tiles ahead
    int tx = int( engineContext.positionX + engineContext.directionX * reach );
    int ty = int( engineContext.positionY + engineContext.directionY * reach );
    if (tx < 0 || ty < 0 || tx >= engineContext.map.width || ty >= engineContext.map.height) return false;
    int &cell = engineContext.map.tiles[ ty * engineContext.map.width + tx ];
    if (cell == 2)
    {
        cell = 0; return true;
    }      // open (becomes empty)
    if (cell == 0)
    {                               
        bool canClose = false;
        if (tx > 0 && tx < engineContext.map.width - 1)
        {
            int L = engineContext.map.tiles[ ty * engineContext.map.width + (tx - 1) ];
            int R = engineContext.map.tiles[ ty * engineContext.map.width + (tx + 1) ];
            if ((L == 1 && R == 1) || (L == 2 && R == 2)) canClose = true;
        }
        if (ty > 0 && ty < engineContext.map.height - 1)
        {
            int U = engineContext.map.tiles[ (ty - 1) * engineContext.map.width + tx ];
            int D = engineContext.map.tiles[ (ty + 1) * engineContext.map.width + tx ];
            if ((U == 1 && D == 1) || (U == 2 && D == 2)) canClose = true;
        }
        if (canClose)
        {
            cell = 2; return true;
        }
    }
    return false;
}


inline float cross2( float ax, float ay, float bx, float by ) {
    return ax * by - ay * bx;
}

// Solve p + t*r = a + u*s  for intersection of ray (p,r) with segment [a,box].
// Thanks stack overflow!
inline bool ray_segment_intersect( float px, float py,
    float rdx, float rdy,
    float ax, float ay,
    float bx, float by,
    float &tOut, float &uOut ) {
    const float sx = bx - ax, sy = by - ay;
    const float rxs = cross2( rdx, rdy, sx, sy );
    if (std::fabs( rxs ) < 1e-8f) return false; // parallel

    const float apx = ax - px, apy = ay - py;
    const float t = cross2( apx, apy, sx, sy ) / rxs;
    const float u = cross2( apx, apy, rdx, rdy ) / rxs;

    if (t > 0.f && u >= 0.f && u <= 1.f)
    {
        tOut = t; uOut = u; return true;
    }
    return false;
}


inline bool is_key( Uint32 c ) {
    return (((c >> 16) & 255) == 255 && ((c >> 8) & 255) == 0 && (c & 255) == 255);
}
inline float byte_to_f( Uint32 v ) {
    return float( v ) / 255.0f;
}
inline Uint8  f_to_byte( float f ) {
    return Uint8( std::clamp( f, 0.0f, 1.0f ) * 255.0f + 0.5f );
}

inline bool isNearMagenta( Uint32 color, int tol = 12 ) {
    Uint8 r = (color >> 16) & 255;
    Uint8 g = (color >> 8) & 255;
    Uint8 box = color & 255;
    return (abs( int( r ) - 255 ) <= tol) && (g <= tol) && (abs( int( box ) - 255 ) <= tol);
}

// Sample using normalized UV in [0,1]. If you pass values outside, they clamp.
// Again thanks stack overflow
inline Uint32 sample_bilinear_uv_keyed( const Image &texture, float u, float v ) {
    if (texture.width == 0 || texture.height == 0) return 0;

    // Clamp UVs (change to wrap if you prefer tiling)
    u = std::clamp( u, 0.0f, 1.0f );
    v = std::clamp( v, 0.0f, 1.0f );

    float fx = u * (texture.width - 1);
    float fy = v * (texture.height - 1);

    int x0 = int( fx ), y0 = int( fy );
    int x1 = std::min( x0 + 1, texture.width - 1 );
    int y1 = std::min( y0 + 1, texture.height - 1 );
    float tx = fx - x0, ty = fy - y0;

    Uint32 c00 = texture.sample( x0, y0 );
    Uint32 c10 = texture.sample( x1, y0 );
    Uint32 c01 = texture.sample( x0, y1 );
    Uint32 c11 = texture.sample( x1, y1 );

    // Treat magenta as alpha=0, everything else alpha=1
    float a00 = is_key( c00 ) ? 0.0f : 1.0f;
    float a10 = is_key( c10 ) ? 0.0f : 1.0f;
    float a01 = is_key( c01 ) ? 0.0f : 1.0f;
    float a11 = is_key( c11 ) ? 0.0f : 1.0f;

    auto comp = [&]( int shift )->Uint8 {
        float v00 = byte_to_f( (c00 >> shift) & 255 ) * a00;
        float v10 = byte_to_f( (c10 >> shift) & 255 ) * a10;
        float v01 = byte_to_f( (c01 >> shift) & 255 ) * a01;
        float v11 = byte_to_f( (c11 >> shift) & 255 ) * a11;

        // bilinear on premultiplied channels
        float v0 = v00 * (1.0f - tx) + v10 * tx;
        float v1 = v01 * (1.0f - tx) + v11 * tx;
        float vp = v0 * (1.0f - ty) + v1 * ty;

        // bilinear on alpha, then unpremultiply (avoid division by tiny)
        float a0 = a00 * (1.0f - tx) + a10 * tx;
        float a1 = a01 * (1.0f - tx) + a11 * tx;
        float ap = a0 * (1.0f - ty) + a1 * ty;

        float out = (ap > 1e-5f) ? (vp / ap) : 0.0f;
        return f_to_byte( out );
        };

    // Output stays opaque here; caller can still treat key as transparent if desired.
    Uint8 r = comp( 16 ), g = comp( 8 ), box = comp( 0 );

    // If blended alpha is ~0, return the key color so your existing checks skip it.
    float a0 = a00 * (1.0f - tx) + a10 * tx;
    float a1 = a01 * (1.0f - tx) + a11 * tx;
    float ap = a0 * (1.0f - ty) + a1 * ty;
    if (ap <= 1e-5f) return rgb( 255, 0, 255 );

    return rgb( r, g, box );
}


inline void draw_vertical_face( Engine &engineContext, float ax, float ay, float bx, float by, float height,   const Image &texture ) {
    // Transform endpoints to camera space
    auto to_cam = [&]( float wx, float wy ) {
        float dx = wx - engineContext.positionX, dy = wy - engineContext.positionY;
        float invDet = 1.0f / (engineContext.planeX * engineContext.directionY - engineContext.directionX * engineContext.planeY);
        float centerX = invDet * (engineContext.directionY * dx - engineContext.directionX * dy);  // right (+) left (-)
        float centerY = invDet * (-engineContext.planeY * dx + engineContext.planeX * dy);  // forward (+)
        return std::array<float, 2>{centerX, centerY};
        };

    std::array<float, 2> A = to_cam( ax, ay );
    std::array<float, 2> B = to_cam( bx, by );

    // Clip segment to the near plane in camera space (centerY > near)
    const float NEAR_Z = 0.05f;

    // If both behind camera, drop
    if (A[ 1 ] <= NEAR_Z && B[ 1 ] <= NEAR_Z) return;

    auto lerp = []( float a, float box, float t ) { return a + (box - a) * t; };

    // If one endpoint is behind, clip it to near
    auto clip_to_near = [&]( std::array<float, 2> &P, const std::array<float, 2> &Q ) {
        // Find t where centerY == NEAR_Z between P (behind) and Q (in front)
        float t = (NEAR_Z - P[ 1 ]) / (Q[ 1 ] - P[ 1 ]);
        P[ 0 ] = lerp( P[ 0 ], Q[ 0 ], t );
        P[ 1 ] = NEAR_Z;
        };

    std::array<float, 2> A0 = A, B0 = B; // keep originals for u
    float segLen = std::sqrt( (bx - ax) * (bx - ax) + (by - ay) * (by - ay) );
    if (segLen < 1e-6f) return;

    if (A[ 1 ] < NEAR_Z && B[ 1 ] > NEAR_Z) clip_to_near( A, B );
    else if (B[ 1 ] < NEAR_Z && A[ 1 ] > NEAR_Z) clip_to_near( B, A );

    // Project to screen X
    auto to_screen_x = [&]( const std::array<float, 2> &P ) {
        return int( (RENDER_W * 0.5f) * (1.0f + P[ 0 ] / P[ 1 ]) );
        };
    int x0 = to_screen_x( A ), x1 = to_screen_x( B );
    if (x0 == x1) return;
    if (x0 > x1)
    {
        std::swap( x0, x1 ); std::swap( A, B ); std::swap( A0, B0 );
    }

    // Prepare perspective-correct interpolation:
    // We'll interpolate q = 1/z and u*q across the screen span.
    // u is along the segment A->B in world space.
    auto cam_to_world_u = [&]( float centerX, float centerY ) {
        // Find world point parameter u via world-space segment parameterization
        // Using original unclipped endpoints A0,B0 and world endpoints ax,ay..bx,by.
        // Reconstruct world position by inverting the camera transform is overkill;
        // we can just compute u by projecting onto the world segment axis using screen-space A/B ratios.
        // Simpler: compute u from the fraction along the *projected* segment:
        // This is affine in screen space; to be more accurate we do perspective correction below anyway.
        (void)centerX; (void)centerY; // not used here (we'll build u from x interpolation)
        return 0.0f;
        };

    // Precompute for endpoints:
    float q0 = 1.0f / std::max( A[ 1 ], NEAR_Z );
    float q1 = 1.0f / std::max( B[ 1 ], NEAR_Z );

    // texture u at endpoints in world space (0 at A0, 1 at B0)
    // If x0/x1 got swapped we already swapped A0/B0 with A/B.
    // So define u0=0, u1=1 consistently with current A->B screen order.
    float u0 = 0.0f, u1 = 1.0f;

    // We'll interpolate uq = u * q (for perspective correct u)
    // across screen X from x0..x1.
    int xBeg = std::max( 0, x0 );
    int xEnd = std::min( RENDER_W - 1, x1 );
    if (xBeg > xEnd) return;

    for (int x = xBeg; x <= xEnd; ++x)
    {
        // Barycentric t in screen space
        float t = (x1 == x0) ? 0.0f : ((x - x0) / float( x1 - x0 ));

        // Perspective correct depth
        float q = lerp( q0, q1, t );
        float z = 1.0f / q;

        // Depth test vs walls
        if (z >= engineContext.zbuffer[ x ]) continue;

        // Perspective-correct u
        float uq0 = u0 * q0, uq1 = u1 * q1;
        float uq = lerp( uq0, uq1, t );
        float u = std::clamp( uq / q, 0.0f, 1.0f );

        // Column height for world height = 1
        int unitH = int( RENDER_H / z );
        // Face occupies "height * unitH" pixels, bottom sits at floor line
        int faceH = std::max( 1, int( height * unitH ) );
        int bottom = std::min( RENDER_H - 1, RENDER_H / 2 + unitH / 2 );
        int top = std::max( 0, bottom - faceH );
        if (bottom <= top) continue;

        // Texture x from u
        int textureX = std::clamp( int( u * (texture.width - 1) ), 0, texture.width - 1 );

        // Simple distance shading
        float shade = std::clamp( 1.0f / (0.35f * z), 0.25f, 1.0f );

        // Draw column
        int span = std::max( 1, bottom - top );
        for (int y = top; y <= bottom; ++y)
        {
            float v = (y - top) / float( span );
            int textureY = std::clamp( int( v * (texture.height - 1) ), 0, texture.height - 1 );
            Uint32 c = texture.sample( textureX, textureY );
            // magenta transparent
            if (((c >> 16) & 255) == 255 && ((c >> 8) & 255) == 0 && (c & 255) == 255) continue;

            Uint8 rr = Uint8( ((c >> 16) & 255) * shade );
            Uint8 gg = Uint8( ((c >> 8) & 255) * shade );
            Uint8 bb = Uint8( (c & 255) * shade );
            putPix( engineContext, x, y, rgb( rr, gg, bb ) );
        }
    }
}


inline void box_corners( const BoxProp &box, float &x0, float &y0, float &x1, float &y1, float &x2, float &y2, float &x3, float &y3 ) {
    const float c = std::cos( box.angle ), s = std::sin( box.angle );
    // local axes
    const float ux = box.halfLength * c, uy = box.halfLength * s;
    const float vx = -box.halfDepth * s, vy = box.halfDepth * c;

    // CCW rectangle (0..3)
    x0 = box.centerX - ux - vx; 
    y0 = box.centerY - uy - vy;
    x1 = box.centerX + ux - vx; 
    y1 = box.centerY + uy - vy;
    x2 = box.centerX + ux + vx; 
    y2 = box.centerY + uy + vy;
    x3 = box.centerX - ux + vx; 
    y3 = box.centerY - uy + vy;
}

inline void render_box( Engine &engineContext, const BoxProp &box ) {
    float x0, y0, x1, y1, x2, y2, x3, y3;
    box_corners( box, x0, y0, x1, y1, x2, y2, x3, y3 );

    const Image &texture = box.sideTexure;

    // Four faces around the seat (0-1, 1-2, 2-3, 3-0)
    draw_vertical_face( engineContext, x0, y0, x1, y1, box.height, texture );
    draw_vertical_face( engineContext, x1, y1, x2, y2, box.height, texture );
    draw_vertical_face( engineContext, x2, y2, x3, y3, box.height, texture );
    draw_vertical_face( engineContext, x3, y3, x0, y0, box.height, texture );
}

inline void render_legs( Engine &engineContext, const BoxProp &box ) {
    const float c = std::cos( box.angle );
    const float s = std::sin( box.angle );
    const float ux = box.halfLength * c, uy = box.halfLength * s;
    const float vx = -box.halfDepth * s, vy = box.halfDepth * c;

    const float insetU = std::max( 0.f, box.halfLength - box.legInsetLength );
    const float insetV = std::max( 0.f, box.halfDepth - box.legInsetDepth );

    // leg centers (four corners, inset)
    struct P
    {
        float x, y;
    };
    P centers[ 4 ] = {
        { box.centerX - insetU * c - insetV * (-s), box.centerY - insetU * s - insetV * (c) }, // near-left
        { box.centerX + insetU * c - insetV * (-s), box.centerY + insetU * s - insetV * (c) }, // near-right
        { box.centerX + insetU * c + insetV * (-s), box.centerY + insetU * s + insetV * (c) }, // far-right
        { box.centerX - insetU * c + insetV * (-s), box.centerY - insetU * s + insetV * (c) }  // far-left
    };

    const float width = box.legHalf; // half width (square leg)
    const Image &texture = (box.legTexure.width > 0 && box.legTexure.height > 0) ? box.legTexure : box.sideTexure;

    for (int i = 0; i < 4; ++i)
    {
        // A leg is a tiny axis-aligned (by bench) box around centers[i].
        // Build its 4 side faces in world space (just like seat, with much smaller halfLength/halfDepth).
        BoxProp leg;
        leg.centerX = centers[ i ].x; leg.centerY = centers[ i ].y;
        leg.halfLength = width; leg.halfDepth = width;
        leg.height = box.height;    // full height to floor
        leg.angle = box.angle;
        leg.sideTexure = texture;

        float x0, y0, x1, y1, x2, y2, x3, y3;
        box_corners( leg, x0, y0, x1, y1, x2, y2, x3, y3 );

        draw_vertical_face( engineContext, x0, y0, x1, y1, leg.height, texture );
        draw_vertical_face( engineContext, x1, y1, x2, y2, leg.height, texture );
        draw_vertical_face( engineContext, x2, y2, x3, y3, leg.height, texture );
        draw_vertical_face( engineContext, x3, y3, x0, y0, leg.height, texture );
    }
}

inline void render_box_top( Engine &engineContext, const BoxProp &box, const Image &texture ) {
    const int half = RENDER_H / 2;
    const float posZ = 0.5f * RENDER_H;
    const float camZ = 0.5f;

    // Tiny lowering so the cap matches the side tops in screen space
    const float heightBias = 0.003f;                 
    const float planeZ = std::clamp( box.height - heightBias, 0.0f, camZ - 1e-4f );

    const float alpha = (camZ - planeZ) / camZ;
    if (alpha <= 0.0f) return;

    // Bench local half-axes (world space vectors)
    const float c = std::cos( box.angle ), s = std::sin( box.angle );
    const float ux = box.halfLength * c, uy = box.halfLength * s;
    const float vx = -box.halfDepth * s, vy = box.halfDepth * c;

    // edge rays 
    const float rdx0 = engineContext.directionX - engineContext.planeX, rdy0 = engineContext.directionY - engineContext.planeY;
    const float rdx1 = engineContext.directionX + engineContext.planeX, rdy1 = engineContext.directionY + engineContext.planeY;

    for (int y = half + 1; y < RENDER_H; ++y)
    {
        const int   p = y - half;
        const float baseRowDist = posZ / float( p );
        const float rowDist = baseRowDist * alpha;

        float stepX = rowDist * (rdx1 - rdx0) / float( RENDER_W );
        float stepY = rowDist * (rdy1 - rdy0) / float( RENDER_W );
        float worldX = engineContext.positionX + rowDist * rdx0;
        float worldY = engineContext.positionY + rowDist * rdy0;

        for (int x = 0; x < RENDER_W; ++x)
        {
            if (rowDist < engineContext.zbuffer[ x ])
            {
                const float dx = worldX - box.centerX, dy = worldY - box.centerY;

                const float invLL = 1.0f / (box.halfLength * box.halfLength);
                const float invDD = 1.0f / (box.halfDepth * box.halfDepth);
                float du = (dx * ux + dy * uy) * invLL;   // [-1,1] ideally
                float dv = (dx * vx + dy * vy) * invDD;

                const float insideEps = 1.001f;          // ~0.1% leniency
                if (std::fabs( du ) <= insideEps && std::fabs( dv ) <= insideEps)
                {
                    // so bilinear can pick the border texels
                    const float invSpan = 0.5f * (1.0f / insideEps);
                    float u = du * invSpan + 0.5f;
                    float v = dv * invSpan + 0.5f;

                    Uint32 color = sample_bilinear_uv_keyed( texture, u, v );
                    if (!(((color >> 16) & 255) == 255 && ((color >> 8) & 255) == 0 && (color & 255) == 255))
                    {
                        float sh = std::clamp( 1.0f / (0.02f * rowDist), 0.30f, 1.0f );
                        Uint8 r = Uint8( ((color >> 16) & 255) * sh );
                        Uint8 g = Uint8( ((color >> 8) & 255) * sh );
                        Uint8 bcol = Uint8( (color & 255) * sh );
                        putPix( engineContext, x, y, rgb( r, g, bcol ) );
                    }
                }
            }
            worldX += stepX; 
            worldY += stepY;
        }
    }
}
