#include <iostream>

#include "decs/decs.h"


#include "decs/PackedComponentContainer.h"

void Print(const std::string& message)
{
	std::cout << message << "\n";
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


	Position(const Position& other)
	{
		Print("Copy Constructor");
	}

	Position(Position&& other) noexcept
	{
		Print("Move Constructor");
	}

	~Position()
	{
		Print("Position Destructor");
	}

	Position& operator=(const Position& other)
	{
		Print("Copy assigment");
		return *this;
	}

	Position& operator=(Position&& other) noexcept
	{
		Print("Move assigment");
		return *this;
	}
};

int main()
{
	decs::Container prefabsContainer = { };
	decs::Container container = { };

	decs::Entity prefab = prefabsContainer.CreateEntity();
	prefab.AddComponent<Position>(111.f, 111.f);
	prefab.AddComponent<int>(111);
	prefab.AddComponent<float>(1.f);
	prefab.GetComponent<int>();

	std::vector<decs::Entity> spawnedEntities = {};
	//container.Spawn(prefab, spawnedEntities, 3, true);
	decs::Entity testEntity = container.Spawn(prefab);
	std::cout << "\n";
	container.Spawn(prefab);
	std::cout << "\n";
	container.Spawn(prefab);
	std::cout << "\n";
	container.Spawn(prefab);
	std::cout << "\n";
	container.Spawn(prefab);
	std::cout << "\n";
	container.Spawn(prefab);

	std::cout << "\n";
	testEntity.Destroy();

	decs::ArchetypesShrinkToFitState shrinkState = { 1, 1.f };

	/*do
	{
		container.ShrinkArchetypesToFit(shrinkState); // naprawiæ bo nie dzia³a
	} while (!shrinkState.IsEnded());

	Print("Spawned entities count " + std::to_string(spawnedEntities.size()));
	for (int i = 0; i < spawnedEntities.size(); i++)
	{
		Print("Entity ID: " + std::to_string(spawnedEntities[i].ID()));
	}
	std::cout << "\n";

	using ViewType = decs::View<int, Position>;
	ViewType testView = { container };

	auto lambda = [&] (const decs::Entity& e, int& i, Position& position)
	{
		i *= e.ID() + 1;
		std::cout << "Entity ID: " << e.ID() << ": int = " << i << "\n";
	};
	auto lambda2 = [&] (int& i, Position& position)
	{
		i *= 2;
		std::cout << "int = " << i << "\n";
	};

	testView.ForEachForward(lambda);
	testView.ForEachBackward(lambda);
	std::cout << "\n";
	testView.ForEachBackward(lambda2);
	testView.ForEachForward(lambda2);

	std::vector< ViewType::BatchIterator> iterators;
	testView.CreateBatchIterators(iterators, 2, 3);

	std::cout << "\n";
	for (auto& it : iterators)
	{
		it.ForEach(lambda);
	}
	std::cout << "\n";
	for (auto& it : iterators)
	{
		it.ForEach(lambda2);
	}

	container.DestroyOwnedEntities();*/

	return 0;
}