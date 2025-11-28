#include "GameUIManager.h"

#include "HUD.h"
#include "InventoryScreen.h"
#include "PauseMenuScreen.h"
#include "ChestScreen.h"
#include "CraftingScreen.h"
#include "FarmMonitorScreen.h"
#include "FurnaceScreen.h"
#include "MainMenuScreen.h"
#include "RedstoneConfigScreen.h"
#include "SettingsScreen.h"
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
    
    CreateHUD();
}

void GameUIManager::Shutdown()
{
    CleanupTempScreens();
    
    // HUD由UIManager管理，这里只清空引用
    m_hudScreen = nullptr;
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
        return;  // 已经打开
    }
    
    m_inventoryScreen = new InventoryScreen(m_uiSystem);
    m_inventoryScreen->Build();
    m_uiManager->PushScreen(m_inventoryScreen);
}

void GameUIManager::CloseInventory()
{
    if (!IsInventoryOpen())
    {
        return;
    }
    
    // 从栈中弹出
    if (m_uiManager->GetTopScreen() == m_inventoryScreen)
    {
        m_uiManager->PopScreen();
    }
    
    m_inventoryScreen = nullptr;
}

bool GameUIManager::IsInventoryOpen() const
{
    return m_uiManager->HasScreenType(UIScreenType::INVENTORY);
}

void GameUIManager::OpenPauseMenu()
{
    if (IsPauseMenuOpen())
    {
        return;
    }
    
    m_pauseMenuScreen = new PauseMenuScreen(m_uiSystem);
    m_pauseMenuScreen->Build();
    m_uiManager->PushScreen(m_pauseMenuScreen);
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
    
    m_pauseMenuScreen = nullptr;
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
    return m_uiManager->HasScreenType(UIScreenType::PAUSE_MENU);
}

void GameUIManager::OpenChest(int chestID)
{
    if (IsChestOpen())
    {
        CloseChest();  // 关闭当前箱子
    }
    
    m_chestScreen = new ChestScreen(m_uiSystem, chestID);
    m_chestScreen->Build();
    m_uiManager->PushScreen(m_chestScreen);
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
    
    m_chestScreen = nullptr;
}

bool GameUIManager::IsChestOpen() const
{
    return m_uiManager->HasScreenType(UIScreenType::CHEST);
}

// ========== 合成台管理 ==========

void GameUIManager::OpenCraftingTable()
{
    if (IsCraftingTableOpen())
    {
        return;
    }
    
    m_craftingScreen = new CraftingScreen(m_uiSystem);
    m_craftingScreen->Build();
    m_uiManager->PushScreen(m_craftingScreen);
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
    
    m_craftingScreen = nullptr;
}

bool GameUIManager::IsCraftingTableOpen() const
{
    return m_uiManager->HasScreenType(UIScreenType::CRAFTING_TABLE);
}

void GameUIManager::OpenFurnace(int furnaceID)
{
    if (IsFurnaceOpen())
    {
        CloseFurnace();
    }
    
    m_furnaceScreen = new FurnaceScreen(m_uiSystem, furnaceID);
    m_furnaceScreen->Build();
    m_uiManager->PushScreen(m_furnaceScreen);
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
    
    m_furnaceScreen = nullptr;
}

bool GameUIManager::IsFurnaceOpen() const
{
    return m_uiManager->HasScreenType(UIScreenType::FURNACE);
}

void GameUIManager::OpenSettings()
{
    if (IsSettingsOpen())
    {
        return;
    }
    
    m_settingsScreen = new SettingsScreen(m_uiSystem);
    m_settingsScreen->Build();
    m_uiManager->PushScreen(m_settingsScreen);
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
    
    m_settingsScreen = nullptr;
}

bool GameUIManager::IsSettingsOpen() const
{
    return m_uiManager->HasScreenType(UIScreenType::OPTIONS);
}

// ========== 主菜单管理 ==========

void GameUIManager::OpenMainMenu()
{
    if (IsMainMenuOpen())
    {
        return;
    }
    
    m_mainMenuScreen = new MainMenuScreen(m_uiSystem);
    m_mainMenuScreen->Build();
    m_uiManager->PushScreen(m_mainMenuScreen);
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
    
    m_mainMenuScreen = nullptr;
}

bool GameUIManager::IsMainMenuOpen() const
{
    return m_uiManager->HasScreenType(UIScreenType::MAIN_MENU);
}

void GameUIManager::CloseTopScreen()
{
    if (m_uiManager)
    {
        UIScreen* topScreen = m_uiManager->GetTopScreen();
        
        // 不关闭HUD
        if (topScreen && topScreen != m_hudScreen)
        {
            m_uiManager->PopScreen();
            
            // 清空对应的指针
            if (topScreen == m_inventoryScreen) m_inventoryScreen = nullptr;
            else if (topScreen == m_pauseMenuScreen) m_pauseMenuScreen = nullptr;
            else if (topScreen == m_chestScreen) m_chestScreen = nullptr;
            else if (topScreen == m_craftingScreen) m_craftingScreen = nullptr;
            else if (topScreen == m_furnaceScreen) m_furnaceScreen = nullptr;
            else if (topScreen == m_settingsScreen) m_settingsScreen = nullptr;
            else if (topScreen == m_mainMenuScreen) m_mainMenuScreen = nullptr;
        }
    }
}

void GameUIManager::CloseAllScreens()
{
    if (m_uiManager)
    {
        // 弹出所有屏幕（除了HUD）
        while (m_uiManager->GetTopScreen() != m_hudScreen && m_uiManager->GetTopScreen() != nullptr)
        {
            m_uiManager->PopScreen();
        }
        
        CleanupTempScreens();
    }
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
}

// ========== 状态查询 ==========

bool GameUIManager::IsAnyMenuOpen() const
{
    return IsInventoryOpen() || IsPauseMenuOpen() || IsChestOpen() || 
           IsCraftingTableOpen() || IsFurnaceOpen() || IsSettingsOpen() ||
           IsMainMenuOpen();
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
    return topScreen ? topScreen->GetType() : UIScreenType::UNKNOWN;
}

// ========== HUD 数据更新 ==========

void GameUIManager::UpdateHealth(float healthPercent)
{
    if (m_hudScreen)
    {
        m_hudScreen->UpdateHealth(healthPercent);
    }
}

void GameUIManager::UpdateHunger(float hungerPercent)
{
    if (m_hudScreen)
    {
        m_hudScreen->UpdateHunger(hungerPercent);
    }
}

void GameUIManager::UpdateExperience(float expPercent)
{
    if (m_hudScreen)
    {
        m_hudScreen->UpdateExperience(expPercent);
    }
}

void GameUIManager::UpdateArmor(float armorPercent)
{
    if (m_hudScreen)
    {
        m_hudScreen->UpdateArmor(armorPercent);
    }
}

void GameUIManager::SelectHotbarSlot(int slotIndex)
{
    if (m_hudScreen)
    {
        m_hudScreen->SelectHotbarSlot(slotIndex);
    }
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
    if (m_inventoryScreen)
    {
        m_inventoryScreen->SetItemSlot(slotIndex, item);
    }
}

ItemData GameUIManager::GetInventoryItem(int slotIndex) const
{
    if (m_inventoryScreen)
    {
        return m_inventoryScreen->GetItemSlot(slotIndex);
    }
    return ItemData();
}

bool GameUIManager::AddItemToInventory(ItemData const& item)
{
    if (m_inventoryScreen)
    {
        return m_inventoryScreen->AddItemToInventory(item);
    }
    return false;
}

void GameUIManager::OpenFarmMonitor()
{
    if (IsFarmMonitorOpen())
    {
        return;
    }
    
    // 获取 World 指针（需要从 Game 获取）
    World* world = nullptr;
    if (g_theGame && g_theGame->m_currentWorld)
    {
        world = g_theGame->m_currentWorld;
    }
    
    m_farmMonitorScreen = new FarmMonitorScreen(m_uiSystem, world);
    m_farmMonitorScreen->Build();
    m_uiManager->PushScreen(m_farmMonitorScreen);
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
    m_farmMonitorScreen = nullptr;
}

bool GameUIManager::IsFarmMonitorOpen() const
{
    return m_farmMonitorScreen != nullptr && 
           m_uiManager->HasScreenType(UIScreenType::CUSTOM);
}

void GameUIManager::SetFarmMonitorArea(const IntVec3& minCorner, const IntVec3& maxCorner)
{
    if (m_farmMonitorScreen)
    {
        m_farmMonitorScreen->SetMonitorArea(minCorner, maxCorner);
    }
}

void GameUIManager::OpenRedstoneConfig(const IntVec3& blockPos)
{
    if (IsRedstoneConfigOpen())
    {
        CloseRedstoneConfig();
    }
    
    World* world = nullptr;
    if (g_theGame && g_theGame->m_currentWorld)
    {
        world = g_theGame->m_currentWorld;
    }
    
    m_redstoneConfigScreen = new RedstoneConfigScreen(m_uiSystem, world, blockPos);
    m_redstoneConfigScreen->Build();
    m_uiManager->PushScreen(m_redstoneConfigScreen);
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
    m_redstoneConfigScreen = nullptr;
}

bool GameUIManager::IsRedstoneConfigOpen() const
{
    return m_redstoneConfigScreen != nullptr;
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
        // 如果有其他菜单打开，先关闭
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
        else
        {
            // 切换暂停菜单
            TogglePauseMenu();
        }
    }
    
    // E - 打开/关闭背包
    if (input->WasKeyJustPressed('E'))
    {
        if (IsInventoryOpen())
        {
            CloseInventory();
        }
        else if (!IsAnyMenuOpen())  // 只在没有其他菜单时打开
        {
            OpenInventory();
        }
    }
}
