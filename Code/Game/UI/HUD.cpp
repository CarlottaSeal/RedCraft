// HUDScreen.cpp
#include "HUD.h"
#include "Engine/UI/Canvas.hpp"
#include "Engine/UI/Panel.h"
#include "Engine/UI/ProgressBar.h"
#include "Engine/UI/Sprite.h"
#include "Engine/UI/Text.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"

HUD::HUD(UISystem* uiSystem)
    : UIScreen(uiSystem, UIScreenType::HUD, false)  // HUD 不阻挡输入
{
}

HUD::~HUD()
{
}

void HUD::Build()
{
    if (!m_canvas || !m_camera)
    {
        return;
    }
    // 设置相机为全屏UI视图
    m_camera->SetOrthographicView(Vec2(0, 0), Vec2(1600, 900));
    
    BuildCrosshair();
    // BuildHealthBar();
    // BuildArmorBar(); 
    // BuildHungerBar();
    // BuildExperienceBar();
    BuildHotbar();
    BuildActionText();
}

void HUD::BuildCrosshair()
{
    // 十字准星在屏幕中心
    Vec2 center(800, 450);
    float size = 16.0f;
    
    AABB2 crosshairBounds(
        center.x - size * 0.5f,
        center.y - size * 0.5f,
        center.x + size * 0.5f,
        center.y + size * 0.5f
    );
    
    // 使用 Sprite 或 Panel 创建十字准星
    m_crosshair = new Sprite(m_canvas, crosshairBounds, nullptr);
    m_crosshair->SetColor(Rgba8::WHITE);
    m_elements.push_back(m_crosshair);
}

// void HUD::BuildHealthBar()
// {
//     // 生命值条在左下角，血条样式
//     AABB2 healthBounds(20, 40, 220, 60);
//     
//     m_healthBar = new ProgressBar(
//         m_canvas,
//         healthBounds,
//         0.0f,      // minValue
//         100.0f,    // maxValue
//         Rgba8(50, 50, 50, 180),    // 背景色（半透明深灰）
//         Rgba8(220, 20, 20),         // 填充色（红色）
//         Rgba8::WHITE,               // 边框色
//         true                        // 有边框
//     );
//     
//     m_healthBar->SetValue(100.0f);  // 初始满血
//     m_elements.push_back(m_healthBar);
// }
//
// void HUD::BuildHungerBar()
// {
//     // 饥饿值条在生命值右侧
//     AABB2 hungerBounds(240, 40, 440, 60);
//     
//     m_hungerBar = new ProgressBar(
//         m_canvas,
//         hungerBounds,
//         0.0f,
//         100.0f,
//         Rgba8(50, 50, 50, 180),
//         Rgba8(205, 133, 63),  // 棕褐色（食物色）
//         Rgba8::WHITE,
//         true
//     );
//     
//     m_hungerBar->SetValue(100.0f);
//     m_elements.push_back(m_hungerBar);
// }
//
// void HUD::BuildArmorBar()
// {
//     AABB2 armorBounds(20, 70, 220, 90);
//     
//     m_armorBar = new ProgressBar(
//         m_canvas,
//         armorBounds,
//         0.0f,      // minValue
//         100.0f,    // maxValue
//         Rgba8(50, 50, 50, 180),      // 背景色（半透明深灰）
//         Rgba8(180, 180, 180),        // 填充色（银灰色，代表护甲）
//         Rgba8::WHITE,                // 边框色
//         true                         // 有边框
//     );
//     
//     m_armorBar->SetValue(0.0f);  // 初始无护甲
//     m_elements.push_back(m_armorBar);
//     
//     // 可选：添加护甲图标
//     // TextSetting iconSetting;
//     // iconSetting.m_text = "🛡";  // 盾牌emoji，或者用纹理
//     // iconSetting.m_color = Rgba8(200, 200, 200);
//     // iconSetting.m_height = 16.0f;
//     // Vec2 iconPos(5, 75);
//     // Text* armorIcon = new Text(m_canvas, iconPos, iconSetting);
//     // m_elements.push_back(armorIcon);
// }
//
// void HUD::BuildExperienceBar()
// {
//     // 经验值条在屏幕底部中央，快捷栏上方
//     AABB2 expBounds(600, 100, 1000, 110);
//     
//     m_experienceBar = new ProgressBar(
//         m_canvas,
//         expBounds,
//         0.0f,
//         100.0f,
//         Rgba8(0, 0, 0, 100),
//         Rgba8(127, 255, 0),  // 绿色经验条
//         Rgba8(0, 200, 0),
//         true
//     );
//     
//     m_experienceBar->SetValue(0.0f);
//     m_elements.push_back(m_experienceBar);
// }

void HUD::BuildHotbar()
{
    // 快捷栏背景面板（屏幕底部中央）
    float hotbarWidth = 720.0f;
    float hotbarHeight = 80.0f;
    float hotbarX = 800 - hotbarWidth * 0.5f;  // 居中
    float hotbarY = 10.0f;
    
    AABB2 hotbarBounds(
        hotbarX,
        hotbarY,
        hotbarX + hotbarWidth,
        hotbarY + hotbarHeight
    );
    
    m_hotbarPanel = new Panel(
        m_canvas,
        hotbarBounds,
        Rgba8(0, 0, 0, 150),  // 半透明黑色背景
        nullptr,
        true,                 // 有边框
        Rgba8(100, 100, 100)
    );
    m_elements.push_back(m_hotbarPanel);
    
    // 创建 9 个物品槽
    float slotSize = 70.0f;
    float slotPadding = 5.0f;
    float startX = hotbarX + 5.0f;
    float startY = hotbarY + 5.0f;
    
    for (int i = 0; i < HOTBAR_SLOT_COUNT; i++)
    {
        float slotX = startX + i * (slotSize + slotPadding);
        AABB2 slotBounds(
            slotX,
            startY,
            slotX + slotSize,
            startY + slotSize
        );
        
        // 物品槽背景
        Panel* slot = new Panel(
            m_canvas,
            slotBounds,
            Rgba8(55, 55, 55),
            nullptr,
            true,
            Rgba8(139, 139, 139)
        );
        m_hotbarSlots.push_back(slot);
        m_elements.push_back(slot);
        
        // 物品图标（默认为空）
        Sprite* icon = new Sprite(m_canvas, slotBounds, nullptr);
        m_hotbarIcons.push_back(icon);
        m_elements.push_back(icon);
    }
    
    // 选中框（高亮边框）
    AABB2 selectionBounds = m_hotbarSlots[0]->GetBounds();
    m_hotbarSelectionFrame = new Panel(
        m_canvas,
        selectionBounds,
        Rgba8(255, 255, 255, 0),  // 透明背景
        nullptr,
        true,
        Rgba8::WHITE  // 白色边框
    );
    m_elements.push_back(m_hotbarSelectionFrame);
}

void HUD::BuildActionText()
{
    // 屏幕中心偏下显示动作提示文本
    TextSetting setting;
    setting.m_text = "";
    setting.m_color = Rgba8::WHITE;
    setting.m_height = 30.0f;
    
    Vec2 textPos(800, 350);  // 屏幕中心偏下
    
    m_actionText = new Text(m_canvas, textPos, setting);
    m_actionText->SetEnabled(false);  // 默认隐藏
    m_elements.push_back(m_actionText);
}

void HUD::Update(float deltaSeconds)
{
    UIScreen::Update(deltaSeconds);

    if (m_actionTextTimer > 0.0f)
    {
        m_actionTextTimer -= deltaSeconds;
        
        if (m_actionTextTimer <= 0.0f)
        {
            if (m_actionText)
            {
                m_actionText->SetEnabled(false);
            }
        }
        else
        {
            float alpha = 255.0f;
            if (m_actionTextTimer < 0.5f)
            {
                alpha = m_actionTextTimer / 0.5f * 255.0f;
            }
            
            if (m_actionText)
            {
                Rgba8 color = m_actionText->GetColor();
                color.a = (unsigned char)alpha;
                m_actionText->SetColor(color);
            }
        }
    }
    
    // 处理快捷栏数字键输入
    HandleHotbarInput();
}

void HUD::HandleHotbarInput()
{
    if (!m_canvas)
    {
        return;
    }
    
    InputSystem* input = m_canvas->GetSystemInputSystem();
    if (!input)
    {
        return;
    }

    for (int i = 0; i < HOTBAR_SLOT_COUNT; i++)
    {
        unsigned char key = (unsigned char)('1' + i);
        if (input->WasKeyJustPressed(key))
        {
            SelectHotbarSlot(i);
        }
    }
    
    // 鼠标滚轮切换
    float wheelDelta = input->GetMouseWheelDelta();
    if (wheelDelta != 0.0f)
    {
        int newSlot = m_selectedHotbarSlot;
        
        if (wheelDelta > 0)
        {
            newSlot = (m_selectedHotbarSlot + 1) % HOTBAR_SLOT_COUNT;
        }
        else
        {
            newSlot = (m_selectedHotbarSlot - 1 + HOTBAR_SLOT_COUNT) % HOTBAR_SLOT_COUNT;
        }
        
        SelectHotbarSlot(newSlot);
    }
}

// void HUD::UpdateHealth(float healthPercent)
// {
//     if (m_healthBar)
//     {
//         m_healthBar->SetValueNormalized(healthPercent);
//     }
// }
//
// void HUD::UpdateHunger(float hungerPercent)
// {
//     if (m_hungerBar)
//     {
//         m_hungerBar->SetValueNormalized(hungerPercent);
//     }
// }
//
// void HUD::UpdateArmor(float armorPercent)
// {
//     if (m_armorBar)
//     {
//         m_armorBar->SetValueNormalized(armorPercent);
//         
//         // 可选：没有护甲时隐藏护甲条
//         if (armorPercent <= 0.0f)
//         {
//             m_armorBar->SetEnabled(false);
//         }
//         else
//         {
//             m_armorBar->SetEnabled(true);
//         }
//     }
// }
//
// void HUD::UpdateExperience(float expPercent)
// {
//     if (m_experienceBar)
//     {
//         m_experienceBar->SetValueNormalized(expPercent);
//     }
// }

void HUD::SetHotbarSlot(int slotIndex, Texture* itemTexture)
{
    if (slotIndex < 0 || slotIndex >= HOTBAR_SLOT_COUNT)
    {
        return;
    }
    
    if (m_hotbarIcons[slotIndex])
    {
        m_hotbarIcons[slotIndex]->SetTexture(itemTexture);
    }
}

void HUD::SelectHotbarSlot(int slotIndex)
{
    if (slotIndex < 0 || slotIndex >= HOTBAR_SLOT_COUNT)
    {
        return;
    }
    
    m_selectedHotbarSlot = slotIndex;

    if (m_hotbarSelectionFrame && slotIndex < (int)m_hotbarSlots.size())
    {
        AABB2 newBounds = m_hotbarSlots[slotIndex]->GetBounds();
        m_hotbarSelectionFrame->SetBounds(newBounds);
    }
}

void HUD::ShowActionMessage(std::string const& message, float duration)
{
    if (m_actionText)
    {
        m_actionText->SetText(message);
        m_actionText->SetEnabled(true);
        m_actionTextTimer = duration;
        m_actionTextDuration = duration;
    }
}