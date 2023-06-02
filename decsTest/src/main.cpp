#include <iostream>
#include <format>

#include "decs/decs.h"

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

	void TestFunc(int& i)
	{
		PrintLine("Is working");
		//i += 1;
	}
};

int main()
{

	PrintLine(std::format("Sizeof of Query<int>: {} bytes", sizeof(decs::Query<int>)));
	PrintLine(std::format("Sizeof of Query<int, float>: {} bytes", sizeof(decs::Query<int, float>)));
	PrintLine(std::format("Sizeof of Multi Query: {} bytes", sizeof(decs::MultiQuery<int>)));
	PrintLine();

	std::cout << "decs::Container size = " << sizeof(decs::Container) << " bytes" << "\n";
	std::cout << "decs::EntityData size = " << sizeof(decs::EntityData) << " bytes" << "\n";
	std::cout << "decs::Entity size = " << sizeof(decs::Entity) << " bytes" << "\n";
	std::cout << "ComponentRef<Position> size: " << sizeof(decs::ComponentRef<Position>) << " bytes" << "\n";
	std::cout << "Archetype size: " << sizeof(decs::Archetype) << " bytes" << "\n";
	PrintLine();

	decs::Container prefabContainer = {};
	decs::Container container = {};

	container.SetStableComponentChunkSize<float>(100);

	auto prefab = prefabContainer.CreateEntity();
	prefab.AddStableComponent<float>();
	prefab.AddStableComponent<int>();
	prefab.AddStableComponent<double>();
	prefab.AddComponent<Position>(1.f, 2.f);


	std::hash<decs::Entity>{}(prefab);

	//prefabContainer.Spawn(entity1, 3, true);
	container.SetStableComponentChunkSize<double>(100);
	container.SetStableComponentChunkSize<int>(100);
	container.SetStableComponentChunkSize(decs::Type<Position>::ID(), 100);

	container.Spawn(prefab, 1, true);

	float* floatCompPtr = nullptr;
	if (prefab.HasStableComponent<float>() && prefab.TryGetStableComponent<float>(floatCompPtr))
	{
		PrintLine("Prefab has stable float component");
	}

	decs::ComponentRef<Position> compPosRef = { prefab };
	if (!compPosRef.IsNull())
	{
		compPosRef->X = 11.f;
		compPosRef->Y = 22.f;
	}

	container.Spawn(prefab, 3, true);
	/*auto e2 = container.CreateEntity();
	e2.AddComponent<Position>();
	e2.AddComponent<decs::Stable<float>>();*/


	using QueryType = decs::Query<Position>;
	QueryType query = { container };

	uint64_t iterationCount = 0;
	auto lambda = [&](decs::Entity& e, Position& pos)
	{
		PrintLine("Enityt ID: " + std::to_string(e.GetID()) + " pos: " + std::to_string(pos.X) + ", " + std::to_string(pos.Y));
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

	auto queryLambda = [&](decs::Entity& e, Position& pos)
	{
		//std::cout << "Query lambda -> Entity ID: " << e.ID() << ". Container ptr:"<< e.GetContainer() << "\n";
		std::cout << "Query lambda -> Entity ID: " << e.GetID() << ". Hash: " << std::hash<decs::Entity>{}(e) << "\n";
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