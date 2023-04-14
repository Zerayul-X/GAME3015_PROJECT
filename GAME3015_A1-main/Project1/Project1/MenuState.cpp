//***************************************************************************************
// MenuState.cpp
// by Zijie Wang and Wanhao Sun
//***************************************************************************************
#include "MenuState.h"
#include "SpriteNode.h"
#include "Game.hpp"

// Constructor
MenuState::MenuState(StateStack* stack, Context* context) : State(stack, context)
{

    // Clear the render items list
    mAllRitems.clear();

    // Reset the frame resources for the game
    mContext->game->ResetFrameResources();

    // Build the materials for the game
    mContext->game->BuildMaterials();

    // Create a background sprite for the menu
    std::unique_ptr<SpriteNode> backgroundSprite = std::make_unique<SpriteNode>(this);
    backgroundSprite->SetMatGeoDrawName("Aircrafts_Menu", "boxGeo", "box");
    backgroundSprite->setScale(60, 1.0, 50.0);
    backgroundSprite->setPosition(0, 0, 0);
    mSceneGraph->attachChild(std::move(backgroundSprite));

    // Build the scene graph
    mSceneGraph->build();
    mContext->game->BuildFrameResources(mAllRitems.size());
}

// Destructor
MenuState::~MenuState()
{
}

// Draw function
void MenuState::draw()
{
    // Draw the scene graph
    mSceneGraph->draw();
}

// Update function
bool MenuState::update(const GameTimer& gt)
{
    mSceneGraph->update(gt);
    return true;
}

// Handle event function
bool MenuState::handleEvent(WPARAM btnState)
{
    // Check S for start
    if (d3dUtil::IsKeyDown('S'))
    {
        requestStackPop();
        requestStackPush(States::Game);
    }
    // Check Q for quit
    else if (d3dUtil::IsKeyDown('Q'))
    {
        PostQuitMessage(0);
    }
    return true;
}

// Handle realtime input function for the MenuState class
bool MenuState::handleRealtimeInput()
{
    return true;
}
