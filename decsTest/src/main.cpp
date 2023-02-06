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
	std::cout << "Entity size = " << sizeof(decs::Entity) << "\n";
	std::cout << "\n";

	decs::Container prefabContainer = {};
	decs::Container container = {};

	auto entity1 = prefabContainer.CreateEntity();
	entity1.AddComponent<Position>();

	auto entity2 = container.Spawn(entity1, 4, true);

	using QueryType = decs::Query<Position>;
	QueryType query = { container };

	uint64_t iterationCount = 0;
	auto lambda = [&] (decs::Entity& e, Position& pos)
	{
		Print(std::to_string(e.ID()));
	};

	query.ForEachForward(lambda);
	query.ForEachBackward(lambda);

	std::cout << "Iterated entites count: " << iterationCount << "\n";

	std::vector<QueryType::BatchIterator> iterators;
	query.CreateBatchIterators(iterators, 2, 3);

	Print("\n");
	Print("Query iterators:");
	for (auto& it : iterators)
	{
		it.ForEach(lambda);
	}

	using MultiQueryType = decs::MultiQuery<Position>;
	MultiQueryType testMultiQuery = {};
	testMultiQuery.AddContainer(&container);
	testMultiQuery.AddContainer(&prefabContainer);

	auto queryLambda = [&] (decs::Entity& e, Position& pos)
	{
		std::cout << "Query lambda -> Entity ID: " << e.ID() << "\n";
	};

	Print("\nQuery Iterations:");
	testMultiQuery.ForEachForward(queryLambda);
	testMultiQuery.ForEachBackward(queryLambda);

	std::vector<MultiQueryType::BatchIterator> multiQueryIterators;


	return 0;
}