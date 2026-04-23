#pragma once
#include <cmath>
#include <algorithm>

// Unified lighting calculation system for consistent rendering across all surfaces
// This ensures floors, ceilings, walls, and 3D models all receive identical lighting treatment

static float calculateDistanceAttenuation(float perpDist, bool caveMode, 
    float indoorShadeLinear, float indoorShadeQuadratic, float indoorShadeMin,
    float caveAmbient, float lightRadius, float lightFalloff)
{
    if (caveMode)
    {
        // Cave: distance-based with falloff and ambient floor
        float baseShade = std::clamp(1.0f / (0.4f * perpDist), 0.15f, 1.0f);
        float t = std::clamp(1.0f - std::pow(perpDist / std::max(0.001f, lightRadius), lightFalloff), 0.0f, 1.0f);
        float lightLevel = std::max(caveAmbient, t);
        return baseShade * lightLevel;
    }
    else
    {
        // Museum: quadratic falloff with minimum - UNIFIED FORMULA
        float shade = 1.0f / (1.0f + indoorShadeLinear * perpDist + indoorShadeQuadratic * perpDist * perpDist);
        return std::clamp(shade, indoorShadeMin, 1.0f);
    }
}

// Apply lighting to a color with all modifiers
static Uint32 applyLighting(Uint32 color, float shade, 
    float ambianceMul, Uint32 ambianceTint, float darknessOverride)
{
    // Apply darkness override if active (for cutscenes like wake cutscene)
    if (darknessOverride >= 0.0f)
    {
        shade *= (1.0f - darknessOverride);
    }

    float tr = float((ambianceTint >> 16) & 255) / 255.0f;
    float tg = float((ambianceTint >> 8) & 255) / 255.0f;
    float tb = float(ambianceTint & 255) / 255.0f;

    float finalMul = shade * ambianceMul;

    Uint8 r = Uint8(std::clamp(float((color >> 16) & 255) * tr * finalMul, 0.0f, 255.0f));
    Uint8 g = Uint8(std::clamp(float((color >> 8) & 255) * tg * finalMul, 0.0f, 255.0f));
    Uint8 b = Uint8(std::clamp(float(color & 255) * tb * finalMul, 0.0f, 255.0f));

    return rgb(r, g, b);
}

// Apply wall-specific shading (for geometry depth effect)
static float applyWallSideShading(float shade, int side)
{
    // Makes walls facing one axis darker than the other to show geometry depth
    if (side == 1) shade *= 0.75f;
    return shade;
}

// Apply wall edge ambient occlusion
static float applyWallEdgeAO(float shade, float wallX)
{
    float edgeDist = std::min(wallX, 1.0f - wallX);
    float aoThreshold = 0.03f;
    if (edgeDist < aoThreshold)
    {
        float t = edgeDist / aoThreshold;
        float ao = t * t * (3.0f - 2.0f * t);
        shade *= (0.96f + 0.04f * ao);
    }
    return shade;
}
