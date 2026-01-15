#pragma once
#include "CropCommon.h"
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

class World;
class BlockIterator;

class CropDefinition
{
public:
    std::string m_name;
    std::string m_displayName;
    CropCategory m_category = CropCategory::CROP;
    GrowthPattern m_pattern = GrowthPattern::STAGE_CHANGE;

    std::vector<CropStage> m_stages;
    std::vector<GrowthRequirement> m_requirements;

    float m_baseGrowthChance = 0.15f;
    bool m_usesRandomTick = true;
    float m_fixedGrowthInterval = 0.0f;

    int m_maxHeight = 1;
    int m_spreadRadius = 0;
    int m_maxSpreadCount = -1;

    bool m_canBeBonemealed = true;
    bool m_breaksWithoutSupport = true;
    bool m_replantOnHarvest = false;
    int m_replantStage = 0;

    bool m_needsWetFarmland = false;
    bool m_canGrowInDark = false;

    std::function<bool(const BlockIterator&, World*)> m_customConditionCheck;
    std::function<void(const BlockIterator&, World*, const CropDefinition&)> m_customGrowthBehavior;
    std::function<void(const BlockIterator&, World*, const CropDefinition&)> m_customHarvestBehavior;

public:
    CropDefinition() = default;
    CropDefinition(const std::string& name, const std::string& displayName);

    CropDefinition& SetCategory(CropCategory cat);
    CropDefinition& SetPattern(GrowthPattern pattern);
    CropDefinition& SetGrowthChance(float chance);
    CropDefinition& SetMaxHeight(int height);
    CropDefinition& SetSpreadParams(int radius, int maxCount = -1);
    CropDefinition& SetBonemealed(bool can);
    CropDefinition& SetReplant(bool replant, int stage = 0);
    CropDefinition& SetFixedInterval(float seconds);

    CropDefinition& AddStage(const CropStage& stage);
    CropDefinition& AddStage(uint8_t blockType, bool mature = false, bool harvestable = false, float height = 1.0f);
    CropDefinition& AddRequirement(const GrowthRequirement& req);

    CropDefinition& SetCustomCondition(std::function<bool(const BlockIterator&, World*)> func);
    CropDefinition& SetCustomGrowth(std::function<void(const BlockIterator&, World*, const CropDefinition&)> func);
    CropDefinition& SetCustomHarvest(std::function<void(const BlockIterator&, World*, const CropDefinition&)> func);

    bool IsBlockTypeInDefinition(uint8_t blockType) const;
    int GetStageIndex(uint8_t blockType) const;
    bool IsMature(uint8_t blockType) const;
    bool IsHarvestable(uint8_t blockType) const;
    float GetGrowthProgress(uint8_t blockType) const;

    static void InitializeAllDefinitions();
    static const CropDefinition* GetDefinition(uint8_t blockType);
    static const CropDefinition* GetDefinitionByName(const std::string& name);
    static const std::vector<CropDefinition>& GetAllDefinitions();
    static bool IsGrowable(uint8_t blockType);

private:
    static std::vector<CropDefinition> s_definitions;
    static std::unordered_map<uint8_t, const CropDefinition*> s_blockTypeToDefinition;
    static std::unordered_map<std::string, const CropDefinition*> s_nameToDefinition;
    static bool s_initialized;

    static void RegisterDefinition(const CropDefinition& def);
};

namespace CropConditions
{
    GrowthRequirement NeedLight(uint8_t minLevel);
    GrowthRequirement NeedDarkness(uint8_t maxLevel);
    GrowthRequirement NeedBlockBelow(uint8_t blockType);
    GrowthRequirement NeedBlockAdjacent(uint8_t blockType);
    GrowthRequirement NeedWaterNearby(int radius = 4);
    GrowthRequirement NeedSkyAccess();
    GrowthRequirement RandomChance(float chance);
}
