#include <iostream>
#include <format>

#include "decs/decs.h"

void PrintLine(std::string message = "")
{
	std::cout << message << "\n";
}

struct Position
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

class PositionSerializer : public decs::ComponentSerializer<Position, void>
{
public:
	// Inherited via ComponentSerializer
	virtual void SerializeComponent(const Position& component, void* serializerData) override
	{
		PrintLine(std::format("\tPosition: X: {0}, Y: {1}", component.X, component.Y));
	}
};

class FloatSerializer : public decs::ComponentSerializer<decs::stable<float>, void>
{
public:
	// Inherited via ComponentSerializer
	virtual void SerializeComponent(const float& component, void* serializerData) override
	{
		PrintLine(std::format("\tFloat: {0}", component));
	}
};

class IntSerializer : public decs::ComponentSerializer< decs::stable<int>, void>
{
public:
	// Inherited via ComponentSerializer
	virtual void SerializeComponent(const int& component, void* serializerData) override
	{
		PrintLine(std::format("\tInt: {0}", component));
	}
};

class DoubleSerializer : public decs::ComponentSerializer< decs::stable<double>, void>
{
public:
	// Inherited via ComponentSerializer
	virtual void SerializeComponent(const double& component, void* serializerData) override
	{
		PrintLine(std::format("\tDouble: {0}", component));
	}
};


class TestSerializer : public decs::ContainerSerializer<void>
{
public:

protected:
	// Inherited via ContainerSerializer
	virtual bool BeginEntitySerialize(const decs::Entity& entity) override
	{
		PrintLine(std::format("Entity ID: {0}, is active: {1}", entity.GetID(), entity.IsActive()));

		return true;
	}

	virtual void EndEntitySerialize(const decs::Entity& entity) override
	{
	}
};

int main()
{

	PrintLine(std::format("Sizeof of Query<int>: {} bytes", sizeof(decs::Query<int>)));
	PrintLine(std::format("Sizeof of Query<int, float>: {} bytes", sizeof(decs::Query<int, float>)));
	PrintLine(std::format("Sizeof of Multi Query: {} bytes", sizeof(decs::MultiQuery<int>)));
	PrintLine(std::format("Sizeof of StableComponentRef: {} bytes", sizeof(decs::StableComponentRef)));
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

	// print prefab components names
	{
		PrintLine();
		PrintLine("Prefab component names:");
		for (uint32_t i = 0; i < prefab.GetArchetype()->ComponentCount(); i++)
		{
			std::cout << "\t" << i + 1 << ". " << prefab.GetArchetype()->GetComponentTypeClassName(i) << "\n";
		}

		PrintLine();
	}


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
	e2.AddComponent<decs::stable<float>>();*/


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
	query.ForEachBackward(lambda);

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
	testMultiQuery.AddContainer(&container, true);
	testMultiQuery.AddContainer(&prefabContainer, true);

	auto queryLambda = [&](decs::Entity& e, Position& pos)
	{
		//std::cout << "Query lambda -> Entity ID: " << e.ID() << ". Container ptr:"<< e.GetContainer() << "\n";
		std::cout << "Query lambda -> Entity ID: " << e.GetID() << ". Hash: " << std::hash<decs::Entity>{}(e) << "\n";
	};

	PrintLine("");
	PrintLine("Multi Query foreach forward:");
	testMultiQuery.ForEachForward(queryLambda);
	PrintLine("Multi Query foreach backwards:");
	testMultiQuery.ForEach(queryLambda);

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

#pragma region Container serializator test:
	// Serializer test:
	TestSerializer serializer = {};
	PositionSerializer positionSerializer = {};
	FloatSerializer floatSerializer = {};
	IntSerializer intSerializer = {};
	DoubleSerializer doubleSerializer = {};

	serializer.SetComponentSerializer(&positionSerializer);
	serializer.SetComponentSerializer(&floatSerializer);
	serializer.SetComponentSerializer(&intSerializer);
	serializer.SetComponentSerializer(&doubleSerializer);

	PrintLine();
	PrintLine("Serialization: Container");
	serializer.Serialize(container, nullptr);
	PrintLine();
	PrintLine("Serialization: Prefab Container");
	serializer.Serialize(prefabContainer, nullptr);

#pragma endregion


	{
		Position p;
		PrintLine();


		std::cout << "\"" << decs::Type<decs::component_type<decs::stable<decs::Entity>>::Type>::Name() << "\"" << std::endl;
		decs::Entity entity = {};
	}

	return 0;
}