#include "CropSystem.h"
#include "Game/World.h"
#include "Game/Chunk.h"
#include "Game/Block.h"
#include "Game/BlockIterator.h"
#include "Game/Gamecommon.hpp" 
#include <random>
#include <algorithm>

#include "Game/Game.hpp"
#include "Game/Player.hpp"
#include "Game/UI/GameUIManager.h"

extern Game* g_theGame;

CropSystem::CropSystem(World* world)
    : m_world(world)
{
    Initialize();
}

CropSystem::~CropSystem()
{
    m_trackedCrops.clear();
    m_cropIndexByHash.clear();
}

void CropSystem::Initialize()
{
    CropDefinition::InitializeAllDefinitions();
    m_trackedCrops.reserve(1000);  
}

void CropSystem::Update(float deltaSeconds)
{
    m_tickTimer += deltaSeconds;
    
    while (m_tickTimer >= RANDOM_TICK_INTERVAL)
    {
        m_tickTimer -= RANDOM_TICK_INTERVAL;
        ProcessRandomTicks();
    }
    
    ProcessFixedIntervalGrowth(deltaSeconds);
    
    m_dropManager.Update(deltaSeconds);
}

void CropSystem::Render(Renderer* renderer) const
{
    m_dropManager.Render(renderer, *(Camera*)g_theGame->m_player->m_gameCamera, g_theGameUIManager->m_uiAtlas);
}

void CropSystem::ProcessRandomTicks()
{
    if (m_trackedCrops.empty())
        return;
    
    static std::mt19937 rng(std::random_device{}());
    int actualNum = MinI(CROPS_PER_TICK, (int)m_trackedCrops.size());
    
    for (int i = 0; i < actualNum; i++)
    {
        std::uniform_int_distribution<size_t> dist(0, m_trackedCrops.size() - 1);
        size_t index = dist(rng);
        
        TrackedCrop& crop = m_trackedCrops[index];
        BlockIterator iter = m_world->GetBlockIterator(crop.m_position);
        
        if (!iter.IsValid())
        {
            UnregisterCropByIndex(index);
            i--;
            continue;
        }
        
        Block* block = iter.GetBlock();
        if (!block || block->m_typeIndex != crop.m_blockType)
        {
            const CropDefinition* newDef = CropDefinition::GetDefinition(block ? block->m_typeIndex : 0);
            if (newDef)
            {
                crop.m_blockType = block->m_typeIndex;
                crop.m_definition = newDef;
            }
            else
            {
                UnregisterCropByIndex(index);
                i--;
                continue;
            }
        }
        
        if (crop.m_definition && crop.m_definition->m_usesRandomTick)
        {
            TryGrow(iter);
        }
    }
    // static std::mt19937 rng(std::random_device{}());
    //
    // for (const auto& [coords, chunk] : m_world->GetActiveChunks())
    // {
    //     if (!chunk) continue;
    //     
    //     std::uniform_int_distribution<int> xDist(0, CHUNK_SIZE_X - 1);
    //     std::uniform_int_distribution<int> yDist(0, CHUNK_SIZE_Y - 1);
    //     std::uniform_int_distribution<int> zDist(0, CHUNK_SIZE_Z - 1);
    //     
    //     for (int i = 0; i < RANDOM_TICKS_PER_CHUNK; i++)
    //     {
    //         int lx = xDist(rng);
    //         int ly = yDist(rng);
    //         int lz = zDist(rng);
    //
    //         BlockIterator iter = BlockIterator(chunk, IntVec3(lx, ly, lz));
    //         if (!iter.IsValid()) continue;
    //         
    //         Block* block = iter.GetBlock();
    //         if (!block) continue;
    //         if (!IsCrop(block->m_typeIndex)) continue;
    //         const CropDefinition* def = CropDefinition::GetDefinition(block->m_typeIndex);
    //         if (def && def->m_usesRandomTick)
    //         {
    //             TryGrow(iter);
    //         }
    //     }
    // }
}

void CropSystem::ProcessFixedIntervalGrowth(float deltaSeconds)
{
    for (size_t i = 0; i < m_trackedCrops.size(); )
    {
        TrackedCrop& crop = m_trackedCrops[i];
        
        const CropDefinition* def = crop.m_definition;
        
        // 更新计时器
        crop.m_growthTimer += deltaSeconds;
        
        // 检查是否到达生长间隔
        if (crop.m_growthTimer >= def->m_fixedGrowthInterval)
        {
            // 验证方块是否仍然有效
            BlockIterator iter = m_world->GetBlockIterator(crop.m_position);
            if (!iter.IsValid())
            {
                // 方块位置无效（区块卸载等），移除追踪
                UnregisterCropByIndex(i);
                continue;  // 不增加i，因为删除会改变数组
            }
            
            Block* block = iter.GetBlock();
            if (!block)
            {
                UnregisterCropByIndex(i);
                continue;
            }
            
            // 检查方块类型是否仍在定义范围内
            if (!def->IsBlockTypeInDefinition(block->m_typeIndex))
            {
                // 方块已被改变（玩家破坏/替换），移除追踪
                UnregisterCropByIndex(i);
                continue;
            }
            
            // 尝试生长
            if (TryGrow(iter))
            {
                // 重置计时器
                crop.m_growthTimer = 0.0f;
                
                // 更新方块类型
                crop.m_blockType = block->m_typeIndex;
                
                // 如果成熟，从固定间隔列表中移除（成熟后不再需要生长）
                if (def->IsMature(block->m_typeIndex))
                {
                    UnregisterCropByIndex(i);
                    continue;
                }
            }
        }
        
        i++;  // 只有在没有删除时才增加索引
    }
}


void CropSystem::RegisterCrop(const IntVec3& position, uint8_t blockType)
{
    const CropDefinition* def = CropDefinition::GetDefinition(blockType);
    
    if (!def || !def->m_usesRandomTick) 
        return;
    uint64_t hash = HashBlockPos(position);
    
    if (m_cropIndexByHash.find(hash) != m_cropIndexByHash.end())
        return;
    
    m_trackedCrops.emplace_back(position, blockType, def);
    m_cropIndexByHash[hash] = m_trackedCrops.size() - 1;
}

void CropSystem::UnregisterCrop(const IntVec3& position)
{
    uint64_t hash = HashBlockPos(position);
    auto it = m_cropIndexByHash.find(hash);
    
    if (it == m_cropIndexByHash.end())
        return;
    UnregisterCropByIndex(it->second);
}

// 内部辅助函数：通过索引删除作物
void CropSystem::UnregisterCropByIndex(size_t index)
{
    if (index >= m_trackedCrops.size())
        return;
    
    // 从哈希表中移除
    uint64_t hash = HashBlockPos(m_trackedCrops[index].m_position);
    m_cropIndexByHash.erase(hash);
    
    // 使用 swap-and-pop 技巧快速删除（O(1)而非O(n)）
    if (index != m_trackedCrops.size() - 1)
    {
        // 将最后一个元素移到当前位置
        m_trackedCrops[index] = m_trackedCrops.back();
        
        // 更新被移动元素的索引映射
        uint64_t movedHash = HashBlockPos(m_trackedCrops[index].m_position);
        m_cropIndexByHash[movedHash] = index;
    }
    
    m_trackedCrops.pop_back();
}

void CropSystem::UpdateCropType(const IntVec3& position, uint8_t newType)
{
    uint64_t hash = HashBlockPos(position);
    auto it = m_cropIndexByHash.find(hash);
    
    if (it != m_cropIndexByHash.end())
    {
        TrackedCrop& crop = m_trackedCrops[it->second];
        crop.m_blockType = newType;
        // 如果阶段变化很大，可能需要更新definition
    }
}

bool CropSystem::TryGrow(const BlockIterator& block)
{
    if (!block.IsValid()) return false;
    
    Block* b = block.GetBlock();
    if (!b) return false;
    
    const CropDefinition* def = CropDefinition::GetDefinition(b->m_typeIndex);
    if (!def) return false;
    
    // 已成熟
    if (def->IsMature(b->m_typeIndex)) return false;
    
    if (!CheckGrowthConditions(block, *def)) return false;
    
    if (def->m_usesRandomTick)
    {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        if (dist(rng) > def->m_baseGrowthChance) return false;
    }
    DebuggerPrintf("Tried to grow");
    PerformGrowth(block, *def);
    return true;
}

bool CropSystem::CheckGrowthConditions(const BlockIterator& block, const CropDefinition& def)
{
    // 自定义条件优先
    if (def.m_customConditionCheck)
    {
        if (!def.m_customConditionCheck(block, m_world))
            return false;
    }
    
    // 标准条件
    for (const GrowthRequirement& req : def.m_requirements)
    {
        switch (req.m_type)
        {
        case GrowthCondition::NEED_LIGHT:
        {
            uint8_t light = (uint8_t)MaxI(block.GetIndoorLight(), block.GetOutdoorLight());
            if (light < req.m_lightLevel) return false;
            break;
        }
        case GrowthCondition::NEED_DARKNESS:
        {
            uint8_t light = (uint8_t)MaxI(block.GetIndoorLight(), block.GetOutdoorLight());
            if (light > req.m_lightLevel) return false;
            break;
        }
        case GrowthCondition::NEED_BLOCK_BELOW:
        {
            BlockIterator below = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
            if (!below.IsValid()) return false;
            Block* bb = below.GetBlock();
            if (!bb || bb->m_typeIndex != req.m_requiredBlockType) return false;
            break;
        }
        case GrowthCondition::NEED_BLOCK_ADJACENT:
        {
            bool found = false;
            Direction dirs[] = {DIRECTION_NORTH, DIRECTION_SOUTH, DIRECTION_EAST, DIRECTION_WEST};
            for (Direction d : dirs)
            {
                BlockIterator adj = block.GetNeighborCrossBoundary(d);
                if (adj.IsValid())
                {
                    Block* ab = adj.GetBlock();
                    if (ab && ab->m_typeIndex == req.m_requiredBlockType)
                    {
                        found = true;
                        break;
                    }
                }
            }
            if (!found) return false;
            break;
        }
        case GrowthCondition::NEED_WATER_NEARBY:
        {
            bool found = false;
            IntVec3 center = block.GetGlobalCoords();
            for (int dx = -req.m_radius; dx <= req.m_radius && !found; dx++)
            {
                for (int dy = -req.m_radius; dy <= req.m_radius && !found; dy++)
                {
                    Block b = m_world->GetBlockAtWorldCoords(center.x + dx, center.y + dy, center.z);
                    if (b.m_typeIndex == BLOCK_TYPE_WATER)
                        found = true;
                }
            }
            if (!found) return false;
            break;
        }
        case GrowthCondition::NEED_SKY_ACCESS:
        {
            if (block.GetOutdoorLight() < 15) return false;
            break;
        }
        case GrowthCondition::RANDOM_CHANCE:
        {
            static std::mt19937 rng(std::random_device{}());
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            if (dist(rng) > req.m_chance) return false;
            break;
        }
        default:
            break;
        }
    }
    
    return true;
}

void CropSystem::PerformGrowth(const BlockIterator& block, const CropDefinition& def)
{
    if (def.m_customGrowthBehavior)
    {
        def.m_customGrowthBehavior(block, m_world, def);
        return;
    }
    
    Block* b = block.GetBlock();
    if (!b) return;
    
    uint8_t oldType = b->m_typeIndex;
    
    switch (def.m_pattern)
    {
    case GrowthPattern::STAGE_CHANGE:
    {
        int currentStage = def.GetStageIndex(b->m_typeIndex);
        if (currentStage < 0 || currentStage >= (int)def.m_stages.size() - 1) return;
        
        uint8_t newType = def.m_stages[currentStage + 1].m_blockType;
        b->SetType(newType);
        break;
    }
    
    case GrowthPattern::GROW_UP:
    {
        BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
        if (!above.IsValid()) return;
        
        Block* ab = above.GetBlock();
        if (!ab || ab->m_typeIndex != BLOCK_TYPE_AIR) return;
        
        int currentHeight = GetCurrentHeight(block, def);
        if (currentHeight >= def.m_maxHeight) return;
        
        if (b->m_typeIndex == def.m_stages[0].m_blockType)
            b->SetType(def.m_stages[1].m_blockType);
        
        ab->SetType(def.m_stages[0].m_blockType);
        
        if (Chunk* c = above.GetChunk())
        {
            c->MarkSelfDirty();
            m_world->m_hasDirtyChunk = true;
        }
        break;
    }
    
    default:
        break;
    }
    
    if (Chunk* c = block.GetChunk())
    {
        c->MarkSelfDirty();
        m_world->m_hasDirtyChunk = true;
    }
    
    m_world->OnBlockStateChanged(block, oldType, b->m_typeIndex);
}

bool CropSystem::ApplyBonemeal(const BlockIterator& block)
{
    if (!block.IsValid()) return false;
    
    Block* b = block.GetBlock();
    if (!b) return false;
    
    const CropDefinition* def = CropDefinition::GetDefinition(b->m_typeIndex);
    if (!def || !def->m_canBeBonemealed) return false;
    
    if (def->IsMature(b->m_typeIndex)) return false;
    
    // 自定义生长（如海草变高海草）
    if (def->m_customGrowthBehavior)
    {
        def->m_customGrowthBehavior(block, m_world, *def);
        return true;
    }
    
    uint8_t oldType = b->m_typeIndex;
    int currentStage = def->GetStageIndex(b->m_typeIndex);
    if (currentStage < 0) return false;
    
    // 随机前进 1-3 阶段
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, 3);
    int advance = dist(rng);
    
    int newStage = MinI(currentStage + advance, (int)def->m_stages.size() - 1);
    uint8_t newType = def->m_stages[newStage].m_blockType;
    
    b->SetType(newType);
    
    if (Chunk* c = block.GetChunk())
    {
        c->MarkSelfDirty();
        m_world->m_hasDirtyChunk = true;
    }
    m_world->OnBlockStateChanged(block, oldType, newType);
    
    return true;
}

// ============================================================================
// 收获（保持不变）
// ============================================================================
std::vector<CropDrop> CropSystem::Harvest(const BlockIterator& block, bool replant)
{
    std::vector<CropDrop> result;
    
    if (!block.IsValid()) return result;
    
    Block* b = block.GetBlock();
    if (!b) return result;
    
    const CropDefinition* def = CropDefinition::GetDefinition(b->m_typeIndex);
    if (!def) return result;
    
    int currentStage = def->GetStageIndex(b->m_typeIndex);
    if (currentStage < 0) return result;
    
    const CropStage& stage = def->m_stages[currentStage];
    if (!stage.m_isHarvestable) return result;
    
    // 自定义收获行为
    if (def->m_customHarvestBehavior)
    {
        def->m_customHarvestBehavior(block, m_world, *def);
    }
    
    // 收集掉落物
    result = stage.m_drops;
    
    uint8_t oldType = b->m_typeIndex;
    uint8_t newType;
    
    // 重新种植或移除
    if (replant && def->m_replantOnHarvest)
    {
        newType = def->m_stages[def->m_replantStage].m_blockType;
    }
    else
    {
        newType = BLOCK_TYPE_AIR;
        UnregisterCrop(block.GetGlobalCoords());
    }
    
    b->SetType(newType);
    
    if (Chunk* c = block.GetChunk())
    {
        c->MarkSelfDirty();
        m_world->m_hasDirtyChunk = true;
    }
    m_world->OnBlockStateChanged(block, oldType, newType);
    
    return result;
}

void CropSystem::RegisterFarmArea(const FarmArea& area)
{
    m_farmAreas.push_back(area);
}

void CropSystem::UnregisterFarmArea(const std::string& name)
{
    m_farmAreas.erase(
        std::remove_if(m_farmAreas.begin(), m_farmAreas.end(),
            [&name](const FarmArea& a) { return a.m_name == name; }),
        m_farmAreas.end());
}

std::vector<BlockIterator> CropSystem::GetCropsInArea(const FarmArea& area)
{
    std::vector<BlockIterator> result;
    
    for (int x = area.m_minCorner.x; x <= area.m_maxCorner.x; x++)
    {
        for (int y = area.m_minCorner.y; y <= area.m_maxCorner.y; y++)
        {
            for (int z = area.m_minCorner.z; z <= area.m_maxCorner.z; z++)
            {
                BlockIterator iter = m_world->GetBlockIterator(IntVec3(x, y, z));
                if (!iter.IsValid()) continue;
                
                Block* b = iter.GetBlock();
                if (b && CropDefinition::IsGrowable(b->m_typeIndex))
                {
                    result.push_back(iter);
                }
            }
        }
    }
    
    return result;
}

std::vector<BlockIterator> CropSystem::GetMatureCropsInArea(const FarmArea& area)
{
    std::vector<BlockIterator> result;
    
    for (int x = area.m_minCorner.x; x <= area.m_maxCorner.x; x++)
    {
        for (int y = area.m_minCorner.y; y <= area.m_maxCorner.y; y++)
        {
            for (int z = area.m_minCorner.z; z <= area.m_maxCorner.z; z++)
            {
                BlockIterator iter = m_world->GetBlockIterator(IntVec3(x, y, z));
                if (!iter.IsValid()) continue;
                
                Block* b = iter.GetBlock();
                if (!b) continue;
                
                const CropDefinition* def = CropDefinition::GetDefinition(b->m_typeIndex);
                if (def && def->IsHarvestable(b->m_typeIndex))
                {
                    result.push_back(iter);
                }
            }
        }
    }
    
    return result;
}

int CropSystem::HarvestArea(const FarmArea& area, bool replant)
{
    std::vector<BlockIterator> crops = GetMatureCropsInArea(area);
    int count = 0;
    
    for (const BlockIterator& crop : crops)
    {
        std::vector<CropDrop> drops = Harvest(crop, replant);
        if (!drops.empty())
        {
            count++;
            // 生成掉落物实体
            IntVec3 pos = crop.GetGlobalCoords();
            for (const CropDrop& drop : drops)
            {
                int dropCount = drop.RollCount();
                if (dropCount > 0)
                {
                    SpawnDropAtBlock(pos, drop.m_itemName, dropCount);
                }
            }
        }
    }
    
    return count;
}

CropStats CropSystem::GetStatsInArea(const FarmArea& area)
{
    CropStats stats;
    
    for (int x = area.m_minCorner.x; x <= area.m_maxCorner.x; x++)
    {
        for (int y = area.m_minCorner.y; y <= area.m_maxCorner.y; y++)
        {
            for (int z = area.m_minCorner.z; z <= area.m_maxCorner.z; z++)
            {
                BlockIterator iter = m_world->GetBlockIterator(IntVec3(x, y, z));
                if (!iter.IsValid()) continue;
                
                Block* b = iter.GetBlock();
                if (!b) continue;
                
                const CropDefinition* def = CropDefinition::GetDefinition(b->m_typeIndex);
                if (!def) continue;
                
                stats.m_totalCrops++;
                stats.m_countByType[def->m_name]++;
                
                if (def->IsMature(b->m_typeIndex))
                    stats.m_matureCrops++;
                else
                    stats.m_growingCrops++;
            }
        }
    }
    
    return stats;
}

bool CropSystem::IsGrowable(uint8_t blockType) const
{
    return CropDefinition::IsGrowable(blockType);
}

float CropSystem::GetGrowthProgress(const BlockIterator& block) const
{
    if (!block.IsValid()) return 0.0f;
    
    Block* b = block.GetBlock();
    if (!b) return 0.0f;
    
    const CropDefinition* def = CropDefinition::GetDefinition(b->m_typeIndex);
    if (!def) return 0.0f;
    
    return def->GetGrowthProgress(b->m_typeIndex);
}

int CropSystem::GetCurrentHeight(const BlockIterator& block, const CropDefinition& def)
{
    int height = 1;
    BlockIterator current = block;
    
    // 向下计数同类方块
    while (true)
    {
        BlockIterator below = current.GetNeighborCrossBoundary(DIRECTION_DOWN);
        if (!below.IsValid()) break;
        
        Block* bb = below.GetBlock();
        if (!bb || !def.IsBlockTypeInDefinition(bb->m_typeIndex)) break;
        
        height++;
        current = below;
    }
    
    return height;
}

uint64_t CropSystem::HashBlockPos(const IntVec3& pos) const
{
    uint64_t h = 0;
    h ^= std::hash<int>()(pos.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>()(pos.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
    h ^= std::hash<int>()(pos.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
    return h;
}


void CropSystem::OnCropDestroyed(const IntVec3& position, ItemType droppedItem)
{
    // 直接使用内部的掉落物管理器
    m_dropManager.SpawnCropDrops(position, droppedItem);
    UnregisterCrop(position);
}

void CropSystem::SpawnDrop(const Vec3& position, uint8_t itemType, int count)
{
    m_dropManager.SpawnDrop(position, itemType, count);
}

void CropSystem::SpawnDropAtBlock(const IntVec3& blockPos, std::string itemName, int count)
{
    ItemType itemType = StringToItemType(itemName);
    m_dropManager.SpawnDropAtBlock(blockPos, itemType, count);
}

std::vector<PendingDrop> CropSystem::CollectDropsNear(const Vec3& position, float radius)
{
    return m_dropManager.CollectDropsNear(position, radius);
}

std::vector<PendingDrop> CropSystem::CollectDropsInArea(const IntVec3& minCorner, const IntVec3& maxCorner)
{
    return m_dropManager.CollectDropsInArea(minCorner, maxCorner);
}

void CropSystem::ClearAllDrops()
{
    m_dropManager.ClearAllDrops();
}

DropStatistics CropSystem::GetDropStatistics() const
{
    return m_dropManager.GetStatistics();
}

int CropSystem::GetTotalDropCount() const
{
    return m_dropManager.GetTotalDropCount();
}

int CropSystem::GetTotalItemCount() const
{
    return m_dropManager.GetTotalItemCount();
}

void CropSystem::SetDropDebugRender(bool enable)
{
    m_dropManager.SetDebugRender(enable);
}