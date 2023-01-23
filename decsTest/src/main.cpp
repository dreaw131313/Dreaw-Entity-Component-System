#include <iostream>

#include "decs/decs.h"

#include "decs/PackedComponentContainer.h"


void Print(const std::string& message)
{
	std::cout << message << std::endl;
}

class Position
{
public:
	float X = 0, Y = 0;

public:
	Position()
	{
		Print("Position Constructor");
	}

	~Position()
	{
		Print("Position Destruction");
	}


	Position(const Position& other)
	{
		Print("Position Copy Constructor");
	}
	
	Position(Position&& other) noexcept
	{
		Print("Position Move Constructor");
	}

	Position& operator = (const Position& other)
	{
		Print("Position Copy Assigment");
		return *this;
	}

	Position& operator = (Position&& other) noexcept
	{
		Print("Position Move Assigment");
		return *this;
	}
	
};


int main()
{
	Print("Start:");

	decs::Container container = {};
	decs::Entity e1 = container.CreateEntity();
	decs::Entity e2 = container.CreateEntity();

 	e1.AddComponent<Position>();
	e1.AddComponent<float>();
	e1.AddComponent<int>();

 	e2.AddComponent<Position>();
	e2.AddComponent<float>();
	e2.AddComponent<int>();

	e1.RemoveComponent<int>();
	e1.RemoveComponent<float>();
	e1.RemoveComponent<Position>();

	decs::View<Position> testView = { container };

	auto lambda = [&] (const Position& position)
	{

	};
	testView.ForEachForward(lambda);


	return 0;
}