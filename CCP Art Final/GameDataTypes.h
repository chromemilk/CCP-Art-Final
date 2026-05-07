#pragma once

#include "GameEngine.h"

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <glm/glm.hpp>

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
    Levels level = Levels::MUSEUM;
};

struct KeyPickup
{
    std::string keyName;
    float x = 0.f;
    float y = 0.f;
    bool collected = false;
    int propIndex = -1;
    int modelIndex = -1;
    Levels level = Levels::MUSEUM;
    float modelHeightOffset = 0.2f;
};

struct CaveQuizQuestion
{
    std::string question;
    std::array<std::string, 4> options;
    int correctOption = 0;
};

struct MindTrapPhase
{
    std::string prompt;
    std::array<std::string, 3> commands;
    std::array<std::string, 3> options;
    std::array<std::string, 3> results;
    int surrenderOption = -1;
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
    Levels level = Levels::MUSEUM;
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
    int targetCombo[ 3 ] = { 0, 0, 0 };
    float x = 0.f;
    float y = 0.f;
    bool solved = false;
    std::string rewardKey;
};

struct WeaponPickup
{
    std::string weaponName;
    float x = 0.f;
    float y = 0.f;
    bool collected = false;
    int modelIndex = -1;
    Levels level = Levels::MUSEUM;
};

struct CombatState
{
    bool active = false;
    bool hasRevolver = false;
    int loadedAmmo = 0;
    int reserveAmmo = 0;
};

struct DialogueLine
{
    std::string text;
    float duration = 3.0f;
};

struct CpuModel
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec2> uvs;
    std::vector<uint32_t> indices;
    std::vector<Image> baseColorTextures;
    std::vector<SDL_Texture *> hwTextures;
    std::vector<int> triangleTextureIndex;
    std::vector<glm::vec4> triangleBaseColorFactor;
    glm::vec3 boundsMin{ 0.0f };
    glm::vec3 boundsMax{ 0.0f };
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
    Uint32 tint = 0;
    bool visible = true;
    bool editorPlaced = false;
    std::string editorAssetName;
    float editorTargetScale = 1.0f;
};

struct EditorAssetDef
{
    std::string label;
    std::string assetName;
    float worldSize = 0.7f;
    Uint32 tint = 0;
    float pitch = 0.0f;
};

struct LevelDef
{
    std::string name;
    std::string folder;
    std::string mapFile = "map.txt";
    float spawnX = 2.0f, spawnY = 9.5f, spawnDirDeg = 0.f;
    int levelId = 0;
    Uint32 ambianceTint = 0;
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

struct NotePickupVisual
{
    int propIndex = -1;
    int modelIndex = -1;
};

enum GameState
{
    STATE_MENU,
    STATE_GAME,
    STATE_ENDING,
    STATE_MIND_TRAP,
    STATE_BRIEFING
};
