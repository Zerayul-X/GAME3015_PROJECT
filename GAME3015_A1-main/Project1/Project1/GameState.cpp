//***************************************************************************************
// GameState.cpp
// by Zijie Wang and Wanhao Sun
//***************************************************************************************
#include "GameState.hpp"
#include "Game.hpp"

// Constructor
GameState::GameState(StateStack* stack, Context* context)
	: State(stack, context)
	, mWorld(this)
{
	mAllRitems.clear();
	mContext->game->ResetFrameResources();
	mContext->game->BuildMaterials();
	
	mWorld.buildScene();
	
	mContext->game->BuildFrameResources(mAllRitems.size());
}

// Destructor
GameState::~GameState()
{
}

// Draws the game world
void GameState::draw()
{
	mWorld.draw();
}

// Updates the game world with the given GameTimer object and handles input
bool GameState::update(const GameTimer& gt)
{
	ProcessInput();

	mWorld.update(gt);

	return true;
}

// Handles events for the game state
bool GameState::handleEvent(WPARAM btnState)
{
	// If the P key is pressed, request to pop the current state and push the pause state onto the stack
	if (d3dUtil::IsKeyDown('P'))
	{
		requestStackPop();
		requestStackPush(States::Pause);
	}
	
	return true;
}

// Handles realtime input for the game state
bool GameState::handleRealtimeInput()
{
	return true;
}

// Processes input from the player
void GameState::ProcessInput()
{
	CommandQueue& commands = mWorld.getCommandQueue();
	mContext->player->handleEvent(commands);
	mContext->player->handleRealtimeInput(commands);
}
