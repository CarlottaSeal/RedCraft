#include "WorldConfig.h"
#include "Game/Generator/WorldGenPipeline.h"
#include "Game/Player.hpp"
#include "PlayerInventory.h"
#include "Game/Gamecommon.hpp"

void WorldConfigManager::ApplyConfig(WorldGenPipeline* pipeline, const FarmWorldConfig& config)
{
    if (!pipeline)
        return;
    pipeline->SetWorldGenMode(WorldGenPipeline::WorldGenMode::FLAT_FARM);
    
    pipeline->SetFarmWorldCenter(config.m_centerChunk);
    pipeline->SetFarmWorldSize(config.m_chunkRadius);
    pipeline->SetFarmWorldHeight(config.m_farmlandHeight);
}

void WorldConfigManager::ApplyPlayerConfig(Player* player, const FarmWorldConfig& config)
{
    if (!player)
        return;
    player->SetPosition(config.m_playerSpawnPos);
    
    // if (config.giveStarterItems && player->m_inventory)
    // {
    //     PlayerInventory* inventory = player->m_inventory;
    //     
    //     // inventory->Clear();
    //     
    //     if (config.wheatSeeds > 0)
    //         inventory->AddItem(ItemType::WHEAT, config.wheatSeeds);
    //     
    //     if (config.carrotSeeds > 0)
    //         inventory->AddItem(ItemType::CARROT, config.carrotSeeds);
    //     
    //     if (config.potatoSeeds > 0)
    //         inventory->AddItem(ItemType::POTATO, config.potatoSeeds);
    //     
    //     if (config.beetrootSeeds > 0)
    //         inventory->AddItem(ItemType::BEETROOT, config.beetrootSeeds);
    //     
    //     if (config.bonemeal > 0)
    //         inventory->AddItem(ItemType::BONEMEAL, config.bonemeal);
    //     
    //     inventory->AddItem(ItemType::DIRT, 64);
    //     inventory->AddItem(ItemType::COBBLESTONE, 64);
    //     inventory->AddItem(ItemType::GLOWSTONE, 16);
    // }
}

void WorldConfigManager::SetupFarmWorld(
    WorldGenPipeline* pipeline, 
    Player* player, 
    const FarmWorldConfig& config)
{
    ApplyConfig(pipeline, config);
    ApplyPlayerConfig(player, config);
}