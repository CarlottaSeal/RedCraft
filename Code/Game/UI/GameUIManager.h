#pragma once
#include "Engine/UI/UIManager.h"
#include "Engine/UI/UISystem.h"

struct ItemData;
class HUD;
class InventoryScreen;
class PauseMenuScreen;
class ChestScreen;
class CraftingScreen;
class FurnaceScreen;
class MainMenuScreen;
class SettingsScreen;
class FarmMonitorScreen;
class RedstoneConfigScreen;

class GameUIManager
{
public:
    GameUIManager(UISystem* uiSystem);
    ~GameUIManager();
    
    void Startup();
    void Shutdown();
    void Update(float deltaSeconds);
    void Render() const;
    

    void ShowHUD();
    void HideHUD();
    HUD* GetHUD() const { return m_hudScreen; }
    

    void OpenInventory();
    void CloseInventory();
    bool IsInventoryOpen() const;
    

    void OpenPauseMenu();
    void ClosePauseMenu();
    void TogglePauseMenu();
    bool IsPauseMenuOpen() const;
    

    void OpenChest(int chestID); 
    void CloseChest();
    bool IsChestOpen() const;
    

    void OpenCraftingTable();
    void CloseCraftingTable();
    bool IsCraftingTableOpen() const;
    

    void OpenFurnace(int furnaceID);
    void CloseFurnace();
    bool IsFurnaceOpen() const;
    

    void OpenSettings();
    void CloseSettings();
    bool IsSettingsOpen() const;
    

    void OpenMainMenu();
    void CloseMainMenu();
    bool IsMainMenuOpen() const;
    
    void CloseTopScreen();
    void CloseAllScreens();
    
    bool IsAnyMenuOpen() const; 
    bool IsGameInputBlocked() const; 
    UIScreenType GetCurrentScreenType() const;
    
    void UpdateHealth(float healthPercent);
    void UpdateHunger(float hungerPercent);
    void UpdateExperience(float expPercent);
    void UpdateArmor(float armorPercent);
    void SelectHotbarSlot(int slotIndex);
    void SetHotbarItem(int slotIndex, Texture* itemTexture);
    void ShowActionMessage(std::string const& message, float duration = 2.0f);
    
    void SetInventoryItem(int slotIndex, ItemData const& item);
    ItemData GetInventoryItem(int slotIndex) const;
    bool AddItemToInventory(ItemData const& item);
    
    void OpenFarmMonitor();
    void CloseFarmMonitor();
    bool IsFarmMonitorOpen() const;
    void SetFarmMonitorArea(const IntVec3& minCorner, const IntVec3& maxCorner);

    void OpenRedstoneConfig(const IntVec3& blockPos);
    void CloseRedstoneConfig();
    bool IsRedstoneConfigOpen() const;
    
    void HandleUIInput();  
    
private:
    UISystem* m_uiSystem = nullptr;
    UIManager* m_uiManager = nullptr;
    
    HUD* m_hudScreen = nullptr;
    //以下是临时的 随时销毁
    InventoryScreen* m_inventoryScreen = nullptr;
    PauseMenuScreen* m_pauseMenuScreen = nullptr;
    ChestScreen* m_chestScreen = nullptr;
    CraftingScreen* m_craftingScreen = nullptr;
    FurnaceScreen* m_furnaceScreen = nullptr;
    SettingsScreen* m_settingsScreen = nullptr;
    MainMenuScreen* m_mainMenuScreen = nullptr;
    FarmMonitorScreen* m_farmMonitorScreen = nullptr;
    RedstoneConfigScreen* m_redstoneConfigScreen = nullptr;
    
    void CreateHUD();
    void CleanupTempScreens();
    
    template<typename T>
    T* FindScreen(UIScreenType type) const;
};

extern GameUIManager* g_theGameUIManager;