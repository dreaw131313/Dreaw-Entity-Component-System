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
	}

	Position(float x, float y) : X(x), Y(y)
	{
	}

	~Position()
	{
		Print("Position Destructor");
	}

};


int main()
{
	Print("Start:");

	decs::Container container = {};
	decs::Entity e1 = container.CreateEntity();
	decs::Entity e2 = container.CreateEntity();

	e1.AddComponent<Position>(1, 1);
	e1.AddComponent<float>();
	e2.AddComponent<Position>(2, 2);
	e2.AddComponent<float>();

	decs::View<Position> testView = { container };

	auto lambda = [&] (const Position& position)
	{
		std::cout << "X: " << position.X << " Y: " << position.Y << std::endl;
	};
	testView.ForEach(lambda);


	container.DestroyOwnedEntities();

	return 0;
}