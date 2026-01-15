// PlayerInventory.h
#pragma once
#include "Item.h"
#include <string>

#include "Game/Gamecommon.hpp"

class PlayerInventory
{
public:
    PlayerInventory();
    ~PlayerInventory();
    
    void SetHotbarSlot(int index, std::string const& itemName);
    void SetSelectedHotbarSlot(int slot);
    int GetSelectedHotbarSlot() const { return m_selectedHotbarSlot; }
    
    Item const& GetHotbarSlot(int index) const;
    Item const& GetSelectedItem() const;
    
    Item const& GetMainInventorySlot(int index) const;
    
    bool HasItem(std::string const& itemName, int minCount = 1) const;
    int GetItemCount(std::string const& itemName) const;
    
    void InitializeDefaults();  
    void Clear();
    void SwitchHotbarRow();

    int GetActiveRow() const;
    int GetSelectedSlotInRow() const; 

private:
    Item m_hotbar[TOTAL_EQUIPMENT_SLOTS];
    int m_activeRow = 0;   
    Item m_mainInventory[27];      // 主背包（0-26）
    
    int m_selectedHotbarSlot = 0;  // 当前选中快捷栏槽位
    
    // 辅助函数
    int FindSlotWithItem(std::string const& itemName) const;
    int FindEmptySlot() const;
};
