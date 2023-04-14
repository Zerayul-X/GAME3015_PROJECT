//***************************************************************************************
// CommandQueue.cpp
// by Zijie Wang and Wanhao Sun
//***************************************************************************************
#include "CommandQueue.hpp"
#include "SceneNode.hpp"

// Adds a new command to the end of the command queue
void CommandQueue::push(const Command& command)
{
	mQueue.push(command);
}

// Removes and returns the first command in the queue
Command CommandQueue::pop()
{
	Command command = mQueue.front();
	mQueue.pop();
	return command;
}

//Checks if the command queue is empty
bool CommandQueue::isEmpty() const
{
	return mQueue.empty();
}


