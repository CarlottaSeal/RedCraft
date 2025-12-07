#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "Engine/Math/IntVec3.h"

class CropDefinition;
class World;
class BlockIterator;

enum class GrowthCondition : uint8_t
{
    NONE,                    // 无条件
    NEED_LIGHT,              // 需要光照 >= 阈值
    NEED_DARKNESS,           // 需要黑暗 <= 阈值
    NEED_BLOCK_BELOW,        // 需要特定方块在下方
    NEED_BLOCK_ADJACENT,     // 需要特定方块相邻
    NEED_WATER_NEARBY,       // 需要水在附近
    NEED_SKY_ACCESS,         // 需要能看到天空
    RANDOM_CHANCE,           // 随机概率
    CUSTOM                   // 自定义函数
};

// 单个生长条件
struct GrowthRequirement
{
    GrowthCondition m_type = GrowthCondition::NONE;
    
    // 根据条件类型使用不同字段
    union
    {
        uint8_t m_lightLevel;              // NEED_LIGHT / NEED_DARKNESS
        uint8_t m_requiredBlockType;       // NEED_BLOCK_BELOW / ADJACENT
        float m_chance;                    // RANDOM_CHANCE (0.0 - 1.0)
        int m_radius;                      // NEED_WATER_NEARBY 搜索半径
    };
    
    GrowthRequirement() : m_type(GrowthCondition::NONE), m_lightLevel(0) {}
};

// 生长阶段定义
struct GrowthStage
{
    uint8_t m_blockType;                   // 这个阶段的方块类型
    bool m_isMature;                       // 是否为成熟阶段
    
    // 成熟时的掉落物 (可选，需要物品系统)
    // std::vector<ItemDrop> m_drops;
};

// 掉落物定义
struct CropDrop
{
    uint8_t m_itemType = 0;
    int m_minCount = 1;
    int m_maxCount = 1;
    float m_chance = 1.0f;
    bool m_onlyWhenMature = true;

    CropDrop() = default;
    CropDrop(uint8_t type, int minC, int maxC, float chance = 1.0f, bool matureOnly = true)
        : m_itemType(type), m_minCount(minC), m_maxCount(maxC)
        , m_chance(chance), m_onlyWhenMature(matureOnly) {}

    int RollCount() const;
};

// 生长阶段定义
struct CropStage
{
    uint8_t m_blockType = 0;
    bool m_isMature = false;
    bool m_isHarvestable = false;
    float m_visualHeight = 1.0f;
    std::vector<CropDrop> m_drops;

    CropStage() = default;
    CropStage(uint8_t type, bool mature = false, bool harvestable = false, float height = 1.0f)
        : m_blockType(type), m_isMature(mature), m_isHarvestable(harvestable), m_visualHeight(height) {}

    CropStage& AddDrop(uint8_t type, int minC, int maxC, float chance = 1.0f);
};

// 生长模式
enum class GrowthPattern : uint8_t
{
    STAGE_CHANGE,   // 改变自身方块类型（小麦、胡萝卜）
    GROW_UP,        // 向上生长（甘蔗、竹子、海带）
    SPREAD,         // 蔓延生长（草方块、蘑菇）
    MULTI_BLOCK,    // 多方块结构（向日葵、高草）
    HATCH,          // 孵化后消失（海龟蛋、蛙卵）
    TREE            // 树木生长（树苗 → 树）
};

// 作物分类
enum class CropCategory : uint8_t
{
    CROP,           // 农作物
    PLANT,          // 植物
    AQUATIC,        // 水生
    FUNGUS,         // 真菌
    GRASS,          // 草类
    FLOWER,         // 花卉
    EGG,            // 卵类
    TREE_SAPLING,   // 树苗
    OTHER
};

struct FarmArea
{
    IntVec3 m_minCorner;
    IntVec3 m_maxCorner;
    std::string m_name;
    
    FarmArea() = default;
    FarmArea(const IntVec3& minC, const IntVec3& maxC, const std::string& name = "")
        : m_minCorner(minC), m_maxCorner(maxC), m_name(name) {}
};

// 作物统计
struct CropStats
{
    int m_totalCrops = 0;
    int m_matureCrops = 0;
    int m_growingCrops = 0;
    std::unordered_map<std::string, int> m_countByType;
};


struct TrackedCrop
{
    IntVec3 m_position;                  // 作物的世界坐标
    uint8_t m_blockType;                 // 当前方块类型
    const CropDefinition* m_definition;  // 缓存的定义指针（避免重复查找）
    float m_growthTimer;                 // 生长计时器
    
    TrackedCrop(const IntVec3& pos, uint8_t type, const CropDefinition* def)
        : m_position(pos)
        , m_blockType(type)
        , m_definition(def)
        , m_growthTimer(0.0f)
    {}
};