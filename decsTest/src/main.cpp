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
	decs::Container container;

	auto e = container.CreateEntity();
	e.AddComponent<decs::Stable<Position>>(101.f, 202.f);
	e.AddComponent<float>(1.f);

	decs::ComponentRef<decs::Stable<Position>> compRef = { e };

	if (compRef.Get() != nullptr)
	{
		Print("ComponentRef works correct with stable components!");
	}

	Position* p1 = compRef.Get();

	decs::View<decs::Stable<Position>> view = { container };

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