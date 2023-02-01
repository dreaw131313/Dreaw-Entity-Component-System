#include <iostream>

#include "decs/decs.h"
#include "decs/ComponentContainers/StableContainer.h"

void Print(std::string message)
{
	std::cout << message << "\n";
}

class Position
{
public:
	float X = 0, Y = 0;

	Position()
	{

	}

	Position(float x, float y) :X(x), Y(y)
	{

	}
};


int main()
{
	std::cout << sizeof(decs::EntityData) << std::endl;

	decs::Container container;

	auto e = container.CreateEntity();
	Position* p1 = e.AddComponent<decs::Stable<Position>>(101.f, 202.f);
	Position* p2 = e.AddComponent<Position>(101.f, 202.f);

	e.RemoveComponent<Position>();
	e.RemoveComponent<decs::Stable<Position>>();

	decs::View<Position> view = { container };

	auto lambda = [] (Position& pos)
	{
		std::cout << "X = " << pos.X << ", Y = " << pos.Y << "\n";
		pos.X *= 2;
		pos.Y *= 2;
	};

	view.ForEach(lambda);
	view.ForEach(lambda);



	return 0;
}