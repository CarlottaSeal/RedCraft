#include "CropSystem.h"
#include "Game/World.h"
#include "Game/Chunk.h"
#include "Game/Block.h"
#include <random>

#include "Engine/Core/Clock.hpp"
#include "Game/Game.hpp"

CropSystem::CropSystem(World* world)
    : m_world(world)
{
}

void CropSystem::Initialize()
{
    {
        GrowableDefinition wheat;
        wheat.m_name = "wheat";
        wheat.m_category = GrowableDefinition::Category::PLANT;
        wheat.m_baseGrowthChance = 0.15f;
        wheat.m_usesRandomTick = true;
        
        // 8个生长阶段
        for (int i = 0; i <= 7; i++)
        {
            GrowthStage stage;
            stage.m_blockType = BLOCK_TYPE_WHEAT_0 + i;
            stage.m_isMature = (i == 7);
            wheat.m_stages.push_back(stage);
        }
        
        // 生长条件: 需要农田 + 光照 >= 9
        wheat.m_requirements.push_back(GrowthConditions::NeedBlockBelow(BLOCK_TYPE_FARMLAND));
        wheat.m_requirements.push_back(GrowthConditions::NeedLight(9));
        
        RegisterGrowable(wheat);
    }
    {
        GrowableDefinition carrot;
        carrot.m_name = "carrot";
        carrot.m_category = GrowableDefinition::Category::PLANT;
        carrot.m_baseGrowthChance = 0.20f;
        carrot.m_usesRandomTick = true;
        
        for (int i = 0; i <= 3; i++)
        {
            GrowthStage stage;
            stage.m_blockType = BLOCK_TYPE_CARROTS_0 + i;
            stage.m_isMature = (i == 3);
            carrot.m_stages.push_back(stage);
        }
        
        carrot.m_requirements.push_back(GrowthConditions::NeedBlockBelow(BLOCK_TYPE_FARMLAND));
        carrot.m_requirements.push_back(GrowthConditions::NeedLight(9));
        
        RegisterGrowable(carrot);
    }
    {
        GrowableDefinition potato;
        potato.m_name = "potato";
        potato.m_category = GrowableDefinition::Category::PLANT;
        potato.m_baseGrowthChance = 0.20f;
        potato.m_usesRandomTick = true;
        
        for (int i = 0; i <= 3; i++)
        {
            GrowthStage stage;
            stage.m_blockType = BLOCK_TYPE_POTATOES_0 + i;
            stage.m_isMature = (i == 3);
            potato.m_stages.push_back(stage);
        }
        
        potato.m_requirements.push_back(GrowthConditions::NeedBlockBelow(BLOCK_TYPE_FARMLAND));
        potato.m_requirements.push_back(GrowthConditions::NeedLight(9));
        
        RegisterGrowable(potato);
    }
    
    // ========== 示例: 地狱疣 (需要黑暗 + 灵魂沙) ==========
    /*
    {
        GrowableDefinition netherWart;
        netherWart.m_name = "nether_wart";
        netherWart.m_category = GrowableDefinition::Category::FUNGUS;
        netherWart.m_baseGrowthChance = 0.10f;
        netherWart.m_usesRandomTick = true;
        
        for (int i = 0; i <= 3; i++)
        {
            GrowthStage stage;
            stage.m_blockType = BLOCK_TYPE_NETHER_WART_0 + i;
            stage.m_isMature = (i == 3);
            netherWart.m_stages.push_back(stage);
        }
        
        // 地狱疣不需要光照，需要灵魂沙
        netherWart.m_requirements.push_back(GrowthConditions::NeedBlockBelow(BLOCK_TYPE_SOUL_SAND));
        
        RegisterGrowable(netherWart);
    }
    */
    // ========== 示例: 海带 (水生) ==========
    /*
    {
        GrowableDefinition kelp;
        kelp.m_name = "kelp";
        kelp.m_category = GrowableDefinition::Category::AQUATIC;
        kelp.m_baseGrowthChance = 0.14f;
        kelp.m_usesRandomTick = true;
        
        // 海带向上生长，只有2个状态：茎和顶端
        GrowthStage stem, top;
        stem.m_blockType = BLOCK_TYPE_KELP;
        stem.m_isMature = false;
        top.m_blockType = BLOCK_TYPE_KELP_TOP;
        top.m_isMature = true;
        
        kelp.m_stages.push_back(stem);
        kelp.m_stages.push_back(top);
        
        // 需要在水中
        kelp.m_requirements.push_back(GrowthConditions::NeedBlockBelow(BLOCK_TYPE_KELP)); // 或沙子
        
        // 自定义生长行为：向上生长而不是改变自身
        kelp.m_customGrowthBehavior = [](const BlockIterator& block, World* world)
        {
            BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
            if (above.IsValid())
            {
                Block* aboveBlock = above.GetBlock();
                if (aboveBlock && aboveBlock->m_typeIndex == BLOCK_TYPE_WATER)
                {
                    // 当前变成茎，上方变成新顶端
                    block.GetBlock()->SetType(BLOCK_TYPE_KELP);
                    aboveBlock->SetType(BLOCK_TYPE_KELP_TOP);
                }
            }
        };
        
        RegisterGrowable(kelp);
    }
    */
    // ========== 示例: 鱼卵 (固定时间孵化) ==========
    /*
    {
        GrowableDefinition fishEgg;
        fishEgg.m_name = "fish_egg";
        fishEgg.m_category = GrowableDefinition::Category::AQUATIC;
        fishEgg.m_usesRandomTick = false;           // 不使用随机tick
        fishEgg.m_fixedGrowthInterval = 60.0f;      // 60秒孵化
        
        GrowthStage egg, hatching, hatched;
        egg.m_blockType = BLOCK_TYPE_FISH_EGG;
        egg.m_isMature = false;
        hatching.m_blockType = BLOCK_TYPE_FISH_EGG_HATCHING;
        hatching.m_isMature = false;
        hatched.m_blockType = BLOCK_TYPE_AIR;  // 孵化后消失，生成实体
        hatched.m_isMature = true;
        
        fishEgg.m_stages.push_back(egg);
        fishEgg.m_stages.push_back(hatching);
        fishEgg.m_stages.push_back(hatched);
        
        // 自定义行为：孵化时生成鱼实体
        fishEgg.m_customGrowthBehavior = [](const BlockIterator& block, World* world)
        {
            Block* b = block.GetBlock();
            if (b->m_typeIndex == BLOCK_TYPE_FISH_EGG_HATCHING)
            {
                // 孵化！移除方块，生成鱼
                IntVec3 pos = block.GetGlobalCoords();
                b->SetType(BLOCK_TYPE_AIR);
                // world->SpawnEntity(ENTITY_FISH, Vec3(pos.x + 0.5f, pos.y + 0.5f, pos.z + 0.5f));
            }
        };
        
        RegisterGrowable(fishEgg);
    }
    */
    // ========== 示例: 蜜蜂巢 (条件复杂) ==========
    /*
    {
        GrowableDefinition beeNest;
        beeNest.m_name = "bee_nest";
        beeNest.m_category = GrowableDefinition::Category::ANIMAL;
        beeNest.m_baseGrowthChance = 0.05f;  // 很低的概率
        beeNest.m_usesRandomTick = true;
        
        // 蜂蜜等级 0-5
        for (int i = 0; i <= 5; i++)
        {
            GrowthStage stage;
            stage.m_blockType = BLOCK_TYPE_BEE_NEST_0 + i;
            stage.m_isMature = (i == 5);
            beeNest.m_stages.push_back(stage);
        }
        
        // 需要附近有花
        beeNest.m_customConditionCheck = [](const BlockIterator& block, World* world) -> bool
        {
            // 检查5格范围内是否有花
            IntVec3 center = block.GetGlobalCoords();
            for (int dx = -5; dx <= 5; dx++)
            {
                for (int dy = -5; dy <= 5; dy++)
                {
                    for (int dz = -5; dz <= 5; dz++)
                    {
                        BlockIterator check = world->GetBlockIterator(
                            IntVec3(center.x + dx, center.y + dy, center.z + dz));
                        if (check.IsValid())
                        {
                            Block* b = check.GetBlock();
                            if (b && IsFlower(b->m_typeIndex))  // 假设有IsFlower函数
                                return true;
                        }
                    }
                }
            }
            return false;
        };
        
        RegisterGrowable(beeNest);
    }
    */
}

// 注册可生长实体
void CropSystem::RegisterGrowable(const GrowableDefinition& def)
{
    m_definitions.push_back(def);
    const GrowableDefinition* defPtr = &m_definitions.back();
    
    // 为每个阶段的方块类型建立映射
    for (const GrowthStage& stage : def.m_stages)
    {
        m_blockToDefinition[stage.m_blockType] = defPtr;
    }
}

void CropSystem::Update(float deltaSeconds)
{
    // 处理随机tick
    m_tickTimer += deltaSeconds;
    while (m_tickTimer >= RANDOM_TICK_INTERVAL)
    {
        m_tickTimer -= RANDOM_TICK_INTERVAL;
        ProcessRandomTicks();
    }
    
    // 处理固定间隔生长
    ProcessFixedIntervalGrowth(deltaSeconds);
}

void CropSystem::ProcessRandomTicks()
{
    if (!m_world) return;
    
    static std::mt19937 rng(std::random_device{}()); //TODO
    
    for (auto& pair :m_world->m_activeChunks)
    {
        Chunk* chunk = pair.second;
        if (!chunk)
            continue;
        
        // 每个区块随机选择几个方块
        for (int i = 0; i < RANDOM_TICKS_PER_CHUNK; i++)
        {
            int localX = rng() % CHUNK_SIZE_X;
            int localY = rng() % CHUNK_SIZE_Y;
            int localZ = rng() % CHUNK_SIZE_Z;
            
            int blockIndex = chunk->GetBlockLocalIndexFromLocalCoords(localX, localY, localZ);
            BlockIterator block(chunk, blockIndex);
            
            if (!block.IsValid())
                continue;
            
            Block* b = block.GetBlock();
            if (!b)
                continue;
            
            // 检查是否是可生长方块
            auto it = m_blockToDefinition.find(b->m_typeIndex);
            if (it == m_blockToDefinition.end())
                continue;
            
            const GrowableDefinition* def = it->second;
            if (!def->m_usesRandomTick)
                continue;
            
            TryGrow(block);
        }
    }
}

void CropSystem::ProcessFixedIntervalGrowth(float deltaSeconds)
{
    float currentTime = m_world ? (float)m_world->m_owner->m_gameClock->GetTotalSeconds() : 0.0f;
    
    for (auto it = m_fixedGrowthEntities.begin(); it != m_fixedGrowthEntities.end(); )
    {
        FixedGrowthEntry& entry = *it;
        
        if (!entry.m_block.IsValid())
        {
            it = m_fixedGrowthEntities.erase(it);
            continue;
        }
        
        if (currentTime >= entry.m_nextGrowthTime)
        {
            Block* b = entry.m_block.GetBlock();
            auto defIt = m_blockToDefinition.find(b->m_typeIndex);
            
            if (defIt != m_blockToDefinition.end())
            {
                const GrowableDefinition* def = defIt->second;
                
                if (CheckGrowthConditions(entry.m_block, *def))
                {
                    PerformGrowth(entry.m_block, *def);
                    entry.m_nextGrowthTime = currentTime + def->m_fixedGrowthInterval;
                }
            }
        }
        
        ++it;
    }
}

bool CropSystem::TryGrow(const BlockIterator& block)
{
    if (!block.IsValid())
        return false;
    
    Block* b = block.GetBlock();
    if (!b)
        return false;
    
    auto it = m_blockToDefinition.find(b->m_typeIndex);
    if (it == m_blockToDefinition.end())
        return false;
    
    const GrowableDefinition* def = it->second;
    
    // 检查是否已成熟
    int currentStage = GetCurrentStageIndex(b->m_typeIndex, *def);
    if (currentStage < 0 || def->m_stages[currentStage].m_isMature)
        return false;
    
    // 检查生长条件
    if (!CheckGrowthConditions(block, *def))
        return false;
    
    // 基础概率检查
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    if (dist(rng) > def->m_baseGrowthChance)
        return false;
    
    // 执行生长
    PerformGrowth(block, *def);
    return true;
}

bool CropSystem::CheckGrowthConditions(const BlockIterator& block, const GrowableDefinition& def) const
{
    // 先检查自定义条件
    if (def.m_customConditionCheck)
    {
        if (!def.m_customConditionCheck(block, m_world))
            return false;
    }
    
    // 检查所有标准条件
    for (const GrowthRequirement& req : def.m_requirements)
    {
        if (!CheckSingleCondition(block, req))
            return false;
    }
    
    return true;
}

bool CropSystem::CheckSingleCondition(const BlockIterator& block, const GrowthRequirement& req) const
{
    if (!block.IsValid())
        return false;
    
    Block* b = block.GetBlock();
    if (!b)
        return false;
    
    switch (req.m_type)
    {
    case GrowthCondition::NONE:
        return true;
        
    case GrowthCondition::NEED_LIGHT:
    {
        uint8_t light = b->GetOutdoorLight();
        return light >= req.m_lightLevel;
    }
    
    case GrowthCondition::NEED_DARKNESS:
    {
        uint8_t light = b->GetOutdoorLight();
        return light <= req.m_lightLevel;
    }
    
    case GrowthCondition::NEED_BLOCK_BELOW:
    {
        BlockIterator below = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
        if (!below.IsValid())
            return false;
        Block* belowBlock = below.GetBlock();
        return belowBlock && belowBlock->m_typeIndex == req.m_requiredBlockType;
    }
    
    case GrowthCondition::NEED_BLOCK_ADJACENT:
    {
        for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
        {
            BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
            if (neighbor.IsValid())
            {
                Block* nb = neighbor.GetBlock();
                if (nb && nb->m_typeIndex == req.m_requiredBlockType)
                    return true;
            }
        }
        return false;
    }
    
    case GrowthCondition::NEED_WATER_NEARBY:
        return HasWaterNearby(block, req.m_radius);
    
    case GrowthCondition::NEED_SKY_ACCESS:
        return b->IsSky();
    
    case GrowthCondition::RANDOM_CHANCE:
    {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng) <= req.m_chance;
    }
    
    case GrowthCondition::CUSTOM:
        // 自定义条件通过 m_customConditionCheck 处理
        return true;
        
    default:
        return true;
    }
}

bool CropSystem::HasWaterNearby(const BlockIterator& block, int radius) const
{
    if (!block.IsValid() || !m_world)
        return false;
    
    IntVec3 center = block.GetGlobalCoords();
    
    for (int dx = -radius; dx <= radius; dx++)
    {
        for (int dy = -radius; dy <= radius; dy++)
        {
            // 只检查同一层和下一层
            for (int dz = -1; dz <= 0; dz++)
            {
                BlockIterator check = m_world->GetBlockIterator(
                    IntVec3(center.x + dx, center.y + dy, center.z + dz));
                
                if (check.IsValid())
                {
                    Block* b = check.GetBlock();
                    if (b && b->m_typeIndex == BLOCK_TYPE_WATER)
                        return true;
                }
            }
        }
    }
    
    return false;
}

void CropSystem::PerformGrowth(const BlockIterator& block, const GrowableDefinition& def)
{
    if (!block.IsValid())
        return;
    
    Block* b = block.GetBlock();
    if (!b)
        return;
    
    // 如果有自定义生长行为，使用它
    if (def.m_customGrowthBehavior)
    {
        def.m_customGrowthBehavior(block, m_world);
        return;
    }
    
    // 默认行为：进入下一阶段
    int currentStage = GetCurrentStageIndex(b->m_typeIndex, def);
    if (currentStage < 0 || currentStage >= (int)def.m_stages.size() - 1)
        return;
    
    uint8_t oldType = b->m_typeIndex;
    uint8_t newType = def.m_stages[currentStage + 1].m_blockType;
    
    b->SetType(newType);
    
    // 通知世界方块状态改变（用于侦测器）
    if (m_world)
    {
        m_world->OnBlockStateChanged(block, oldType, newType);
        
        // 标记区块需要重建mesh
        Chunk* chunk = block.GetChunk();
        if (chunk)
        {
            chunk->MarkSelfDirty();
            m_world->m_hasDirtyChunk = true;
        }
    }
}

bool CropSystem::IsGrowable(uint8_t blockType) const
{
    return m_blockToDefinition.find(blockType) != m_blockToDefinition.end();
}

bool CropSystem::IsMature(uint8_t blockType) const
{
    auto it = m_blockToDefinition.find(blockType);
    if (it == m_blockToDefinition.end())
        return false;
    
    const GrowableDefinition* def = it->second;
    for (const GrowthStage& stage : def->m_stages)
    {
        if (stage.m_blockType == blockType)
            return stage.m_isMature;
    }
    
    return false;
}

const GrowableDefinition* CropSystem::GetDefinition(uint8_t blockType) const
{
    auto it = m_blockToDefinition.find(blockType);
    return (it != m_blockToDefinition.end()) ? it->second : nullptr;
}

int CropSystem::GetCurrentStageIndex(uint8_t blockType, const GrowableDefinition& def) const
{
    for (int i = 0; i < (int)def.m_stages.size(); i++)
    {
        if (def.m_stages[i].m_blockType == blockType)
            return i;
    }
    return -1;
}

uint8_t CropSystem::Harvest(const BlockIterator& block)
{
    if (!block.IsValid())
        return BLOCK_TYPE_AIR;
    
    Block* b = block.GetBlock();
    if (!b)
        return BLOCK_TYPE_AIR;
    
    auto it = m_blockToDefinition.find(b->m_typeIndex);
    if (it == m_blockToDefinition.end())
        return BLOCK_TYPE_AIR;
    
    const GrowableDefinition* def = it->second;
    
    // 检查是否成熟
    if (!IsMature(b->m_typeIndex))
    {
        // 未成熟，直接破坏，返回空气
        return BLOCK_TYPE_AIR;
    }
    
    // 成熟作物，重置为第一阶段或移除
    // 这里简化处理：移除方块
    // 实际游戏中应该：生成掉落物，然后设为空气或第一阶段
    
    // TODO: 调用物品系统生成掉落物
    // for (const ItemDrop& drop : def->m_stages.back().m_drops)
    //     m_world->SpawnItemEntity(block.GetGlobalCoords(), drop);
    
    return BLOCK_TYPE_AIR;
}

void CropSystem::SetGrowthStage(const BlockIterator& block, int stage)
{
    if (!block.IsValid())
        return;
    
    Block* b = block.GetBlock();
    if (!b)
        return;
    
    auto it = m_blockToDefinition.find(b->m_typeIndex);
    if (it == m_blockToDefinition.end())
        return;
    
    const GrowableDefinition* def = it->second;
    
    if (stage < 0 || stage >= (int)def->m_stages.size())
        return;
    
    uint8_t oldType = b->m_typeIndex;
    uint8_t newType = def->m_stages[stage].m_blockType;
    
    if (oldType != newType)
    {
        b->SetType(newType);
        
        if (m_world)
        {
            m_world->OnBlockStateChanged(block, oldType, newType);
            
            Chunk* chunk = block.GetChunk();
            if (chunk)
            {
                chunk->MarkSelfDirty();
                m_world->m_hasDirtyChunk = true;
            }
        }
    }
}