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
	std::cout << "Entity Data size: " << sizeof(decs::EntityData) << "\n";
	std::cout << "\n";

	decs::Container prefabsContainer;
	decs::ObserversManager observerManager;
	decs::Container container;
	container.SetObserversManager(&observerManager);

	container.SetStableComponentChunkSize<Position>(1000);

	StableComponentObserverTest testStableObserver = {};
	observerManager.SetComponentCreateObserver<decs::Stable<Position>>(&testStableObserver);
	observerManager.SetComponentDestroyObserver<decs::Stable<Position>>(&testStableObserver);

	auto prefabEntity = prefabsContainer.CreateEntity();
	prefabEntity.AddComponent<float>(1.f);
	prefabEntity.AddComponent<decs::Stable<Position>>(1.f, 2.f);
	Position* p = prefabEntity.AddComponent<decs::Stable<Position>>(10.f, 20.f);
	prefabEntity.AddComponent<int>(1);


	std::vector<decs::Entity> entities;
	container.Spawn(prefabEntity, entities, 10, true);


	// ITERATIONS TESTS
	using ViewType = decs::View<decs::Stable<Position>>;
	ViewType view = { container };

	auto lambda = [] (const decs::Entity& e, Position& pos)
	{
		pos.X *= e.ID() + 1;
		pos.Y *= e.ID() + 1;
		std::cout << "Entity ID = " << e.ID() << ", X = " << pos.X << ", Y = " << pos.Y << "\n";
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

	for (auto& entity : entities)
	{
		entity.Destroy();
	}

	container.DestroyOwnedEntities();

	return 0;
}