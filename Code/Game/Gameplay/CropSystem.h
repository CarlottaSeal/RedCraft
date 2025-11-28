#pragma once
#include "CropCommon.h"
#include "Game/BlockIterator.h"

class CropSystem
{
public:
    CropSystem(World* world);
    ~CropSystem() = default;
    
    // 初始化 - 注册所有可生长实体
    void Initialize();
    
    // 主更新
    void Update(float deltaSeconds);
    
    // === 注册接口 ===
    void RegisterGrowable(const GrowableDefinition& def);
    
    // === 查询接口 ===
    bool IsGrowable(uint8_t blockType) const;
    bool IsMature(uint8_t blockType) const;
    const GrowableDefinition* GetDefinition(uint8_t blockType) const;
    
    // === 操作接口 ===
    // 尝试生长指定方块
    bool TryGrow(const BlockIterator& block);
    
    // 收获作物 (返回下一个状态的方块类型，如果完全移除则返回AIR)
    uint8_t Harvest(const BlockIterator& block);
    
    // 强制设置生长阶段
    void SetGrowthStage(const BlockIterator& block, int stage);
    
private:
    World* m_world = nullptr;
    
    float m_tickTimer = 0.0f;
    static constexpr float RANDOM_TICK_INTERVAL = 1.0f;
    static constexpr int RANDOM_TICKS_PER_CHUNK = 3;
    
    // 固定间隔生长的实体追踪
    struct FixedGrowthEntry
    {
        BlockIterator m_block;
        float m_nextGrowthTime;
    };
    std::vector<FixedGrowthEntry> m_fixedGrowthEntities;
    
    // 方块类型 → 定义的映射
    std::unordered_map<uint8_t, const GrowableDefinition*> m_blockToDefinition;
    
    // 所有注册的定义
    std::vector<GrowableDefinition> m_definitions;
    
private:
    // 处理随机tick
    void ProcessRandomTicks();
    
    // 处理固定间隔生长
    void ProcessFixedIntervalGrowth(float deltaSeconds);
    
    // 检查生长条件
    bool CheckGrowthConditions(const BlockIterator& block, const GrowableDefinition& def) const;
    bool CheckSingleCondition(const BlockIterator& block, const GrowthRequirement& req) const;
    
    // 执行生长
    void PerformGrowth(const BlockIterator& block, const GrowableDefinition& def);
    
    // 获取当前阶段索引
    int GetCurrentStageIndex(uint8_t blockType, const GrowableDefinition& def) const;
    
    // 辅助：检查附近是否有水
    bool HasWaterNearby(const BlockIterator& block, int radius) const;
};

// ============================================================================
// 预定义的常用条件构建器
// ============================================================================
namespace GrowthConditions
{
    inline GrowthRequirement NeedLight(uint8_t minLevel)
    {
        GrowthRequirement req;
        req.m_type = GrowthCondition::NEED_LIGHT;
        req.m_lightLevel = minLevel;
        return req;
    }
    
    inline GrowthRequirement NeedDarkness(uint8_t maxLevel)
    {
        GrowthRequirement req;
        req.m_type = GrowthCondition::NEED_DARKNESS;
        req.m_lightLevel = maxLevel;
        return req;
    }
    
    inline GrowthRequirement NeedBlockBelow(uint8_t blockType)
    {
        GrowthRequirement req;
        req.m_type = GrowthCondition::NEED_BLOCK_BELOW;
        req.m_requiredBlockType = blockType;
        return req;
    }
    
    inline GrowthRequirement NeedWaterNearby(int radius = 4)
    {
        GrowthRequirement req;
        req.m_type = GrowthCondition::NEED_WATER_NEARBY;
        req.m_radius = radius;
        return req;
    }
    
    inline GrowthRequirement NeedSkyAccess()
    {
        GrowthRequirement req;
        req.m_type = GrowthCondition::NEED_SKY_ACCESS;
        return req;
    }
    
    inline GrowthRequirement RandomChance(float chance)
    {
        GrowthRequirement req;
        req.m_type = GrowthCondition::RANDOM_CHANCE;
        req.m_chance = chance;
        return req;
    }
}






