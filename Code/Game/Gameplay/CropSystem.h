#pragma once
#include "CropDefinitions.h"
#include <vector>
#include <unordered_map>

#include "Engine/Math/IntVec3.h"

class Chunk;
class World;
class BlockIterator;


class CropSystem
{
public:
    CropSystem(World* world);
    
    void Initialize();
    void Update(float deltaSeconds);
    
    bool TryGrow(const BlockIterator& block);
    bool ApplyBonemeal(const BlockIterator& block);
    std::vector<CropDrop> Harvest(const BlockIterator& block, bool replant = true);
    
    void RegisterCrop(const IntVec3& position, uint8_t blockType);
    void UnregisterCrop(const IntVec3& position);
    void UpdateCropType(const IntVec3& position, uint8_t newType);
    
    void RegisterFarmArea(const FarmArea& area);
    void UnregisterFarmArea(const std::string& name);
    std::vector<BlockIterator> GetCropsInArea(const FarmArea& area);
    std::vector<BlockIterator> GetMatureCropsInArea(const FarmArea& area);
    int HarvestArea(const FarmArea& area, bool replant = true);
    CropStats GetStatsInArea(const FarmArea& area);
    
    // 查询
    bool IsGrowable(uint8_t blockType) const;
    float GetGrowthProgress(const BlockIterator& block) const;
    
    // 🚀 调试接口
    int GetTrackedCropCount() const { return (int)m_trackedCrops.size(); }
    
private:
    World* m_world = nullptr;
    float m_tickTimer = 0.0f;
    std::vector<FarmArea> m_farmAreas;
    
    // 🚀 优化：维护作物列表而非遍历所有方块
    std::vector<TrackedCrop> m_trackedCrops;
    std::unordered_map<uint64_t, size_t> m_cropIndexByHash;  // hash -> index in m_trackedCrops
    
    static constexpr float RANDOM_TICK_INTERVAL = 1.0f;
    static constexpr int RANDOM_TICKS_PER_CHUNK = 3;
    
    void ProcessRandomTicks();
    void ProcessFixedIntervalGrowth(float deltaSeconds);
    bool CheckGrowthConditions(const BlockIterator& block, const CropDefinition& def);
    void PerformGrowth(const BlockIterator& block, const CropDefinition& def);
    
    // 🚀 内部辅助函数
    void UnregisterCropByIndex(size_t index);
    
    int GetCurrentHeight(const BlockIterator& block, const CropDefinition& def);
    uint64_t HashBlockPos(const IntVec3& pos) const;
};