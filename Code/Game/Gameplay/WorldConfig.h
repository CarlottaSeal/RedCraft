#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Game/Player.hpp"
#include "Game/Generator/WorldGenPipeline.h"

class WorldGenPipeline;
class Player;

inline std::string GetWorldTypeNameStatic(WorldGenPipeline::WorldGenMode mode)
{
    switch(mode)
    {
    case WorldGenPipeline::WorldGenMode::NORMAL:   return "Normal";
    case WorldGenPipeline::WorldGenMode::FLAT_FARM: return "Farmland";
    }
    return "Normal";
}
inline WorldGenPipeline::WorldGenMode GetWorldTypeFromString(const std::string& typeName)
{
    if (typeName == "Normal")   return WorldGenPipeline::WorldGenMode::NORMAL;
    if (typeName == "Farmland") return WorldGenPipeline::WorldGenMode::FLAT_FARM;
    return WorldGenPipeline::WorldGenMode::NORMAL; 
}

struct FarmWorldConfig
{
    int m_chunkRadius = 6;           
    int m_farmlandHeight = 64;        
    IntVec2 m_centerChunk = IntVec2(0, 0);
    
    Vec3 m_playerSpawnPos = Vec3(0.0f, 0.0f, 65.0f); 
    
    bool m_giveStarterItems = true;     // 是否给予初始物品
    int wheatSeeds = 64;              // 小麦种子数量
    int carrotSeeds = 32;             // 胡萝卜种子数量
    int potatoSeeds = 32;             // 土豆种子数量
    int beetrootSeeds = 16;           // 甜菜根种子数量
    int bonemeal = 16;                // 骨粉数量
    
    void GetWorldBounds(int& minX, int& maxX, int& minY, int& maxY) const
    {
        minX = (m_centerChunk.x - m_chunkRadius) * 16;
        maxX = (m_centerChunk.x + m_chunkRadius + 1) * 16;
        minY = (m_centerChunk.y - m_chunkRadius) * 16;
        maxY = (m_centerChunk.y + m_chunkRadius + 1) * 16;
    }
    
    int GetTotalChunks() const
    {
        int diameter = m_chunkRadius * 2 + 1;
        return diameter * diameter;
    }
    
    int GetTotalBlocks() const
    {
        int diameter = m_chunkRadius * 2 + 1;
        int blockDiameter = diameter * 16;
        return blockDiameter * blockDiameter;
    }
};

// struct NormalWorldConfig
// {
//     Vec3 playerSpawnPos = Vec3(0.0f, 0.0f, 80.0f);
//     bool giveStarterItems = false;
// };

class WorldConfigManager
{
public:
    enum class WorldType
    {
        NORMAL,     
        FLAT_FARM   
    };
    
    static FarmWorldConfig CreateDefaultFarmWorld()
    {
        FarmWorldConfig config;
        config.m_chunkRadius = 6;             
        config.m_farmlandHeight = 64;
        config.m_centerChunk = IntVec2(0, 0);
        config.m_playerSpawnPos = Vec3(0.0f, 0.0f, 65.0f);
        config.m_giveStarterItems = true;
        return config;
    }
    
    static FarmWorldConfig CreateCustomFarmWorld(
        int chunkRadius, 
        int farmHeight = 64,
        IntVec2 center = IntVec2(0, 0))
    {
        FarmWorldConfig config;
        config.m_chunkRadius = chunkRadius;
        config.m_farmlandHeight = farmHeight;
        config.m_centerChunk = center;
        config.m_playerSpawnPos = Vec3(
            (float)(center.x * 16), 
            (float)(center.y * 16), 
            (float)(farmHeight + 1)
        );
        config.m_giveStarterItems = true;
        return config;
    }
    
    static void ApplyConfig(WorldGenPipeline* pipeline, const FarmWorldConfig& config);
    static void ApplyPlayerConfig(Player* player, const FarmWorldConfig& config);
    static void SetupFarmWorld(
        WorldGenPipeline* pipeline, 
        Player* player, 
        const FarmWorldConfig& config);
};

namespace WorldSetup
{
    inline void CreateSmallFarmWorld(WorldGenPipeline* pipeline, Player* player)
    {
        auto config = WorldConfigManager::CreateDefaultFarmWorld();
        WorldConfigManager::SetupFarmWorld(pipeline, player, config);
    }
    inline void CreateLargeFarmWorld(WorldGenPipeline* pipeline, Player* player)
    {
        auto config = WorldConfigManager::CreateCustomFarmWorld(10); // radius=10 → 20x20 chunks
        WorldConfigManager::SetupFarmWorld(pipeline, player, config);
    }
    inline void CreateFarmAtPlayerLocation(
        WorldGenPipeline* pipeline, 
        Player* player,
        int chunkRadius = 3)
    {
        Vec3 playerPos = player->GetPosition();
        IntVec2 playerChunk(
            static_cast<int>(playerPos.x) / 16,
            static_cast<int>(playerPos.y) / 16
        );
        
        auto config = WorldConfigManager::CreateCustomFarmWorld(
            chunkRadius, 
            64, 
            playerChunk
        );
        WorldConfigManager::SetupFarmWorld(pipeline, player, config);
    }
}