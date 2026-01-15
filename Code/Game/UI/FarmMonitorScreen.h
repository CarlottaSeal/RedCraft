#pragma once
#include "Engine/UI/UIScreen.h"
#include "ItemSlot.h"
#include <vector>

#include "Engine/Math/IntVec3.h"
#include "Game/Gamecommon.hpp"

//显示监控区域内所有作物的生长状态
//实时进度条显示生长进度
//侦测器状态监控（触发/空闲）
//统计信息（总作物数、成熟作物、平均生长进度）
//一键收获所有成熟作物

enum Direction : uint8_t;
class Panel;
class Text;
class ProgressBar;
class Button;
class Sprite;
class World;
class CropSystem;
struct BlockIterator;

struct CropMonitorData
{
    IntVec3 m_worldPos;
    uint8_t m_blockType = 0;
    int m_currentStage = 0;
    int m_maxStage = 0;
    float m_growthProgress = 0.0f;  // 0.0 - 1.0
    bool m_isMature = false;
    std::string m_cropName;
};

struct ObserverMonitorData
{
    IntVec3 m_worldPos;
    Direction m_facing = DIRECTION_NORTH;
    bool m_isTriggered = false;
    float m_lastTriggerTime = 0.0f;
};

class FarmMonitorScreen : public UIScreen
{
public:
    FarmMonitorScreen(UISystem* uiSystem, World* world);
    virtual ~FarmMonitorScreen() override;
    
    virtual void Build() override;
    virtual void Update(float deltaSeconds) override;
    virtual void OnEnter() override;
    virtual void OnExit() override;
    
    void SetMonitorArea(const IntVec3& minCorner, const IntVec3& maxCorner);
    
    void RefreshData();
    
private:
    static const int MAX_DISPLAYED_CROPS = 12;
    static const int MAX_DISPLAYED_OBSERVERS = 4;
    static const int CROPS_PER_ROW = 4;
    
    World* m_world = nullptr;
    
    IntVec3 m_monitorMin;
    IntVec3 m_monitorMax;
    bool m_hasMonitorArea = false;
    
    Panel* m_backgroundDimmer = nullptr;
    Panel* m_mainPanel = nullptr;
    Text* m_titleText = nullptr;
    
    Panel* m_cropsPanel = nullptr;
    Text* m_cropsSectionTitle = nullptr;
    std::vector<Panel*> m_cropSlotPanels;
    std::vector<Sprite*> m_cropIcons;
    std::vector<ProgressBar*> m_cropProgressBars;
    std::vector<Text*> m_cropNameTexts;
    std::vector<Text*> m_cropStageTexts;
    
    Panel* m_observersPanel = nullptr;
    Text* m_observersSectionTitle = nullptr;
    std::vector<Panel*> m_observerSlotPanels;
    std::vector<Panel*> m_observerIndicators;  
    std::vector<Text*> m_observerPosTexts;
    std::vector<Text*> m_observerStatusTexts;
    
    Panel* m_statsPanel = nullptr;
    Text* m_totalCropsText = nullptr;
    Text* m_matureCropsText = nullptr;
    Text* m_observerCountText = nullptr;
    Text* m_avgGrowthText = nullptr;
    Text* m_dropsCountText = nullptr;
    Text* m_dropsItemsText = nullptr;
    
    Button* m_refreshButton = nullptr;
    Button* m_harvestAllButton = nullptr;
    Button* m_closeButton = nullptr;
    
    std::vector<CropMonitorData> m_cropData;
    std::vector<ObserverMonitorData> m_observerData;
    
    float m_refreshTimer = 0.0f;
    static constexpr float AUTO_REFRESH_INTERVAL = 1.0f;
    
    void BuildBackground();
    void BuildCropsSection();
    void BuildObserversSection();
    void BuildStatsSection();
    void BuildButtons();
    
    void CollectCropData();
    void CollectObserverData();
    
    void UpdateCropDisplays();
    void UpdateObserverDisplays();
    void UpdateStatsDisplay();
    
    void OnRefreshClicked();
    void OnHarvestAllClicked();
    void OnCloseClicked();
    
    Rgba8 GetGrowthProgressColor(float progress) const;
    std::string GetCropDisplayName(uint8_t blockType) const;
    Texture* GetCropTexture(uint8_t blockType) const;
};