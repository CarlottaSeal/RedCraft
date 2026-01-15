#pragma once
#include "Engine/UI/UIElement.h"
#include "Engine/UI/UIEvent.h"
#include "Engine/Math/AABB2.hpp"

class Canvas;
class Panel;
class Sprite;
class Text;
class Texture;

struct ItemData //TODO
{
    Texture* texture = nullptr;
    std::string name = "";
    std::string displayName = "";  // 显示名称
    int count = 0;
    int maxStack = 64;
    int itemID = -1;  // 物品ID，用于判断是否可堆叠
    
    bool IsEmpty() const { return texture == nullptr || count <= 0; }
    bool CanStackWith(ItemData const& other) const 
    { 
        return itemID == other.itemID && itemID != -1 && count < maxStack; 
    }
};

class ItemSlot : public UIElement
{
public:
    ItemSlot(Canvas* canvas, AABB2 const& bounds, int slotIndex);
    virtual ~ItemSlot() override;
    
    virtual void StartUp() override;
    virtual void ShutDown() override;
    virtual void Update() override;
    virtual void Render() const override;

    void SetItem(ItemData const& item);
    void ClearItem();
    ItemData const& GetItem() const { return m_itemData; }
    ItemData TakeItem();  // 拿走物品（返回并清空）
    int AddItem(ItemData const& item);  // 添加物品（返回未能添加的数量）
    
    // 视觉状态
    void SetHighlighted(bool highlighted);
    void SetSelected(bool selected);
    void SetDisabled(bool disabled);

    size_t OnClickEvent(UICallbackFunctionPointer callback);
    size_t OnRightClickEvent(UICallbackFunctionPointer callback);
    size_t OnHoverEnterEvent(UICallbackFunctionPointer callback);
    size_t OnHoverExitEvent(UICallbackFunctionPointer callback);
    
    int GetSlotIndex() const { return m_slotIndex; }
    bool IsEmpty() const { return m_itemData.IsEmpty(); }
    
private:
    Canvas* m_canvas = nullptr;
    int m_slotIndex = -1;
    

    Panel* m_background = nullptr;
    Sprite* m_itemIcon = nullptr;
    Text* m_countText = nullptr;
    Panel* m_highlightFrame = nullptr;
    Panel* m_selectionFrame = nullptr;

    ItemData m_itemData;

    bool m_isHighlighted = false;
    bool m_isSelected = false;
    bool m_isDisabled = false;
    //bool m_isHovered = false;

    UIEvent m_onClickEvent;
    UIEvent m_onRightClickEvent;
    UIEvent m_onHoverEnterEvent;
    UIEvent m_onHoverExitEvent;

    void HandleInput();
    void UpdateVisuals();
};