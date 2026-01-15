#pragma once
#include "CropDefinitions.h"
#include "CropDropManager.h"  // 🚀 新增：包含掉落物管理器
#include <vector>
#include <unordered_map>

#include "Engine/Math/IntVec3.h"
#include "Engine/Math/Vec3.hpp"  // 🚀 新增：Vec3 类型

class Chunk;
class World;
class BlockIterator;
class Renderer;

class CropSystem
{
public:
    CropSystem(World* world);
    ~CropSystem();  
    
    void Initialize();
    void Update(float deltaSeconds);
    void Render(Renderer* renderer) const; 
    
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
    
    bool IsGrowable(uint8_t blockType) const;
    float GetGrowthProgress(const BlockIterator& block) const;
    
    int GetTrackedCropCount() const { return (int)m_trackedCrops.size(); }
    
    // 当作物被破坏时调用（活塞推动、玩家破坏等）
    void OnCropDestroyed(const IntVec3& position, ItemType droppedItem);
    
    // 生成掉落物
    void SpawnDrop(const Vec3& position, uint8_t itemType, int count = 1);
    void SpawnDropAtBlock(const IntVec3& blockPos, std::string itemName, int count = 1);
    
    // 收集掉落物（给玩家或漏斗用）
    std::vector<PendingDrop> CollectDropsNear(const Vec3& position, float radius);
    std::vector<PendingDrop> CollectDropsInArea(const IntVec3& minCorner, const IntVec3& maxCorner);
    
    // 清除所有掉落物
    void ClearAllDrops();
    
    DropStatistics GetDropStatistics() const;
    int GetTotalDropCount() const;
    int GetTotalItemCount() const;
    
    void SetDropDebugRender(bool enable);

    CropDropManager* GetCropDropManager() { return &m_dropManager; }
    
private:
    World* m_world = nullptr;
    float m_tickTimer = 0.0f;
    std::vector<FarmArea> m_farmAreas;
    
    std::vector<TrackedCrop> m_trackedCrops;
    std::unordered_map<uint64_t, size_t> m_cropIndexByHash;  // hash -> index in m_trackedCrops
    
    CropDropManager m_dropManager;
    
    static constexpr float RANDOM_TICK_INTERVAL = 1.0f;
    static constexpr int RANDOM_TICKS_PER_CHUNK = 3000;
    static constexpr int CROPS_PER_TICK = 1;  
    
    void ProcessRandomTicks();
    void ProcessFixedIntervalGrowth(float deltaSeconds);
    bool CheckGrowthConditions(const BlockIterator& block, const CropDefinition& def);
    void PerformGrowth(const BlockIterator& block, const CropDefinition& def);
    
    void UnregisterCropByIndex(size_t index);
    
    int GetCurrentHeight(const BlockIterator& block, const CropDefinition& def);
    uint64_t HashBlockPos(const IntVec3& pos) const;
};