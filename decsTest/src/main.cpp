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

	Position(float x, float y): X(x), Y(y)
	{
	}

	void TestFunc(int& i)
	{
		PrintLine("Is working");
		//i += 1;
	}

};

struct TestComponent 
{
public:
	int table[10];

	TestComponent()
	{
		//PrintLine("TestComponent::TestComponent");
	}

	TestComponent(const TestComponent& other)
	{
		//PrintLine("TestComponent::TestComponent(const TestComponent&)");
	}

	~TestComponent()
	{
		//PrintLine("TestComponent::~TestComponent");
	}

};

struct Renderer 
{
public:
	double mesh;
};

void NormalTest()
{
	using FloatTag = decs::tag<float>;
	using IntTag = decs::tag<int>;
	using DoubleTag = decs::tag<double>;
	using BoolTag = decs::tag<bool>;

	const decs::ContainerConfig containerConfig{
		.EntityChunkSize = 1000,
		.DefaultComponentChunkSize = 200,
		.ArchetypeChunkSize = 200,
	};

	decs::Container prefabContainer(containerConfig);
	decs::Entity prefab{};
	{
		prefab = prefabContainer.CreateEntity();

		prefab.AddComponent<TestComponent>();
		prefab.AddComponent<Renderer>();
		prefab.AddTag<FloatTag>();
		prefab.AddTag<IntTag>();
		prefab.AddTag<BoolTag>();
		prefab.AddComponent<Position>(10.f, 10.f);

	}

	{
		decs::Container container = { containerConfig };
		container.Spawn(prefab);
		container.Spawn(prefab, 9);


		/*{
			decs::ComponentTypeGroup<TestComponent, Renderer, Position> componetns{};
			decs::TagTypeGroup<FloatTag, IntTag, BoolTag> tags{};

			auto initFunc = [](const decs::Entity& e, TestComponent& component, Renderer& renderer, Position& position)
			{
				PrintLine("Init from helepr create entity func!");
			};

			container.CreateEntities(componetns, tags, 10, initFunc);
		}*/

		/*{
			decs::Entity e = container.CreateEntity();
			e.AddTag<FloatTag>();
			e.AddTag<IntTag>();
			e.AddTag<BoolTag>();

			e.AddComponent<TestComponent>();
			e.AddComponent<Renderer>();
			e.AddComponent<Position>();

			e.RemoveComponent<Renderer>();
			e.RemoveTag<BoolTag>();

			e.HasTag<FloatTag>();
			e.HasComponent<Position>();
			e.GetComponent<TestComponent>();

			container.Spawn(e, 9);
		}*/

		/*{
			decs::ComponentTypeGroup<TestComponent, Renderer, Position> comps{};
			decs::TagTypeGroup<FloatTag, IntTag, BoolTag> tags{};

			auto entityInit = [](const decs::Entity& e, TestComponent& component, Renderer& renderer, Position& position)
			{
				PrintLine("Only entity created!");
			};

			decs::Entity newEntity = container.CreateEntity(comps, tags, entityInit);
		}*/

		uint32_t counter = 0;
		auto testFunc = [&](const TestComponent& test)
		{
			PrintLine("Test func!");
		};
		auto testFuncWithEntity = [&](const decs::Entity& entity, const TestComponent& test)
		{
			PrintLine(std::format("Entity: {0} TestComponent", entity.GetID()));
		};


		if (true)
		{
			using QueryType = decs::Query< const TestComponent>;
			QueryType query(&container);
			query.With< Renderer, Position, FloatTag, IntTag, BoolTag>();

			PrintLine("ForEach");
			query.ForEach(testFunc);
			PrintLine("ForEach With Entity");
			query.ForEach(testFuncWithEntity);
			PrintLine("ForEach Safe");
			query.ForEach_Safe(testFunc);
			PrintLine("ForEach With Entity Safe");
			query.ForEach_Safe(testFuncWithEntity);
			PrintLine("ForEachBackward");
			query.ForEachBackward(testFunc);
			PrintLine("ForEachBackward With Entity");
			query.ForEachBackward(testFuncWithEntity);
			PrintLine("ForEachBackward Safe");
			query.ForEachBackward_Safe(testFunc);
			PrintLine("ForEachBackward With Entity Safe");
			query.ForEachBackward_Safe(testFuncWithEntity);

			if (false)
			{
				std::vector<QueryType::BatchIterator> iterators{};
				query.CreateBatchIterators(iterators, 10, 3);

				PrintLine("BatchIterator::ForEach");
				for (auto& it : iterators)
				{
					it.ForEach(testFunc);
				}

				PrintLine("BatchIterator::ForEach With Entity");
				for (auto& it : iterators)
				{
					it.ForEach(testFuncWithEntity);
				}
			}
		}

		if (true)
		{
			decs::TypeGroup<int, float> t{};

			auto st = t;

			using QueryType = decs::MultiQuery<const TestComponent>;
			QueryType query{};
			query.With< Renderer, Position, FloatTag, IntTag, BoolTag>();
			query.AddContainer(&container);

			PrintLine("ForEach");
			query.ForEach(testFunc);
			PrintLine("ForEach With Entity");
			query.ForEach(testFuncWithEntity);
			PrintLine("ForEach Safe");
			query.ForEach_Safe(testFunc);
			PrintLine("ForEach With Entity Safe");
			query.ForEach_Safe(testFuncWithEntity);
			PrintLine("ForEachBackward");
			query.ForEachBackward(testFunc);
			PrintLine("ForEachBackward With Entity");
			query.ForEachBackward(testFuncWithEntity);
			PrintLine("ForEachBackward Safe");
			query.ForEachBackward_Safe(testFunc);
			PrintLine("ForEachBackward With Entity Safe");
			query.ForEachBackward_Safe(testFuncWithEntity);
			PrintLine("ForEach_IngoreEntityActiveState");

			if (false)
			{
				std::vector<QueryType::BatchIterator> iterators{};
				query.CreateBatchIteratorsWithMaxNumberPerBatch(iterators, 7);

				PrintLine("BatchIterator::ForEach");
				for (auto& it : iterators)
				{
					it.ForEach(testFunc);
				}
				PrintLine("BatchIterator::ForEach With Entity");
				for (auto& it : iterators)
				{
					it.ForEach(testFuncWithEntity);
				}
			}
		}

	}
}

void CreatingEntitiesTest()
{
	const decs::ContainerConfig containerConfig{
		.EntityChunkSize = 1000,
		.DefaultComponentChunkSize = 200,
		.ArchetypeChunkSize = 200,
	};

	decs::Container container{ containerConfig };

	for (uint32_t i = 0; i < 10; i++)
	{
		decs::Entity e = container.CreateEntity();

		e.AddComponent<TestComponent>();
		e.AddComponent<Renderer>();
	}

	std::cout << "Archetypes count: " << container.GetArchetypeCount();
}

int main()
{
	NormalTest();

	return 0;
}
