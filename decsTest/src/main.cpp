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

	Position(const Position& other)
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
	e2.AddComponent<Position>();
	e2.AddComponent<float>();

	decs::View<Position> testView = { container };

	auto lambda = [&] (Position& position)
	{
		std::cout << "X: " << position.X << " Y: " << position.Y << std::endl;
		position.X += 1.f;
		position.Y += 1.f;
	};
	testView.ForEach(lambda);

	e1.Destroy();

	testView.ForEach(lambda);

	e2.Destroy();

	container.DestroyOwnedEntities();

	float* f = new float(1.f);
	float& fref = *f;

	delete f;

	fref = 10;

	return 0;
}