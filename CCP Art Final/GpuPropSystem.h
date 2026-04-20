#pragma once
#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#if __has_include("tiny_gltf.h")
#include "tiny_gltf.h"
#define MUSEUM3D_HAS_TINYGLTF 1
#elif __has_include("tiny_gltf_v3.h")
#include "tiny_gltf_v3.h"
#define MUSEUM3D_HAS_TINYGLTF 1
#else
#define MUSEUM3D_HAS_TINYGLTF 0
#endif

namespace museum3d
{
    constexpr uint32_t kMaxLocalLights = 8;

    struct Vertex
    {
        glm::vec3 position{0.0f};
        glm::vec3 normal{0.0f, 1.0f, 0.0f};
        glm::vec2 uv{0.0f};
        glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    };

    struct Transform
    {
        glm::vec3 position{0.0f};
        glm::vec3 rotationEulerRadians{0.0f};
        glm::vec3 scale{1.0f, 1.0f, 1.0f};

        [[nodiscard]] glm::mat4 matrix() const
        {
            const glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
            const glm::mat4 r = glm::eulerAngleXYZ(rotationEulerRadians.x, rotationEulerRadians.y, rotationEulerRadians.z);
            const glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
            return t * r * s;
        }
    };

    struct Material
    {
        glm::vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
        float metallic = 0.0f;
        float roughness = 0.8f;
        glm::vec3 emissive{0.0f};
    };

    struct AABB
    {
        glm::vec3 min{0.0f};
        glm::vec3 max{0.0f};

        [[nodiscard]] bool intersects(const AABB& other) const
        {
            return min.x <= other.max.x && max.x >= other.min.x &&
                min.y <= other.max.y && max.y >= other.min.y &&
                min.z <= other.max.z && max.z >= other.min.z;
        }

        [[nodiscard]] AABB transformed(const glm::mat4& world) const
        {
            const glm::vec3 corners[8] = {
                {min.x, min.y, min.z}, {max.x, min.y, min.z}, {min.x, max.y, min.z}, {max.x, max.y, min.z},
                {min.x, min.y, max.z}, {max.x, min.y, max.z}, {min.x, max.y, max.z}, {max.x, max.y, max.z}
            };

            AABB out;
            out.min = glm::vec3(std::numeric_limits<float>::max());
            out.max = glm::vec3(std::numeric_limits<float>::lowest());

            for (const glm::vec3& c : corners)
            {
                const glm::vec3 t = glm::vec3(world * glm::vec4(c, 1.0f));
                out.min = glm::min(out.min, t);
                out.max = glm::max(out.max, t);
            }
            return out;
        }
    };

    enum class LocalLightType : uint32_t
    {
        Point = 0,
        Spot = 1
    };

    struct DirectionalLight
    {
        glm::vec3 direction = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.2f));
        float intensity = 1.0f;
        glm::vec3 color{1.0f};
        float _pad0 = 0.0f;
    };

    struct LocalLight
    {
        glm::vec3 position{0.0f};
        float range = 8.0f;
        glm::vec3 direction{0.0f, -1.0f, 0.0f};
        float spotCosInner = 0.95f;
        glm::vec3 color{1.0f};
        float intensity = 5.0f;
        float spotCosOuter = 0.80f;
        uint32_t type = static_cast<uint32_t>(LocalLightType::Point);
        glm::vec2 _pad0{0.0f};
    };

    struct SceneLights
    {
        DirectionalLight sun{};
        std::array<LocalLight, kMaxLocalLights> locals{};
        uint32_t localCount = 0;
        glm::vec3 ambientColor{0.08f, 0.08f, 0.09f};
    };

    struct alignas(16) SceneUniforms
    {
        glm::mat4 viewProj{1.0f};
        glm::vec4 cameraPos{0.0f, 0.0f, 0.0f, 1.0f};
        glm::vec4 ambient{0.08f, 0.08f, 0.09f, 1.0f};
        glm::vec4 sunDirectionIntensity{0.0f, -1.0f, 0.0f, 1.0f};
        glm::vec4 sunColor{1.0f};

        struct alignas(16) PackedLight
        {
            glm::vec4 positionRange{0.0f};
            glm::vec4 directionInner{0.0f};
            glm::vec4 colorIntensity{0.0f};
            glm::vec4 outerAndType{0.0f};
        };

        std::array<PackedLight, kMaxLocalLights> lights{};
        glm::uvec4 counts{0u, 0u, 0u, 0u};
    };

    struct alignas(16) ObjectUniforms
    {
        glm::mat4 world{1.0f};
        glm::vec4 baseColorMetallic{1.0f, 1.0f, 1.0f, 0.0f};
        glm::vec4 roughnessEmissive{0.8f, 0.0f, 0.0f, 0.0f};
    };

    inline SceneUniforms BuildSceneUniforms(const glm::mat4& viewProj, const glm::vec3& cameraPos, const SceneLights& lights)
    {
        SceneUniforms uniforms{};
        uniforms.viewProj = viewProj;
        uniforms.cameraPos = glm::vec4(cameraPos, 1.0f);
        uniforms.ambient = glm::vec4(lights.ambientColor, 1.0f);
        uniforms.sunDirectionIntensity = glm::vec4(glm::normalize(lights.sun.direction), lights.sun.intensity);
        uniforms.sunColor = glm::vec4(lights.sun.color, lights.sun.intensity);

        const uint32_t count = std::min<uint32_t>(lights.localCount, kMaxLocalLights);
        uniforms.counts.x = count;

        for (uint32_t i = 0; i < count; ++i)
        {
            const LocalLight& src = lights.locals[i];
            SceneUniforms::PackedLight packed{};
            packed.positionRange = glm::vec4(src.position, src.range);
            packed.directionInner = glm::vec4(glm::normalize(src.direction), src.spotCosInner);
            packed.colorIntensity = glm::vec4(src.color, src.intensity);
            packed.outerAndType = glm::vec4(src.spotCosOuter, static_cast<float>(src.type), 0.0f, 0.0f);
            uniforms.lights[i] = packed;
        }

        return uniforms;
    }

    inline ObjectUniforms BuildObjectUniforms(const Transform& transform, const Material& material)
    {
        ObjectUniforms uniforms{};
        uniforms.world = transform.matrix();
        uniforms.baseColorMetallic = glm::vec4(material.baseColorFactor.r, material.baseColorFactor.g, material.baseColorFactor.b, material.metallic);
        uniforms.roughnessEmissive = glm::vec4(material.roughness, material.emissive.r, material.emissive.g, material.emissive.b);
        return uniforms;
    }

    struct CompiledShaderBlob
    {
        std::vector<uint8_t> code;
        SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
        std::string entryPoint;
    };

    struct ShaderSourcePack
    {
        std::string glslVertex;
        std::string glslFragment;
        std::string hlslVertex;
        std::string hlslPixel;
    };

    inline ShaderSourcePack BuildPBRLiteSources()
    {
        ShaderSourcePack out;

        out.glslVertex = R"(
#version 450
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNrm;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec4 iRow0;
layout(location = 5) in vec4 iRow1;
layout(location = 6) in vec4 iRow2;
layout(location = 7) in vec4 iRow3;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vNormal;
layout(location = 2) out vec2 vUV;
layout(location = 3) out vec4 vColor;

struct PackedLight
{
    vec4 positionRange;
    vec4 directionInner;
    vec4 colorIntensity;
    vec4 outerAndType;
};

layout(set = 1, binding = 0, std140) uniform SceneUbo
{
    mat4 uViewProj;
    vec4 uCameraPos;
    vec4 uAmbient;
    vec4 uSunDirectionIntensity;
    vec4 uSunColor;
    PackedLight uLights[8];
    uvec4 uCounts;
} scene;

void main()
{
    mat4 inst = mat4(iRow0, iRow1, iRow2, iRow3);
    vec4 worldPos = inst * vec4(inPos, 1.0);
    mat3 normalMat = mat3(inst);

    vWorldPos = worldPos.xyz;
    vNormal = normalize(normalMat * inNrm);
    vUV = inUV;
    vColor = inColor;

    gl_Position = scene.uViewProj * worldPos;
}
)";

        out.glslFragment = R"(
#version 450
layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;
layout(location = 3) in vec4 vColor;

layout(location = 0) out vec4 outColor;

const uint LIGHT_TYPE_POINT = 0u;
const uint LIGHT_TYPE_SPOT = 1u;

struct PackedLight
{
    vec4 positionRange;
    vec4 directionInner;
    vec4 colorIntensity;
    vec4 outerAndType;
};

layout(set = 2, binding = 0, std140) uniform ObjectUbo
{
    mat4 uWorld;
    vec4 uBaseColorMetallic;
    vec4 uRoughnessEmissive;
} objectUbo;

layout(set = 1, binding = 0, std140) uniform SceneUbo
{
    mat4 uViewProj;
    vec4 uCameraPos;
    vec4 uAmbient;
    vec4 uSunDirectionIntensity;
    vec4 uSunColor;
    PackedLight uLights[8];
    uvec4 uCounts;
} scene;

void main()
{
    vec3 N = normalize(vNormal);
    vec3 V = normalize(scene.uCameraPos.xyz - vWorldPos);
    vec3 Ld = normalize(-scene.uSunDirectionIntensity.xyz);

    vec3 albedo = objectUbo.uBaseColorMetallic.rgb * vColor.rgb;
    float metallic = clamp(objectUbo.uBaseColorMetallic.a, 0.0, 1.0);
    float roughness = clamp(objectUbo.uRoughnessEmissive.x, 0.04, 1.0);

    float NdotL = max(dot(N, Ld), 0.0);
    vec3 diffuse = albedo * NdotL;

    vec3 H = normalize(V + Ld);
    float NdotH = max(dot(N, H), 0.0);
    float specPow = mix(128.0, 8.0, roughness);
    float spec = pow(NdotH, specPow) * (1.0 - roughness * 0.6);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 specular = F0 * spec;

    vec3 color = (scene.uAmbient.rgb * albedo) + (scene.uSunColor.rgb * scene.uSunColor.a) * (diffuse + specular);

    uint localCount = min(scene.uCounts.x, 8u);
    for (uint i = 0u; i < localCount; ++i)
    {
        PackedLight light = scene.uLights[i];
        vec3 toLight = light.positionRange.xyz - vWorldPos;
        float dist = length(toLight);
        float range = max(light.positionRange.w, 0.0001);
        if (dist >= range) continue;

        vec3 L = toLight / max(dist, 0.0001);
        float attenuation = 1.0 - clamp(dist / range, 0.0, 1.0);
        attenuation *= attenuation;

        uint type = uint(light.outerAndType.y + 0.5);
        if (type == LIGHT_TYPE_SPOT)
        {
            vec3 spotDir = normalize(-light.directionInner.xyz);
            float cd = dot(L, spotDir);
            float innerCos = light.directionInner.w;
            float outerCos = light.outerAndType.x;
            float denom = max(innerCos - outerCos, 0.0001);
            float spotFactor = clamp((cd - outerCos) / denom, 0.0, 1.0);
            attenuation *= spotFactor;
        }

        if (attenuation <= 0.0) continue;

        float localNdotL = max(dot(N, L), 0.0);
        if (localNdotL <= 0.0) continue;

        vec3 Hlocal = normalize(V + L);
        float NdotHLocal = max(dot(N, Hlocal), 0.0);
        float localSpec = pow(NdotHLocal, specPow) * (1.0 - roughness * 0.6);

        vec3 lightColor = light.colorIntensity.rgb * light.colorIntensity.w;
        vec3 localDiffuse = albedo * localNdotL;
        vec3 localSpecular = F0 * localSpec;
        color += lightColor * attenuation * (localDiffuse + localSpecular);
    }

    outColor = vec4(color, 1.0);
}
)";

        out.hlslVertex = R"(
struct VSIn {
    float3 pos : TEXCOORD0;
    float3 nrm : TEXCOORD1;
    float2 uv  : TEXCOORD2;
    float4 color : TEXCOORD3;
    float4 i0  : TEXCOORD4;
    float4 i1  : TEXCOORD5;
    float4 i2  : TEXCOORD6;
    float4 i3  : TEXCOORD7;
};

struct PackedLight {
    float4 positionRange;
    float4 directionInner;
    float4 colorIntensity;
    float4 outerAndType;
};

cbuffer SceneUbo : register(b0, space1)
{
    float4x4 uViewProj;
    float4 uCameraPos;
    float4 uAmbient;
    float4 uSunDirectionIntensity;
    float4 uSunColor;
    PackedLight uLights[8];
    uint4 uCounts;
};

struct VSOut {
    float4 posH : SV_Position;
    float3 worldPos : TEXCOORD0;
    float3 normalW : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float4 color : TEXCOORD3;
};

VSOut main(VSIn i)
{
    VSOut o;
    float4x4 inst = float4x4(i.i0, i.i1, i.i2, i.i3);
    float4 wp = mul(inst, float4(i.pos, 1.0));
    o.posH = mul(uViewProj, wp);
    o.worldPos = wp.xyz;
    o.normalW = normalize(mul((float3x3)inst, i.nrm));
    o.uv = i.uv;
    o.color = i.color;
    return o;
}
)";

        out.hlslPixel = R"(
struct PackedLight {
    float4 positionRange;
    float4 directionInner;
    float4 colorIntensity;
    float4 outerAndType;
};

cbuffer SceneUbo : register(b0, space1)
{
    float4x4 uViewProj;
    float4 uCameraPos;
    float4 uAmbient;
    float4 uSunDirectionIntensity;
    float4 uSunColor;
    PackedLight uLights[8];
    uint4 uCounts;
};

cbuffer ObjectUbo : register(b0, space2)
{
    float4x4 uWorld;
    float4 uBaseColorMetallic;
    float4 uRoughnessEmissive;
};

struct PSIn {
    float4 posH : SV_Position;
    float3 worldPos : TEXCOORD0;
    float3 normalW : TEXCOORD1;
    float2 uv : TEXCOORD2;
    float4 color : TEXCOORD3;
};

float4 main(PSIn i) : SV_Target0
{
    const uint LIGHT_TYPE_SPOT = 1u;

    float3 N = normalize(i.normalW);
    float3 V = normalize(uCameraPos.xyz - i.worldPos);
    float3 Ld = normalize(-uSunDirectionIntensity.xyz);

    float3 albedo = uBaseColorMetallic.rgb * i.color.rgb;
    float metallic = saturate(uBaseColorMetallic.a);
    float roughness = clamp(uRoughnessEmissive.x, 0.04, 1.0);

    float NdotL = saturate(dot(N, Ld));
    float3 diffuse = albedo * NdotL;

    float3 H = normalize(V + Ld);
    float NdotH = saturate(dot(N, H));
    float specPow = lerp(128.0, 8.0, roughness);
    float spec = pow(NdotH, specPow) * (1.0 - roughness * 0.6);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedo, metallic);

    float3 color = uAmbient.rgb * albedo + (uSunColor.rgb * uSunColor.a) * (diffuse + F0 * spec);

    uint localCount = min(uCounts.x, 8u);
    [loop]
    for (uint lightIndex = 0u; lightIndex < localCount; ++lightIndex)
    {
        PackedLight light = uLights[lightIndex];
        float3 toLight = light.positionRange.xyz - i.worldPos;
        float dist = length(toLight);
        float range = max(light.positionRange.w, 0.0001);
        if (dist >= range) continue;

        float3 L = toLight / max(dist, 0.0001);
        float attenuation = 1.0 - saturate(dist / range);
        attenuation *= attenuation;

        uint type = (uint)(light.outerAndType.y + 0.5);
        if (type == LIGHT_TYPE_SPOT)
        {
            float3 spotDir = normalize(-light.directionInner.xyz);
            float cd = dot(L, spotDir);
            float innerCos = light.directionInner.w;
            float outerCos = light.outerAndType.x;
            float denom = max(innerCos - outerCos, 0.0001);
            float spotFactor = saturate((cd - outerCos) / denom);
            attenuation *= spotFactor;
        }

        if (attenuation <= 0.0) continue;

        float localNdotL = saturate(dot(N, L));
        if (localNdotL <= 0.0) continue;

        float3 Hlocal = normalize(V + L);
        float NdotHLocal = saturate(dot(N, Hlocal));
        float localSpec = pow(NdotHLocal, specPow) * (1.0 - roughness * 0.6);

        float3 lightColor = light.colorIntensity.rgb * light.colorIntensity.w;
        float3 localDiffuse = albedo * localNdotL;
        float3 localSpecular = F0 * localSpec;
        color += lightColor * attenuation * (localDiffuse + localSpecular);
    }

    return float4(color, 1.0);
}
)";

        return out;
    }

    class Mesh
    {
    public:
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        AABB localBounds{};

        ~Mesh() = default;

        [[nodiscard]] uint32_t indexCount() const { return static_cast<uint32_t>(indices.size()); }

        bool upload(SDL_GPUDevice* device, SDL_GPUCommandBuffer* commandBuffer, std::string* error = nullptr)
        {
            if (!device || !commandBuffer)
            {
                if (error) *error = "Mesh::upload requires a valid SDL_GPUDevice and SDL_GPUCommandBuffer.";
                return false;
            }
            if (vertices.empty() || indices.empty())
            {
                if (error) *error = "Mesh::upload called with empty vertex/index data.";
                return false;
            }

            const uint32_t vertexBytes = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
            const uint32_t indexBytes = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));
            const uint32_t totalUploadBytes = vertexBytes + indexBytes;

            SDL_GPUBufferCreateInfo vbInfo{};
            vbInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
            vbInfo.size = vertexBytes;
            m_vertexBuffer = SDL_CreateGPUBuffer(device, &vbInfo);

            SDL_GPUBufferCreateInfo ibInfo{};
            ibInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
            ibInfo.size = indexBytes;
            m_indexBuffer = SDL_CreateGPUBuffer(device, &ibInfo);

            if (!m_vertexBuffer || !m_indexBuffer)
            {
                if (error) *error = "SDL_CreateGPUBuffer failed while creating mesh buffers.";
                return false;
            }

            SDL_GPUTransferBufferCreateInfo uploadInfo{};
            uploadInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            uploadInfo.size = totalUploadBytes;
            SDL_GPUTransferBuffer* staging = SDL_CreateGPUTransferBuffer(device, &uploadInfo);
            if (!staging)
            {
                if (error) *error = "SDL_CreateGPUTransferBuffer failed for mesh upload.";
                return false;
            }

            void* mapped = SDL_MapGPUTransferBuffer(device, staging, false);
            if (!mapped)
            {
                SDL_ReleaseGPUTransferBuffer(device, staging);
                if (error) *error = "SDL_MapGPUTransferBuffer failed for mesh upload.";
                return false;
            }

            std::memcpy(mapped, vertices.data(), vertexBytes);
            std::memcpy(static_cast<uint8_t*>(mapped) + vertexBytes, indices.data(), indexBytes);
            SDL_UnmapGPUTransferBuffer(device, staging);

            SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);

            SDL_GPUTransferBufferLocation srcVB{};
            srcVB.transfer_buffer = staging;
            srcVB.offset = 0;
            SDL_GPUBufferRegion dstVB{};
            dstVB.buffer = m_vertexBuffer;
            dstVB.offset = 0;
            dstVB.size = vertexBytes;
            SDL_UploadToGPUBuffer(copyPass, &srcVB, &dstVB, false);

            SDL_GPUTransferBufferLocation srcIB{};
            srcIB.transfer_buffer = staging;
            srcIB.offset = vertexBytes;
            SDL_GPUBufferRegion dstIB{};
            dstIB.buffer = m_indexBuffer;
            dstIB.offset = 0;
            dstIB.size = indexBytes;
            SDL_UploadToGPUBuffer(copyPass, &srcIB, &dstIB, false);

            SDL_EndGPUCopyPass(copyPass);
            SDL_ReleaseGPUTransferBuffer(device, staging);

            m_uploaded = true;
            return true;
        }

        bool updateInstances(
            SDL_GPUDevice* device,
            SDL_GPUCommandBuffer* commandBuffer,
            std::span<const glm::mat4> worldMatrices,
            std::string* error = nullptr)
        {
            if (!device || !commandBuffer)
            {
                if (error) *error = "Mesh::updateInstances requires a valid SDL_GPUDevice and SDL_GPUCommandBuffer.";
                return false;
            }
            if (worldMatrices.empty())
            {
                m_instanceCount = 0;
                return true;
            }

            const uint32_t needed = static_cast<uint32_t>(worldMatrices.size());
            if (needed > m_instanceCapacity || !m_instanceBuffer)
            {
                if (m_instanceBuffer)
                {
                    SDL_ReleaseGPUBuffer(device, m_instanceBuffer);
                    m_instanceBuffer = nullptr;
                }

                m_instanceCapacity = needed;
                SDL_GPUBufferCreateInfo instInfo{};
                instInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
                instInfo.size = m_instanceCapacity * sizeof(glm::mat4);
                m_instanceBuffer = SDL_CreateGPUBuffer(device, &instInfo);
                if (!m_instanceBuffer)
                {
                    if (error) *error = "SDL_CreateGPUBuffer failed for instance buffer.";
                    return false;
                }
            }

            const uint32_t uploadBytes = needed * sizeof(glm::mat4);
            SDL_GPUTransferBufferCreateInfo transferInfo{};
            transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
            transferInfo.size = uploadBytes;
            SDL_GPUTransferBuffer* staging = SDL_CreateGPUTransferBuffer(device, &transferInfo);
            if (!staging)
            {
                if (error) *error = "SDL_CreateGPUTransferBuffer failed for instance upload.";
                return false;
            }

            void* mapped = SDL_MapGPUTransferBuffer(device, staging, false);
            if (!mapped)
            {
                SDL_ReleaseGPUTransferBuffer(device, staging);
                if (error) *error = "SDL_MapGPUTransferBuffer failed for instance upload.";
                return false;
            }

            std::memcpy(mapped, worldMatrices.data(), uploadBytes);
            SDL_UnmapGPUTransferBuffer(device, staging);

            SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
            SDL_GPUTransferBufferLocation src{};
            src.transfer_buffer = staging;
            src.offset = 0;

            SDL_GPUBufferRegion dst{};
            dst.buffer = m_instanceBuffer;
            dst.offset = 0;
            dst.size = uploadBytes;

            SDL_UploadToGPUBuffer(copyPass, &src, &dst, true);
            SDL_EndGPUCopyPass(copyPass);
            SDL_ReleaseGPUTransferBuffer(device, staging);

            m_instanceCount = needed;
            return true;
        }

        void draw(SDL_GPURenderPass* renderPass) const
        {
            drawInstanced(renderPass, m_instanceCount == 0 ? 1u : m_instanceCount);
        }

        void drawInstanced(SDL_GPURenderPass* renderPass, uint32_t instanceCount, uint32_t firstInstance = 0) const
        {
            if (!renderPass || !m_uploaded || !m_vertexBuffer || !m_indexBuffer || indices.empty())
            {
                return;
            }

            SDL_GPUBufferBinding vertexBindings[2]{};
            vertexBindings[0].buffer = m_vertexBuffer;
            vertexBindings[0].offset = 0;

            uint32_t numVertexBindings = 1;
            if (m_instanceBuffer)
            {
                vertexBindings[1].buffer = m_instanceBuffer;
                vertexBindings[1].offset = 0;
                numVertexBindings = 2;
            }

            SDL_BindGPUVertexBuffers(renderPass, 0, vertexBindings, numVertexBindings);

            SDL_GPUBufferBinding indexBinding{};
            indexBinding.buffer = m_indexBuffer;
            indexBinding.offset = 0;
            SDL_BindGPUIndexBuffer(renderPass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

            SDL_DrawGPUIndexedPrimitives(
                renderPass,
                static_cast<uint32_t>(indices.size()),
                instanceCount,
                0,
                0,
                firstInstance);
        }

        void release(SDL_GPUDevice* device)
        {
            if (!device) return;
            if (m_vertexBuffer) { SDL_ReleaseGPUBuffer(device, m_vertexBuffer); m_vertexBuffer = nullptr; }
            if (m_indexBuffer) { SDL_ReleaseGPUBuffer(device, m_indexBuffer); m_indexBuffer = nullptr; }
            if (m_instanceBuffer) { SDL_ReleaseGPUBuffer(device, m_instanceBuffer); m_instanceBuffer = nullptr; }
            m_uploaded = false;
            m_instanceCount = 0;
            m_instanceCapacity = 0;
        }

    private:
        SDL_GPUBuffer* m_vertexBuffer = nullptr;
        SDL_GPUBuffer* m_indexBuffer = nullptr;
        SDL_GPUBuffer* m_instanceBuffer = nullptr;
        bool m_uploaded = false;
        uint32_t m_instanceCount = 0;
        uint32_t m_instanceCapacity = 0;
    };

    struct Prop
    {
        Transform transform{};
        std::shared_ptr<Mesh> mesh;
        Material material{};

        [[nodiscard]] AABB worldBounds() const
        {
            if (!mesh) return {};
            return mesh->localBounds.transformed(transform.matrix());
        }

        [[nodiscard]] bool collidesWith(const AABB& playerAabb) const
        {
            if (!mesh) return false;
            return worldBounds().intersects(playerAabb);
        }

        void draw(SDL_GPURenderPass* renderPass) const
        {
            if (mesh) mesh->draw(renderPass);
        }
    };

    class Model
    {
    public:
        Model() = default;

        explicit Model(std::shared_ptr<Mesh> inMesh)
            : m_mesh(std::move(inMesh)) {}

        [[nodiscard]] bool valid() const { return m_mesh != nullptr; }

        [[nodiscard]] std::shared_ptr<Mesh> mesh() const { return m_mesh; }

        [[nodiscard]] const AABB& localAabb() const
        {
            static AABB dummy{};
            return m_mesh ? m_mesh->localBounds : dummy;
        }

        void draw(SDL_GPURenderPass* renderPass) const
        {
            if (m_mesh) m_mesh->draw(renderPass);
        }

        void draw(SDL_GPURenderPass* renderPass, uint32_t instanceCount) const
        {
            if (m_mesh) m_mesh->drawInstanced(renderPass, instanceCount, 0);
        }

        [[nodiscard]] AABB worldAabb(const Transform& t) const
        {
            if (!m_mesh) return {};
            return m_mesh->localBounds.transformed(t.matrix());
        }

    private:
        std::shared_ptr<Mesh> m_mesh;
    };

    class PropBatch
    {
    public:
        void clear()
        {
            m_batches.clear();
        }

        void add(const Prop& prop)
        {
            if (!prop.mesh) return;
            BatchKey key{};
            key.mesh = prop.mesh.get();

            Batch& batch = m_batches[key];
            if (!batch.mesh)
            {
                batch.mesh = prop.mesh;
                batch.material = prop.material;
            }

            batch.transforms.push_back(prop.transform.matrix());
        }

        void add(const Model& model, const Transform& transform, const Material& material = {})
        {
            if (!model.valid()) return;
            Prop prop{};
            prop.mesh = model.mesh();
            prop.transform = transform;
            prop.material = material;
            add(prop);
        }

        bool uploadInstances(SDL_GPUDevice* device, SDL_GPUCommandBuffer* commandBuffer, std::string* error = nullptr)
        {
            for (auto& [_, batch] : m_batches)
            {
                if (!batch.mesh) continue;
                if (!batch.mesh->updateInstances(device, commandBuffer, std::span<const glm::mat4>(batch.transforms.data(), batch.transforms.size()), error))
                {
                    return false;
                }
            }
            return true;
        }

        void draw(SDL_GPURenderPass* renderPass, std::span<const Material> overrideMaterials = {}) const
        {
            size_t materialCursor = 0;
            for (const auto& [_, batch] : m_batches)
            {
                if (!batch.mesh) continue;

                if (!overrideMaterials.empty() && materialCursor < overrideMaterials.size())
                {
                    (void)overrideMaterials[materialCursor];
                }

                batch.mesh->drawInstanced(renderPass, static_cast<uint32_t>(batch.transforms.size()));
                ++materialCursor;
            }
        }

        [[nodiscard]] size_t batchCount() const
        {
            return m_batches.size();
        }

    private:
        struct BatchKey
        {
            Mesh* mesh = nullptr;

            bool operator==(const BatchKey& other) const
            {
                return mesh == other.mesh;
            }
        };

        struct BatchKeyHash
        {
            size_t operator()(const BatchKey& key) const noexcept
            {
                return std::hash<Mesh*>{}(key.mesh);
            }
        };

        struct Batch
        {
            std::shared_ptr<Mesh> mesh;
            Material material{};
            std::vector<glm::mat4> transforms;
        };

        std::unordered_map<BatchKey, Batch, BatchKeyHash> m_batches;
    };

    class RenderPipeline3D
    {
    public:
        ~RenderPipeline3D() = default;

        bool create(
            SDL_GPUDevice* device,
            const CompiledShaderBlob& vertexShader,
            const CompiledShaderBlob& fragmentShader,
            SDL_GPUTextureFormat colorFormat,
            SDL_GPUTextureFormat depthFormat,
            bool withDepth,
            std::string* error = nullptr)
        {
            if (!device)
            {
                if (error) *error = "RenderPipeline3D::create called with null device.";
                return false;
            }
            if (vertexShader.code.empty() || fragmentShader.code.empty())
            {
                if (error) *error = "RenderPipeline3D::create requires compiled shader bytecode.";
                return false;
            }

            SDL_GPUShaderCreateInfo vsInfo{};
            vsInfo.code = vertexShader.code.data();
            vsInfo.code_size = vertexShader.code.size();
            vsInfo.entrypoint = vertexShader.entryPoint.c_str();
            vsInfo.format = vertexShader.format;
            vsInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
            vsInfo.num_uniform_buffers = 2;

            SDL_GPUShaderCreateInfo fsInfo{};
            fsInfo.code = fragmentShader.code.data();
            fsInfo.code_size = fragmentShader.code.size();
            fsInfo.entrypoint = fragmentShader.entryPoint.c_str();
            fsInfo.format = fragmentShader.format;
            fsInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
            fsInfo.num_uniform_buffers = 2;

            SDL_GPUShader* vs = SDL_CreateGPUShader(device, &vsInfo);
            SDL_GPUShader* fs = SDL_CreateGPUShader(device, &fsInfo);
            if (!vs || !fs)
            {
                if (error) *error = "SDL_CreateGPUShader failed while creating render pipeline shaders.";
                if (vs) SDL_ReleaseGPUShader(device, vs);
                if (fs) SDL_ReleaseGPUShader(device, fs);
                return false;
            }

            SDL_GPUVertexBufferDescription vbDesc[2]{};
            vbDesc[0].slot = 0;
            vbDesc[0].pitch = sizeof(Vertex);
            vbDesc[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

            vbDesc[1].slot = 1;
            vbDesc[1].pitch = sizeof(glm::mat4);
            vbDesc[1].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;

            SDL_GPUVertexAttribute attrs[8]{};
            attrs[0] = {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, static_cast<Uint32>(offsetof(Vertex, position))};
            attrs[1] = {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, static_cast<Uint32>(offsetof(Vertex, normal))};
            attrs[2] = {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2, static_cast<Uint32>(offsetof(Vertex, uv))};
            attrs[3] = {3, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, static_cast<Uint32>(offsetof(Vertex, color))};

            attrs[4] = {4, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 0};
            attrs[5] = {5, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 16};
            attrs[6] = {6, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 32};
            attrs[7] = {7, 1, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, 48};

            SDL_GPUColorTargetDescription colorTarget{};
            colorTarget.format = colorFormat;
            colorTarget.blend_state.enable_blend = false;

            SDL_GPUGraphicsPipelineCreateInfo gpci{};
            gpci.vertex_shader = vs;
            gpci.fragment_shader = fs;
            gpci.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
            gpci.vertex_input_state.vertex_buffer_descriptions = vbDesc;
            gpci.vertex_input_state.num_vertex_buffers = 2;
            gpci.vertex_input_state.vertex_attributes = attrs;
            gpci.vertex_input_state.num_vertex_attributes = 8;
            gpci.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
            gpci.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
            gpci.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
            gpci.target_info.color_target_descriptions = &colorTarget;
            gpci.target_info.num_color_targets = 1;
            gpci.target_info.has_depth_stencil_target = withDepth;
            gpci.target_info.depth_stencil_format = depthFormat;
            gpci.depth_stencil_state.enable_depth_test = withDepth;
            gpci.depth_stencil_state.enable_depth_write = withDepth;
            gpci.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS;

            m_pipeline = SDL_CreateGPUGraphicsPipeline(device, &gpci);

            SDL_ReleaseGPUShader(device, vs);
            SDL_ReleaseGPUShader(device, fs);

            if (!m_pipeline)
            {
                if (error) *error = "SDL_CreateGPUGraphicsPipeline failed.";
                return false;
            }

            return true;
        }

        void bind(SDL_GPURenderPass* renderPass) const
        {
            if (m_pipeline && renderPass)
            {
                SDL_BindGPUGraphicsPipeline(renderPass, m_pipeline);
            }
        }

        void release(SDL_GPUDevice* device)
        {
            if (m_pipeline && device)
            {
                SDL_ReleaseGPUGraphicsPipeline(device, m_pipeline);
                m_pipeline = nullptr;
            }
        }

        [[nodiscard]] SDL_GPUGraphicsPipeline* raw() const { return m_pipeline; }

    private:
        SDL_GPUGraphicsPipeline* m_pipeline = nullptr;
    };

    class ModelLoader
    {
    public:
        explicit ModelLoader(SDL_GPUDevice* device = nullptr)
            : m_device(device)
        {
        }

        void setDevice(SDL_GPUDevice* device) { m_device = device; }

        Model load(const std::string& path)
        {
#if !MUSEUM3D_HAS_TINYGLTF
            throw std::runtime_error("tiny_gltf header not found. Add tiny_gltf_v3.h (or tiny_gltf.h) to include paths.");
#else
            tinygltf::TinyGLTF loader;
            tinygltf::Model gltf;
            std::string warn;
            std::string err;

            const bool ok = loader.LoadBinaryFromFile(&gltf, &err, &warn, path);
            if (!ok)
            {
                throw std::runtime_error("Failed to load glTF: " + path + " | " + err);
            }

            auto mesh = std::make_shared<Mesh>();
            parseModel(gltf, *mesh);

            if (m_device)
            {
                SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(m_device);
                if (commandBuffer)
                {
                    std::string uploadError;
                    if (!mesh->upload(m_device, commandBuffer, &uploadError))
                    {
                        SDL_SubmitGPUCommandBuffer(commandBuffer);
                        throw std::runtime_error("Mesh upload failed: " + uploadError);
                    }
                    if (!SDL_SubmitGPUCommandBuffer(commandBuffer))
                    {
                        throw std::runtime_error("SDL_SubmitGPUCommandBuffer failed after mesh upload.");
                    }
                }
            }

            return Model(mesh);
#endif
        }

    private:
#if MUSEUM3D_HAS_TINYGLTF
        static const tinygltf::Accessor& getAccessor(const tinygltf::Model& model, int accessorIndex)
        {
            if (accessorIndex < 0 || accessorIndex >= static_cast<int>(model.accessors.size()))
            {
                throw std::runtime_error("Invalid glTF accessor index.");
            }
            return model.accessors[accessorIndex];
        }

        static const uint8_t* accessorDataPtr(const tinygltf::Model& model, const tinygltf::Accessor& accessor)
        {
            const tinygltf::BufferView& view = model.bufferViews.at(accessor.bufferView);
            const tinygltf::Buffer& buffer = model.buffers.at(view.buffer);
            return buffer.data.data() + view.byteOffset + accessor.byteOffset;
        }

        static size_t accessorStride(const tinygltf::Model& model, const tinygltf::Accessor& accessor)
        {
            const tinygltf::BufferView& view = model.bufferViews.at(accessor.bufferView);
            const size_t stride = accessor.ByteStride(view);
            if (stride > 0) return stride;

            int components = 1;
            switch (accessor.type)
            {
            case TINYGLTF_TYPE_VEC2: components = 2; break;
            case TINYGLTF_TYPE_VEC3: components = 3; break;
            case TINYGLTF_TYPE_VEC4: components = 4; break;
            default: break;
            }

            size_t compSize = 4;
            switch (accessor.componentType)
            {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            case TINYGLTF_COMPONENT_TYPE_BYTE:
                compSize = 1; break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            case TINYGLTF_COMPONENT_TYPE_SHORT:
                compSize = 2; break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
            case TINYGLTF_COMPONENT_TYPE_INT:
            case TINYGLTF_COMPONENT_TYPE_FLOAT:
            default:
                compSize = 4; break;
            }

            return compSize * components;
        }

        static void readVec2(const tinygltf::Model& model, const tinygltf::Accessor& accessor, std::vector<glm::vec2>& out)
        {
            if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || accessor.type != TINYGLTF_TYPE_VEC2)
            {
                throw std::runtime_error("Unsupported UV accessor format. Expected float vec2.");
            }

            out.resize(accessor.count);
            const uint8_t* src = accessorDataPtr(model, accessor);
            const size_t stride = accessorStride(model, accessor);
            for (size_t i = 0; i < accessor.count; ++i)
            {
                const float* f = reinterpret_cast<const float*>(src + i * stride);
                out[i] = glm::vec2(f[0], f[1]);
            }
        }

        static void readVec3(const tinygltf::Model& model, const tinygltf::Accessor& accessor, std::vector<glm::vec3>& out)
        {
            if (accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT || accessor.type != TINYGLTF_TYPE_VEC3)
            {
                throw std::runtime_error("Unsupported vec3 accessor format. Expected float vec3.");
            }

            out.resize(accessor.count);
            const uint8_t* src = accessorDataPtr(model, accessor);
            const size_t stride = accessorStride(model, accessor);
            for (size_t i = 0; i < accessor.count; ++i)
            {
                const float* f = reinterpret_cast<const float*>(src + i * stride);
                out[i] = glm::vec3(f[0], f[1], f[2]);
            }
        }

        static void readIndices(const tinygltf::Model& model, const tinygltf::Accessor& accessor, std::vector<uint32_t>& out)
        {
            out.resize(accessor.count);
            const uint8_t* src = accessorDataPtr(model, accessor);
            const size_t stride = accessorStride(model, accessor);

            for (size_t i = 0; i < accessor.count; ++i)
            {
                const uint8_t* p = src + i * stride;
                switch (accessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    out[i] = *reinterpret_cast<const uint8_t*>(p);
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    out[i] = *reinterpret_cast<const uint16_t*>(p);
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    out[i] = *reinterpret_cast<const uint32_t*>(p);
                    break;
                default:
                    throw std::runtime_error("Unsupported index accessor format.");
                }
            }
        }

        static void parseModel(const tinygltf::Model& gltf, Mesh& outMesh)
        {
            outMesh.vertices.clear();
            outMesh.indices.clear();

            outMesh.localBounds.min = glm::vec3(std::numeric_limits<float>::max());
            outMesh.localBounds.max = glm::vec3(std::numeric_limits<float>::lowest());

            for (const tinygltf::Mesh& mesh : gltf.meshes)
            {
                for (const tinygltf::Primitive& primitive : mesh.primitives)
                {
                    glm::vec4 bakedColor(1.0f, 1.0f, 1.0f, 1.0f);
                    if (primitive.material >= 0 && primitive.material < static_cast<int>(gltf.materials.size()))
                    {
                        const tinygltf::Material& mat = gltf.materials[primitive.material];
                        const auto& factor = mat.pbrMetallicRoughness.baseColorFactor;
                        if (factor.size() >= 4)
                        {
                            bakedColor = glm::vec4(
                                static_cast<float>(factor[0]),
                                static_cast<float>(factor[1]),
                                static_cast<float>(factor[2]),
                                static_cast<float>(factor[3]));
                        }
                    }

                    const auto posIt = primitive.attributes.find("POSITION");
                    if (posIt == primitive.attributes.end()) continue;

                    std::vector<glm::vec3> positions;
                    std::vector<glm::vec3> normals;
                    std::vector<glm::vec2> uvs;
                    std::vector<uint32_t> primIndices;

                    readVec3(gltf, getAccessor(gltf, posIt->second), positions);

                    const auto nrmIt = primitive.attributes.find("NORMAL");
                    if (nrmIt != primitive.attributes.end())
                    {
                        readVec3(gltf, getAccessor(gltf, nrmIt->second), normals);
                    }
                    else
                    {
                        normals.assign(positions.size(), glm::vec3(0.0f, 1.0f, 0.0f));
                    }

                    const auto uvIt = primitive.attributes.find("TEXCOORD_0");
                    if (uvIt != primitive.attributes.end())
                    {
                        readVec2(gltf, getAccessor(gltf, uvIt->second), uvs);
                    }
                    else
                    {
                        uvs.assign(positions.size(), glm::vec2(0.0f));
                    }

                    if (primitive.indices >= 0)
                    {
                        readIndices(gltf, getAccessor(gltf, primitive.indices), primIndices);
                    }
                    else
                    {
                        primIndices.resize(positions.size());
                        for (uint32_t i = 0; i < static_cast<uint32_t>(positions.size()); ++i)
                        {
                            primIndices[i] = i;
                        }
                    }

                    const uint32_t baseVertex = static_cast<uint32_t>(outMesh.vertices.size());
                    outMesh.vertices.reserve(outMesh.vertices.size() + positions.size());
                    for (size_t i = 0; i < positions.size(); ++i)
                    {
                        Vertex v{};
                        v.position = positions[i];
                        v.normal = i < normals.size() ? normals[i] : glm::vec3(0.0f, 1.0f, 0.0f);
                        v.uv = i < uvs.size() ? uvs[i] : glm::vec2(0.0f);
                        v.color = bakedColor;
                        outMesh.vertices.push_back(v);

                        outMesh.localBounds.min = glm::min(outMesh.localBounds.min, v.position);
                        outMesh.localBounds.max = glm::max(outMesh.localBounds.max, v.position);
                    }

                    outMesh.indices.reserve(outMesh.indices.size() + primIndices.size());
                    for (uint32_t idx : primIndices)
                    {
                        outMesh.indices.push_back(baseVertex + idx);
                    }
                }
            }

            if (outMesh.vertices.empty() || outMesh.indices.empty())
            {
                throw std::runtime_error("glTF did not contain renderable mesh primitives.");
            }
        }
#endif

        SDL_GPUDevice* m_device = nullptr;
    };

} // namespace museum3d
