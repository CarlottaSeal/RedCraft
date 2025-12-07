#pragma once
#include "Engine/UI/UIScreen.h"

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
    void HandleHotbarInput();

    // void UpdateHealth(float healthPercent);
    // void UpdateHunger(float hungerPercent);
    // void UpdateArmor(float armorPercent);
    // void UpdateExperience(float expPercent);
    void SetHotbarSlot(int slotIndex, Texture* itemTexture = nullptr);
    void SelectHotbarSlot(int slotIndex);
    void ShowActionMessage(std::string const& message, float duration = 2.0f);

private:    
    void BuildCrosshair();
    // void BuildHealthBar();
    // void BuildHungerBar();
    // void BuildArmorBar();
    // void BuildExperienceBar();
    void BuildHotbar();
    void BuildActionText();
    
private:
    Sprite* m_crosshair = nullptr;
    // ProgressBar* m_healthBar = nullptr;
    // ProgressBar* m_hungerBar = nullptr;
    // ProgressBar* m_armorBar = nullptr;  
    // ProgressBar* m_experienceBar = nullptr;
    
    Panel* m_hotbarPanel = nullptr;
    std::vector<Panel*> m_hotbarSlots;
    std::vector<Sprite*> m_hotbarIcons;
    Panel* m_hotbarSelectionFrame = nullptr;
    
    Text* m_actionText = nullptr;
    float m_actionTextTimer = 0.0f;
    float m_actionTextDuration = 0.0f;
    
    int m_selectedHotbarSlot = 0;
    static const int HOTBAR_SLOT_COUNT = 9;
};