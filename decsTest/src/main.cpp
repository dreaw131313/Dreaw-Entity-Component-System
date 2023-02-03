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
		Print("Stable position on create");
	}

	virtual void OnDestroyComponent(Position& component, decs::Entity& entity)
	{
		Print("Stable position on destroy");
	}
};


int main()
{
	decs::Container prefabsContainer;
	decs::ObserversManager observerManager;
	decs::Container container;
	container.SetObserversManager(&observerManager);

	container.SetStableComponentChunkSize<Position>(1000);

	StableComponentObserverTest testStableObserver = {};
	observerManager.SetComponentCreateObserver<decs::Stable<Position>>(&testStableObserver);
	observerManager.SetComponentDestroyObserver<decs::Stable<Position>>(&testStableObserver);

	int entitiesCount = 2;
	auto prefabEntity = prefabsContainer.CreateEntity();
	prefabEntity.AddComponent<float>(1.f);
	prefabEntity.AddComponent<decs::Stable<Position>>(1.f, 2.f);
	prefabEntity.AddComponent<int>(1);

	container.Spawn(prefabEntity, 10, true);


	// ITERATIONS TESTS
	using ViewType = decs::View<decs::Stable<Position>>;
	ViewType view = { container };

	auto lambda = [] (decs::Entity& e, Position& pos)
	{
		std::cout << "Entity ID = " << e.ID() << ", X = " << pos.X << ", Y = " << pos.Y << "\n";
		pos.X *= 2;
		pos.Y *= 2;
	};

	std::cout << "\n";
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

	std::cout << "\n";
	container.DestroyOwnedEntities();

	return 0;
}