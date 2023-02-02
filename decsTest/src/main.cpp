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

	for (int i = 0; i < 4; i++)
	{
		auto e = container.CreateEntity();
		e.AddComponent<uint8_t>(true);
		e.AddComponent<decs::Stable<Position>>(i + 1.f, i + 2.f);

		bool b1 = e.HasComponent<Position>();
		bool b2 = e.HasComponent<decs::Stable<Position>>();

		Position* p1 = e.GetComponent<Position>();
		Position* p2 = e.GetComponent<decs::Stable<Position>>();
		Position* p3 = e.GetComponent<decs::Stable<Position>>();
	}


	using ViewType = decs::View<decs::Stable<Position>>;
	ViewType view = { container };

	auto lambda = [] (decs::Entity& e, Position& pos)
	{
		std::cout << "Entity ID = " << e.ID() << ", X = " << pos.X << ", Y = " << pos.Y << "\n";
		pos.X *= 2;
		pos.Y *= 2;
	};

	view.ForEachForward(lambda);
	std::cout << "\n";
	view.ForEachForward(lambda);
	std::cout << "\n";

	std::vector<ViewType::BatchIterator> iterators = {};
	view.CreateBatchIterators(iterators, 3, 2);

	for (auto& iterator : iterators)
	{
		iterator.ForEach(lambda);
	}

	return 0;
}