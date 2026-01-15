#include "GameUIManager.h"

#include "HUD.h"
#include "InventoryScreen.h"
#include "PauseMenuScreen.h"
#include "ChestScreen.h"
#include "CraftingScreen.h"
#include "FarmMonitorScreen.h"
#include "FurnaceScreen.h"
#include "IconAtlas.h"
#include "MainMenuScreen.h"
#include "RedstoneConfigScreen.h"
#include "SettingsScreen.h"
#include "WorldSelectScreen.h"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Game/Game.hpp"

class World;
GameUIManager* g_theGameUIManager = nullptr;
extern Game* g_theGame;

GameUIManager::GameUIManager(UISystem* uiSystem)
    : m_uiSystem(uiSystem)
{
    m_uiManager = uiSystem->m_theUIManager;

    m_uiAtlas = new IconAtlas(g_theRenderer, "Data/Images/CustomItem.png",
        IntVec2(8,8), 128);
    
    //Startup();
}

GameUIManager::~GameUIManager()
{
    Shutdown();
}

void GameUIManager::Startup()
{
    if (!m_uiSystem || !m_uiManager)
    {
        ERROR_AND_DIE("GameUIManager: UISystem or UIManager is null!");
    }
    CreateAllScreens();
    BuildAllScreens();
    CreateHUD();
}

void GameUIManager::Shutdown()
{
    if (m_hudScreen)
    {
        delete m_hudScreen;
        m_hudScreen = nullptr;
    }
    if (m_inventoryScreen)
    {
        delete m_inventoryScreen;
        m_inventoryScreen = nullptr;
    }
    if (m_pauseMenuScreen)
    {
        delete m_pauseMenuScreen;
        m_pauseMenuScreen = nullptr;
    }
    if (m_chestScreen)
    {
        delete m_chestScreen;
        m_chestScreen = nullptr;
    }
    if (m_craftingScreen)
    {
        delete m_craftingScreen;
        m_craftingScreen = nullptr;
    }
    if (m_furnaceScreen)
    {
        delete m_furnaceScreen;
        m_furnaceScreen = nullptr;
    }
    if (m_settingsScreen)
    {
        delete m_settingsScreen;
        m_settingsScreen = nullptr;
    }
    if (m_mainMenuScreen)
    {
        delete m_mainMenuScreen;
        m_mainMenuScreen = nullptr;
    }
    if (m_worldSelectScreen)
    {
        delete m_worldSelectScreen;
        m_worldSelectScreen = nullptr;
    }
    if (m_farmMonitorScreen)
    {
        delete m_farmMonitorScreen;
        m_farmMonitorScreen = nullptr;
    }
    if (m_redstoneConfigScreen)
    {
        delete m_redstoneConfigScreen;
        m_redstoneConfigScreen = nullptr;
    }
    
    if (m_uiManager)
    {
        delete m_uiManager;
        m_uiManager = nullptr;
    }
}

void GameUIManager::Update(float deltaSeconds)
{
    HandleUIInput();
    if (m_uiManager)
    {
        m_uiManager->Update(deltaSeconds);
    }
}

void GameUIManager::Render() const
{
    if (m_uiManager)
    {
        g_theRenderer->BindTexture(m_uiAtlas->GetTexture());
        g_theRenderer->SetBlendMode(BlendMode::ALPHA);
        m_uiManager->Render();
    }
}

void GameUIManager::CreateHUD()
{
    m_hudScreen = new HUD(m_uiSystem);
    m_hudScreen->Build();
    m_uiManager->PushScreen(m_hudScreen);
}

void GameUIManager::ShowHUD()
{
    if (m_hudScreen)
    {
        m_hudScreen->SetActive(true);
    }
}

void GameUIManager::HideHUD()
{
    if (m_hudScreen)
    {
        m_hudScreen->SetActive(false);
    }
}

void GameUIManager::OpenInventory()
{
    if (IsInventoryOpen())
    {
        return;  
    }
    
    m_inventoryScreen->SetActive(true);
    m_uiManager->PushScreen(m_inventoryScreen);
    UpdateHUDVisibility();
}

void GameUIManager::CloseInventory()
{
    if (!IsInventoryOpen())
    {
        return;
    }
    
    if (m_uiManager->GetTopScreen() == m_inventoryScreen)
    {
        m_uiManager->PopScreen();
    }
    
    m_inventoryScreen->SetActive(false);
    UpdateHUDVisibility();
}

bool GameUIManager::IsInventoryOpen() const
{
    if (!m_inventoryScreen) return false;
    return m_inventoryScreen->IsActive() && 
           m_uiManager->HasScreenType(UIScreenType::INVENTORY);
}

void GameUIManager::OpenPauseMenu()
{
    if (IsPauseMenuOpen())
    {
        return;
    }
    
    m_pauseMenuScreen->SetActive(true);
    m_uiManager->PushScreen(m_pauseMenuScreen);
    UpdateHUDVisibility();
}

void GameUIManager::ClosePauseMenu()
{
    if (!IsPauseMenuOpen())
    {
        return;
    }
    
    if (m_uiManager->GetTopScreen() == m_pauseMenuScreen)
    {
        m_uiManager->PopScreen();
    }
    
    m_pauseMenuScreen->SetActive(false);
    UpdateHUDVisibility();
}

void GameUIManager::TogglePauseMenu()
{
    if (IsPauseMenuOpen())
    {
        ClosePauseMenu();
    }
    else
    {
        OpenPauseMenu();
    }
}

bool GameUIManager::IsPauseMenuOpen() const
{
    if (!m_pauseMenuScreen) return false;
    return m_pauseMenuScreen->IsActive() &&
           m_uiManager->HasScreenType(UIScreenType::PAUSE_MENU);
}

void GameUIManager::OpenChest(int chestID)
{
    if (IsChestOpen())
    {
        return;
    }
    
    // 设置 ChestID（如果 ChestScreen 有 SetChestID 方法）
    // m_chestScreen->SetChestID(chestID);
    
    m_chestScreen->SetActive(true);
    m_uiManager->PushScreen(m_chestScreen);
    UpdateHUDVisibility();
}

void GameUIManager::CloseChest()
{
    if (!IsChestOpen())
    {
        return;
    }
    
    if (m_uiManager->GetTopScreen() == m_chestScreen)
    {
        m_uiManager->PopScreen();
    }
    
    m_chestScreen->SetActive(false);
    UpdateHUDVisibility();
}

bool GameUIManager::IsChestOpen() const
{
    if (!m_chestScreen) return false;
    return m_chestScreen->IsActive() &&
           m_uiManager->HasScreenType(UIScreenType::CHEST);
}

void GameUIManager::OpenCraftingTable()
{
    if (IsCraftingTableOpen())
    {
        return;
    }
    
    m_craftingScreen->SetActive(true);
    m_uiManager->PushScreen(m_craftingScreen);
    UpdateHUDVisibility();
}

void GameUIManager::CloseCraftingTable()
{
    if (!IsCraftingTableOpen())
    {
        return;
    }
    
    if (m_uiManager->GetTopScreen() == m_craftingScreen)
    {
        m_uiManager->PopScreen();
    }
    
    m_craftingScreen->SetActive(false);
    UpdateHUDVisibility();
}

bool GameUIManager::IsCraftingTableOpen() const
{
    if (!m_craftingScreen) return false;
    return m_craftingScreen->IsActive() &&
           m_uiManager->HasScreenType(UIScreenType::CRAFTING_TABLE);
}

void GameUIManager::OpenFurnace(int furnaceID)
{
    if (IsFurnaceOpen())
    {
        return;
    }
    
    // 设置 FurnaceID（如果 FurnaceScreen 有 SetFurnaceID 方法）
    // m_furnaceScreen->SetFurnaceID(furnaceID);
    
    m_furnaceScreen->SetActive(true);
    m_uiManager->PushScreen(m_furnaceScreen);
    UpdateHUDVisibility();
}

void GameUIManager::CloseFurnace()
{
    if (!IsFurnaceOpen())
    {
        return;
    }
    
    if (m_uiManager->GetTopScreen() == m_furnaceScreen)
    {
        m_uiManager->PopScreen();
    }
    
    m_furnaceScreen->SetActive(false);
    UpdateHUDVisibility();
}

bool GameUIManager::IsFurnaceOpen() const
{
    if (!m_furnaceScreen) return false;
    return m_furnaceScreen->IsActive() &&
           m_uiManager->HasScreenType(UIScreenType::FURNACE);
}

void GameUIManager::OpenSettings()
{
    if (IsSettingsOpen())
    {
        return;
    }
    
    m_settingsScreen->SetActive(true);
    m_uiManager->PushScreen(m_settingsScreen);
    UpdateHUDVisibility();
}

void GameUIManager::CloseSettings()
{
    if (!IsSettingsOpen())
    {
        return;
    }
    
    if (m_uiManager->GetTopScreen() == m_settingsScreen)
    {
        m_uiManager->PopScreen();
    }
    
    m_settingsScreen->SetActive(false);
    UpdateHUDVisibility();
}

bool GameUIManager::IsSettingsOpen() const
{
    if (!m_settingsScreen) return false;
    return m_settingsScreen->IsActive() &&
           m_uiManager->HasScreenType(UIScreenType::OPTIONS);
}

void GameUIManager::OpenMainMenu()
{
    if (IsMainMenuOpen())
    {
        return;
    }
    
    m_mainMenuScreen->SetActive(true);
    m_uiManager->PushScreen(m_mainMenuScreen);
    UpdateHUDVisibility();
}

void GameUIManager::CloseMainMenu()
{
    if (!IsMainMenuOpen())
    {
        return;
    }
    
    if (m_uiManager->GetTopScreen() == m_mainMenuScreen)
    {
        m_uiManager->PopScreen();
    }
    
    m_mainMenuScreen->SetActive(false);
    UpdateHUDVisibility();
}

bool GameUIManager::IsMainMenuOpen() const
{
    if (!m_mainMenuScreen) return false;
    return m_mainMenuScreen->IsActive() &&
           m_uiManager->HasScreenType(UIScreenType::MAIN_MENU);
}

void GameUIManager::CloseTopScreen()
{
    if (!m_uiManager)
    {
        return;
    }
    UIScreen* topScreen = m_uiManager->GetTopScreen();
    
    if (!topScreen || topScreen == m_hudScreen)
    {
        return;  
    }
    
    if (topScreen == m_inventoryScreen)
    {
        m_inventoryScreen->SetActive(false);
    }
    else if (topScreen == m_pauseMenuScreen)
    {
        m_pauseMenuScreen->SetActive(false);
    }
    else if (topScreen == m_chestScreen)
    {
        m_chestScreen->SetActive(false);
    }
    else if (topScreen == m_craftingScreen)
    {
        m_craftingScreen->SetActive(false);
    }
    else if (topScreen == m_furnaceScreen)
    {
        m_furnaceScreen->SetActive(false);
    }
    else if (topScreen == m_settingsScreen)
    {
        m_settingsScreen->SetActive(false);
    }
    else if (topScreen == m_mainMenuScreen)
    {
        m_mainMenuScreen->SetActive(false);
    }
    else if (topScreen == m_worldSelectScreen)
    {
        m_worldSelectScreen->SetActive(false);
    }
    else if (topScreen == m_farmMonitorScreen)
    {
        m_farmMonitorScreen->SetActive(false);
    }
    else if (topScreen == m_redstoneConfigScreen)
    {
        m_redstoneConfigScreen->SetActive(false);
    }
    
    m_uiManager->PopScreen();
    UpdateHUDVisibility();
}

void GameUIManager::CloseAllScreens()
{
    if (m_inventoryScreen) m_inventoryScreen->SetActive(false);
    if (m_pauseMenuScreen) m_pauseMenuScreen->SetActive(false);
    if (m_chestScreen) m_chestScreen->SetActive(false);
    if (m_craftingScreen) m_craftingScreen->SetActive(false);
    if (m_furnaceScreen) m_furnaceScreen->SetActive(false);
    if (m_settingsScreen) m_settingsScreen->SetActive(false);
    if (m_mainMenuScreen) m_mainMenuScreen->SetActive(false);
    if (m_worldSelectScreen) m_worldSelectScreen->SetActive(false);
    if (m_farmMonitorScreen) m_farmMonitorScreen->SetActive(false);
    if (m_redstoneConfigScreen) m_redstoneConfigScreen->SetActive(false);
    
    while (m_uiManager->GetScreenStackSize() > 1)
    {
        m_uiManager->PopScreen();
    }
    
    UpdateHUDVisibility();
}

void GameUIManager::CleanupTempScreens()
{
    m_inventoryScreen = nullptr;
    m_pauseMenuScreen = nullptr;
    m_chestScreen = nullptr;
    m_craftingScreen = nullptr;
    m_furnaceScreen = nullptr;
    m_settingsScreen = nullptr;
    m_mainMenuScreen = nullptr;
    m_worldSelectScreen = nullptr; 
}

bool GameUIManager::IsAnyMenuOpen() const
{
    return IsInventoryOpen() || IsPauseMenuOpen() || IsChestOpen() || 
           IsCraftingTableOpen() || IsFurnaceOpen() || IsSettingsOpen() ||
           IsMainMenuOpen() || IsWorldSelectOpen();  
}

bool GameUIManager::IsGameInputBlocked() const
{
    if (!m_uiManager)
    {
        return false;
    }
    
    return m_uiManager->IsInputBlocked();
}

UIScreenType GameUIManager::GetCurrentScreenType() const
{
    if (!m_uiManager)
    {
        return UIScreenType::UNKNOWN;
    }
    
    UIScreen* topScreen = m_uiManager->GetTopScreen();
    if (!topScreen)
    {
        return UIScreenType::UNKNOWN;
    }
    
    return topScreen->GetType();
}

void GameUIManager::SetHotbarItem(int slotIndex, Texture* itemTexture)
{
    if (m_hudScreen)
    {
        m_hudScreen->SetHotbarSlot(slotIndex, itemTexture);
    }
}

void GameUIManager::ShowActionMessage(std::string const& message, float duration)
{
    if (m_hudScreen)
    {
        m_hudScreen->ShowActionMessage(message, duration);
    }
}

void GameUIManager::SetInventoryItem(int slotIndex, ItemData const& item)
{
    // 实现设置背包物品的逻辑
}

ItemData GameUIManager::GetInventoryItem(int slotIndex) const
{
    // 实现获取背包物品的逻辑
    return ItemData();
}

bool GameUIManager::AddItemToInventory(ItemData const& item)
{
    // 实现添加物品到背包的逻辑
    return false;
}

void GameUIManager::OpenFarmMonitor()
{
    if (IsFarmMonitorOpen())
    {
        return;
    }
    
    // 设置 World（如果 FarmMonitorScreen 有 SetWorld 方法）
    World* world = nullptr;
    if (g_theGame && g_theGame->m_currentWorld)
    {
        world = g_theGame->m_currentWorld;
    }
    // m_farmMonitorScreen->SetWorld(world);
    
    m_farmMonitorScreen->SetActive(true);
    m_uiManager->PushScreen(m_farmMonitorScreen);
    UpdateHUDVisibility();
}

void GameUIManager::CloseFarmMonitor()
{
    if (!IsFarmMonitorOpen())
    {
        return;
    }
    
    if (m_uiManager->GetTopScreen() == m_farmMonitorScreen)
    {
        m_uiManager->PopScreen();
    }
    
    m_farmMonitorScreen->SetActive(false);
    UpdateHUDVisibility();
}

bool GameUIManager::IsFarmMonitorOpen() const
{
    if (!m_farmMonitorScreen) return false;
    return m_farmMonitorScreen->IsActive() &&
           m_uiManager->HasScreenType(UIScreenType::CUSTOM);
}

void GameUIManager::SetFarmMonitorArea(const IntVec3& minCorner, const IntVec3& maxCorner)
{
    if (m_farmMonitorScreen)
    {
        m_farmMonitorScreen->SetMonitorArea(minCorner, maxCorner);
    }
}

void GameUIManager::OpenWorldSelect()
{
    if (IsWorldSelectOpen())
    {
        return;
    }
    
    m_worldSelectScreen->SetActive(true);
    m_uiManager->PushScreen(m_worldSelectScreen);
    UpdateHUDVisibility();
}

void GameUIManager::CloseWorldSelect()
{
    if (!IsWorldSelectOpen())
    {
        return;
    }
    
    if (m_uiManager->GetTopScreen() == m_worldSelectScreen)
    {
        m_uiManager->PopScreen();
    }
    
    m_worldSelectScreen->SetActive(false);
    UpdateHUDVisibility();
}

bool GameUIManager::IsWorldSelectOpen() const
{
    if (!m_worldSelectScreen) return false;
    return m_worldSelectScreen->IsActive() &&
           m_uiManager->HasScreenType(UIScreenType::WORLD_SELECT);
}

void GameUIManager::OpenRedstoneConfig(const IntVec3& blockPos)
{
    if (IsRedstoneConfigOpen())
    {
        return;
    }
    
    World* world = nullptr;
    if (g_theGame && g_theGame->m_currentWorld)
    {
        world = g_theGame->m_currentWorld;
    }
    // m_redstoneConfigScreen->SetWorld(world);
    // m_redstoneConfigScreen->SetBlockPosition(blockPos);
    
    m_redstoneConfigScreen->SetActive(true);
    m_uiManager->PushScreen(m_redstoneConfigScreen);
    UpdateHUDVisibility();
}

void GameUIManager::CloseRedstoneConfig()
{
    if (!IsRedstoneConfigOpen())
    {
        return;
    }
    
    if (m_uiManager->GetTopScreen() == m_redstoneConfigScreen)
    {
        m_uiManager->PopScreen();
    }
    
    m_redstoneConfigScreen->SetActive(false);
    UpdateHUDVisibility();
}

bool GameUIManager::IsRedstoneConfigOpen() const
{
    if (!m_redstoneConfigScreen) return false;
    return m_redstoneConfigScreen->IsActive() &&
           m_uiManager->HasScreenType(UIScreenType::CUSTOM);
}

void GameUIManager::HandleUIInput()
{
    if (!m_uiSystem)
    {
        return;
    }
    InputSystem* input = m_uiSystem->GetInputSystem();
    if (!input)
    {
        return;
    }
    if (input->WasKeyJustPressed(KEYCODE_ESC))
    {
        if (IsInventoryOpen())
        {
            CloseInventory();
        }
        else if (IsChestOpen())
        {
            CloseChest();
        }
        else if (IsCraftingTableOpen())
        {
            CloseCraftingTable();
        }
        else if (IsFurnaceOpen())
        {
            CloseFurnace();
        }
        else if (IsSettingsOpen())
        {
            CloseSettings();
        }
        else if (IsFarmMonitorOpen())
        {
            CloseFarmMonitor();
        }
        else if (IsRedstoneConfigOpen())
        {
            CloseRedstoneConfig();
        }
        // else
        // {
        //     TogglePauseMenu(); //TODO 有问题
        // }
    }
}

void GameUIManager::UpdateHUDVisibility()
{
    if (!m_uiManager)
    {
        ShowHUD();
        return;
    }
    
    UIScreen* topScreen = m_uiManager->GetTopScreen();
    
    if (!topScreen || topScreen == m_hudScreen)
    {
        ShowHUD();
        return;
    }
    
    UIScreenType screenType = topScreen->GetType();
    
    if (ShouldHideHUD(screenType))
    {
        HideHUD();
    }
    else
    {
        ShowHUD();
    }
}

bool GameUIManager::ShouldHideHUD(UIScreenType screenType) const
{
    switch (screenType)
    {
    // 需要隐藏 HUD 的全屏界面
    case UIScreenType::MAIN_MENU:
        return true;
        
    case UIScreenType::WORLD_SELECT:
        return true;
        
    case UIScreenType::PAUSE_MENU:
        return true;
        
    case UIScreenType::OPTIONS:
        return true;
        
    case UIScreenType::SERVER_LIST:
        return true; 

    // 游戏内界面 - 显示 HUD
    case UIScreenType::INVENTORY:
        return false;  
        
    case UIScreenType::CHEST:
        return false;  
        
    case UIScreenType::CRAFTING_TABLE:
        return false;  
        
    case UIScreenType::FURNACE:
        return false;  
        
    case UIScreenType::HUD:
        return false; 
        
    case UIScreenType::CUSTOM:
        return false; 
        
    default:
        return false; 
    }
}

void GameUIManager::CreateAllScreens()
{
    // HUD 在 Startup() 中创建
    // m_hudScreen = new HUD(m_uiSystem);
    
    // 创建其他所有 Screen
    m_inventoryScreen = new InventoryScreen(m_uiSystem);
    m_pauseMenuScreen = new PauseMenuScreen(m_uiSystem);
    m_chestScreen = new ChestScreen(m_uiSystem, 0);  
    m_craftingScreen = new CraftingScreen(m_uiSystem);
    m_furnaceScreen = new FurnaceScreen(m_uiSystem, 0);  
    m_settingsScreen = new SettingsScreen(m_uiSystem);
    m_mainMenuScreen = new MainMenuScreen(m_uiSystem);
    m_worldSelectScreen = new WorldSelectScreen(m_uiSystem);
    m_farmMonitorScreen = new FarmMonitorScreen(m_uiSystem, nullptr);  
    m_redstoneConfigScreen = new RedstoneConfigScreen(m_uiSystem, nullptr, IntVec3(0,0,0));  // World 和 BlockPos 后续设置
}

void GameUIManager::BuildAllScreens()
{
    if (m_hudScreen) m_hudScreen->Build();
    //if (m_inventoryScreen) m_inventoryScreen->Build();
    //if (m_pauseMenuScreen) m_pauseMenuScreen->Build();
    //if (m_chestScreen) m_chestScreen->Build();
    //if (m_craftingScreen) m_craftingScreen->Build();
    //if (m_furnaceScreen) m_furnaceScreen->Build();
    //if (m_settingsScreen) m_settingsScreen->Build();
    //if (m_mainMenuScreen) m_mainMenuScreen->Build();
    if (m_worldSelectScreen) m_worldSelectScreen->Build();
    if (m_farmMonitorScreen) m_farmMonitorScreen->Build();
    if (m_redstoneConfigScreen) m_redstoneConfigScreen->Build();
    
    //if (m_inventoryScreen) m_inventoryScreen->SetActive(false);
    //if (m_pauseMenuScreen) m_pauseMenuScreen->SetActive(false);
    //if (m_chestScreen) m_chestScreen->SetActive(false);
    //if (m_craftingScreen) m_craftingScreen->SetActive(false);
    //if (m_furnaceScreen) m_furnaceScreen->SetActive(false);
    //if (m_settingsScreen) m_settingsScreen->SetActive(false);
    //if (m_mainMenuScreen) m_mainMenuScreen->SetActive(false);
    if (m_worldSelectScreen) m_worldSelectScreen->SetActive(false);
    if (m_farmMonitorScreen) m_farmMonitorScreen->SetActive(false);
    if (m_redstoneConfigScreen) m_redstoneConfigScreen->SetActive(false);
}