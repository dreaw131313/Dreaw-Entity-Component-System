#include <iostream>

#include "decs/decs.h"
#include "decs/ComponentContainers/StableContainer.h"

void PrintLine(std::string message = "")
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
	
	decs::Container prefabContainer = {};
	decs::Container container = {};

	container.SetStableComponentChunkSize<float>(100);

	auto prefab = prefabContainer.CreateEntity();
	prefab.AddComponent<Position>(1.f, 2.f);
	prefab.AddComponent<float>();

	//prefabContainer.Spawn(entity1, 3, true);
	container.Spawn(prefab, 1, true);
	container.Spawn(prefab, 3, true);
	auto e2 = container.CreateEntity();
	e2.AddComponent<Position>();

	using QueryType = decs::Query<Position>;
	QueryType query = { container };

	uint64_t iterationCount = 0;
	auto lambda = [&] (decs::Entity& e, Position& pos)
	{
		PrintLine("Enityt ID: " + std::to_string(e.ID()) + " pos: " + std::to_string(pos.X) + ", " + std::to_string(pos.Y));
	};

	PrintLine();
	PrintLine("Query foreach:");
	query.ForEachForward(lambda);
	//query.ForEachBackward(lambda);

	std::vector<QueryType::BatchIterator> iterators;
	query.CreateBatchIterators(iterators, 2, 4);

	PrintLine();
	PrintLine("Query iterators:");
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
		//std::cout << "Query lambda -> Entity ID: " << e.ID() << ". Container ptr:"<< e.GetContainer() << "\n";
		std::cout << "Query lambda -> Entity ID: " << e.ID() << ". Hash:" << std::hash<decs::Entity>{}(e) << "\n";
	};

	PrintLine("");
	PrintLine("Multi Query foreach:");
	testMultiQuery.ForEachForward(queryLambda);

	std::vector<MultiQueryType::BatchIterator> multiQueryIterators;
	testMultiQuery.CreateBatchIterators(multiQueryIterators, 3, 3);
	PrintLine("");
	PrintLine("Multi Query itertotrs:");

	for (uint64_t i = 0; i < multiQueryIterators.size(); i++)
	{
		auto& it = multiQueryIterators[i];
		PrintLine("Iterator " + std::to_string(i + 1));
		it.ForEach(queryLambda);
	}

	return 0;
}