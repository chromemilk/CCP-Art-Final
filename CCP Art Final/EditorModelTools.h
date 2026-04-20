#pragma once

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
