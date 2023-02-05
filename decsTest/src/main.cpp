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
	float X = 0;
	float Y = 0;

public:
	Position()
	{

	}
	Position(float x, float y) : X(x), Y(y)
	{

	}
};


int main()
{
	std::cout << "ecsSet size = " << sizeof(decs::ecsSet<decs::Archetype*>) << "\n";
	std::cout << "\n";
	decs::Container prefabContainer = {};
	decs::Container container = {};

	auto entity1 = prefabContainer.CreateEntity();
	entity1.AddComponent<Position>();

	auto entity2 = container.Spawn(entity1, 3, true);

	using ViewType = decs::View<Position>;
	ViewType view = { container };

	uint64_t iterationCount = 0;
	auto lambda = [&] (decs::Entity& e, Position& pos)
	{
		Print(std::to_string(e.ID()));
	};

	view.ForEachForward(lambda);

	std::cout << "Iterated entites count: " << iterationCount << "\n";

	using QueryType = decs::Query<Position>;
	QueryType testQuery = {};
	testQuery.AddContainer(&container);
	testQuery.AddContainer(&prefabContainer);

	auto queryLambda = [&] (decs::Entity& e, Position& pos)
	{
		if (e.GetContainer() == &prefabContainer)
		{
			std::cout << "Query lambda -> Prefab Entity ID: " << e.ID() << "\n";
		}
		else
		{
			std::cout << "Query lambda -> Entity ID: " << e.ID() << "\n";
		}
	};

	Print("\nQuery Iterations:");
	testQuery.ForEachForward(queryLambda);
	testQuery.ForEachBackward(queryLambda);

	return 0;
}