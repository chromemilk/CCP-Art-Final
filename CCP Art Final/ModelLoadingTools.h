#pragma once

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

struct WepingStatueState {
    int modelIndex = -1;
    float baseX = 0.0f;
    float baseY = 0.0f;
    float basePitch = 0.0f;
    float baseYaw = 0.0f;
    float baseRoll = 0.0f;
    float lastLookedAwayTime = 0.0f;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    float positionOffsetX = 0.0f;
    float positionOffsetY = 0.0f;
    bool wasLookedAt = false;
};



static std::vector<WepingStatueState> g_wepingStatues;
static constexpr float kStatueRotationSpeed = 0.4f;
static constexpr float kStatueMaxRotation = 0.3f;
static constexpr float kStatueMovementSpeed = 0.05f;
static constexpr float kStatueMaxMovement = 0.15f;
static constexpr float kStatueLookDistance = 8.0f;


static bool isPlayerLookingAtStatue(const Engine& engineContext, const WorldModelInstance& statue) {
    const float dx = statue.x - engineContext.positionX;
    const float dy = statue.y - engineContext.positionY;
    const float distSq = dx * dx + dy * dy;

    if (distSq > (kStatueLookDistance * kStatueLookDistance)) {
        return false;
    }

    const float dotProduct = (dx * engineContext.directionX + dy * engineContext.directionY);
    return dotProduct > 0.3f;
}
static void updateWepingStatues(Engine& engineContext, float dt) {
    for (auto& statue : g_wepingStatues)
    {
        if (statue.modelIndex < 0 || statue.modelIndex >= (int)g_worldModels.size())
        {
            continue;
        }

        auto& model = g_worldModels[statue.modelIndex];
        const bool isLookedAt = isPlayerLookingAtStatue(engineContext, model);

        if (isLookedAt)
        {
            statue.wasLookedAt = true;
            statue.lastLookedAwayTime = 0.0f;
        }
        else if (statue.wasLookedAt)
        {
            statue.lastLookedAwayTime += dt;

            if (statue.lastLookedAwayTime > 0.18f)
            {
                const float rotationAmount = kStatueRotationSpeed * dt;
                const float movementAmount = kStatueMovementSpeed * dt;

                auto jitter = []() -> float {
                    return float(std::rand() % 101 - 50) / 100.0f;
                    };

                statue.rotationX = std::clamp(
                    statue.rotationX + jitter() * rotationAmount,
                    -kStatueMaxRotation,
                    kStatueMaxRotation);

                statue.rotationY = std::clamp(
                    statue.rotationY + jitter() * rotationAmount,
                    -kStatueMaxRotation,
                    kStatueMaxRotation);

                statue.rotationZ = std::clamp(
                    statue.rotationZ + jitter() * rotationAmount,
                    -kStatueMaxRotation,
                    kStatueMaxRotation);

                statue.positionOffsetX = std::clamp(
                    statue.positionOffsetX + jitter() * movementAmount,
                    -kStatueMaxMovement,
                    kStatueMaxMovement);

                statue.positionOffsetY = std::clamp(
                    statue.positionOffsetY + jitter() * movementAmount,
                    -kStatueMaxMovement,
                    kStatueMaxMovement);
            }
        }

        model.pitch = statue.basePitch + statue.rotationX;
        model.yaw = statue.baseYaw + statue.rotationY;
        model.roll = statue.baseRoll + statue.rotationZ;
        model.x = statue.baseX + statue.positionOffsetX;
        model.y = statue.baseY + statue.positionOffsetY;
    }
}


static void registerWepingStatue(int modelIndex) {
    for (size_t i = 0; i < g_wepingStatues.size(); ++i)
    {
        if (g_wepingStatues[i].modelIndex == modelIndex)
        {
            auto& model = g_worldModels[modelIndex];
            g_wepingStatues[i].baseX = model.x;
            g_wepingStatues[i].baseY = model.y;
            g_wepingStatues[i].basePitch = model.pitch;
            g_wepingStatues[i].baseYaw = model.yaw;
            g_wepingStatues[i].baseRoll = model.roll;
            g_wepingStatues[i].rotationX = 0.0f;
            g_wepingStatues[i].rotationY = 0.0f;
            g_wepingStatues[i].rotationZ = 0.0f;
            g_wepingStatues[i].positionOffsetX = 0.0f;
            g_wepingStatues[i].positionOffsetY = 0.0f;
            g_wepingStatues[i].wasLookedAt = false;
            g_wepingStatues[i].lastLookedAwayTime = 0.0f;
            model.pitch = g_wepingStatues[i].basePitch;
            model.yaw = g_wepingStatues[i].baseYaw;
            model.roll = g_wepingStatues[i].baseRoll;
            model.x = g_wepingStatues[i].baseX;
            model.y = g_wepingStatues[i].baseY;
            return;
        }
    }

    if (modelIndex < 0 || modelIndex >= (int)g_worldModels.size())
    {
        return;
    }

    const auto& model = g_worldModels[modelIndex];
    WepingStatueState statue;
    statue.modelIndex = modelIndex;
    statue.baseX = model.x;
    statue.baseY = model.y;
    statue.basePitch = model.pitch;
    statue.baseYaw = model.yaw;
    statue.baseRoll = model.roll;
    statue.rotationX = 0.0f;
    statue.rotationY = 0.0f;
    statue.rotationZ = 0.0f;
    statue.positionOffsetX = 0.0f;
    statue.positionOffsetY = 0.0f;
    statue.wasLookedAt = false;
    statue.lastLookedAwayTime = 0.0f;
    g_wepingStatues.push_back(statue);
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

    const std::string lowerPath = toLowerCopy(modelPath);
    if (lowerPath.find("statue") != std::string::npos ||
        lowerPath.find("warden") != std::string::npos ||
        lowerPath.find("angel") != std::string::npos)
    {
        registerWepingStatue(index);
    }




    return index;
}
