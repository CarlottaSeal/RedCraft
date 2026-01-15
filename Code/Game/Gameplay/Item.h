#pragma once
#include <string>

#include "Engine/Math/Vec3.hpp"

struct Item
{
    std::string m_name = "";
    
    bool IsEmpty() const { return m_name.empty(); }
    std::string GetName() const { return m_name; }
    void Clear() { m_name = ""; }
    void Set(std::string const& name) { m_name = name; }

    static std::string GetBlockFromItem(std::string const& itemName);
    static std::string GetBlockFromItem(Item item);
    static bool CanPlaceItem(std::string const& itemName);
    static bool CanPlaceItem(Item item);

    Vec3 m_position;              
    float m_lifetime = 300.0f;    
    float m_bobTimer = 0.0f;      
    float m_rotationAngle = 0.0f; 
    
    bool IsDroppedItem() const { return !IsEmpty() && m_lifetime > 0.0f; }
};
