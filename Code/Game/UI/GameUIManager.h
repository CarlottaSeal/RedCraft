#pragma once
#include "ItemSlot.h"
#include "Engine/UI/UIManager.h"
#include "Engine/UI/UISystem.h"

class IconAtlas;
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
class WorldSelectScreen;

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
    
    void OpenWorldSelect();
    void CloseWorldSelect();
    bool IsWorldSelectOpen() const;
    
    void OpenFarmMonitor();
    void CloseFarmMonitor();
    bool IsFarmMonitorOpen() const;
    
    void OpenRedstoneConfig(const IntVec3& blockPos);
    void CloseRedstoneConfig();
    bool IsRedstoneConfigOpen() const;
    
    void CloseTopScreen();
    void CloseAllScreens();
    void CleanupTempScreens();

    bool IsAnyMenuOpen() const;
    bool IsGameInputBlocked() const;
    UIScreenType GetCurrentScreenType() const;
    
    // Hotbar/Inventory 操作
    void SetHotbarItem(int slotIndex, Texture* itemTexture);
    void ShowActionMessage(std::string const& message, float duration = 2.0f);
    void SetInventoryItem(int slotIndex, ItemData const& item);
    ItemData GetInventoryItem(int slotIndex) const;
    bool AddItemToInventory(ItemData const& item);
    void SetFarmMonitorArea(const IntVec3& minCorner, const IntVec3& maxCorner);
    
    void HandleUIInput();
private:
    void UpdateHUDVisibility();
    bool ShouldHideHUD(UIScreenType screenType) const;
    
public:
    IconAtlas* m_uiAtlas = nullptr;
    UIManager* m_uiManager = nullptr;
    
private:
    UISystem* m_uiSystem = nullptr;
    
    HUD* m_hudScreen = nullptr; 
    
    InventoryScreen* m_inventoryScreen = nullptr;
    PauseMenuScreen* m_pauseMenuScreen = nullptr;
    ChestScreen* m_chestScreen = nullptr;
    CraftingScreen* m_craftingScreen = nullptr;
    FurnaceScreen* m_furnaceScreen = nullptr;
    SettingsScreen* m_settingsScreen = nullptr;
    MainMenuScreen* m_mainMenuScreen = nullptr;
    WorldSelectScreen* m_worldSelectScreen = nullptr;
    FarmMonitorScreen* m_farmMonitorScreen = nullptr;
    RedstoneConfigScreen* m_redstoneConfigScreen = nullptr;
    
    void CreateAllScreens(); 
    void BuildAllScreens();  
    void CreateHUD();
};

extern GameUIManager* g_theGameUIManager;