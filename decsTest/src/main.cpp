#include <iostream>

#include "decs/decs.h"
#include "decs/ComponentContainers/StableContainer.h"

void PrintLine(std::string message)
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
	std::cout << "Type ID float = \t" << decs::Type<float>::ID() << "\n";
	std::cout << "Type ID int = \t" << decs::Type<int>::ID() << "\n";
	std::cout << "Type ID uint64_t = \t" << decs::Type<uint64_t>::ID() << "\n";
	std::cout << "Type ID Position = \t" << decs::Type<Position>::ID() << "\n";
	std::cout << "\n";

	decs::Container prefabContainer = {};
	decs::Container container = {};

	auto entity1 = prefabContainer.CreateEntity();
	entity1.AddComponent<Position>(1.f, 2.f);
	entity1.AddComponent<float>();

	//prefabContainer.Spawn(entity1, 3, true);
	container.Spawn(entity1, 1, true);
	container.Spawn(entity1, 3, true);
	auto e2 = container.CreateEntity();
	e2.AddComponent<Position>();

	using QueryType = decs::Query<Position>;
	QueryType query = { container };

	uint64_t iterationCount = 0;
	auto lambda = [&] (decs::Entity& e, Position& pos)
	{
		PrintLine(std::to_string(e.ID()));
		PrintLine("Enityt ID: " + std::to_string(e.ID()) + " pos: " + std::to_string(pos.X) + ", " + std::to_string(pos.Y));
	};

	query.ForEachForward(lambda);
	//query.ForEachBackward(lambda);

	std::cout << "Iterated entites count: " << iterationCount << "\n";

	std::vector<QueryType::BatchIterator> iterators;
	query.CreateBatchIterators(iterators, 2, 4);

	PrintLine("");
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
	PrintLine("Multi Query itertotrs iterations:");
	testMultiQuery.ForEachForward(queryLambda);

	std::vector<MultiQueryType::BatchIterator> multiQueryIterators;
	testMultiQuery.CreateBatchIterators(multiQueryIterators, 3, 3);
	PrintLine("");
	PrintLine("Multi Query itertotrs iterations:");

	for (uint64_t i = 0; i < multiQueryIterators.size(); i++)
	{
		auto& it = multiQueryIterators[i];
		PrintLine("Iterator " + std::to_string(i + 1));
		it.ForEach(queryLambda);
	}

	return 0;
}