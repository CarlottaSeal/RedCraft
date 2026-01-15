// Game/Redstone/RedstoneSimulator.cpp
#include "RedstoneSimulator.h"

#include <algorithm>
#include <queue>

#include "CropSystem.h"
#include "Engine/Core/Clock.hpp"
#include "Game/World.h"
#include "Game/Chunk.h"
#include "Game/Block.h"
#include "Game/BlockDefinition.h"
#include "Game/ChunkUtils.h"
#include "Game/Game.hpp"

RedstoneSimulator::RedstoneSimulator(World* world)
    : m_world(world)
{
}

void RedstoneSimulator::Update(float deltaSeconds)
{
    m_tickTimer += deltaSeconds;
    
    while (m_tickTimer >= REDSTONE_TICK_INTERVAL)
    {
        m_tickTimer -= REDSTONE_TICK_INTERVAL;
        m_currentTick++;
        
        // 处理常规更新队列
        int updatesThisTick = 0;
        const int MAX_UPDATES = 1000;
        
        while (!m_updateQueue.empty() && updatesThisTick < MAX_UPDATES)
        {
            RedstoneUpdate& update = m_updateQueue.front();
            
            if (update.m_scheduledTick > m_currentTick)
                break;
            
            BlockIterator block = update.m_block;
            m_updateQueue.pop_front();
            
            if (block.IsValid())
            {
                int64_t key = PosToKey(block.GetGlobalCoords());
                m_queuedPositions.erase(key);
                block.GetBlock()->SetRedstoneDirty(false);
                ProcessBlockUpdate(block);
            }
            updatesThisTick++;
        }
        // 处理侦测器脉冲结束
        ProcessObserverUpdates();
    }
    
    m_lastBurnoutCleanup += deltaSeconds;
    if (m_lastBurnoutCleanup >= 10.0f)
    {
        m_lastBurnoutCleanup = 0.0f;
        CleanupBurnoutTracker();
    }
}

void RedstoneSimulator::ScheduleUpdate(const BlockIterator& block, uint8_t delayTicks)
{
    if (!block.IsValid())
        return;
    
    int64_t key = PosToKey(block.GetGlobalCoords());
    
    // 避免重复添加
    if (m_queuedPositions.find(key) != m_queuedPositions.end())
        return;
    
    m_queuedPositions.insert(key);
    block.GetBlock()->SetRedstoneDirty(true);
    
    RedstoneUpdate update;
    update.m_block = block;
    update.m_scheduledTick = m_currentTick + delayTicks;
    
    // 按scheduledTick排序插入
    if (delayTicks == 0 || m_updateQueue.empty())
    {
        m_updateQueue.push_back(update);
    }
    else
    {
        auto it = m_updateQueue.begin();
        while (it != m_updateQueue.end() && it->m_scheduledTick <= update.m_scheduledTick)
            ++it;
        m_updateQueue.insert(it, update);
    }
}

void RedstoneSimulator::ScheduleNeighborUpdates(const BlockIterator& block)
{
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
        if (neighbor.IsValid())
        {
            ScheduleUpdate(neighbor, 0);
        }
    }
}

uint8_t RedstoneSimulator::GetInputPower(const BlockIterator& block) const
{
    if (!block.IsValid())
        return 0;
    uint8_t maxPower = 0;
    
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
        if (!neighbor.IsValid())
            continue;
        
        Block* nb = neighbor.GetBlock();
        if (!nb)
            continue;
        
        uint8_t neighborType = nb->m_typeIndex;
        //if (IsBlockProvidingPowerToDirection(neighbor, GetOppositeDir((Direction)dir)))
        if (IsBlockProvidingPowerToDirection(neighbor, (Direction)dir))
        {
            maxPower = 15;
            continue;
        }
        
        // 红石线（衰减传播）
        if (IsRedstoneWire(neighborType))
        {
            uint8_t wirePower = nb->GetRedstonePower();
            if (wirePower > 0)
            {
                uint8_t propagated = wirePower - 1;
                if (propagated > maxPower)
                    maxPower = propagated;
            }
        }
    }
    
    return maxPower;
}

bool RedstoneSimulator::IsPowered(const BlockIterator& block) const
{
    return GetInputPower(block) > 0;
}

bool RedstoneSimulator::IsPistonPowered(const BlockIterator& block) const
{
    if (!block.IsValid())
        return false;
    
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
        if (!neighbor.IsValid())
            continue;
        
        Block* nb = neighbor.GetBlock();
        if (!nb)
            continue;
        
        //if (IsBlockProvidingPowerToDirection(neighbor, GetOppositeDir((Direction)dir)))
        if (IsBlockProvidingPowerToDirection(neighbor, (Direction)dir))
            return true;
        
        if (IsRedstoneWire(nb->m_typeIndex) && nb->GetRedstonePower() > 0)
            return true;
        
        if (IsStronglyPowered(neighbor))
            return true;
    }
    
    return false;
}

void RedstoneSimulator::ProcessBlockUpdate(const BlockIterator& block)
{
    if (!block.IsValid())
        return;
    Block* b = block.GetBlock();
    if (!b)
        return;
    
    uint8_t blockType = b->m_typeIndex;
    if (IsRedstoneWire(blockType))
    {
        UpdateRedstoneWire(block);
        return;
    }
    
    switch (b->m_typeIndex)
    {
    case BLOCK_TYPE_REDSTONE_TORCH:
    case BLOCK_TYPE_REDSTONE_TORCH_OFF:
        UpdateRedstoneTorch(block);
        break;
    case BLOCK_TYPE_REPEATER_OFF:
    case BLOCK_TYPE_REPEATER_ON:
        UpdateRepeater(block);
        break;
    case BLOCK_TYPE_REDSTONE_LAMP_OFF:
    case BLOCK_TYPE_REDSTONE_LAMP_ON:
        UpdateLamp(block);
        break;
    case BLOCK_TYPE_REDSTONE_PISTON:
    case BLOCK_TYPE_REDSTONE_STICKY_PISTON:
        UpdatePiston(block);
        break;
    case BLOCK_TYPE_BUTTON_STONE:
    //case BLOCK_TYPE_BUTTON_WOOD:
        UpdateButton(block);
        break;
    }
}

void RedstoneSimulator::ProcessObserverUpdates()
{
    int processed = 0;
    const int MAX_UPDATES = 100;
    while (!m_observerQueue.empty() && processed < MAX_UPDATES)
    {
        RedstoneUpdate& update = m_observerQueue.front();
        
        if (update.m_scheduledTick > m_currentTick)
            break;
        
        BlockIterator observer = update.m_block;
        m_observerQueue.pop_front();
        
        if (observer.IsValid())
        {
            int64_t key = PosToKey(observer.GetGlobalCoords());
            m_queuedObservers.erase(key);
            UpdateObserver(observer);
        }
        processed++;
    }
}

uint8_t RedstoneSimulator::GetDirectPowerInput(const BlockIterator& block) const
{
    uint8_t maxPower = 0;
    
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
        if (!neighbor.IsValid())
            continue;
        
        Block* nb = neighbor.GetBlock();
        if (!nb)
            continue;
        
        if (IsBlockProvidingPowerToDirection(neighbor, (Direction)dir))
        {
            return 15; 
        }
        
        if (IsStronglyPowered(neighbor))
        {
            return 15;
        }
    }
    
    return maxPower;
}

uint8_t RedstoneSimulator::GetAdjacentWirePower(const BlockIterator& block) const
{
    uint8_t maxPower = 0;
    Direction horizontalDirs[] = {
        DIRECTION_NORTH, DIRECTION_SOUTH, 
        DIRECTION_EAST, DIRECTION_WEST
    };
    
    for (Direction dir : horizontalDirs)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary(dir);
        if (!neighbor.IsValid())
            continue;
        
        Block* nb = neighbor.GetBlock();
        if (!nb || !IsRedstoneWire(nb->m_typeIndex))
            continue;
        
        uint8_t neighborPower = nb->GetRedstonePower();
        if (neighborPower > 0)
        {
            uint8_t propagated = neighborPower - 1;  // 衰减1
            if (propagated > maxPower)
                maxPower = propagated;
        }
    }
    
    return maxPower;
}

uint8_t RedstoneSimulator::GetClimbingWirePower(const BlockIterator& block) const
{
    uint8_t maxPower = 0;
    Direction horizontalDirs[] = {
        DIRECTION_NORTH, DIRECTION_SOUTH, 
        DIRECTION_EAST, DIRECTION_WEST
    };
    for (Direction hDir : horizontalDirs)
    {
        // ===== 检查上方侧边红石线（下爬到我们） =====
        BlockIterator side = block.GetNeighborCrossBoundary(hDir);
        if (side.IsValid())
        {
            Block* sideBlock = side.GetBlock();
            
            // 侧边方块必须是透明的，红石才能爬升
            if (!sideBlock || !sideBlock->IsOpaque())
            {
                BlockIterator aboveSide = side.GetNeighborCrossBoundary(DIRECTION_UP);
                if (aboveSide.IsValid())
                {
                    Block* aboveSideBlock = aboveSide.GetBlock();
                    if (aboveSideBlock && 
                        aboveSideBlock->m_typeIndex)
                    {
                        // 还要检查我们上方是否有方块阻挡
                        BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
                        if (above.IsValid())
                        {
                            Block* aboveBlock = above.GetBlock();
                            if (!aboveBlock || !aboveBlock->IsOpaque())
                            {
                                uint8_t abovePower = aboveSideBlock->GetRedstonePower();
                                if (abovePower > 0)
                                {
                                    uint8_t propagated = abovePower - 1;
                                    if (propagated > maxPower)
                                        maxPower = propagated;
                                }
                            }
                        }
                    }
                }
            }
        }
        // ===== 检查下方侧边红石线（上爬到我们） =====
        BlockIterator below = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
        if (below.IsValid())
        {
            BlockIterator belowSide = below.GetNeighborCrossBoundary(hDir);
            if (belowSide.IsValid())
            {
                Block* belowSideBlock = belowSide.GetBlock();
                if (belowSideBlock && 
                    IsRedstoneWire(belowSideBlock->m_typeIndex))
                {
                    // 检查侧边方块是否阻挡
                    if (side.IsValid())
                    {
                        Block* sideBlock = side.GetBlock();
                        if (!sideBlock || !sideBlock->IsOpaque())
                        {
                            uint8_t belowPower = belowSideBlock->GetRedstonePower();
                            if (belowPower > 0)
                            {
                                uint8_t propagated = belowPower - 1;
                                if (propagated > maxPower)
                                    maxPower = propagated;
                            }
                        }
                    }
                }
            }
        }
    }
    return maxPower;
}

uint8_t RedstoneSimulator::GetCornerFacing(bool north, bool south, bool east, bool west) const
{
    // 返回角落的朝向（0-3）
    // 朝向定义：facing表示角落的"外角"指向
    
    if (north && east)  return 0;  // NE角，外角指向东北
    if (east && south)  return 1;  // SE角，外角指向东南
    if (south && west)  return 2;  // SW角，外角指向西南
    if (west && north)  return 3;  // NW角，外角指向西北
    
    return 0;  // 默认
}

void RedstoneSimulator::ScheduleClimbingWireUpdates(const BlockIterator& block)
{
    Direction horizontalDirs[] = {
        DIRECTION_NORTH, DIRECTION_SOUTH, 
        DIRECTION_EAST, DIRECTION_WEST
    };
    
    for (Direction hDir : horizontalDirs)
    {
        // 上方侧边
        BlockIterator side = block.GetNeighborCrossBoundary(hDir);
        if (side.IsValid())
        {
            BlockIterator aboveSide = side.GetNeighborCrossBoundary(DIRECTION_UP);
            if (aboveSide.IsValid())
            {
                Block* b = aboveSide.GetBlock();
                if (b && IsRedstoneWire(b->m_typeIndex))
                {
                    ScheduleUpdate(aboveSide, 0);
                }
            }
        }
        
        // 下方侧边
        BlockIterator below = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
        if (below.IsValid())
        {
            BlockIterator belowSide = below.GetNeighborCrossBoundary(hDir);
            if (belowSide.IsValid())
            {
                Block* b = belowSide.GetBlock();
                if (b && IsRedstoneWire(b->m_typeIndex))
                {
                    ScheduleUpdate(belowSide, 0);
                }
            }
        }
    }
}

void RedstoneSimulator::UpdateRedstoneWire(const BlockIterator& block)
{
    Block* wire = block.GetBlock();
    if (!wire || !IsRedstoneWire(wire->m_typeIndex))
        return;
    uint8_t oldPower = wire->GetRedstonePower();
    uint8_t newPower = 0;
    //检查直接相邻的电源方块
    newPower = GetDirectPowerInput(block);
    
    //检查相邻红石线（信号衰减）
    if (newPower < 15)
    {
        uint8_t wirePower = GetAdjacentWirePower(block);
        if (wirePower > newPower)
            newPower = wirePower;
    }
    //检查上下方红石线（爬升/下降）
    if (newPower < 15)
    {
        uint8_t climbPower = GetClimbingWirePower(block);
        if (climbPower > newPower)
            newPower = climbPower;
    }
    // 更新连接外观
    UpdateWireConnections(block);
    
    //信号改变时更新
    if (oldPower != newPower)
    {
        wire->SetRedstonePower(newPower);
        // 标记区块需要重建网格（颜色变化）
        MarkChunkDirty(block);
        // 触发邻居更新（传播信号变化）
        ScheduleNeighborUpdates(block);
        // 额外：触发上下方红石线更新（爬升连接）
        ScheduleClimbingWireUpdates(block);
    }
}

// void RedstoneSimulator::UpdateWireAppearance(const IntVec3& pos)
// {
//     Block* block = m_world->GetBlockAt(pos);
//     if (!IsRedstoneWire(block->GetType()))
//         return;
//     
//     // 计算连接状态
//     bool north = CanConnectTo(pos + IntVec3(0, 0, 1));
//     bool south = CanConnectTo(pos + IntVec3(0, 0, -1));
//     bool east  = CanConnectTo(pos + IntVec3(1, 0, 0));
//     bool west  = CanConnectTo(pos + IntVec3(-1, 0, 0));
//     
//     int connections = (north ? 1 : 0) + (south ? 1 : 0) + (east ? 1 : 0) + (west ? 1 : 0);
//     
//     // 根据连接数和方向选择方块类型
//     uint8_t newType;
//     uint8_t facing = 0;
//     
//     if (connections == 0)
//     {
//         newType = BLOCK_TYPE_REDSTONE_WIRE_DOT;
//     }
//     else if (connections == 2 && north && south) {
//         newType = BLOCK_TYPE_REDSTONE_WIRE_LINE;
//         facing = 0; // 南北向
//     }
//     else if (connections == 2 && east && west) {
//         newType = BLOCK_TYPE_REDSTONE_WIRE_LINE;
//         facing = 1; // 东西向
//     }
//     else if (connections == 4) {
//         newType = BLOCK_TYPE_REDSTONE_WIRE_CROSS;
//     }
//     else {
//         // 角落或T型，用Corner类型 + 旋转
//         newType = BLOCK_TYPE_REDSTONE_WIRE_CORNER;
//         facing = CalculateCornerFacing(north, south, east, west);
//     }
//     
//     block->SetType(newType);
//     block->SetBlockFacing(facing);
// }

void RedstoneSimulator::UpdateRedstoneTorch(const BlockIterator& block)
{
    if (CheckTorchBurnout(block))
    {
        Block* torch = block.GetBlock();
        if (torch && torch->m_typeIndex == BLOCK_TYPE_REDSTONE_TORCH)
        {
            // 强制熄灭
            torch->m_typeIndex = BLOCK_TYPE_REDSTONE_TORCH_OFF;
            MarkChunkDirty(block);
            m_world->MarkLightingDirty(block);
            // 不触发邻居更新，防止继续闪烁
            return;
        }
    }
    
    Block* torch = block.GetBlock();
    if (!torch)
        return;
    
    uint8_t torchType = torch->m_typeIndex;
    if (torchType != BLOCK_TYPE_REDSTONE_TORCH && 
        torchType != BLOCK_TYPE_REDSTONE_TORCH_OFF)
        return;
    
    // ===== 获取附着方块 =====
    // 火把的朝向存储在 BlockFacing 中
    // 0 = 附着在下方（立在方块上）
    // 1-4 = 附着在侧边（北、南、东、西）
    Direction attachDir = GetTorchAttachmentDirection(torch);
    BlockIterator attached = block.GetNeighborCrossBoundary(attachDir);
    
    // ===== 检查附着方块是否被充能 =====
    bool attachedPowered = false;
    if (attached.IsValid())
    {
        // 检查附着方块是否被红石信号充能
        // 注意：不是检查附着方块本身是否是电源
        attachedPowered = IsBlockReceivingPower(attached);
    }
    
    bool isCurrentlyOn = (torchType == BLOCK_TYPE_REDSTONE_TORCH);
    
    // ===== 状态切换 =====
    if (attachedPowered && isCurrentlyOn)
    {
        // 附着方块被充能 → 火把熄灭
        torch->m_typeIndex = BLOCK_TYPE_REDSTONE_TORCH_OFF;
        
        // 更新光照（从7降到0）
        MarkChunkDirty(block);
        m_world->MarkLightingDirty(block);
        
        // 触发邻居更新
        ScheduleNeighborUpdates(block);
        
        // 火把状态改变会影响上方方块
        BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
        if (above.IsValid())
        {
            ScheduleUpdate(above, 0);
        }
    }
    else if (!attachedPowered && !isCurrentlyOn)
    {
        // 附着方块未充能 → 火把点亮
        torch->m_typeIndex = BLOCK_TYPE_REDSTONE_TORCH;
        
        // 更新光照
        MarkChunkDirty(block);
        m_world->MarkLightingDirty(block);
        
        // 触发邻居更新
        ScheduleNeighborUpdates(block);
        
        BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
        if (above.IsValid())
        {
            ScheduleUpdate(above, 0);
        }
    }
    // Block* torch = block.GetBlock();
    // if (!torch)
    //     return;
    //
    // // 检查附着方块是否被充能
    // BlockIterator attached = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
    // bool shouldBeOff = false;
    //
    // if (attached.IsValid())
    // {
    //     shouldBeOff = IsPowered(attached);
    // }
    //
    // bool isCurrentlyOn = (torch->m_typeIndex == BLOCK_TYPE_REDSTONE_TORCH);
    //
    // if (shouldBeOff && isCurrentlyOn)
    // {
    //     torch->m_typeIndex = BLOCK_TYPE_REDSTONE_TORCH_OFF;
    //     MarkChunkDirty(block);
    //     ScheduleNeighborUpdates(block);
    // }
    // else if (!shouldBeOff && !isCurrentlyOn)
    // {
    //     torch->m_typeIndex = BLOCK_TYPE_REDSTONE_TORCH;
    //     MarkChunkDirty(block);
    //     ScheduleNeighborUpdates(block);
    // }
}

void RedstoneSimulator::UpdateRepeater(const BlockIterator& block)
{
    Block* repeater = block.GetBlock();
    if (!repeater)
        return;
    
    uint8_t repeaterType = repeater->m_typeIndex;
    if (repeaterType != BLOCK_TYPE_REPEATER_OFF && 
        repeaterType != BLOCK_TYPE_REPEATER_ON)
        return;
    
    // ===== 检查锁定状态 =====
    bool wasLocked = IsRepeaterLocked(repeater);
    bool nowLocked = CheckRepeaterLock(block);
    
    if (nowLocked != wasLocked)
    {
        SetRepeaterLocked(repeater, nowLocked);
        MarkChunkDirty(block);
    }
    
    // 如果被锁定，保持当前状态
    if (nowLocked)
    {
        return;
    }
    
    // ===== 检查输入信号 =====
    Direction facing = (Direction)repeater->GetBlockFacing();
    Direction inputDir = GetOppositeDir(facing);
    
    bool hasInput = CheckRepeaterInput(block, inputDir);
    bool isCurrentlyOn = (repeaterType == BLOCK_TYPE_REPEATER_ON);
    
    // ===== 状态转换 =====
    if (hasInput && !isCurrentlyOn)
    {
        // 关→开：需要延迟
        uint8_t delay = GetRepeaterDelay(repeater);
        
        // 立即切换类型（视觉），但延迟输出信号
        repeater->m_typeIndex = BLOCK_TYPE_REPEATER_ON;
        MarkChunkDirty(block);
        
        // 计划延迟后的邻居更新
        ScheduleDelayedOutput(block, delay);
    }
    else if (!hasInput && isCurrentlyOn)
    {
        // 开→关：同样有延迟
        uint8_t delay = GetRepeaterDelay(repeater);
        
        repeater->m_typeIndex = BLOCK_TYPE_REPEATER_OFF;
        MarkChunkDirty(block);
        
        ScheduleDelayedOutput(block, delay);
    }
}

void RedstoneSimulator::UpdateLamp(const BlockIterator& block)
{
    Block* lamp = block.GetBlock();
    if (!lamp)
        return;
    uint8_t lampType = lamp->m_typeIndex;
    if (lampType != BLOCK_TYPE_REDSTONE_LAMP_OFF && 
        lampType != BLOCK_TYPE_REDSTONE_LAMP_ON)
        return;
        
    bool powered = IsPowered(block);
    bool isCurrentlyOn = (lampType == BLOCK_TYPE_REDSTONE_LAMP_ON);
    
    if (powered && !isCurrentlyOn)
    {
        // 开灯：立即
        lamp->m_typeIndex = BLOCK_TYPE_REDSTONE_LAMP_ON;
        MarkChunkDirty(block);
        UpdateLampLighting(block, true);
    }
    else if (!powered && isCurrentlyOn)
    {
        // ✅ 关灯：延迟2 tick (MC标准)
        ScheduleUpdate(block, 2);
        
        // 检查是否真的应该关闭（延迟结束后再次检查）
        if (!IsPowered(block))
        {
            lamp->m_typeIndex = BLOCK_TYPE_REDSTONE_LAMP_OFF;
            MarkChunkDirty(block);
            UpdateLampLighting(block, false);
        }
    }
}

void RedstoneSimulator::UpdateLampLighting(const BlockIterator& block, bool turnOn)
{
    if (!block.IsValid())
        return;
    Block* lamp = block.GetBlock();
    if (!lamp)
        return;
    // 设置室内光照
    if (turnOn)
    {
        lamp->SetIndoorLight(15);
    }
    else
    {
        lamp->SetIndoorLight(0);
    }
    // 标记周围方块需要光照更新
    lamp->SetLightDirty(true);
    
    // 将方块加入世界的脏光照列表
    if (m_world)
    {
        //IntVec3 pos = block.GetGlobalCoords();
        m_world->MarkLightingDirty(block);
        
        // 也标记邻居
        for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
        {
            BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
            if (neighbor.IsValid())
            {
                Block* nb = neighbor.GetBlock();
                if (nb)
                {
                    nb->SetLightDirty(true);
                    m_world->MarkLightingDirty(neighbor);
                }
            }
        }
    }
}

void RedstoneSimulator::CleanupBurnoutTracker()
{
    if (!m_world || !m_world->m_owner || !m_world->m_owner->m_gameClock)
        return;
        
    float currentTime = (float)m_world->m_owner->m_gameClock->GetTotalSeconds();
    
    auto it = m_torchBurnoutTracker.begin();
    while (it != m_torchBurnoutTracker.end())
    {
        // 清理超过2秒无活动的条目
        if (currentTime - it->second.m_timestamp > 2.0f)
        {
            it = m_torchBurnoutTracker.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void RedstoneSimulator::UpdatePiston(const BlockIterator& block)
{
    Block* piston = block.GetBlock();
    if (!piston)
        return;
    uint8_t pistonType = piston->m_typeIndex;
    if (pistonType != BLOCK_TYPE_REDSTONE_PISTON && 
        pistonType != BLOCK_TYPE_REDSTONE_STICKY_PISTON)
        return;
    
    bool powered = IsPistonPowered(block);
    bool extended = piston->GetSpecialState();
    Direction facing = (Direction)piston->GetBlockFacing();
    
    if (powered && !extended)
    {
        TryExtendPiston(block);
    }
    else if (!powered && extended)
    {
        TryRetractPiston(block);
    }
}

void RedstoneSimulator::TryExtendPiston(const BlockIterator& block)
{
    Block* piston = block.GetBlock();
    if (piston->IsPistonExtended())
        return;
    if (!IsPistonPowered(block))
        return;
    ExtendPiston(block);
    // Block* piston = block.GetBlock();
    // if (!piston)
    //     return;
    // Direction facing = GetPistonFacing(piston);
    // bool isSticky = (piston->m_typeIndex == BLOCK_TYPE_STICKY_PISTON);
    // // 收集要推动的方块
    // std::vector<IntVec3> blocksToPush;
    // std::vector<IntVec3> blocksToDestroy;
    // if (!CalculatePushList(block, facing, blocksToPush, blocksToDestroy))
    // {
    //     // 无法推动
    //     return;
    // }
    // //执行推动 
    // // 1. 销毁需要销毁的方块（如火把、红石线等）
    // for (const IntVec3& pos : blocksToDestroy)
    // {
    //     BlockIterator iter = m_world->GetBlockIterator(pos);
    //     if (iter.IsValid())
    //     {
    //         Block* b = iter.GetBlock();
    //         if (b)
    //         {
    //             // 可选：掉落物品
    //             // DropBlockItem(iter);
    //             
    //             b->SetType(BLOCK_TYPE_AIR);
    //             MarkChunkDirty(iter);
    //         }
    //     }
    // }
    // // 2. 从最远的方块开始移动（避免覆盖）
    // // 按距离活塞的距离排序（远的先移动）
    // std::sort(blocksToPush.begin(), blocksToPush.end(),
    //     [&](const IntVec3& a, const IntVec3& b) {
    //         IntVec3 pistonPos = block.GetGlobalCoords();
    //         return GetTaxicabDistance3D(a, pistonPos) > 
    //                GetTaxicabDistance3D(b, pistonPos);
    //     });
    // for (const IntVec3& srcPos : blocksToPush)
    // {
    //     IntVec3 dstPos = srcPos + GetDirectionVector(facing);
    //     
    //     BlockIterator src = m_world->GetBlockIterator(srcPos);
    //     BlockIterator dst = m_world->GetBlockIterator(dstPos);
    //     
    //     if (src.IsValid() && dst.IsValid())
    //     {
    //         Block* srcBlock = src.GetBlock();
    //         Block* dstBlock = dst.GetBlock();
    //         
    //         // 复制方块数据
    //         *dstBlock = *srcBlock;
    //         
    //         // 清空源位置
    //         srcBlock->SetType(BLOCK_TYPE_AIR);
    //         srcBlock->m_redstoneData = 0;
    //         
    //         MarkChunkDirty(src);
    //         MarkChunkDirty(dst);
    //         
    //         // 触发红石更新
    //         ScheduleNeighborUpdates(dst);
    //     }
    // }
    // // 3. 放置活塞头
    // BlockIterator headPos = block.GetNeighborCrossBoundary(facing);
    // if (headPos.IsValid())
    // {
    //     Block* head = headPos.GetBlock();
    //     head->SetType(BLOCK_TYPE_PISTON_HEAD);
    //     head->SetBlockFacing((uint8_t)facing);
    //     
    //     // 标记是否为粘性活塞头
    //     if (isSticky)
    //     {
    //         head->SetRedstonePower(head->GetRedstonePower() | 0x08);
    //     }
    //     
    //     MarkChunkDirty(headPos);
    // }
    // // 4. 标记活塞为已伸出
    // SetPistonExtended(piston, true);
    // MarkChunkDirty(block);
}

void RedstoneSimulator::TryRetractPiston(const BlockIterator& block)
{
    Block* piston = block.GetBlock();
    if (!piston->IsPistonExtended())
        return;
    if (IsPistonPowered(block))
        return;
    RetractPiston(block);
    // Block* piston = block.GetBlock();
    // if (!piston)
    //     return;
    // Direction facing = GetPistonFacing(piston);
    // bool isSticky = (piston->m_typeIndex == BLOCK_TYPE_STICKY_PISTON);
    //
    // // 1. 移除活塞头
    // BlockIterator headPos = block.GetNeighborCrossBoundary(facing);
    // if (headPos.IsValid())
    // {
    //     Block* head = headPos.GetBlock();
    //     if (head && head->m_typeIndex == BLOCK_TYPE_PISTON_HEAD)
    //     {
    //         head->SetType(BLOCK_TYPE_AIR);
    //         head->m_redstoneData = 0;
    //         MarkChunkDirty(headPos);
    //     }
    // }
    // // 2. 粘性活塞：尝试拉回前方方块
    // if (isSticky)
    // {
    //     BlockIterator pullPos = headPos.GetNeighborCrossBoundary(facing);
    //     if (pullPos.IsValid())
    //     {
    //         Block* pullBlock = pullPos.GetBlock();
    //         if (pullBlock && 
    //             pullBlock->m_typeIndex != BLOCK_TYPE_AIR &&
    //             CanBePulled(pullBlock->m_typeIndex))
    //         {
    //             // 将方块拉回到活塞头位置
    //             Block* dst = headPos.GetBlock();
    //             *dst = *pullBlock;
    //             
    //             pullBlock->SetType(BLOCK_TYPE_AIR);
    //             pullBlock->m_redstoneData = 0;
    //             
    //             MarkChunkDirty(headPos);
    //             MarkChunkDirty(pullPos);
    //             
    //             // 触发红石更新
    //             ScheduleNeighborUpdates(headPos);
    //             ScheduleNeighborUpdates(pullPos);
    //         }
    //     }
    // }
    // // 3. 标记活塞为收回状态
    // SetPistonExtended(piston, false);
    // MarkChunkDirty(block);
}

void RedstoneSimulator::ExtendPiston(const BlockIterator& piston)
{
    if (!piston.IsValid()) return;
    
    Block* pistonBlock = piston.GetBlock();
    if (!pistonBlock) return;
    
    if (pistonBlock->IsPistonExtended()) return;
    
    Direction facing = GetPistonFacing(pistonBlock);
    IntVec3 pistonPos = piston.GetGlobalCoords();
    IntVec3 headPos = pistonPos + GetDirectionVector(facing);
    
    std::vector<IntVec3> blocksToPush;
    std::vector<IntVec3> blocksToDestroy;
    if (!CalculatePushList(piston, facing, blocksToPush, blocksToDestroy))
        return;
    
    // 从远到近移动方块
    for (int i = (int)blocksToPush.size() - 1; i >= 0; i--)
    {
        IntVec3 fromPos = blocksToPush[i];
        IntVec3 toPos = fromPos + GetDirectionVector(facing);
        
        Block fromBlock = m_world->GetBlockAtWorldCoords(fromPos.x, fromPos.y, fromPos.z);
        
        BlockIterator toIter = m_world->GetBlockIterator(toPos);
        if (toIter.IsValid())
        {
            Block* toBlock = toIter.GetBlock();
            if (toBlock)
            {
                toBlock->SetType(fromBlock.m_typeIndex);
                toBlock->m_redstoneData = fromBlock.m_redstoneData;
            }
        }
        
        BlockIterator fromIter = m_world->GetBlockIterator(fromPos);
        if (fromIter.IsValid())
        {
            Block* b = fromIter.GetBlock();
            if (b)
            {
                b->SetType(BLOCK_TYPE_AIR);
                b->m_redstoneData = 0;
            }
            MarkChunkDirty(fromIter);
        }
    }
    
    // 处理被破坏的方块（包括作物掉落）
    for (const IntVec3& pos : blocksToDestroy)
    {
        BlockIterator iter = m_world->GetBlockIterator(pos);
        if (!iter.IsValid())
            continue;
        
        Block* b = iter.GetBlock();
        if (!b)
            continue;
        
        uint8_t blockType = b->m_typeIndex;
        // 如果是作物，触发掉落
        if (IsCrop(blockType))
        {
            std::string droppedItemName = BlockDefinition::GetBlockDef(blockType).m_droppedItemName;
            ItemType item = StringToItemType(droppedItemName);
            m_world->m_cropSystem->OnCropDestroyed(pos, item);
        }
        // 破坏方块
        b->SetType(BLOCK_TYPE_AIR);
        b->m_redstoneData = 0;
        MarkChunkDirty(iter);
    }
    
    // 放置活塞头
    BlockIterator headIter = m_world->GetBlockIterator(headPos);
    if (headIter.IsValid())
    {
        Block* head = headIter.GetBlock();
        if (head)
        {
            head->SetType(BLOCK_TYPE_PISTON_HEAD);
            head->SetBlockFacing((uint8_t)facing);
        }
        MarkChunkDirty(headIter);
    }
    
    // 标记活塞为已伸出
    pistonBlock->SetPistonExtended(true);
    MarkChunkDirty(piston);
}

void RedstoneSimulator::RetractPiston(const BlockIterator& piston)
{
    if (!piston.IsValid()) return;
    
    Block* pistonBlock = piston.GetBlock();
    if (!pistonBlock) return;
    
    if (!pistonBlock->IsPistonExtended()) return;
    
    Direction facing = GetPistonFacing(pistonBlock);
    IntVec3 pistonPos = piston.GetGlobalCoords();
    IntVec3 headPos = pistonPos + GetDirectionVector(facing);
    
    Block headBlock = m_world->GetBlockAtWorldCoords(headPos.x, headPos.y, headPos.z);
    
    if (headBlock.m_typeIndex == BLOCK_TYPE_PISTON_HEAD )
    {
        Block toDestroy = m_world->GetBlockAtWorldCoords(headPos.x, headPos.y, headPos.z);
        toDestroy.SetType(BLOCK_TYPE_AIR);
        //m_world->SetBlockType(headPos.x, headPos.y, headPos.z, BLOCK_TYPE_AIR);
        
        // if (pistonBlock->m_typeIndex == BLOCK_TYPE_STICKY_PISTON) //TODO 暂时先不做
        // {
        //     IntVec3 pullPos = headPos + GetDirectionVector(facing);
        //     Block pullBlock = m_world->GetBlockAtWorldCoords(pullPos.x, pullPos.y, pullPos.z);
        //     
        //     if (CanBePulled(pullBlock.m_typeIndex))
        //     {
        //         m_world->SetBlockType(headPos.x, headPos.y, headPos.z, pullBlock.m_typeIndex);
        //         m_world->SetBlockType(pullPos.x, pullPos.y, pullPos.z, BLOCK_TYPE_AIR);
        //     }
        // }
    }
    
    pistonBlock->SetPistonExtended(false);
    MarkChunkDirty(piston);
}

bool RedstoneSimulator::CanPushBlockChain(const BlockIterator& start, Direction pushDir, int& chainLength) const
{
    chainLength = 0;
    
    BlockIterator current = start.GetNeighborCrossBoundary(pushDir);
    
    while (current.IsValid() && chainLength < MAX_PISTON_PUSH)
    {
        Block* b = current.GetBlock();
        if (!b) return false;
        
        if (b->m_typeIndex == BLOCK_TYPE_AIR)
            return true;
        
        if (!CanBePushed(b->m_typeIndex))
            return false;
        
        if (IsDestroyedWhenPushed(b->m_typeIndex))
        {
            chainLength++;
            return true;
        }
        
        chainLength++;
        current = current.GetNeighborCrossBoundary(pushDir);
    }
    
    return false;
}

bool RedstoneSimulator::CalculatePushList(const BlockIterator& piston, Direction pushDir,
                                          std::vector<IntVec3>& blocksToPush, std::vector<IntVec3>& blocksToDestroy) const
{
    blocksToPush.clear();
    blocksToDestroy.clear();
    
    std::queue<IntVec3> toCheck;
    std::unordered_set<int64_t> checked;
    
    IntVec3 startPos = piston.GetGlobalCoords() + GetDirectionVector(pushDir);
    toCheck.push(startPos);
    
    while (!toCheck.empty())
    {
        IntVec3 pos = toCheck.front();
        toCheck.pop();
        
        int64_t key = PosToKey(pos);
        if (checked.find(key) != checked.end())
            continue;
        checked.insert(key);
        
        BlockIterator iter = m_world->GetBlockIterator(pos);
        if (!iter.IsValid())
            return false;
        
        Block* b = iter.GetBlock();
        if (!b)
            continue;
        
        uint8_t blockType = b->m_typeIndex;
        
        if (blockType == BLOCK_TYPE_AIR)
            continue;
        
        if (!CanBePushed(blockType))
            return false;
        
        if (IsDestroyedWhenPushed(blockType))
        {
            blocksToDestroy.push_back(pos);
            continue;
        }
        
        // 中继器特殊处理：只能从输入/输出方向推动
        if (blockType == BLOCK_TYPE_REPEATER_OFF || blockType == BLOCK_TYPE_REPEATER_ON)
        {
            Direction repeaterFacing = (Direction)b->GetBlockFacing();
            // 只能从正面或背面推动，不能从侧面推动
            if (pushDir != repeaterFacing && pushDir != GetOppositeDir(repeaterFacing))
            {
                return false;  // 侧面推动失败
            }
        }
        
        blocksToPush.push_back(pos);
        
        if (blocksToPush.size() > MAX_PISTON_PUSH)
            return false;
        
        IntVec3 nextPos = pos + GetDirectionVector(pushDir);
        toCheck.push(nextPos);
    }
    
    return true;
}

bool RedstoneSimulator::IsStronglyPowered(const BlockIterator& block) const
{
    if (!block.IsValid())
        return false;
    
    Block* b = block.GetBlock();
    if (!b || !b->IsSolid() || !b->IsOpaque())
        return false;
    
    // 检查是否有电源直接指向这个方块
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
        if (!neighbor.IsValid())
            continue;
        
        Block* nb = neighbor.GetBlock();
        if (!nb)
            continue;
        
        uint8_t neighborType = nb->m_typeIndex;
        
        // 红石火把在下方（附着）会强充能上方方块
        if (neighborType == BLOCK_TYPE_REDSTONE_TORCH && 
            (Direction)dir == DIRECTION_DOWN)
        {
            return true;
        }
        
        // 开启的中继器向前方强充能
        if (neighborType == BLOCK_TYPE_REPEATER_ON)
        {
            Direction facing = (Direction)nb->GetBlockFacing();
            if ((Direction)dir == GetOppositeDir(facing))
                return true;
        }
        if (neighborType == BLOCK_TYPE_REDSTONE_OBSERVER && nb->GetSpecialState())
        {
            if ((Direction)dir == GetOppositeDir(GetObserverOutputDirection(nb)))
                return true;
        }
    }
    
    return false;
}

bool RedstoneSimulator::IsBlockReceivingPower(const BlockIterator& block) const
{
    if (!block.IsValid())
        return false;
    Block* b = block.GetBlock();
    if (!b)
        return false;
    
    // 如果是红石组件，检查其自身状态
    if (IsRedstoneComponent(b->m_typeIndex))
    {
        return b->GetRedstonePower() > 0;
    }
    // 对于普通方块，检查是否有信号输入
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
        if (!neighbor.IsValid())
            continue;
        
        Block* nb = neighbor.GetBlock();
        if (!nb)
            continue;
        
        uint8_t neighborType = nb->m_typeIndex;
        
        // 红石线可以弱充能方块
        if (IsRedstoneWire(neighborType))
        {
            if (nb->GetRedstonePower() > 0)
                return true;
        }
        
        // 红石块可以充能
        if (neighborType == BLOCK_TYPE_REDSTONE_BLOCK)
            return true;
        
        // 开启的拉杆
        if (neighborType == BLOCK_TYPE_REDSTONE_LEVER && nb->GetSpecialState())
            return true;
        
        // 按下的按钮
        if ((neighborType == BLOCK_TYPE_BUTTON_STONE ) && 
            nb->GetSpecialState())
            return true;
        
        // 开启的中继器（只向输出方向充能）
        if (neighborType == BLOCK_TYPE_REPEATER_ON)
        {
            Direction facing = (Direction)nb->GetBlockFacing();
            if (GetOppositeDir(facing) == (Direction)dir)
                return true;
        }
    }
    return false;
}

bool RedstoneSimulator::CanBePushed(uint8_t blockType) const
{
    switch (blockType)
    {
    case BLOCK_TYPE_OBSIDIAN:
    case BLOCK_TYPE_PISTON_HEAD:
        return false;
    default:
        return true;
    }
    // //TODO: 什么类型？
    // if (IsSolid(blockType))
    //     return false;
    // return true;
}

bool RedstoneSimulator::CanBePulled(uint8_t blockType) const
{
    // 大部分可推动的方块也可被拉动
    if (!CanBePushed(blockType))
        return false;
    // 会被破坏的方块不会被拉动
    if (IsDestroyedWhenPushed(blockType))
        return false;
    return true;
}

bool RedstoneSimulator::IsDestroyedWhenPushed(uint8_t blockType) const
{
    if (IsRedstoneWire(blockType))
        return true;
    switch (blockType)
    {
    case BLOCK_TYPE_REDSTONE_TORCH:
    case BLOCK_TYPE_REDSTONE_TORCH_OFF:
    case BLOCK_TYPE_REDSTONE_LEVER:
    case BLOCK_TYPE_BUTTON_STONE:
    //case BLOCK_TYPE_BUTTON_WOOD:
        return true;
    default:
        break;
    }
    if (IsCrop(blockType))
        return true;
    return false;
}

Direction RedstoneSimulator::GetPistonFacing(Block* piston)
{
    return (Direction)piston->GetBlockFacing();
}

void RedstoneSimulator::SetPistonExtended(Block* piston, bool extended)
{
    piston->SetSpecialState(extended);
}

IntVec3 RedstoneSimulator::GetDirectionVector(Direction dir) const
{
    switch (dir)
    {
    case DIRECTION_DOWN:  return IntVec3(0, 0, -1);
    case DIRECTION_UP:    return IntVec3(0, 0, 1);
    case DIRECTION_NORTH: return IntVec3(0, 1, 0);
    case DIRECTION_SOUTH: return IntVec3(0, -1, 0);
    case DIRECTION_EAST:  return IntVec3(1, 0, 0);
    case DIRECTION_WEST:  return IntVec3(-1, 0, 0);
    default: return IntVec3(0, 0, 0);
    }
}

Direction RedstoneSimulator::GetTorchAttachmentDirection(Block* torch) const
{
    uint8_t facing = torch->GetBlockFacing();
    switch (facing)
    {
    case 0: return DIRECTION_DOWN; 
    case 1: return DIRECTION_NORTH;
    case 2: return DIRECTION_SOUTH;
    case 3: return DIRECTION_EAST;
    case 4: return DIRECTION_WEST;
    default: return DIRECTION_DOWN;
    }
}

// uint8_t RedstoneSimulator::GetTorchOutputPower(const BlockIterator& torch, Direction toDir) const
// {
//     Block* b = torch.GetBlock();
//     if (!b || b->m_typeIndex != BLOCK_TYPE_REDSTONE_TORCH)
//         return 0;
//     
//     // 火把不向附着方向输出
//     Direction attachDir = GetTorchAttachmentDirection(b);
//     if (toDir == attachDir)
//         return 0;
//     return 15;
// }

bool RedstoneSimulator::CheckTorchBurnout(const BlockIterator& block)
{
    int64_t key = PosToKey(block.GetGlobalCoords());
    float currentTime = (float)m_world->m_owner->m_gameClock->GetTotalSeconds();
    
    auto it = m_torchBurnoutTracker.find(key);
    if (it == m_torchBurnoutTracker.end())
    {
        // 首次切换
        TorchBurnoutEntry entry;
        entry.m_posKey = key;
        entry.m_timestamp = currentTime;
        entry.m_toggleCount = 1;
        m_torchBurnoutTracker[key] = entry;
        return false;
    }
    TorchBurnoutEntry& entry = it->second;
    // 检查时间窗口
    if (currentTime - entry.m_timestamp > TORCH_BURNOUT_WINDOW)
    {
        // 重置计数
        entry.m_timestamp = currentTime;
        entry.m_toggleCount = 1;
        return false;
    }
    // 增加计数
    entry.m_toggleCount++;
    
    if (entry.m_toggleCount >= TORCH_BURNOUT_THRESHOLD)
    {
        // 触发保护：火把熄灭并保持一段时间
        return true;
    }
    return false;
}

// void RedstoneSimulator::CleanupBurnoutTracker(float currentTime)
// {
// }

Direction RedstoneSimulator::GetLeverAttachmentDirection(Block* lever) const
{
    // 拉杆朝向存储在 BlockFacing 中
    // 0 = 附着在下方（地面）
    // 1 = 附着在上方（天花板）
    // 2-5 = 附着在侧边（北、南、东、西）
    uint8_t facing = lever->GetBlockFacing();
    switch (facing)
    {
    case 0: return DIRECTION_DOWN;   // 在地面上
    case 1: return DIRECTION_UP;     // 在天花板上
    case 2: return DIRECTION_NORTH;
    case 3: return DIRECTION_SOUTH;
    case 4: return DIRECTION_EAST;
    case 5: return DIRECTION_WEST;
    default: return DIRECTION_DOWN;
    }
}

// uint8_t RedstoneSimulator::GetLeverOutputPower(const BlockIterator& lever) const
// {
//     if (!lever.IsValid())
//         return 0;
//     Block* b = lever.GetBlock();
//     if (!b || b->m_typeIndex != BLOCK_TYPE_LEVER)
//         return 0;
//     return b->GetSpecialState() ? 15 : 0;
// }

// void RedstoneSimulator::PowerAdjacentBlocks(const BlockIterator& source, uint8_t power)
// {
//     for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
//     {
//         BlockIterator neighbor = source.GetNeighborCrossBoundary((Direction)dir);
//         if (!neighbor.IsValid())
//             continue;
//         
//         Block* nb = neighbor.GetBlock();
//         if (!nb)
//             continue;
//         
//         // 红石组件立即更新
//         if (IsRedstoneComponent(nb->m_typeIndex))
//         {
//             ScheduleUpdate(neighbor, 0);
//         }
//     }
// }

bool RedstoneSimulator::CheckRepeaterInput(const BlockIterator& block, Direction inputDir) const
{
    BlockIterator inputBlock = block.GetNeighborCrossBoundary(inputDir);
    if (!inputBlock.IsValid())
        return false;
    
    Block* input = inputBlock.GetBlock();
    if (!input)
        return false;
    
    uint8_t inputType = input->m_typeIndex;
    
    if (IsRedstoneWire(inputType))
    {
        return input->GetRedstonePower() > 0;
    }
    if (inputType == BLOCK_TYPE_REDSTONE_BLOCK)
    {
        return true;
    }
    if (inputType == BLOCK_TYPE_REDSTONE_TORCH)
    {
        return true;
    }
    // 另一个中继器（同向输出）
    if (inputType == BLOCK_TYPE_REPEATER_ON)
    {
        Direction otherFacing = (Direction)input->GetBlockFacing();
        // 检查它是否向我们输出
        if (otherFacing == inputDir)
        {
            return true;
        }
    }
    // // 比较器输出
    // if (inputType == BLOCK_TYPE_COMPARATOR_ON)
    // {
    //     Direction compFacing = (Direction)input->GetBlockFacing();
    //     if (compFacing == inputDir)
    //     {
    //         return input->GetRedstonePower() > 0;
    //     }
    // }
    // 被强充能的实心方块
    if (IsStronglyPowered(inputBlock))
    {
        return true;
    }
    
    return false;
}

bool RedstoneSimulator::CheckRepeaterLock(const BlockIterator& block) const
{
    Block* repeater = block.GetBlock();
    if (!repeater)
        return false;
    
    Direction facing = (Direction)repeater->GetBlockFacing();
    
    // 获取垂直于朝向的两个方向
    Direction leftDir, rightDir;
    GetPerpendicularDirectionsForLeftAndRight(facing, leftDir, rightDir);
    
    // 检查左侧
    BlockIterator leftBlock = block.GetNeighborCrossBoundary(leftDir);
    if (leftBlock.IsValid())
    {
        Block* left = leftBlock.GetBlock();
        if (left && left->m_typeIndex == BLOCK_TYPE_REPEATER_ON)
        {
            Direction leftFacing = (Direction)left->GetBlockFacing();
            // 左侧中继器指向我们（它的输出方向是我们的右边方向）
            if (leftFacing == GetOppositeDir(leftDir))
            {
                return true;
            }
        }
    }
    // 检查右侧
    BlockIterator rightBlock = block.GetNeighborCrossBoundary(rightDir);
    if (rightBlock.IsValid())
    {
        Block* right = rightBlock.GetBlock();
        if (right && right->m_typeIndex == BLOCK_TYPE_REPEATER_ON)
        {
            Direction rightFacing = (Direction)right->GetBlockFacing();
            if (rightFacing == GetOppositeDir(rightDir))
            {
                return true;
            }
        }
    }
    return false;
}

void RedstoneSimulator::ScheduleDelayedOutput(const BlockIterator& block, uint8_t delayTicks)
{
    Block* repeater = block.GetBlock();
    if (!repeater)
        return;
    
    Direction facing = (Direction)repeater->GetBlockFacing();
    
    // 计划前方方块更新
    BlockIterator output = block.GetNeighborCrossBoundary(facing);
    if (output.IsValid())
    {
        ScheduleUpdate(output, delayTicks);
    }
    
    // 也计划周围方块更新
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        if ((Direction)dir == facing || (Direction)dir == GetOppositeDir(facing))
            continue;  // 跳过输入和输出方向
            
        BlockIterator neighbor = block.GetNeighborCrossBoundary((Direction)dir);
        if (neighbor.IsValid())
        {
            ScheduleUpdate(neighbor, delayTicks);
        }
    }
}

uint8_t RedstoneSimulator::GetRepeaterDelay(Block* repeater)
{
    return (repeater->GetRedstonePower() & 0x03) + 1;  // 1-4
}

void RedstoneSimulator::SetRepeaterDelay(Block* repeater, uint8_t delay)
{
    delay = (delay < 1) ? 1 : (delay > 4) ? 4 : delay;
    uint8_t data = repeater->GetRedstonePower();
    data = (data & 0xFC) | ((delay - 1) & 0x03);
    repeater->SetRedstonePower(data);
}

bool RedstoneSimulator::IsRepeaterLocked(Block* repeater)
{
    return (repeater->GetRedstonePower() & 0x04) != 0;
}

void RedstoneSimulator::SetRepeaterLocked(Block* repeater, bool locked)
{
    uint8_t data = repeater->GetRedstonePower();
    if (locked) data |= 0x04;
    else data &= ~0x04;
    repeater->SetRedstonePower(data);
}

void RedstoneSimulator::TriggerObserver(const BlockIterator& observer)
{
    if (!observer.IsValid()) return;
    
    Block* b = observer.GetBlock();
    if (!b || b->m_typeIndex != BLOCK_TYPE_REDSTONE_OBSERVER) return;
    
    if (b->GetSpecialState()) return;
    
    b->SetSpecialState(true);
    b->SetRedstonePower(15);
    MarkChunkDirty(observer);
    
    Direction outputDir = GetObserverOutputDirection(b);
    BlockIterator output = observer.GetNeighborCrossBoundary(outputDir);
    if (output.IsValid())
    {
        ScheduleUpdate(output, 0);
        ScheduleNeighborUpdates(output);
    }
    
    int64_t key = PosToKey(observer.GetGlobalCoords());
    if (m_queuedObservers.find(key) == m_queuedObservers.end())
    {
        m_queuedObservers.insert(key);
        
        RedstoneUpdate update;
        update.m_block = observer;
        update.m_scheduledTick = m_currentTick + OBSERVER_PULSE_TICKS;
        
        auto it = m_observerQueue.begin();
        while (it != m_observerQueue.end() && it->m_scheduledTick <= update.m_scheduledTick)
            ++it;
        m_observerQueue.insert(it, update);
    }
}

void RedstoneSimulator::NotifyObserversOfChange(const BlockIterator& changedBlock)
{
    if (!changedBlock.IsValid()) return;
    
    for (int dir = 0; dir < NUM_DIRECTIONS; dir++)
    {
        BlockIterator neighbor = changedBlock.GetNeighborCrossBoundary((Direction)dir);
        if (!neighbor.IsValid()) continue;
        
        Block* nb = neighbor.GetBlock();
        if (!nb || nb->m_typeIndex != BLOCK_TYPE_REDSTONE_OBSERVER) continue;
        
        Direction observerFacing = GetObserverFacing(nb);
        if (GetOppositeDir(observerFacing) == (Direction)dir)
            TriggerObserver(neighbor);
    }
}

Direction RedstoneSimulator::GetObserverFacing(Block* observer) const
{
    if (!observer) return DIRECTION_NORTH;
    return (Direction)observer->GetBlockFacing();
}

Direction RedstoneSimulator::GetObserverOutputDirection(Block* observer) const
{
    return GetOppositeDir(GetObserverFacing(observer));
}

// uint8_t RedstoneSimulator::GetRepeaterOutputPower(const BlockIterator& repeater, Direction toDir) const
// {
//     if (!repeater.IsValid())
//         return 0;
//     
//     Block* b = repeater.GetBlock();
//     if (!b)
//         return 0;
//     
//     // 只有激活的中继器输出功率
//     if (b->m_typeIndex != BLOCK_TYPE_REPEATER_ON)
//         return 0;
//     
//     // 只向前方输出
//     Direction facing = (Direction)b->GetBlockFacing();
//     if (toDir != facing)
//         return 0;
//     
//     // 中继器输出恒定15级信号（信号增强功能）
//     return 15;
// }

uint8_t RedstoneSimulator::GetRedstoneBlockOutput(const BlockIterator& block) const
{
    if (!block.IsValid())
        return 0;
    
    Block* b = block.GetBlock();
    if (!b || b->m_typeIndex != BLOCK_TYPE_REDSTONE_BLOCK)
        return 0;
    
    // 红石块向所有方向输出15级信号
    return 15;
}

void RedstoneSimulator::UpdateButton(const BlockIterator& block)
{
    if (!block.IsValid())
        return;
    Block* button = block.GetBlock();
    if (!button)
        return;
    uint8_t buttonType = button->m_typeIndex;
    if (buttonType != BLOCK_TYPE_BUTTON_STONE)
        return;
    // 如果按钮是按下状态，弹起
    if (button->GetSpecialState())
    {
        button->SetSpecialState(false);
        button->SetRedstonePower(0);
        
        MarkChunkDirty(block);
        
        ScheduleNeighborUpdates(block);
        
        Direction attachDir = GetButtonAttachmentDirection(button);
        BlockIterator attached = block.GetNeighborCrossBoundary(attachDir);
        if (attached.IsValid())
        {
            ScheduleUpdate(attached, 0);
            ScheduleNeighborUpdates(attached);
        }
    }
}

void RedstoneSimulator::UpdateObserver(const BlockIterator& block)
{
    if (!block.IsValid()) return;
    
    Block* observer = block.GetBlock();
    if (!observer || observer->m_typeIndex != BLOCK_TYPE_REDSTONE_OBSERVER) return;
    
    if (observer->GetSpecialState())
    {
        observer->SetSpecialState(false);
        observer->SetRedstonePower(0);
        MarkChunkDirty(block);
        
        Direction outputDir = GetObserverOutputDirection(observer);
        BlockIterator output = block.GetNeighborCrossBoundary(outputDir);
        if (output.IsValid())
        {
            ScheduleUpdate(output, 0);
            ScheduleNeighborUpdates(output);
        }
    }
}

void RedstoneSimulator::UpdateWireConnections(const BlockIterator& block)
{
   if (!block.IsValid())
        return;
    
    Block* wire = block.GetBlock();
    if (!wire)
        return;
    
    // 检查是否是红石线类型
    uint8_t currentType = wire->m_typeIndex;
    if (!IsRedstoneWire(currentType))
        return;
    
    // 获取连接状态
    WireConnections conn = GetWireConnections(block);
    bool north = (conn.m_north != WireConnection::NONE);
    bool south = (conn.m_south != WireConnection::NONE);
    bool east  = (conn.m_east != WireConnection::NONE);
    bool west  = (conn.m_west != WireConnection::NONE);
    
    int connectionCount = (north ? 1 : 0) + (south ? 1 : 0) + 
                          (east ? 1 : 0) + (west ? 1 : 0);
    
    uint8_t newType = currentType;
    uint8_t newFacing = 0;
    
    if (connectionCount == 0)
    {
        // 孤立点
        newType = BLOCK_TYPE_REDSTONE_WIRE_DOT;
    }
    else if (connectionCount == 1)
    {
        // 单连接 - 显示为直线指向那个方向
        if (north || south)
        {
            newType = BLOCK_TYPE_REDSTONE_WIRE_NS;
        }
        else
        {
            newType = BLOCK_TYPE_REDSTONE_WIRE_EW;
        }
    }
    else if (connectionCount == 2)
    {
        if (north && south)
        {
            // 南北直线
            newType = BLOCK_TYPE_REDSTONE_WIRE_NS;
        }
        else if (east && west)
        {
            // 东西直线
            newType = BLOCK_TYPE_REDSTONE_WIRE_EW;
        }
        else
        {
            // L形角落
            newType = BLOCK_TYPE_REDSTONE_WIRE_CORNER;
            newFacing = GetCornerFacing(north, south, east, west);
        }
    }
    else if (connectionCount == 3)
    {
        // T形 - 用十字代替，或者用角落+facing
        newType = BLOCK_TYPE_REDSTONE_WIRE_CROSS;
    }
    else // connectionCount == 4
    {
        // 十字
        newType = BLOCK_TYPE_REDSTONE_WIRE_CROSS;
    }
    
    // 保留原有的信号强度
    uint8_t power = wire->GetRedstonePower();
    
    // 更新方块
    bool changed = (wire->m_typeIndex != newType) || 
                   (wire->GetBlockFacing() != newFacing);
    
    if (changed)
    {
        wire->SetType(newType);
        wire->SetBlockFacing(newFacing);
        wire->SetRedstonePower(power);  // 恢复信号强度
        MarkChunkDirty(block);
    }
}

void RedstoneSimulator::UpdateNeighborWireConnections(const BlockIterator& block)
{
    // 更新自己
    UpdateWireConnections(block);
    
    // 更新四个水平方向的红石线
    Direction horizontalDirs[] = { 
        DIRECTION_NORTH, DIRECTION_SOUTH, 
        DIRECTION_EAST, DIRECTION_WEST 
    };
    
    for (Direction dir : horizontalDirs)
    {
        BlockIterator neighbor = block.GetNeighborCrossBoundary(dir);
        if (neighbor.IsValid())
        {
            Block* nb = neighbor.GetBlock();
            if (nb && IsRedstoneWire(nb->m_typeIndex))
            {
                UpdateWireConnections(neighbor);
            }
        }
    }
    // 也检查上下方的邻居红石线
    // 上方四周
    BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
    if (above.IsValid())
    {
        for (Direction dir : horizontalDirs)
        {
            BlockIterator aboveSide = above.GetNeighborCrossBoundary(dir);
            if (aboveSide.IsValid())
            {
                Block* b = aboveSide.GetBlock();
                if (b && IsRedstoneWire(b->m_typeIndex))
                {
                    UpdateWireConnections(aboveSide);
                }
            }
        }
    }
    
    // 下方四周
    BlockIterator below = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
    if (below.IsValid())
    {
        for (Direction dir : horizontalDirs)
        {
            BlockIterator belowSide = below.GetNeighborCrossBoundary(dir);
            if (belowSide.IsValid())
            {
                Block* b = belowSide.GetBlock();
                if (b && IsRedstoneWire(b->m_typeIndex))
                {
                    UpdateWireConnections(belowSide);
                }
            }
        }
    }
}

void RedstoneSimulator::ToggleLever(const BlockIterator& block)
{
    if (!block.IsValid())
        return;
    Block* lever = block.GetBlock();
    if (!lever || lever->m_typeIndex != BLOCK_TYPE_REDSTONE_LEVER)
        return;
    
    //切换状态
    bool wasOn = lever->GetSpecialState();
    bool nowOn = !wasOn;
    
    lever->SetSpecialState(nowOn);
    lever->SetRedstonePower(nowOn ? 15 : 0);
    
    MarkChunkDirty(block);
    
    // 触发红石更新 
    // 拉杆自身
    ScheduleUpdate(block, 0);
    // 附着方块
    Direction attachDir = GetLeverAttachmentDirection(lever);
    BlockIterator attached = block.GetNeighborCrossBoundary(attachDir);
    if (attached.IsValid())
    {
        ScheduleUpdate(attached, 0);
        // 附着方块的邻居也需要更新
        ScheduleNeighborUpdates(attached);
    }
    // 拉杆的邻居
    ScheduleNeighborUpdates(block);
    
    // Block* lever = block.GetBlock();
    // if (!lever || lever->m_typeIndex != BLOCK_TYPE_LEVER)
    //     return;
    //
    // bool newState = !lever->GetSpecialState();
    // lever->SetSpecialState(newState);
    // lever->SetRedstonePower(newState ? 15 : 0);
    //
    // MarkChunkDirty(block);
    // ScheduleNeighborUpdates(block);
    //
    // // 也更新附着方块
    // BlockIterator attached = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
    // if (attached.IsValid())
    // {
    //     ScheduleNeighborUpdates(attached);
    // }
}

void RedstoneSimulator::PressButton(const BlockIterator& block)
{
    if (!block.IsValid())
        return;
    Block* button = block.GetBlock();
    if (!button)
        return;
    uint8_t buttonType = button->m_typeIndex;
    if (buttonType != BLOCK_TYPE_BUTTON_STONE)
        return;
    // 如果已经按下，忽略
    if (button->GetSpecialState())
        return;
    
    // 按下按钮
    button->SetSpecialState(true);
    button->SetRedstonePower(15);
    
    MarkChunkDirty(block);
    //触发红石更新
    ScheduleNeighborUpdates(block);
    // 附着方块也需要更新
    Direction attachDir = GetButtonAttachmentDirection(button);
    BlockIterator attached = block.GetNeighborCrossBoundary(attachDir);
    if (attached.IsValid())
    {
        ScheduleUpdate(attached, 0);
        ScheduleNeighborUpdates(attached);
    }
    // 计划自动弹起
    uint8_t delayTicks = (buttonType == BLOCK_TYPE_BUTTON_STONE) 
                         ? STONE_BUTTON_TICKS 
                         : WOOD_BUTTON_TICKS;
    
    ScheduleUpdate(block, delayTicks);
}

Direction RedstoneSimulator::GetButtonAttachmentDirection(Block* button) const
{
    // 与拉杆类似
    uint8_t facing = button->GetBlockFacing();
    switch (facing)
    {
    case 0: return DIRECTION_DOWN;  
    case 1: return DIRECTION_UP;    
    case 2: return DIRECTION_NORTH;
    case 3: return DIRECTION_SOUTH;
    case 4: return DIRECTION_EAST;
    case 5: return DIRECTION_WEST;
    default: return DIRECTION_DOWN;
    }
}

void RedstoneSimulator::CycleRepeaterDelay(const BlockIterator& block)
{
    if (!block.IsValid())
        return;
    Block* repeater = block.GetBlock();
    if (!repeater)
        return;
    
    if (repeater->m_typeIndex != BLOCK_TYPE_REPEATER_OFF &&
        repeater->m_typeIndex != BLOCK_TYPE_REPEATER_ON)
        return;
    
    // 延迟存储在低4位的低2位 (0-3 代表 1-4 tick)
    uint8_t currentDelay = GetRepeaterDelay(repeater);
    uint8_t newDelay = (currentDelay % 4) + 1;
    
    SetRepeaterDelay(repeater, newDelay);
    // uint8_t currentDelay = repeater->GetRedstonePower() & 0x03;
    // uint8_t newDelay = (currentDelay + 1) & 0x03;
    //
    // // 保留其他位，只改延迟
    // repeater->SetRedstonePower((repeater->GetRedstonePower() & 0xFC) | newDelay);
    
    MarkChunkDirty(block);
}

WireConnections RedstoneSimulator::GetWireConnections(const BlockIterator& block) const
{
    WireConnections conn;
    
    if (!block.IsValid())
        return conn;
    
    Block* wire = block.GetBlock();
    if (!wire || !IsRedstoneWire(wire->m_typeIndex))
        return conn;
    
    // 检查4个水平方向
    conn.m_north = GetWireConnectionInDirection(block, DIRECTION_NORTH);
    conn.m_south = GetWireConnectionInDirection(block, DIRECTION_SOUTH);
    conn.m_east = GetWireConnectionInDirection(block, DIRECTION_EAST);
    conn.m_west = GetWireConnectionInDirection(block, DIRECTION_WEST);
    
    return conn;
}

WireConnection RedstoneSimulator::GetWireConnectionInDirection(const BlockIterator& block, Direction dir) const
{
    // 检查同高度连接
    BlockIterator side = block.GetNeighborCrossBoundary(dir);
    if (side.IsValid())
    {
        Block* sideBlock = side.GetBlock();
        if (sideBlock)
        {
            uint8_t type = sideBlock->m_typeIndex;
            
            // 中继器特殊处理：只在输入/输出方向连接
            if (type == BLOCK_TYPE_REPEATER_OFF || type == BLOCK_TYPE_REPEATER_ON)
            {
                Direction repeaterFacing = (Direction)sideBlock->GetBlockFacing();
                // 红石线只连接到中继器的输入端（背面）或输出端（正面）
                if (repeaterFacing == dir || GetOppositeDir(repeaterFacing) == dir)
                {
                    return WireConnection::SIDE;
                }
                return WireConnection::NONE;  // 侧面不连接
            }
            
            // 其他方块
            if (CanConnectToRedstone(side, GetOppositeDir(dir)))
            {
                return WireConnection::SIDE;
            }
        }
    }
    
    // 检查向上连接（爬升）
    if (side.IsValid())
    {
        Block* sideBlock = side.GetBlock();
        // 侧边方块不透明时，检查上方
        if (sideBlock && sideBlock->IsOpaque())
        {
            BlockIterator aboveSide = side.GetNeighborCrossBoundary(DIRECTION_UP);
            if (aboveSide.IsValid())
            {
                Block* aboveSideBlock = aboveSide.GetBlock();
                if (aboveSideBlock && 
                    IsRedstoneWire(aboveSideBlock->m_typeIndex))
                {
                    // 检查我们上方是否有阻挡
                    BlockIterator above = block.GetNeighborCrossBoundary(DIRECTION_UP);
                    if (above.IsValid())
                    {
                        Block* aboveBlock = above.GetBlock();
                        if (!aboveBlock || !aboveBlock->IsOpaque())
                        {
                            return WireConnection::UP;
                        }
                    }
                }
            }
        }
    }
    
    //检查向下连接（下降） 
    BlockIterator below = block.GetNeighborCrossBoundary(DIRECTION_DOWN);
    if (below.IsValid())
    {
        BlockIterator belowSide = below.GetNeighborCrossBoundary(dir);
        if (belowSide.IsValid())
        {
            Block* belowSideBlock = belowSide.GetBlock();
            if (belowSideBlock && 
                IsRedstoneWire(belowSideBlock->m_typeIndex))
            {
                // 检查侧边方块是否阻挡
                if (side.IsValid())
                {
                    Block* sideBlock = side.GetBlock();
                    if (!sideBlock || !sideBlock->IsOpaque())
                    {
                        return WireConnection::DOWN;
                    }
                }
            }
        }
    }
    
    return WireConnection::NONE;
}

bool RedstoneSimulator::CanConnectToRedstone(const BlockIterator& block, Direction fromDir) const
{
    if (!block.IsValid())
        return false;
    
    Block* b = block.GetBlock();
    if (!b)
        return false;
    
    uint8_t blockType = b->m_typeIndex;
    
    if (IsRedstoneWire(blockType))
        return true;
    
    switch (blockType)
    {
    case BLOCK_TYPE_REDSTONE_BLOCK:
    case BLOCK_TYPE_REDSTONE_TORCH:
    case BLOCK_TYPE_REDSTONE_TORCH_OFF:
    case BLOCK_TYPE_REDSTONE_LEVER:
    case BLOCK_TYPE_BUTTON_STONE:
    //case BLOCK_TYPE_BUTTON_WOOD:
    case BLOCK_TYPE_REDSTONE_LAMP_OFF:
    case BLOCK_TYPE_REDSTONE_LAMP_ON:
    case BLOCK_TYPE_REDSTONE_PISTON:
    case BLOCK_TYPE_REDSTONE_STICKY_PISTON:
        return true;
    
        // 中继器：只在输入/输出方向连接
    case BLOCK_TYPE_REPEATER_OFF:
    case BLOCK_TYPE_REPEATER_ON:
        {
            Direction repeaterFacing = (Direction)b->GetBlockFacing();
            // fromDir 是"红石线到中继器"的方向
            // 中继器接收来自背面的输入，向正面输出
            return (fromDir == GetOppositeDir(repeaterFacing) ||  // 输入端（背面）
                    fromDir == repeaterFacing);                     // 输出端（正面）
        }
    
        //侦测器：只在输出方向连接
    case BLOCK_TYPE_REDSTONE_OBSERVER:
        {
            Direction observerFacing = GetObserverFacing(b);
            Direction outputDir = GetOppositeDir(observerFacing);
            return (fromDir == outputDir);  // 只在输出端连接
        }
    
    default:
        return false;
    }
}

void RedstoneSimulator::GetPerpendicularDirectionsForLeftAndRight(Direction facing, Direction& leftDir,
    Direction& rightDir) const
{
    switch (facing)
    {
    case DIRECTION_NORTH:  // +Y
        leftDir = DIRECTION_WEST;   // -X
        rightDir = DIRECTION_EAST;  // +X
        break;
    case DIRECTION_SOUTH:  // -Y
        leftDir = DIRECTION_EAST;   // +X
        rightDir = DIRECTION_WEST;  // -X
        break;
    case DIRECTION_EAST:   // +X
        leftDir = DIRECTION_NORTH;  // +Y
        rightDir = DIRECTION_SOUTH; // -Y
        break;
    case DIRECTION_WEST:   // -X
        leftDir = DIRECTION_SOUTH;  // -Y
        rightDir = DIRECTION_NORTH; // +Y
        break;
    default:
        leftDir = DIRECTION_NORTH;
        rightDir = DIRECTION_SOUTH;
        break;
    }
}

void RedstoneSimulator::OnBlockPlaced(const BlockIterator& block)
{
    if (!block.IsValid())
        return;
    Block* b = block.GetBlock();
    if (!b)
        return;

    NotifyObserversOfChange(block);
    
    if (b->m_typeIndex == BLOCK_TYPE_REDSTONE_BLOCK)
    {
        // 红石块放置时，更新周围所有方块
        ScheduleNeighborUpdates(block);
        // 也更新斜对角的红石线（爬升连接）
        ScheduleClimbingWireUpdates(block);
    }
    // 如果放置的是红石线，更新自己和邻居的连接
        if (IsRedstoneWire(b->m_typeIndex))
        {
            UpdateNeighborWireConnections(block);
        }
        else
        {
            // 放置其他方块也可能影响红石线连接
            // 更新周围的红石线
            UpdateNeighborWireConnections(block);
        }
    
    ScheduleUpdate(block, 0);
    ScheduleNeighborUpdates(block);
}

void RedstoneSimulator::OnBlockRemoved(const BlockIterator& block, uint8_t oldType)
{
    NotifyObserversOfChange(block);
    if (oldType == BLOCK_TYPE_REDSTONE_BLOCK)
    {
        // 红石块移除时，更新周围所有方块
        ScheduleNeighborUpdates(block);
        ScheduleClimbingWireUpdates(block);
    }
    UpdateNeighborWireConnections(block);
    ScheduleNeighborUpdates(block);
}

void RedstoneSimulator::OnBlockStateChanged(const BlockIterator& block, uint8_t oldType, uint8_t newType)
{
    NotifyObserversOfChange(block);
}

// bool RedstoneSimulator::IsRedstoneBlockPowering(const BlockIterator& block, Direction toDir) const
// {
//     if (!block.IsValid())
//         return false;
//     
//     Block* b = block.GetBlock();
//     if (!b || b->m_typeIndex != BLOCK_TYPE_REDSTONE_BLOCK)
//         return false;
//     
//     // 红石块向所有6个方向提供信号
//     return true;
// }

int64_t RedstoneSimulator::PosToKey(const IntVec3& pos) const
{
    return ((int64_t)(pos.x + 30000000) << 40) | 
           ((int64_t)(pos.y + 30000000) << 20) | 
           (int64_t)(pos.z + 30000000);
}

Direction RedstoneSimulator::GetOppositeDir(Direction dir) const
{
    switch (dir)
    {
    case DIRECTION_NORTH: return DIRECTION_SOUTH;
    case DIRECTION_SOUTH: return DIRECTION_NORTH;
    case DIRECTION_EAST:  return DIRECTION_WEST;
    case DIRECTION_WEST:  return DIRECTION_EAST;
    case DIRECTION_UP:    return DIRECTION_DOWN;
    case DIRECTION_DOWN:  return DIRECTION_UP;
    default: return DIRECTION_NORTH;
    }
}

void RedstoneSimulator::MarkChunkDirty(const BlockIterator& block)
{
    Chunk* chunk = block.GetChunk();
    if (chunk)
    {
        chunk->MarkSelfDirty();
        m_world->m_hasDirtyChunk = true;
    }
}

bool RedstoneSimulator::IsBlockProvidingPowerToDirection(
    const BlockIterator& source, 
    Direction toDir) const 
{
    if (!source.IsValid())
        return false;
    
    Block* b = source.GetBlock();
    if (!b)
        return false;
    
    uint8_t type = b->m_typeIndex;
    
    switch (type)
    {
    case BLOCK_TYPE_REDSTONE_BLOCK:
        return true;
        // 2. 红石火把：向除附着方向外的所有方向输出15
    case BLOCK_TYPE_REDSTONE_TORCH:
        {
            Direction attachDir = GetTorchAttachmentDirection(b);
            return toDir != attachDir;
        }
        // 3. 拉杆：开启时向所有方向输出15
    case BLOCK_TYPE_REDSTONE_LEVER:
        return b->GetSpecialState();
        
        // 4. 按钮：按下时向所有方向输出15
    case BLOCK_TYPE_BUTTON_STONE:
        return b->GetSpecialState();
        // 5. 中继器：只向前方输出15
    case BLOCK_TYPE_REPEATER_ON:
        {
            Direction facing = (Direction)b->GetBlockFacing();
            return GetOppositeDir(toDir) == facing;
            //return toDir == facing;
        }
        // 6. 侦测器：只向背面输出15
    case BLOCK_TYPE_REDSTONE_OBSERVER:
        {
            if (!b->GetSpecialState())
                return false;  
            Direction facing = GetObserverFacing(b);
            Direction outputDir = GetOppositeDir(facing);
            return toDir == outputDir;
        }
        
    default:
        return false;
    }
}