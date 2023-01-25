#include <iostream>

#include "decs/decs.h"

#include "decs/PackedComponentContainer.h"


void Print(const std::string& message)
{
	// std::cout << message << std::endl;
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

	Position(float x, float y) : X(x), Y(y)
	{
		Print("Position Constructor");
	}

	/*Position(const Position& other)
	{
		Print("Position Copy Constructor");
	}

	Position(Position&& other) noexcept
	{
		Print("Position Move Constructor");
	}

	~Position()
	{
		Print("Position Destructor");
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
	}*/
};


int main()
{
	Print("Start:");

	decs::Container container = {};

	for (int i = 0; i < 5; i++)
	{
		decs::Entity e = container.CreateEntity();
		e.AddComponent<Position>(i, 2 * i);
		e.AddComponent<float>();
	}


	decs::View<Position> testView = { container };

	decs::Entity e = container.CreateEntity();
	decs::ComponentRef<int> intRef = { e };
	if (intRef.Get() != nullptr)
	{
		std::cout << "Entity have not added component!" << std::endl;
	}

	e.AddComponent<int>(101);

	if (intRef.Get() != nullptr)
	{
		std::cout << "Entity " << e.ID() << " int = " << *intRef.Get() << std::endl;
	}

	std::cout << std::endl;
	auto lambda = [&] (decs::Entity& e, Position& position)
	{
		std::cout << "Entity ID: " << e.ID() << "\tPosition: X: " << position.X << ", Y: " << position.Y << std::endl;
		position.X += 1.f;
		position.Y += 1.f;
	};
	testView.ForEach(lambda, decs::IterationType::Backward);

	container.DestroyOwnedEntities();

	return 0;
}