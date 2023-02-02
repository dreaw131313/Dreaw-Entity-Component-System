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

class StableComponentObserverTest : 
	public decs::CreateComponentObserver<decs::Stable<Position>>, 
	public decs::DestroyComponentObserver<decs::Stable<Position>>
{
public:
	virtual void OnCreateComponent(Position& component, decs::Entity& entity) override
	{

	}

	virtual void OnDestroyComponent(Position& component, decs::Entity& entity)
	{

	}
};


int main()
{
	decs::Container container;

	auto e = container.CreateEntity();
	Position* p1 = e.AddComponent<decs::Stable<Position>>(101.f, 202.f);
	e.AddComponent<float>(1.f);

	decs::View<decs::Stable<Position>, float> view = { container };

	auto lambda = [] (Position& pos, float& f)
	{
		std::cout << "X = " << pos.X << ", Y = " << pos.Y << "\n";
		pos.X *= 2;
		pos.Y *= 2;
	};

	view.ForEach(lambda);
	view.ForEach(lambda);

	return 0;
}