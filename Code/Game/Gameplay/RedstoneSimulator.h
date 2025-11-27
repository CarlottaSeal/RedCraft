#pragma once

#include <deque>
#include <unordered_set>

#include "RedstoneCommon.h"
#include "Game/BlockIterator.h"
#include "Engine/Math/IntVec3.h"

class World;
class Chunk;

class RedstoneSimulator
{
public:
    RedstoneSimulator(World* world);
    ~RedstoneSimulator() = default;
    
    // 每帧调用
    void Update(float deltaSeconds);
    
    // 标记方块需要红石更新
    void ScheduleUpdate(const BlockIterator& block, uint8_t delayTicks = 0);
    void ScheduleNeighborUpdates(const BlockIterator& block);
    
    // 获取方块的红石信号
    uint8_t GetInputPower(const BlockIterator& block) const;
    bool IsPowered(const BlockIterator& block) const;
    bool IsPistonPowered(const BlockIterator& block) const;
    
    // 事件回调（World调用）
    void OnBlockPlaced(const BlockIterator& block);
    void OnBlockRemoved(const BlockIterator& block, uint8_t oldType);
    bool IsRedstoneBlockPowering(const BlockIterator& block,Direction toDir) const;
    
    void ToggleLever(const BlockIterator& block);
    void PressButton(const BlockIterator& block);
    Direction GetButtonAttachmentDirection(Block* button) const;
    void CycleRepeaterDelay(const BlockIterator& block);

    WireConnections GetWireConnections(const BlockIterator& block) const;
    WireConnection GetWireConnectionInDirection(const BlockIterator& block, Direction dir) const;
    bool CanConnectToRedstone(uint8_t blockType, Direction fromDir) const;
    
private:
    World* m_world = nullptr;
    
    std::deque<RedstoneUpdate> m_updateQueue;
    std::unordered_set<int64_t> m_queuedPositions;  // 避免重复
    
    float m_tickTimer = 0.0f;
    uint32_t m_currentTick = 0;

    std::unordered_map<int64_t, TorchBurnoutEntry> m_torchBurnoutTracker;
    static constexpr int TORCH_BURNOUT_THRESHOLD = 8;  // 8次切换触发保护
    static constexpr float TORCH_BURNOUT_WINDOW = 1.0f; // 1秒内
    
private:    
    // 处理单个方块更新
    void ProcessBlockUpdate(const BlockIterator& block);

    uint8_t GetDirectPowerInput(const BlockIterator& block) const;
    uint8_t GetAdjacentWirePower(const BlockIterator& block) const;
    uint8_t GetClimbingWirePower(const BlockIterator& block) const;
    uint8_t GetCornerFacing(bool north, bool south, bool east, bool west) const;
    void ScheduleClimbingWireUpdates(const BlockIterator& block);
    
    // 各组件更新
    void UpdateRedstoneWire(const BlockIterator& block);
    void UpdateWireAppearance(const IntVec3& pos);
    void UpdateRedstoneTorch(const BlockIterator& block);
    void UpdateRepeater(const BlockIterator& block);
    void UpdateLamp(const BlockIterator& block);
    void UpdateLampLighting(const BlockIterator& block, bool turnOn);
    void UpdatePiston(const BlockIterator& block);
    void UpdateButton(const BlockIterator& block);
    void UpdateWireConnections(const BlockIterator& block);
    void UpdateNeighborWireConnections(const BlockIterator& block);
    
    // 活塞
    bool CanPistonPush(const BlockIterator& piston, Direction pushDir);
    void TryExtendPiston(const BlockIterator& block);
    void TryRetractPiston(const BlockIterator& block);
    bool CalculatePushList(const BlockIterator& piston,Direction pushDir,std::vector<IntVec3>& blocksToPush,
        std::vector<IntVec3>& blocksToDestroy) const;
    
    bool IsStronglyPowered(const BlockIterator& block) const;
    bool IsBlockReceivingPower(const BlockIterator& block) const;
    bool CanBePushed(uint8_t blockType) const;
    bool CanBePulled(uint8_t blockType) const;
    bool IsDestroyedWhenPushed(uint8_t blockType) const;
    bool IsStickyBlock(uint8_t blockType) const;
    IntVec3 GetDirectionVector(Direction dir) const;

    //torch
    Direction GetTorchAttachmentDirection(Block* torch) const;
    uint8_t GetTorchOutputPower(const BlockIterator& torch, Direction toDir) const;
    bool CheckTorchBurnout(const BlockIterator& block);
    void CleanupBurnoutTracker(float currentTime);

    //Lever
    Direction GetLeverAttachmentDirection(Block* lever) const;
    uint8_t GetLeverOutputPower(const BlockIterator& lever) const;
    void PowerAdjacentBlocks(const BlockIterator& source, uint8_t power);

    //repeater
    bool CheckRepeaterInput(const BlockIterator& block, Direction inputDir) const;
    // 检查中继器是否被锁定（侧边有激活的中继器指向它）
    bool CheckRepeaterLock(const BlockIterator& block) const;
    void ScheduleDelayedOutput(const BlockIterator& block, uint8_t delayTicks);
    uint8_t GetRepeaterOutputPower(const BlockIterator& repeater, Direction toDir) const;

    uint8_t GetRedstoneBlockOutput(const BlockIterator& block) const;
    
    int64_t PosToKey(const IntVec3& pos) const;
    Direction GetOppositeDir(Direction dir) const;
    void MarkChunkDirty(const BlockIterator& block);
};