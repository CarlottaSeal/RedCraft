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
    
    void Update(float deltaSeconds);
    
    // 标记方块需要红石更新
    void ScheduleUpdate(const BlockIterator& block, uint8_t delayTicks = 0);
    void ScheduleNeighborUpdates(const BlockIterator& block);
    
    // 获取方块的红石信号
    uint8_t GetInputPower(const BlockIterator& block) const;
    bool IsPowered(const BlockIterator& block) const;
    bool IsStronglyPowered(const BlockIterator& block) const;
    bool IsPistonPowered(const BlockIterator& block) const;
    
    void ExtendPiston(const BlockIterator& piston);
    void RetractPiston(const BlockIterator& piston);
    
    // 事件回调（World调用）
    void OnBlockPlaced(const BlockIterator& block);
    void OnBlockRemoved(const BlockIterator& block, uint8_t oldType);
    void OnBlockStateChanged(const BlockIterator& block, uint8_t oldType, uint8_t newType);
    
    //bool IsRedstoneBlockPowering(const BlockIterator& block,Direction toDir) const;
    
    void ToggleLever(const BlockIterator& block);
    void PressButton(const BlockIterator& block);
    void CycleRepeaterDelay(const BlockIterator& block);

    void UpdateNeighborWireConnections(const BlockIterator& block);
    WireConnections GetWireConnections(const BlockIterator& block) const;

private:    
    void ProcessBlockUpdate(const BlockIterator& block);
    void ProcessObserverUpdates();
    
    uint8_t GetCornerFacing(bool north, bool south, bool east, bool west) const;
    
    void UpdateRedstoneWire(const BlockIterator& block);
    void UpdateRedstoneTorch(const BlockIterator& block);
    void UpdateRepeater(const BlockIterator& block);
    //void UpdateComparator(const BlockIterator& block);
    void UpdateLamp(const BlockIterator& block);
    void UpdatePiston(const BlockIterator& block);
    void UpdateButton(const BlockIterator& block);
    void UpdateObserver(const BlockIterator& block);

    //red stone wire
    uint8_t GetDirectPowerInput(const BlockIterator& block) const;
    uint8_t GetAdjacentWirePower(const BlockIterator& block) const;
    uint8_t GetClimbingWirePower(const BlockIterator& block) const;
    void UpdateWireConnections(const BlockIterator& block);
    void ScheduleClimbingWireUpdates(const BlockIterator& block);
    WireConnection GetWireConnectionInDirection(const BlockIterator& block, Direction dir) const;
    bool CanConnectToRedstone(const BlockIterator& block, Direction fromDir) const;
    //void UpdateWireAppearance(const IntVec3& pos);
    void GetPerpendicularDirectionsForLeftAndRight(Direction facing, Direction& leftDir, Direction& rightDir) const;
    
    void UpdateLampLighting(const BlockIterator& block, bool turnOn);
    void CleanupBurnoutTracker();
    
    void TryExtendPiston(const BlockIterator& block);
    void TryRetractPiston(const BlockIterator& block);
    bool CanPushBlockChain(const BlockIterator& start, Direction pushDir, int& chainLength) const;
    bool CalculatePushList(const BlockIterator& piston,Direction pushDir,std::vector<IntVec3>& blocksToPush,
        std::vector<IntVec3>& blocksToDestroy) const;
    bool CanBePushed(uint8_t blockType) const;
    bool CanBePulled(uint8_t blockType) const;
    bool IsDestroyedWhenPushed(uint8_t blockType) const;
    Direction GetPistonFacing(Block* piston);
    void SetPistonExtended(Block* piston, bool extended);
    
    //torch
    Direction GetTorchAttachmentDirection(Block* torch) const;
    //uint8_t GetTorchOutputPower(const BlockIterator& torch, Direction toDir) const;
    bool CheckTorchBurnout(const BlockIterator& block);
    bool IsBlockReceivingPower(const BlockIterator& block) const;
    //void CleanupBurnoutTracker(float currentTime);
    
    // button
    Direction GetButtonAttachmentDirection(Block* button) const;
    //Lever
    Direction GetLeverAttachmentDirection(Block* lever) const;
    //uint8_t GetLeverOutputPower(const BlockIterator& lever) const;
    //void PowerAdjacentBlocks(const BlockIterator& source, uint8_t power);

    //repeater
    bool CheckRepeaterInput(const BlockIterator& block, Direction inputDir) const;
    bool CheckRepeaterLock(const BlockIterator& block) const;   // 检查中继器是否被锁定（侧边有激活的中继器指向它）
    void ScheduleDelayedOutput(const BlockIterator& block, uint8_t delayTicks);
    uint8_t GetRepeaterDelay(Block* repeater);
    void SetRepeaterDelay(Block* repeater, uint8_t delay);
    bool IsRepeaterLocked(Block* repeater);
    void SetRepeaterLocked(Block* repeater, bool locked);
    //uint8_t GetRepeaterOutputPower(const BlockIterator& repeater, Direction toDir) const;

    // observer
    void TriggerObserver(const BlockIterator& observer);
    void NotifyObserversOfChange(const BlockIterator& changedBlock);
    Direction GetObserverFacing(Block* observer) const;
    Direction GetObserverOutputDirection(Block* observer) const;

    uint8_t GetRedstoneBlockOutput(const BlockIterator& block) const;

    int64_t PosToKey(const IntVec3& pos) const;
    Direction GetOppositeDir(Direction dir) const;
    IntVec3 GetDirectionVector(Direction dir) const;
    void MarkChunkDirty(const BlockIterator& block);

    
private:
    World* m_world = nullptr;
    
    std::deque<RedstoneUpdate> m_updateQueue;
    std::unordered_set<int64_t> m_queuedPositions;  // 避免重复

    float m_lastBurnoutCleanup = 0.0f;
    std::deque<RedstoneUpdate> m_observerQueue;
    std::unordered_set<int64_t> m_queuedObservers;
    
    float m_tickTimer = 0.0f;
    uint32_t m_currentTick = 0;

    std::unordered_map<int64_t, TorchBurnoutEntry> m_torchBurnoutTracker;
};