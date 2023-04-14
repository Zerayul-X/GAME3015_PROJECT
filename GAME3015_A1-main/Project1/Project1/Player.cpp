//***************************************************************************************
// Player.cpp
// by Zijie Wang and Wanhao Sun
//***************************************************************************************

#pragma region step 2
#include "Player.hpp"
#include "CommandQueue.hpp"
#include "Aircraft.hpp"
#include "../../Common/MathHelper.h"
#include "../../Common/d3dApp.h"
#include <map>
#include <string>
#include <algorithm>
#include <stdio.h>

using namespace DirectX;

struct AircraftMover
{
	// Constructor
	AircraftMover(float vx, float vy, float vz)
		: velocity(vx, vy, vz)
	{
	}

	// Overloaded operator that moves the aircraft
	void operator() (Aircraft& aircraft, const GameTimer&) const
	{
		aircraft.accelerate(velocity);
	}

	// Velocity vector for moving the aircraft
	XMFLOAT3 velocity;
};

// This class represents a player object and handles user input for controlling the aircraft
Player::Player()
{
	mKeyBinding[VK_LBUTTON] = MoveLeft;
	mKeyBinding[VK_RBUTTON] = MoveRight;

	mKeyBinding[VkKeyScan('W')] = MoveUp;
	mKeyBinding[VkKeyScan('S')] = MoveDown;
	mKeyBinding[VkKeyScan('A')] = MoveLeft;
	mKeyBinding[VkKeyScan('D')] = MoveRight;

	mKeyBinding[VK_UP] = MoveUp;
	mKeyBinding[VK_DOWN] = MoveDown;
	mKeyBinding[VK_LEFT] = MoveLeft;
	mKeyBinding[VK_RIGHT] = MoveRight;

	initializeActions();

	for (auto pair : mKeyBinding)
		mKeyFlag[pair.first] = false;

	for (auto& pair : mActionBinding)
		pair.second.category = Category::PlayerAircraft;
}

void Player::handleEvent(CommandQueue& commands)
{
	// Loop through all the key bindings
	for (auto pair : mKeyBinding)
	{
		if (!isRealtimeAction(pair.second))
		{
			// If the key was previously pressed
			if (mKeyFlag[pair.first])
			{
				// If the key is no longer being pressed
				if (!GetAsyncKeyState(pair.first))
				{
					mKeyFlag[pair.first] = false;
				}
			}
			else
			{
				// If the key is being pressed
				if (GetAsyncKeyState(pair.first) & 0x8000)
				{
					// Set the key flag and add the action to the command queue
					mKeyFlag[pair.first] = true;
					commands.push(mActionBinding[pair.second]);
				}
			}
		}
	}
}

// If the key is being pressed and it corresponds to a real-time action, add the action to the command queue
void Player::handleRealtimeInput(CommandQueue& commands)
{
	for (auto pair : mKeyBinding)
	{
		if (GetAsyncKeyState(pair.first) & 0x8000 && isRealtimeAction(pair.second))
		{
			commands.push(mActionBinding[pair.second]);
		}
	}
}

// Remove any existing key binding for the specified action
void Player::assignKey(Action action, char key)
{
	for (auto itr = mKeyBinding.begin(); itr != mKeyBinding.end(); )
	{
		if (itr->second == action)
			mKeyBinding.erase(itr++);
		else
			++itr;
	}

	mKeyBinding[key] = action;
}

// Loop through all the key bindings and return the key that is assigned to the specified action
char Player::getAssignedKey(Action action) const
{
	for (auto pair : mKeyBinding)
	{
		if (pair.second == action)
			return pair.first;
	}
	// If the action is not assigned to a key
	return 0x00;
}

void Player::initializeActions()
{    
	// Set the player's speed
	float playerSpeed = 10.f;

	// Bind each action to its corresponding AircraftMover derived action
	mActionBinding[MoveLeft].action = derivedAction<Aircraft>(AircraftMover(-playerSpeed, 0.f, 0.0f));
	mActionBinding[MoveRight].action = derivedAction<Aircraft>(AircraftMover(playerSpeed, 0.f, 0.0f));
	mActionBinding[MoveUp].action = derivedAction<Aircraft>(AircraftMover(0.f, playerSpeed, 0));
	mActionBinding[MoveDown].action = derivedAction<Aircraft>(AircraftMover(0.f, -playerSpeed, 0));
}

// Check if the specified action is real time
bool Player::isRealtimeAction(Action action)
{
	switch (action)
	{
	case MoveLeft:
	case MoveRight:
	case MoveDown:
	case MoveUp:
		return true;

	default:
		return false;
	}
}

#pragma endregion
