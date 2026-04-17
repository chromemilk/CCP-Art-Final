#include "GameEngine.h"
#include "RendererHelpers.h"
#include "PhysicsHelpers.h"
#include "MusicSystem.h"
#include <iostream>
#include <filesystem> 
#include <thread>
#include <array>
#include <memory>
#include <limits>
#include <functional>
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
};

struct KeyPickup
{
    std::string keyName;
    float x = 0.f;
    float y = 0.f;
    bool collected = false;
    int propIndex = -1;
    int modelIndex = -1;
};

struct CaveQuizQuestion
{
    std::string question;
    std::array<std::string, 4> options;
    int correctOption = 0;
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
static bool g_caveFinalNoteCollected = false;
static int g_notesCollectedRun = 0;
static float g_runElapsedSeconds = 0.0f;
static bool g_caveQuizActive = false;
static bool g_caveQuizPassed = false;
static int g_caveQuizQuestionIndex = 0;
static std::vector<CaveQuizQuestion> g_caveQuiz;
static bool g_museumPuzzleInitialized = false;

struct CpuModel
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec2> uvs;
    std::vector<uint32_t> indices;
    std::vector<Image> baseColorTextures;
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
};

static std::unordered_map<std::string, std::shared_ptr<CpuModel>> g_cpuModelCache;
static std::vector<WorldModelInstance> g_worldModels;

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

static bool isPlayerNearPoint( Engine const &engineContext, float px, float py, float tolerance = 1.0f ) {
    float dx = engineContext.positionX - px;
    float dy = engineContext.positionY - py;
    return (dx * dx + dy * dy) <= (tolerance * tolerance);
}

static bool isMuseumLikeLevel( Levels level ) {
    return level == Levels::MUSEUM || level == Levels::MUSEUM_UPPER;
}

static void beginLevelTransition( Levels target, float seconds = 1.05f ) {
    g_levelTransition.active = true;
    g_levelTransition.switched = false;
    g_levelTransition.t = 0.0f;
    g_levelTransition.duration = std::max( 0.2f, seconds );
    g_levelTransition.targetLevel = target;
}

static void triggerInteractionAnim( InteractionAnimType type, const std::string &label, float seconds = 0.55f ) {
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
    std::string header = "WARDEN STATUE TRIAL " + std::to_string( g_caveQuizQuestionIndex + 1 ) + "/" + std::to_string( g_caveQuiz.size() );
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

    drawStringTinyScaled( engineContext, x + 16, y + panelH - 22, "PRESS 1-4 TO ANSWER   ESC TO CANCEL", rgb( 130, 130, 145 ), 1, 1, 1, false );
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
            drawStringTinyScaled( engineContext, x + w - 176, y + h - 16, "TURNING...", rgb( 220, 190, 120 ), 1, 1, 1, false );
        }
    }
    else if (g_interactionAnim.type == InteractionAnimType::NOTE_COLLECT)
    {
        int nx = x + 40;
        int ny = stageY + 6;
        drawTextBox( engineContext, nx, ny, 72, 40, rgb( 210, 198, 164 ), rgb( 120, 96, 64 ) );
        drawStringTinyScaled( engineContext, nx + 10, ny + 12, "NOTE", rgb( 70, 55, 36 ), 1, 1, 1, false );
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

static ClueNote makeClueNote( Engine &engineContext, const std::string &title, const std::string &body, float x, float y ) {
    ClueNote note;
    note.title = title;
    note.body = body;
    note.x = x;
    note.y = y;
    note.collected = false;

    NotePickupVisual vis = addNotePickupModel( engineContext, x, y, title );
    note.propIndex = vis.propIndex;
    note.modelIndex = vis.modelIndex;
    return note;
}

static KeyPickup addKeyPickupModelProxy( Engine &engineContext, const std::string &keyName, float x, float y, Uint32 keyColor, const std::string &modelAsset ) {
    int spriteIndex = addKeyPickupSprite( engineContext, x, y, keyName, keyColor );
    if (spriteIndex >= 0 && spriteIndex < (int)engineContext.props.size())
    {
        engineContext.props[ spriteIndex ].scale = 0.0f;
    }

    int modelIndex = addWorldModelInstance(
        resolveAssetModelPath( modelAsset ),
        x,
        y,
        0.17f,
        keyColor,
        0.0f,
        0.0f,
        -1.5707963f,
        true,
        1.2f,
        0.2f);

    KeyPickup out;
    out.keyName = keyName;
    out.x = x;
    out.y = y;
    out.collected = false;
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

static void initMuseumPuzzle( Engine &engineContext ) {
    g_roomLocks = {
        // Doors blocking the main 4 wings:
        {6, 9, "West Wing", LockType::KEY, "BRONZE KEY", false},
        {10, 6, "North Wing", LockType::CODE, "0300", false},
        {16, 9, "East Wing", LockType::KEY, "GOLD KEY", false},
        {10, 12, "South Wing", LockType::CODE, "7391", false},
        // Doors blocking the new 4 corner rooms:
        {5, 2, "NW Archives", LockType::KEY, "BRONZE KEY", false}, // From NW
        {14, 3, "NE Vault", LockType::KEY, "IRON KEY", false}, // From NW
        {5, 15, "SW Crypt", LockType::KEY, "SILVER KEY", false}, // From SW
        {17, 13, "SE Office", LockType::KEY, "BRONZE KEY", false} // From East
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

    if (!g_stairWallOverlayReady)
    {
        buildStairWallOverlay( g_stairWallOverlay );
        g_stairWallOverlayReady = true;
    }

    g_keyPickups.clear();
    // Bronze Key in main atrium start
    g_keyPickups.push_back( addKeyPickupModelProxy( engineContext, "BRONZE KEY", 8.1f, 7.7f, rgb( 180, 120, 40 ), "Bronze Key.glb" ) );
    // Silver Key in North Wing
    g_keyPickups.push_back( addKeyPickupModelProxy( engineContext, "SILVER KEY", 10.5f, 3.5f, rgb( 190, 190, 200 ), "Silver Key.glb" ) );
    // Fallback Gold Key in North Wing so progression cannot dead-end
   // g_keyPickups.push_back( {"GOLD KEY", 12.5f, 2.5f, false, addKeyPickupSprite( engineContext, 12.5f, 2.5f, "GOLD KEY", rgb( 255, 215, 0 ) )} );

    g_safes.clear();
    g_safeBoxIndices.clear();
    // Safe in SE Office
    g_safes.push_back({"Director's Safe", "2026", 18.7f, 16.7f, false, "GOLD KEY"});
    addSafe3D( engineContext, 18.7f, 16.7f);

    g_symbols.clear();
    g_pedestalBoxIndices.clear();
    // Pedestal in NW Archives
    g_symbols.push_back({"Ancient Pedestal", {1, 3, 0}, 3.5f, 3.5f, false, "IRON KEY"}); // WOLF(1) SERPENT(3) OWL(0)
    addPedestal3D( engineContext, 3.5f, 3.5f );

    // Director room furnishing + decor models
    addWorldModelInstance( resolveAssetModelPath( "Full Desk.glb" ), 16.36f, 16.55f, 0.8f, rgb( 170, 150, 130 ), 3.1415926f, 0, 0 , false, 0, -0.05f);
    addWorldModelInstance( resolveAssetModelPath( "Shelf.glb" ), 18.6f, 14.2f, 0.8f, rgb( 170, 160, 140 ), -1.5707963f, 0, 0, false, 0, -0.05f);
  //  addWorldModelInstance( resolveAssetModelPath( "Note.glb" ), 18.24f, 14.48f, 0.14f, rgb( 230, 218, 184 ), -1.5707963f, -1.5707963f );
  //  addWorldModelInstance( resolveAssetModelPath( "Note.glb" ), 18.38f, 14.52f, 0.13f, rgb( 228, 216, 180 ), -1.5707963f, -1.5707963f );
    addWorldModelInstance(resolveAssetModelPath("Couch.glb"), 17.5f, 16.7f, 0.8f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);
    addWorldModelInstance(resolveAssetModelPath("Boxes.glb"), 16.2f, 15.3f, 0.8f, rgb(184, 130, 98), -1.5707963f, 0, 0, false, 0, -0.05f);
    addWorldModelInstance(resolveAssetModelPath("Whiteboard.glb"), 17.5f, 17.f, 0.8f, rgb(116, 101, 60), -1.5707963, 0, 1.5707963, false, 0, 0.45f);
    addWorldModelInstance(resolveAssetModelPath("Refrigerator.glb"), 17.4f, 14.2f, 0.8f, rgb(116, 101, 60), 2.3415926, -0.03, 0, false, 0, -0.08f);
    addWorldModelInstance(resolveAssetModelPath("FileCabinet.glb"), 16.2f, 16.0f, 0.4f, rgb(69, 41, 34), 1.5707963f, 0, 0, false, 0, -0.05f);


    addWorldModelInstance(resolveAssetModelPath("SimplePillar.glb"), 8.5f, 8.5f, 1.1f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);
    addWorldModelInstance(resolveAssetModelPath("SimplePillar.glb"), 13.5f, 8.5f, 1.1f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);
    addWorldModelInstance(resolveAssetModelPath("SimplePillar.glb"), 8.5, 10.5, 1.1f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);
    addWorldModelInstance(resolveAssetModelPath("SimplePillar.glb"), 13.5, 10.5, 1.1f, rgb(116, 101, 60), 3.1415926, 0, 0, false, 0, -0.05f);

    // Scattered floor paper props (1-3)
    addWorldModelInstance( resolveAssetModelPath( "Scattered Paper.glb" ), 16.4f, 14.9f, 0.22f, rgb( 224, 214, 188 ), 0.45f, 0, 0, false, 0, -0.05f);
    addWorldModelInstance( resolveAssetModelPath( "Scattered Paper.glb" ), 17.1f, 14.4f, 0.20f, rgb( 220, 210, 182 ), -0.20f, 0, 0, false, 0, -0.05f);
    addWorldModelInstance( resolveAssetModelPath( "Scattered Paper.glb" ), 17.8f, 15.0f, 0.18f, rgb( 226, 216, 190 ), 0.95f, 0, 0, false, 0, -0.05f);
    g_clueNotes.clear();
    // Atrium note
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Janitor's Log",
        "Dropped the Bronze Key nearby. It unlocks the West Wing, NW Archives, and SE Office.",
        15.5f, 11.5f ) );
    // West Wing Note
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Archivist Notebook",
        "The NW Archives pedestal requires the predator, the deceiver, and the wise one.",
        3.5f, 8.5f ) );
    // West Wing progression note (guarantees early North Wing access)
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Security Log",
        "The North Wing lockdown code is the year of the four rulers. Do not forget it.",
        4.5f, 10.5f ) );
    // SW Crypt Note
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Director Memo",
        "The SE Office safe code is current year. It contains the Gold Key.",
        3.5f, 15.5f ) );
    // East Wing Note
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Final Code Clue",
        "The South Wing emergency code is 7391.",
        18.5f, 9.5f ) );
    // North Wing fallback note so South code is always obtainable
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Emergency Override Slip",
        "If wing routing fails, South Wing emergency code is 7391.",
        8.5f, 4.5f ) );
    // NE Vault lore note so the room is still meaningful after progression rebalance
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Vault Ledger",
        "Iron access approved. Reserve artifacts moved to East Wing transfer corridor.",
        17.5f, 2.5f ) );

    g_museumPuzzleInitialized = true;
}

static void initCaveQuiz() {
    g_caveQuiz.clear();
    g_caveQuiz.push_back( {
        "Which period is generally associated with Rembrandt's The Night Watch?",
        {"Baroque", "Neoclassical", "Medieval", "Romantic"},
        0
        } );
    g_caveQuiz.push_back( {
        "Roman portraiture is best known for emphasizing what?",
        {"Idealized perfection", "Abstract geometry", "Realistic likeness", "Pure symbolism"},
        2
        } );
    g_caveQuiz.push_back( {
        "Renaissance artists were strongly influenced by the revival of which cultures?",
        {"Mayan and Aztec", "Greek and Roman", "Norse and Celtic", "Persian and Mughal"},
        1
        } );
}

static void initCaveFinalObjective( Engine &engineContext ) {
    g_clueNotes.clear();
    g_foundNotes.clear();
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Camp Note: Warden Test",
        "The cave statue asks 3 questions: Baroque, Roman realism, and Renaissance revival.",
        2.8f, 2.3f ) );
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Visitor Memo",
        "Remember: Roman portrait busts focused on truthful features, not idealized beauty.",
        4.8f, 3.8f ) );
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Archivist Card",
        "Renaissance is the rebirth of Greek and Roman learning. Keep that for the final statue.",
        6.2f, 2.1f ) );
    g_clueNotes.push_back( makeClueNote( engineContext,
        "Last Journal Fragment",
        "You are beneath the museum in buried foundation tunnels.\nThe gallery was built over a much older site.",
        8.5f, 5.2f ) );
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
    g_museumPuzzleInitialized = false;
    g_caveTimerActive = false;
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
	engineContext.hasWallCracks = false;
    engineContext.hasFloorCracks = false;
    engineContext.caveMode = false;
    engineContext.hasFloorPuddles = false;
    engineContext.hasFloorStains = false;
    engineContext.hasWallStains = false;
    engineContext.hasWallOverlay = false;

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
  

    engineContext.ambianceTint = level.ambianceTint;
    engineContext.ambianceMul = level.ambianceMul;
    engineContext.indoorShadeLinear = level.isMuseumFloor ? 0.08f : 0.10f;
    engineContext.indoorShadeQuadratic = level.isMuseumFloor ? 0.02f : 0.03f;
    engineContext.indoorShadeMin = level.isMuseumFloor ? 0.02f : 0.04f;

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
    snprintf( buf, sizeof( buf ), "OXYGEN %02d:%02d", mins, secs );
    drawString16x16( engineContext, x + 12, y + 12, buf, rgb( 255, 100, 100 ), w, 1, 1, false );
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

static void renderSafeEntry( Engine &engineContext ) {
    if (!g_codeEntryActive || g_safeEntryIndex < 0 || g_safeEntryIndex >= (int)g_safes.size()) return;

    const SafePuzzle &safe = g_safes[ g_safeEntryIndex ];
    int w = 380;
    int h = 160;
    int x = (RENDER_W - w) / 2;
    int y = (RENDER_H - h) / 2;
    drawTextBox( engineContext, x, y, w, h, rgb( 15, 10, 10 ), rgb( 150, 150, 150 ) );

    drawString16x16( engineContext, x + 16, y + 14, "DIAL SAFE CODE", rgb( 210, 210, 210 ), w - 32, 1, 1, false );
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

static void renderWorldModels( Engine &engineContext, std::vector<float> &meshDepthBuffer ) {
    if (g_worldModels.empty()) return;

    const float projScaleY = (RENDER_W * 0.5f);
    const float horizon = (RENDER_H * 0.5f) + engineContext.pitchOffset;
    const float camHeight = 0.52f;
    const float nearClip = 0.18f;
    const float invDet = 1.0f / (engineContext.planeX * engineContext.directionY - engineContext.directionX * engineContext.planeY);
    const glm::vec3 lightDir = glm::normalize( glm::vec3( -0.35f, 0.85f, -0.40f ) );

    struct ProjVert { float sx = 0, sy = 0, z = -1; glm::vec3 world{0.0f}; glm::vec3 color{1.0f}; glm::vec2 uv{0.0f}; bool valid = false; };

    for (const auto &inst : g_worldModels)
    {
        if (!inst.visible || !inst.model || inst.model->indices.size() < 3) continue;

        std::vector<ProjVert> projected;
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
            const glm::vec3 v = inst.model->vertices[ i ];
            const glm::vec3 local = (v - pivot) * inst.scale;
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
            if (i < inst.model->colors.size()) projected[ i ].color = inst.model->colors[ i ];
            if (i < inst.model->uvs.size()) projected[ i ].uv = inst.model->uvs[ i ];
            projected[ i ].valid = true;
        }

        for (size_t i = 0; i + 2 < inst.model->indices.size(); i += 3)
        {
            const int triIdx = int( i / 3 );
            const uint32_t i0 = inst.model->indices[ i + 0 ];
            const uint32_t i1 = inst.model->indices[ i + 1 ];
            const uint32_t i2 = inst.model->indices[ i + 2 ];
            if (i0 >= projected.size() || i1 >= projected.size() || i2 >= projected.size()) continue;

            const ProjVert &a = projected[ i0 ];
            const ProjVert &b = projected[ i1 ];
            const ProjVert &c = projected[ i2 ];
            if (!a.valid || !b.valid || !c.valid) continue;
            if (a.z <= nearClip || b.z <= nearClip || c.z <= nearClip) continue;

            const float area = (b.sx - a.sx) * (c.sy - a.sy) - (b.sy - a.sy) * (c.sx - a.sx);
            if (std::fabs( area ) < 1e-4f) continue;

            const int minX = std::max( 0, (int)std::floor( std::min( { a.sx, b.sx, c.sx } ) ) );
            const int maxX = std::min( RENDER_W - 1, (int)std::ceil( std::max( { a.sx, b.sx, c.sx } ) ) );
            const int minY = std::max( 0, (int)std::floor( std::min( { a.sy, b.sy, c.sy } ) ) );
            const int maxY = std::min( RENDER_H - 1, (int)std::ceil( std::max( { a.sy, b.sy, c.sy } ) ) );
            if (minX > maxX || minY > maxY) continue;
            if ((maxX - minX) > (RENDER_W - 8) || (maxY - minY) > (RENDER_H - 8)) continue;

            glm::vec3 nrm = glm::cross( b.world - a.world, c.world - a.world );
            const float nLen = glm::length( nrm );
            if (nLen <= 1e-6f) continue;
            nrm /= nLen;
            const float lambert = std::clamp( 0.35f + 0.65f * std::fabs( glm::dot( nrm, lightDir ) ), 0.20f, 1.0f );

            const float invZ0 = 1.0f / std::max( 0.0001f, a.z );
            const float invZ1 = 1.0f / std::max( 0.0001f, b.z );
            const float invZ2 = 1.0f / std::max( 0.0001f, c.z );

            for (int y = minY; y <= maxY; ++y)
            {
                for (int x = minX; x <= maxX; ++x)
                {
                    const float px = x + 0.5f;
                    const float py = y + 0.5f;

                    const float w0raw = (b.sx - px) * (c.sy - py) - (b.sy - py) * (c.sx - px);
                    const float w1raw = (c.sx - px) * (a.sy - py) - (c.sy - py) * (a.sx - px);
                    const float w2raw = (a.sx - px) * (b.sy - py) - (a.sy - py) * (b.sx - px);

                    if (area > 0.0f)
                    {
                        if (w0raw < 0.0f || w1raw < 0.0f || w2raw < 0.0f) continue;
                    }
                    else
                    {
                        if (w0raw > 0.0f || w1raw > 0.0f || w2raw > 0.0f) continue;
                    }

                    const float w0 = w0raw / area;
                    const float w1 = w1raw / area;
                    const float w2 = w2raw / area;

                    const glm::vec3 vertexColor =
                        (a.color * w0) +
                        (b.color * w1) +
                        (c.color * w2);

                    glm::vec3 materialColor( 1.0f );
                    if (triIdx >= 0 && triIdx < (int)inst.model->triangleBaseColorFactor.size())
                    {
                        const glm::vec4 f = inst.model->triangleBaseColorFactor[ triIdx ];
                        materialColor = glm::vec3( f.r, f.g, f.b );
                    }

                    glm::vec3 texColor( 1.0f );
                    int texIdx = -1;
                    if (triIdx >= 0 && triIdx < (int)inst.model->triangleTextureIndex.size())
                    {
                        texIdx = inst.model->triangleTextureIndex[ triIdx ];
                    }
                    const bool hasTexture = (texIdx >= 0 && texIdx < (int)inst.model->baseColorTextures.size());
                    if (hasTexture)
                    {
                        const Image &tex = inst.model->baseColorTextures[ texIdx ];
                        if (tex.width > 0 && tex.height > 0)
                        {
                            const glm::vec2 uv = (a.uv * w0) + (b.uv * w1) + (c.uv * w2);
                            float uu = uv.x - std::floor( uv.x );
                            float vv = uv.y - std::floor( uv.y );
                            int tx = std::clamp( int( uu * tex.width ), 0, tex.width - 1 );
                            int ty = std::clamp( int( vv * tex.height ), 0, tex.height - 1 );
                            Uint32 tc = tex.sample( tx, ty );
                            texColor.r = float( (tc >> 16) & 255 ) / 255.0f;
                            texColor.g = float( (tc >> 8) & 255 ) / 255.0f;
                            texColor.b = float( tc & 255 ) / 255.0f;
                        }
                    }

                    const float invZ = (w0 * invZ0) + (w1 * invZ1) + (w2 * invZ2);
                    const float z = 1.0f / std::max( 0.0001f, invZ );
                    if (z >= engineContext.zbuffer[ x ]) continue;

                    const int pix = y * RENDER_W + x;
                    if (z >= meshDepthBuffer[ pix ]) continue;

                    float distanceShade;
                    if (engineContext.caveMode)
                    {
                        float R = engineContext.lightRadius;
                        float t = std::clamp( 1.0f - std::pow( z / std::max( 0.001f, R ), engineContext.lightFalloff ), 0.0f, 1.0f );
                        distanceShade = std::max( engineContext.caveAmbient, t );
                    }
                    else
                    {
                        distanceShade = 1.0f / (1.0f + engineContext.indoorShadeLinear * z + engineContext.indoorShadeQuadratic * z * z);
                        distanceShade = std::clamp( distanceShade, engineContext.indoorShadeMin, 1.0f );
                    }

                    const float lit = std::clamp( 0.28f + 0.95f * (lambert * distanceShade), 0.28f, 1.15f );
                    const glm::vec3 vertexMul = hasTexture ? glm::vec3( 1.0f ) : vertexColor;
                    Uint8 r = Uint8( std::clamp( materialColor.r * vertexMul.r * texColor.r * lit * 255.0f, 0.0f, 255.0f ) );
                    Uint8 g = Uint8( std::clamp( materialColor.g * vertexMul.g * texColor.g * lit * 255.0f, 0.0f, 255.0f ) );
                    Uint8 bcol = Uint8( std::clamp( materialColor.b * vertexMul.b * texColor.b * lit * 255.0f, 0.0f, 255.0f ) );
                    putPix( engineContext, x, y, rgb( r, g, bcol ) );
                    meshDepthBuffer[ pix ] = z;
                }
            }
        }
    }
}

static void render( Engine &engineContext, float dt ) {
    (void)dt;

    bool overlayBusy = g_interactionAnim.active || g_levelTransition.active || g_notesOpen || g_codeEntryActive || g_caveQuizActive;

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
        Uint8 r = Uint8( std::clamp( float( (shaded >> 16) & 255 ) * tr * engineContext.ambianceMul, 0.0f, 255.0f ) );
        Uint8 g = Uint8( std::clamp( float( (shaded >> 8) & 255 ) * tg * engineContext.ambianceMul, 0.0f, 255.0f ) );
        Uint8 b = Uint8( std::clamp( float( shaded & 255 ) * tb * engineContext.ambianceMul, 0.0f, 255.0f ) );
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

    static std::vector<float> meshDepthBuffer;
    meshDepthBuffer.assign( RENDER_W * RENDER_H, std::numeric_limits<float>::infinity() );
    renderWorldModels( engineContext, meshDepthBuffer );



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
                        shade = std::clamp( shade, 0.18f, 1.0f );
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



    if (!overlayBusy && lookingAtArt != -1 && engineContext.placardOpen == false && engineContext.journalOpen == false && distanceToArt < 2.5)
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
        drawStringTinyScaled( engineContext, 12, RENDER_H - 20, "CAMP LOGS HOLD CLUES FOR THE WARDEN STATUE", rgb( 170, 180, 210 ), 1, 1, 1, false );
    }

    drawStringTinyScaled(engineContext, 12, RENDER_H - 20, "X: " + to_string(engineContext.positionX) + " " + "Y: " + to_string(engineContext.positionY), rgb(0, 0, 0), 1, 1, 1, false);


    int nearbyKey = getNearbyKeyPickup( engineContext );
    if (!overlayBusy && nearbyKey >= 0 && !g_codeEntryActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 95, (RENDER_H / 2) + 45, "[E] INTERACT", rgb( 255, 240, 140 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    int nearbyNote = getNearbyClueNote( engineContext );
    if (!overlayBusy && nearbyNote >= 0 && !g_codeEntryActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 95, (RENDER_H / 2) + 85, "[E] COLLECT NOTE", rgb( 220, 225, 180 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    int nearbySafe = getNearbySafe( engineContext );
    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM && nearbySafe >= 0 && !g_codeEntryActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 105, (RENDER_H / 2) + 65, "[F] EXAMINE SAFE", rgb( 180, 210, 255 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    int nearbySymbol = getNearbySymbol( engineContext );
    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM && nearbySymbol >= 0 && !g_codeEntryActive)
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 105, (RENDER_H / 2) + 65, "[F] EXAMINE PEDESTAL", rgb( 250, 180, 250 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.currentLevel == Levels::CAVE && isPlayerNearCaveStatue( engineContext ) && !g_caveQuizActive)
    {
        std::string statuePrompt = g_caveQuizPassed ? "WARDEN: PATH OPEN" : "[E] ANSWER WARDEN QUESTIONS";
        drawString16x16( engineContext, (RENDER_W / 2) - 145, (RENDER_H / 2) + 105, statuePrompt, rgb( 210, 220, 255 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.currentLevel == Levels::ENTRANCE && isPlayerNearPoint( engineContext, 11.5f, 2.5f, 1.4f ))
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 160, (RENDER_H / 2) + 105, "[E] CHECK IN", rgb( 220, 220, 180 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM && isPlayerNearPoint( engineContext, kUpperEntryX, kUpperEntryY, kUpperEntryRadius ))
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 140, (RENDER_H / 2) + 105, "[E] ENTER STAIRWELL TO UPPER GALLERY", rgb( 205, 220, 255 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }
    if (!overlayBusy && engineContext.currentLevel == Levels::MUSEUM_UPPER && isPlayerNearPoint( engineContext, 3.5f, 9.3f, 1.1f ))
    {
        drawString16x16( engineContext, (RENDER_W / 2) - 145, (RENDER_H / 2) + 105, "[E] RETURN TO GROUND FLOOR", rgb( 205, 220, 255 ), RENDER_W, 1, 2, true, rgb( 20, 20, 20 ) );
    }

    int doorTx = 0, doorTy = 0;
    if (engineContext.currentLevel == Levels::MUSEUM && getDoorAheadTile( engineContext, doorTx, doorTy ))
    {
        int lockIndex = findDoorLockIndex( doorTx, doorTy );
        if (!overlayBusy && lockIndex >= 0 && !g_roomLocks[ lockIndex ].unlocked && !g_codeEntryActive)
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

    if (engineContext.currentLevel == Levels::MUSEUM || engineContext.currentLevel == Levels::MUSEUM_UPPER)
    {
        renderObjectives( engineContext );
        renderGalleryCard( engineContext ); 
    }
    
    renderCaveHUD( engineContext );

    if (!overlayBusy) renderAccessPopup( engineContext );
    renderNotesScreen( engineContext );
    renderCodeEntry( engineContext );
    renderSafeEntry( engineContext );
    renderSymbolEntry( engineContext );
    renderCaveQuiz( engineContext );
    renderInteractionAnimation( engineContext );
    renderLevelTransitionOverlay( engineContext );

    
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
    {"Museum Entrance", (assetRoot / "levels" / "entrance").string(), "map.txt", 11.5f, 15.5f, 270.f, Levels::ENTRANCE, rgb( 230, 238, 255 ), 1.0f, false, "Check in at the desk"},
    {"Museum Ground", (assetRoot / "levels" / "museum").string(), "map.txt", 10.0f, 9.0f, 90.f, Levels::MUSEUM, rgb( 255, 242, 220 ), 1.06f, true, "Explore both floors and report to the statue"},
    {"Museum Upper", (assetRoot / "levels" / "museum_upper").string(), "map.txt", 3.8f, 9.3f, 0.f, Levels::MUSEUM_UPPER, rgb( 205, 225, 255 ), 0.92f, true, "Explore both floors and report to the statue"},
    {"Transition", (assetRoot / "levels" / "transition").string(), "map.txt", 1.5f, 4.5f, 270.f, Levels::TRANSITION, rgb( 235, 235, 235 ), 1.0f, false, "Proceed through the tunnels"},
    {"Cave", (assetRoot / "levels" / "cave").string(), "map.txt", 2.5f, 2.5f, 90.0f, Levels::CAVE, rgb( 200, 215, 255 ), 0.90f, false, "Find the final journal fragment"}


    };

    calibrateMusicVolumeFromMic();

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
        if (currentState == STATE_GAME) 
        {
            g_runElapsedSeconds += dt;
            if (g_caveTimerActive && !g_caveQuizPassed)
            {
                g_caveTimerSeconds -= dt;
                if (g_caveTimerSeconds <= 0.0f)
                {
                    g_caveTimerActive = false;
                    showAccessPopup( "Oxygen depleted. Returning to entrance.", 4000 );
                    handleLevelChange( engineContext, levels, Levels::ENTRANCE );
                    g_notesCollectedRun = 0;
                    g_runElapsedSeconds = 0.0f;
                }
            }
        }
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
                    if (g_levelTransition.active)
                    {
                        continue;
                    }

                    if (g_notesOpen)
                    {
                        if (ev.key.key == SDLK_N || ev.key.key == SDLK_ESCAPE)
                        {
                            g_notesOpen = false;
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
                    }
                    else if (ev.key.key == SDLK_F1)
                    {
                        engineContext.showHelp = !engineContext.showHelp;
                    }
                    else if (ev.key.scancode == SDL_SCANCODE_E)
                    {
                        if (engineContext.currentLevel == Levels::ENTRANCE && isPlayerNearPoint( engineContext, 11.5f, 2.5f, 1.4f ))
                        {
                            triggerInteractionAnim( InteractionAnimType::DOOR_USE, "CHECKING IN WITH FRONT DESK", 0.9f );
                            beginLevelTransition( Levels::MUSEUM, 1.1f );
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
                        int nearbySafe = getNearbySafe( engineContext );
                        if (nearbySafe >= 0)
                        {
                            g_codeEntryActive = true;
                            g_safeEntryIndex = nearbySafe;
                            g_codeEntryLockIndex = -1;
                            g_symbolEntryIndex = -1;
                            g_codeEntryBuffer.clear();
                            continue;
                        }

                        int nearbySymbol = getNearbySymbol( engineContext );
                        if (nearbySymbol >= 0)
                        {
                            g_codeEntryActive = true;
                            g_symbolEntryIndex = nearbySymbol;
                            g_codeEntryLockIndex = -1;
                            g_safeEntryIndex = -1;
                            g_symbolFocus = 0;
                            g_codeEntryBuffer.clear();
                            continue;
                        }

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
                        handleLevelChange( engineContext, levels, Levels::ENTRANCE );
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
            if (g_codeEntryActive || g_notesOpen || g_caveQuizActive || g_levelTransition.active || g_interactionAnim.active) ms = 0.0f;
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
