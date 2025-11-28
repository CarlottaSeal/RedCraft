#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class World;
class BlockIterator;

// 生长条件类型
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

// 可生长实体定义
struct GrowableDefinition
{
    std::string m_name;                    // 名称 (如 "wheat", "fish", "bee")
    
    // 生长阶段 (按顺序)
    std::vector<GrowthStage> m_stages;
    
    // 生长条件
    std::vector<GrowthRequirement> m_requirements;
    
    // 基础生长概率 (每次随机tick的生长几率)
    float m_baseGrowthChance = 0.15f;
    
    // 是否受随机tick影响 (false = 使用固定时间间隔)
    bool m_usesRandomTick = true;
    
    // 固定生长间隔 (仅当 m_usesRandomTick = false)
    float m_fixedGrowthInterval = 0.0f;
    
    // 分类标签 (用于过滤和查询)
    enum class Category : uint8_t
    {
        PLANT,           // 植物 (小麦、胡萝卜)
        AQUATIC,         // 水生 (海带、鱼卵)
        ANIMAL,          // 动物相关 (蜜蜂巢)
        FUNGUS,          // 真菌 (蘑菇、地狱疣)
        OTHER
    } m_category = Category::PLANT;
    
    // 自定义生长条件检查 (可选)
    std::function<bool(const BlockIterator&, World*)> m_customConditionCheck;
    
    // 自定义生长行为 (可选，用于特殊生长如树木)
    std::function<void(const BlockIterator&, World*)> m_customGrowthBehavior;
};
