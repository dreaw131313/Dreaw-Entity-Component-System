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
		//Print("Position Constructor");
	}

	Position(float x, float y) : X(x), Y(y)
	{
		//Print("Position Constructor");
	}

	~Position()
	{
		//Print("Position Destructor");
	}

};

int main()
{
	decs::Container prefabsContainer = { };
	decs::Container container = { };

	decs::Entity prefab = prefabsContainer.CreateEntity();
	prefab.AddComponent<Position>(111.f, 111.f);
	prefab.AddComponent<int>(111);

	for (int i = 0; i < 1; i++)
	{
		decs::Entity e = container.Spawn(prefab);
	}

	decs::Entity entity = container.CreateEntity();

	using ViewType = decs::View<int, Position>;
	ViewType testView = { container };

	auto lambda = [&] (decs::Entity& e, int& i, Position& position)
	{
		std::cout << "Entity ID: " << e.ID() << ": int = " << i << "\n";
	};
	auto lambda2 = [&] (int& i, Position& position)
	{
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

	container.DestroyOwnedEntities();

	return 0;
}