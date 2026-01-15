#pragma once
#include "Engine/UI/UIScreen.h"

class Player;
class Panel;
class ProgressBar;
class Sprite;
class Text;

class HUD : public UIScreen
{
public:
    HUD(UISystem* uiSystem);
    virtual ~HUD() override;
    
    virtual void Build() override;
    virtual void Update(float deltaSeconds) override;
    virtual void Render() const override;
    
    // void UpdateHealth(float healthPercent);
    // void UpdateHunger(float hungerPercent);
    // void UpdateArmor(float armorPercent);
    // void UpdateExperience(float expPercent);
    void SetHotbarSlot(int slotIndex, Texture* itemTexture = nullptr);
    void ShowActionMessage(std::string const& message, float duration = 2.0f);

    void RefreshHotbarDisplay();
    void UpdateSelectionFrame();

private:    
    void BuildCrosshair();
    // void BuildHealthBar();
    // void BuildHungerBar();
    // void BuildArmorBar();
    // void BuildExperienceBar();
    void BuildHotbar();
    void BuildActionText();
    void BuildItemTooltip();
    
private:
    Sprite* m_crosshair = nullptr;
    // ProgressBar* m_healthBar = nullptr;
    // ProgressBar* m_hungerBar = nullptr;
    // ProgressBar* m_armorBar = nullptr;  
    // ProgressBar* m_experienceBar = nullptr;
    
    Panel* m_hotbarPanel = nullptr;
    std::vector<Panel*> m_hotbarSlots;
    std::vector<Sprite*> m_hotbarIcons;
    std::vector<Text*> m_hotbarCounts;
    Panel* m_hotbarSelectionFrame = nullptr;
    
    Text* m_actionText = nullptr;
    float m_actionTextTimer = 0.0f;
    float m_actionTextDuration = 0.0f;

    Text* m_itemTooltip = nullptr;
    float m_tooltipTimer = 0.0f;
};