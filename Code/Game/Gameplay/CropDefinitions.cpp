#include "CropDefinitions.h"
#include "Game/World.h"
#include "Game/Chunk.h"
#include "Game/Block.h"
#include "Game/BlockIterator.h"
#include "Game/Gamecommon.hpp"
#include <random>

std::vector<CropDefinition> CropDefinition::s_definitions;
std::unordered_map<uint8_t, const CropDefinition*> CropDefinition::s_blockTypeToDefinition;
std::unordered_map<std::string, const CropDefinition*> CropDefinition::s_nameToDefinition;
bool CropDefinition::s_initialized = false;

int CropDrop::RollCount() const
{
    if (m_minCount == m_maxCount) return m_minCount;
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(m_minCount, m_maxCount);
    return dist(rng);
}

// CropStage
CropStage& CropStage::AddDrop(BlockType type, int minC, int maxC, float chance)
{
    ItemType itemType = GetItemTypeFromBlock(type);
    m_drops.emplace_back(itemType, minC, maxC, chance);
    return *this;
}

CropDefinition::CropDefinition(const std::string& name, const std::string& displayName)
    : m_name(name), m_displayName(displayName) {}

CropDefinition& CropDefinition::SetCategory(CropCategory cat) { m_category = cat; return *this; }
CropDefinition& CropDefinition::SetPattern(GrowthPattern pattern) { m_pattern = pattern; return *this; }
CropDefinition& CropDefinition::SetGrowthChance(float chance) { m_baseGrowthChance = chance; return *this; }
CropDefinition& CropDefinition::SetMaxHeight(int height) { m_maxHeight = height; return *this; }
CropDefinition& CropDefinition::SetSpreadParams(int radius, int maxCount) { m_spreadRadius = radius; m_maxSpreadCount = maxCount; return *this; }
CropDefinition& CropDefinition::SetBonemealed(bool can) { m_canBeBonemealed = can; return *this; }
CropDefinition& CropDefinition::SetReplant(bool replant, int stage) { m_replantOnHarvest = replant; m_replantStage = stage; return *this; }
CropDefinition& CropDefinition::SetFixedInterval(float seconds) { m_usesRandomTick = false; m_fixedGrowthInterval = seconds; return *this; }

CropDefinition& CropDefinition::AddStage(const CropStage& stage) { m_stages.push_back(stage); return *this; }
CropDefinition& CropDefinition::AddStage(uint8_t blockType, bool mature, bool harvestable, float height)
{
    m_stages.emplace_back(blockType, mature, harvestable, height);
    return *this;
}
CropDefinition& CropDefinition::AddRequirement(const GrowthRequirement& req) { m_requirements.push_back(req); return *this; }

CropDefinition& CropDefinition::SetCustomCondition(std::function<bool(const BlockIterator&, World*)> func) { m_customConditionCheck = func; return *this; }
CropDefinition& CropDefinition::SetCustomGrowth(std::function<void(const BlockIterator&, World*, const CropDefinition&)> func) { m_customGrowthBehavior = func; return *this; }
CropDefinition& CropDefinition::SetCustomHarvest(std::function<void(const BlockIterator&, World*, const CropDefinition&)> func) { m_customHarvestBehavior = func; return *this; }

bool CropDefinition::IsBlockTypeInDefinition(uint8_t blockType) const
{
    for (const auto& stage : m_stages)
        if (stage.m_blockType == blockType) return true;
    return false;
}

int CropDefinition::GetStageIndex(uint8_t blockType) const
{
    for (int i = 0; i < (int)m_stages.size(); i++)
        if (m_stages[i].m_blockType == blockType) return i;
    return -1;
}

bool CropDefinition::IsMature(uint8_t blockType) const
{
    int idx = GetStageIndex(blockType);
    return idx >= 0 && m_stages[idx].m_isMature;
}

bool CropDefinition::IsHarvestable(uint8_t blockType) const
{
    int idx = GetStageIndex(blockType);
    return idx >= 0 && m_stages[idx].m_isHarvestable;
}

float CropDefinition::GetGrowthProgress(uint8_t blockType) const
{
    if (m_stages.empty()) return 0.0f;
    int idx = GetStageIndex(blockType);
    return idx < 0 ? 0.0f : (float)idx / (float)(m_stages.size() - 1);
}

// ============================================================================
// 静态注册表
// ============================================================================
void CropDefinition::RegisterDefinition(const CropDefinition& def)
{
    s_definitions.push_back(def);
    const CropDefinition* ptr = &s_definitions.back();
    s_nameToDefinition[def.m_name] = ptr;
    for (const auto& stage : def.m_stages)
        s_blockTypeToDefinition[stage.m_blockType] = ptr;
}

const CropDefinition* CropDefinition::GetDefinition(uint8_t blockType)
{
    if (!s_initialized) InitializeAllDefinitions();
    auto it = s_blockTypeToDefinition.find(blockType);
    return it != s_blockTypeToDefinition.end() ? it->second : nullptr;
}

const CropDefinition* CropDefinition::GetDefinitionByName(const std::string& name)
{
    if (!s_initialized) InitializeAllDefinitions();
    auto it = s_nameToDefinition.find(name);
    return it != s_nameToDefinition.end() ? it->second : nullptr;
}

const std::vector<CropDefinition>& CropDefinition::GetAllDefinitions()
{
    if (!s_initialized) InitializeAllDefinitions();
    return s_definitions;
}

bool CropDefinition::IsGrowable(uint8_t blockType) { return GetDefinition(blockType) != nullptr; }

namespace CropConditions
{
    GrowthRequirement NeedLight(uint8_t minLevel)
    {
        GrowthRequirement req;
        req.m_type = GrowthCondition::NEED_LIGHT;
        req.m_lightLevel = minLevel;
        return req;
    }

    GrowthRequirement NeedDarkness(uint8_t maxLevel)
    {
        GrowthRequirement req;
        req.m_type = GrowthCondition::NEED_DARKNESS;
        req.m_lightLevel = maxLevel;
        return req;
    }

    GrowthRequirement NeedBlockBelow(uint8_t blockType)
    {
        GrowthRequirement req;
        req.m_type = GrowthCondition::NEED_BLOCK_BELOW;
        req.m_requiredBlockType = blockType;
        return req;
    }

    GrowthRequirement NeedBlockAdjacent(uint8_t blockType)
    {
        GrowthRequirement req;
        req.m_type = GrowthCondition::NEED_BLOCK_ADJACENT;
        req.m_requiredBlockType = blockType;
        return req;
    }

    GrowthRequirement NeedWaterNearby(int radius)
    {
        GrowthRequirement req;
        req.m_type = GrowthCondition::NEED_WATER_NEARBY;
        req.m_radius = radius;
        return req;
    }

    GrowthRequirement NeedSkyAccess()
    {
        GrowthRequirement req;
        req.m_type = GrowthCondition::NEED_SKY_ACCESS;
        return req;
    }

    GrowthRequirement RandomChance(float chance)
    {
        GrowthRequirement req;
        req.m_type = GrowthCondition::RANDOM_CHANCE;
        req.m_chance = chance;
        return req;
    }
}

void CropDefinition::InitializeAllDefinitions()
{
    if (s_initialized) return;
    s_initialized = true;
    s_definitions.clear();
    s_blockTypeToDefinition.clear();
    s_nameToDefinition.clear();
    s_definitions.reserve(20); 
    // 小麦 (Wheat) - 8阶段 (54-61)
    {
        CropDefinition wheat("wheat", "Wheat");
        wheat.SetCategory(CropCategory::CROP)
             .SetPattern(GrowthPattern::STAGE_CHANGE)
             .SetGrowthChance(0.15f)
             .SetBonemealed(true);

        for (int i = 0; i <= 7; i++)
        {
            CropStage stage(BLOCK_TYPE_WHEAT_0 + i, i == 7, i == 7, 0.125f * (i + 1));
            if (i == 7)
            {
                stage.AddDrop(BLOCK_TYPE_WHEAT_0, 1, 1, 1.0f);
                stage.AddDrop(BLOCK_TYPE_WHEAT_0, 0, 2, 0.57f);
            }
            wheat.AddStage(stage);
        }

        wheat.AddRequirement(CropConditions::NeedBlockBelow(BLOCK_TYPE_FARMLAND));
        wheat.AddRequirement(CropConditions::NeedLight(9));
        RegisterDefinition(wheat);
    }

    // 胡萝卜 (Carrots) - 4阶段 (62-65)
    {
        CropDefinition carrots("carrots", "Carrots");
        carrots.SetCategory(CropCategory::CROP)
               .SetPattern(GrowthPattern::STAGE_CHANGE)
               .SetGrowthChance(0.15f)
               .SetBonemealed(true);

        for (int i = 0; i <= 3; i++)
        {
            CropStage stage(BLOCK_TYPE_CARROTS_0 + i, i == 3, i == 3, 0.25f * (i + 1));
            if (i == 3) stage.AddDrop(BLOCK_TYPE_CARROTS_0, 1, 4, 1.0f);
            carrots.AddStage(stage);
        }

        carrots.AddRequirement(CropConditions::NeedBlockBelow(BLOCK_TYPE_FARMLAND));
        carrots.AddRequirement(CropConditions::NeedLight(9));
        RegisterDefinition(carrots);
    }

    // 土豆 (Potatoes) - 4阶段 (66-69)
    {
        CropDefinition potatoes("potatoes", "Potatoes");
        potatoes.SetCategory(CropCategory::CROP)
                .SetPattern(GrowthPattern::STAGE_CHANGE)
                .SetGrowthChance(0.15f)
                .SetBonemealed(true);

        for (int i = 0; i <= 3; i++)
        {
            CropStage stage(BLOCK_TYPE_POTATOES_0 + i, i == 3, i == 3, 0.25f * (i + 1));
            if (i == 3) stage.AddDrop(BLOCK_TYPE_POTATOES_0, 1, 4, 1.0f);
            potatoes.AddStage(stage);
        }

        potatoes.AddRequirement(CropConditions::NeedBlockBelow(BLOCK_TYPE_FARMLAND));
        potatoes.AddRequirement(CropConditions::NeedLight(9));
        RegisterDefinition(potatoes);
    }

    // 甜菜根 (Beetroot) - 4阶段 (70-73)
    {
        CropDefinition beetroot("beetroot", "Beetroot");
        beetroot.SetCategory(CropCategory::CROP)
                .SetPattern(GrowthPattern::STAGE_CHANGE)
                .SetGrowthChance(0.15f)
                .SetBonemealed(true);

        for (int i = 0; i <= 3; i++)
        {
            CropStage stage(BLOCK_TYPE_BEETROOTS_0 + i, i == 3, i == 3, 0.25f * (i + 1));
            if (i == 3) stage.AddDrop(BLOCK_TYPE_BEETROOTS_0, 1, 1, 1.0f);
            beetroot.AddStage(stage);
        }

        beetroot.AddRequirement(CropConditions::NeedBlockBelow(BLOCK_TYPE_FARMLAND));
        beetroot.AddRequirement(CropConditions::NeedLight(9));
        RegisterDefinition(beetroot);
    }

    // 甘蔗 (Sugar Cane) - 向上生长 (74)
    {
        CropDefinition cane("sugar_cane", "Sugar Cane");
        cane.SetCategory(CropCategory::PLANT)
            .SetPattern(GrowthPattern::GROW_UP)
            .SetGrowthChance(0.10f)
            .SetMaxHeight(3)
            .SetBonemealed(false);

        CropStage stage(BLOCK_TYPE_SUGAR_CANE, false, true, 1.0f);
        stage.AddDrop(BLOCK_TYPE_SUGAR_CANE, 1, 1, 1.0f);
        cane.AddStage(stage);

        cane.AddRequirement(CropConditions::NeedWaterNearby(1));
        cane.SetCustomCondition([](const BlockIterator& block, World* world) -> bool
        {
            BlockIterator below = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
            if (!below.IsValid()) return false;
            Block* b = below.GetBlock();
            if (!b) return false;
            uint8_t t = b->m_typeIndex;
            return t == BLOCK_TYPE_SAND || t == BLOCK_TYPE_DIRT || t == BLOCK_TYPE_GRASS || t == BLOCK_TYPE_SUGAR_CANE;
        });
        RegisterDefinition(cane);
    }

    // ------------------------------------------------------------------------
    // 仙人掌 (Cactus) - 向上生长 (75)
    // ------------------------------------------------------------------------
    {
        CropDefinition cactus("cactus", "Cactus");
        cactus.SetCategory(CropCategory::PLANT)
              .SetPattern(GrowthPattern::GROW_UP)
              .SetGrowthChance(0.10f)
              .SetMaxHeight(3)
              .SetBonemealed(false);

        CropStage stage(BLOCK_TYPE_CACTUS, false, true, 1.0f);
        stage.AddDrop(BLOCK_TYPE_CACTUS, 1, 1, 1.0f);
        cactus.AddStage(stage);

        cactus.SetCustomCondition([](const BlockIterator& block, World* world) -> bool
        {
            BlockIterator below = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
            if (!below.IsValid()) return false;
            Block* b = below.GetBlock();
            if (!b) return false;
            uint8_t t = b->m_typeIndex;
            if (t != BLOCK_TYPE_SAND && t != BLOCK_TYPE_CACTUS && t != BLOCK_TYPE_CACTUS_LOG) return false;

            Direction dirs[] = {DIRECTION_NORTH, DIRECTION_SOUTH, DIRECTION_EAST, DIRECTION_WEST};
            for (Direction d : dirs)
            {
                BlockIterator side = block.GetNeighborCrossBoundary(d);
                if (side.IsValid())
                {
                    Block* sb = side.GetBlock();
                    if (sb && sb->m_typeIndex != BLOCK_TYPE_AIR) return false;
                }
            }
            return true;
        });
        RegisterDefinition(cactus);
    }

    // ------------------------------------------------------------------------
    // 南瓜茎 (Pumpkin Stem) - 8阶段 (76-83), 果实(84)
    // ------------------------------------------------------------------------
    {
        CropDefinition pumpkinStem("pumpkin_stem", "Pumpkin Stem");
        pumpkinStem.SetCategory(CropCategory::CROP)
                   .SetPattern(GrowthPattern::STAGE_CHANGE)
                   .SetGrowthChance(0.10f)
                   .SetBonemealed(true);

        for (int i = 0; i <= 7; i++)
        {
            pumpkinStem.AddStage(BLOCK_TYPE_PUMPKIN_STEM_0 + i, i == 7, false, 0.125f * (i + 1));
        }

        pumpkinStem.AddRequirement(CropConditions::NeedBlockBelow(BLOCK_TYPE_FARMLAND));
        pumpkinStem.AddRequirement(CropConditions::NeedLight(9));

        pumpkinStem.SetCustomGrowth([](const BlockIterator& block, World* world, const CropDefinition& def)
        {
            Block* b = block.GetBlock();
            if (!b) return;

            int stage = def.GetStageIndex(b->m_typeIndex);
            if (stage < 0) return;

            if (stage < 7)
            {
                b->SetType(BLOCK_TYPE_PUMPKIN_STEM_0 + stage + 1);
                if (Chunk* c = block.GetChunk()) { c->MarkSelfDirty(); world->m_hasDirtyChunk = true; }
                return;
            }

            static std::mt19937 rng(std::random_device{}());
            Direction dirs[] = {DIRECTION_NORTH, DIRECTION_SOUTH, DIRECTION_EAST, DIRECTION_WEST};
            std::shuffle(std::begin(dirs), std::end(dirs), rng);

            for (Direction d : dirs)
            {
                BlockIterator side = block.GetNeighborCrossBoundary(d);
                if (!side.IsValid()) continue;
                Block* sideBlock = side.GetBlock();
                if (!sideBlock || sideBlock->m_typeIndex != BLOCK_TYPE_AIR) continue;

                BlockIterator sideBelow = side.GetNeighborCrossBoundary(DIRECTION_DOWN);
                if (!sideBelow.IsValid()) continue;
                Block* belowBlock = sideBelow.GetBlock();
                if (!belowBlock || !belowBlock->IsSolid()) continue;

                sideBlock->SetType(BLOCK_TYPE_PUMPKIN);
                if (Chunk* c = side.GetChunk()) { c->MarkSelfDirty(); world->m_hasDirtyChunk = true; }
                break;
            }
        });
        RegisterDefinition(pumpkinStem);
    }

    // ------------------------------------------------------------------------
    // 西瓜茎 (Melon Stem) - 8阶段 (85-92), 果实(93)
    // ------------------------------------------------------------------------
    {
        CropDefinition melonStem("melon_stem", "Melon Stem");
        melonStem.SetCategory(CropCategory::CROP)
                 .SetPattern(GrowthPattern::STAGE_CHANGE)
                 .SetGrowthChance(0.10f)
                 .SetBonemealed(true);

        for (int i = 0; i <= 7; i++)
        {
            melonStem.AddStage(BLOCK_TYPE_MELON_STEM_0 + i, i == 7, false, 0.125f * (i + 1));
        }

        melonStem.AddRequirement(CropConditions::NeedBlockBelow(BLOCK_TYPE_FARMLAND));
        melonStem.AddRequirement(CropConditions::NeedLight(9));

        melonStem.SetCustomGrowth([](const BlockIterator& block, World* world, const CropDefinition& def)
        {
            Block* b = block.GetBlock();
            if (!b) return;

            int stage = def.GetStageIndex(b->m_typeIndex);
            if (stage < 0) return;

            if (stage < 7)
            {
                b->SetType(BLOCK_TYPE_MELON_STEM_0 + stage + 1);
                if (Chunk* c = block.GetChunk()) { c->MarkSelfDirty(); world->m_hasDirtyChunk = true; }
                return;
            }

            static std::mt19937 rng(std::random_device{}());
            Direction dirs[] = {DIRECTION_NORTH, DIRECTION_SOUTH, DIRECTION_EAST, DIRECTION_WEST};
            std::shuffle(std::begin(dirs), std::end(dirs), rng);

            for (Direction d : dirs)
            {
                BlockIterator side = block.GetNeighborCrossBoundary(d);
                if (!side.IsValid()) continue;
                Block* sideBlock = side.GetBlock();
                if (!sideBlock || sideBlock->m_typeIndex != BLOCK_TYPE_AIR) continue;

                BlockIterator sideBelow = side.GetNeighborCrossBoundary(DIRECTION_DOWN);
                if (!sideBelow.IsValid()) continue;
                Block* belowBlock = sideBelow.GetBlock();
                if (!belowBlock || !belowBlock->IsSolid()) continue;

                sideBlock->SetType(BLOCK_TYPE_MELON);
                if (Chunk* c = side.GetChunk()) { c->MarkSelfDirty(); world->m_hasDirtyChunk = true; }
                break;
            }
        });
        RegisterDefinition(melonStem);
    }

    // ------------------------------------------------------------------------
    // 地狱疣 (Nether Wart) - 4阶段 (95-98)
    // ------------------------------------------------------------------------
    {
        CropDefinition netherWart("nether_wart", "Nether Wart");
        netherWart.SetCategory(CropCategory::FUNGUS)
                  .SetPattern(GrowthPattern::STAGE_CHANGE)
                  .SetGrowthChance(0.10f)
                  .SetBonemealed(false);

        for (int i = 0; i <= 3; i++)
        {
            CropStage stage(BLOCK_TYPE_NETHER_WART_0 + i, i == 3, i == 3, 0.25f * (i + 1));
            if (i == 3) stage.AddDrop(BLOCK_TYPE_NETHER_WART_0, 2, 4, 1.0f);
            netherWart.AddStage(stage);
        }

        netherWart.AddRequirement(CropConditions::NeedBlockBelow(BLOCK_TYPE_SOUL_SAND));
        RegisterDefinition(netherWart);
    }

    // ------------------------------------------------------------------------
    // 海带 (Kelp) - 水下向上生长 (101-102)
    // ------------------------------------------------------------------------
    {
        CropDefinition kelp("kelp", "Kelp");
        kelp.SetCategory(CropCategory::AQUATIC)
            .SetPattern(GrowthPattern::GROW_UP)
            .SetGrowthChance(0.14f)
            .SetMaxHeight(26)
            .SetBonemealed(true);

        kelp.AddStage(CropStage(BLOCK_TYPE_KELP, false, true, 1.0f));
        CropStage top(BLOCK_TYPE_KELP_TOP, false, true, 1.0f);
        top.AddDrop(BLOCK_TYPE_KELP, 1, 1, 1.0f);
        kelp.AddStage(top);

        kelp.SetCustomCondition([](const BlockIterator& block, World* world) -> bool
        {
            BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
            if (!above.IsValid()) return false;
            Block* ab = above.GetBlock();
            return ab && ab->m_typeIndex == BLOCK_TYPE_WATER;
        });

        kelp.SetCustomGrowth([](const BlockIterator& block, World* world, const CropDefinition& def)
        {
            Block* current = block.GetBlock();
            if (!current) return;
            BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
            if (!above.IsValid()) return;
            Block* ab = above.GetBlock();
            if (!ab || ab->m_typeIndex != BLOCK_TYPE_WATER) return;

            current->SetType(BLOCK_TYPE_KELP);
            ab->SetType(BLOCK_TYPE_KELP_TOP);

            if (Chunk* c = block.GetChunk()) { c->MarkSelfDirty(); world->m_hasDirtyChunk = true; }
            if (Chunk* c = above.GetChunk()) c->MarkSelfDirty();
        });
        RegisterDefinition(kelp);
    }

    // ------------------------------------------------------------------------
    // 海草 (Seagrass) - 骨粉可变高海草 (103-105)
    // ------------------------------------------------------------------------
    {
        CropDefinition seagrass("seagrass", "Seagrass");
        seagrass.SetCategory(CropCategory::AQUATIC)
                .SetPattern(GrowthPattern::STAGE_CHANGE)
                .SetGrowthChance(0.0f)
                .SetBonemealed(true);

        seagrass.AddStage(CropStage(BLOCK_TYPE_SEAGRASS, true, true, 0.8f));

        seagrass.SetCustomGrowth([](const BlockIterator& block, World* world, const CropDefinition& def)
        {
            Block* current = block.GetBlock();
            if (!current) return;
            BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
            if (!above.IsValid()) return;
            Block* ab = above.GetBlock();
            if (!ab || ab->m_typeIndex != BLOCK_TYPE_WATER) return;

            current->SetType(BLOCK_TYPE_TALL_SEAGRASS_BOTTOM);
            ab->SetType(BLOCK_TYPE_TALL_SEAGRASS_TOP);

            if (Chunk* c = block.GetChunk()) { c->MarkSelfDirty(); world->m_hasDirtyChunk = true; }
            if (Chunk* c = above.GetChunk()) c->MarkSelfDirty();
        });
        RegisterDefinition(seagrass);
    }

    // ------------------------------------------------------------------------
    // 草方块蔓延 (Grass Spread) - BLOCK_TYPE_GRASS(16)
    // ------------------------------------------------------------------------
    {
        CropDefinition grass("grass_spread", "Grass Block");
        grass.SetCategory(CropCategory::GRASS)
             .SetPattern(GrowthPattern::SPREAD)
             .SetGrowthChance(0.05f)
             .SetSpreadParams(3, -1)
             .SetBonemealed(false);

        grass.AddStage(CropStage(BLOCK_TYPE_GRASS, true, false, 1.0f));
        grass.AddRequirement(CropConditions::NeedLight(4));

        grass.SetCustomGrowth([](const BlockIterator& block, World* world, const CropDefinition& def)
        {
            static std::mt19937 rng(std::random_device{}());
            Direction dirs[] = {DIRECTION_NORTH, DIRECTION_SOUTH, DIRECTION_EAST, DIRECTION_WEST};
            std::uniform_int_distribution<int> dirDist(0, 3);
            std::uniform_int_distribution<int> zDist(-1, 1);

            BlockIterator target = block.GetNeighborCrossBoundary(dirs[dirDist(rng)]);
            if (!target.IsValid()) return;

            int zOff = zDist(rng);
            if (zOff != 0)
            {
                target = target.GetNeighborCrossBoundary(zOff > 0 ? DIRECTION_UP : DIRECTION_DOWN);
                if (!target.IsValid()) return;
            }

            Block* tb = target.GetBlock();
            if (!tb || tb->m_typeIndex != BLOCK_TYPE_DIRT) return;

            BlockIterator above = target.GetNeighborCrossBoundary(DIRECTION_UP);
            if (!above.IsValid()) return;
            Block* ab = above.GetBlock();
            if (ab && ab->IsOpaque()) return;
            if (above.GetOutdoorLight() < 4) return;

            tb->SetType(BLOCK_TYPE_GRASS);
            if (Chunk* c = target.GetChunk()) { c->MarkSelfDirty(); world->m_hasDirtyChunk = true; }
        });
        RegisterDefinition(grass);
    }

    // ------------------------------------------------------------------------
    // 鱼卵 (Fish Egg) - 孵化 (99-100)
    // ------------------------------------------------------------------------
    {
        CropDefinition fishEgg("fish_egg", "Fish Egg");
        fishEgg.SetCategory(CropCategory::EGG)
               .SetPattern(GrowthPattern::HATCH)
               .SetGrowthChance(0.02f)
               .SetBonemealed(false);

        fishEgg.AddStage(CropStage(BLOCK_TYPE_FISH_EGG, false, false, 0.3f));
        fishEgg.AddStage(CropStage(BLOCK_TYPE_FISH_EGG_HATCHING, false, false, 0.3f));
        fishEgg.AddStage(CropStage(BLOCK_TYPE_AIR, true, false, 0.0f));

        fishEgg.SetCustomCondition([](const BlockIterator& block, World* world) -> bool
        {
            Direction dirs[] = {DIRECTION_UP, DIRECTION_NORTH, DIRECTION_SOUTH, DIRECTION_EAST, DIRECTION_WEST};
            for (Direction d : dirs)
            {
                BlockIterator neighbor = block.GetNeighborCrossBoundary(d);
                if (neighbor.IsValid())
                {
                    Block* nb = neighbor.GetBlock();
                    if (nb && nb->m_typeIndex == BLOCK_TYPE_WATER) return true;
                }
            }
            return false;
        });

        fishEgg.SetCustomGrowth([](const BlockIterator& block, World* world, const CropDefinition& def)
        {
            Block* b = block.GetBlock();
            if (!b) return;
            int stage = def.GetStageIndex(b->m_typeIndex);
            if (stage < 0) return;

            if (stage == 1)
            {
                b->SetType(BLOCK_TYPE_AIR);
                // TODO: 生成鱼实体
            }
            else
            {
                b->SetType(def.m_stages[stage + 1].m_blockType);
            }

            if (Chunk* c = block.GetChunk()) { c->MarkSelfDirty(); world->m_hasDirtyChunk = true; }
        });
        RegisterDefinition(fishEgg);
    }
}