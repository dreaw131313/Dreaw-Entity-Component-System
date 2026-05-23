#include "LightTest.h"

#include <iostream>
#include <format>

#include "decs/decs.h"

#include "MeasureTimer.h"
/// <summary>
/// Syntax sugar for defining sturct std::hash for own types. Must be used in global scope.
/// </summary>
#define STD_HASH(dataType)													\
template<>																	\
struct std::hash<dataType>													\
{																			\
public:																		\
	std::size_t operator()(const dataType& v) const noexcept;				\
};																			\
size_t std::hash<dataType>::operator()(const dataType& v) const noexcept	


#define BEGIN_NAMESPACE(name) namespace name{
#define END_NAMESPACE }

BEGIN_NAMESPACE(Light)

void PrintLine(std::string message = "")
{
	std::cout << message << "\n";
}


struct Position
{
public:
	float X = 0;
	float Y = 0;

	//int i[100];

public:
	Position()
	{ }

	Position(float x, float y): X(x), Y(y)
	{ }

	bool operator ==(const Position&) const noexcept = default;

	void TestFunc(int& i)
	{
		PrintLine("Is working");
		i += 1;
	}

};

struct TestComponent
{
public:
	int table[50];

};

struct Renderer
{
public:
	double mesh;
};


struct TestEntityFilter
{
public:
	int Data = 0;

public:
	TestEntityFilter()
	{

	}

	TestEntityFilter(int data):
		Data(data)
	{ }

	~TestEntityFilter()
	{

	}


	bool operator==(const TestEntityFilter& other)const noexcept = default;
};

END_NAMESPACE

STD_HASH(Light::TestEntityFilter)
{
	return std::hash<int>{}(v.Data);
}


BEGIN_NAMESPACE(Light)


void Test::Run()
{
	std::cout << "/////////////////////////////////////" << "\n";
	std::cout << "///////// LIGHT ECS TEST ////////////" << "\n";
	std::cout << "/////////////////////////////////////" << "\n";

	//IterationTest();
	EntityCreatePerformanceTest();
	//QueryManagerTest();
	//FilterTest();
	//RemovingArchetypesTest();
	//ObserversTest();
	//SettingComponents();
}

void Test::IterationTest()
{
	using FloatFilter = decs::filter<float>;

	using FloatTag = decs::tag<float>;
	using IntTag = decs::tag<int>;
	using DoubleTag = decs::tag<double>;
	using BoolTag = decs::tag<bool>;

	using TestFilter = decs::filter<TestEntityFilter>;

	const decs::light::ContainerConfig containerConfig{
		.EntityChunkSize = 1000,
		.ArchetypeChunkSize = 200,
	};

	decs::light::Container container = { containerConfig };

	{
		using ComponentTypeGroup = decs::LightComponentTypeGroup<TestComponent, Renderer, Position>;
		using TagTypeGroup = decs::TagTypeGroup<FloatTag, IntTag, BoolTag>;

		ComponentTypeGroup componetns{};
		TagTypeGroup tags{};
		std::tuple<TestEntityFilter> filters{};

		auto initFunc = [] (const decs::light::Entity& e, TestComponent& component, Renderer& renderer, Position& position)
		{
			PrintLine("Init from entity spawner!");
		};


		decs::light::EntitySpawner<ComponentTypeGroup, TagTypeGroup, decltype(filters)> spawner{ &container ,filters };
		spawner.Spawn(10, initFunc);

		decs::light::EntitySpawner<ComponentTypeGroup> spawner2{ &container };
		spawner.Spawn(initFunc);

		// container.CreateEntities(componetns, 10, initFunc);
		// container.CreateEntity(componetns, initFunc);
	}

	uint32_t counter = 0;
	auto testFunc = [&] (const TestComponent& test, const TestEntityFilter& filter)
	{
		PrintLine("Test func!");
	};
	auto testFuncWithEntity = [&] (const decs::light::Entity& entity, const TestComponent& test, const TestEntityFilter& filter)
	{
		PrintLine(std::format("Entity: {0} TestComponent", entity.GetID()));
	};

	auto forEachArchetypeFunc = [] (std::span<const TestComponent> components, std::span<const TestEntityFilter> filter)
	{
		PrintLine(std::format("{0} component count", components.size()));
	};

	if (true)
	{

		using QueryType = decs::light::Query<const TestComponent, decs::filter<TestEntityFilter>>;
		QueryType query(&container);
		//query.With< Renderer, Position, FloatTag, IntTag, BoolTag>();


		PrintLine("ForEachArchetype");
		query.ForEachArchetype(forEachArchetypeFunc);

		PrintLine("ForEach");
		query.ForEach(testFunc);

		PrintLine("Copy query");
		auto copyQuery = query;
		copyQuery.ForEach(testFunc);

		PrintLine("ForEach With Entity");
		query.ForEach(testFuncWithEntity);
		PrintLine("ForEachBackward");
		query.ForEachBackward(testFunc);
		PrintLine("ForEachBackward With Entity");
		query.ForEachBackward(testFuncWithEntity);

		if (true)
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

	if (false)
	{
		decs::TypeGroup<int, float> t{};

		auto st = t;

		using QueryType = decs::light::MultiQuery<const TestComponent, decs::filter<TestEntityFilter>>;
		QueryType query{};
		query.AddContainer(&container);


		PrintLine("MULTI QUERRY");
		PrintLine("ForEachArchetype");
		query.ForEachArchetype(forEachArchetypeFunc);

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

		if (true)
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

	{
		using EntityQuery = decs::light::Query<>;

		EntityQuery query{ &container };
		query.Without<Position>();

		query.ForEach([] ()
		{
			PrintLine("Empty query iteration");
		});

		query.ForEach([] (const decs::light::Entity& e)
		{
			PrintLine("Empty query iteration with entity");
		});
	}
}

void Test::ComponentCreationTest()
{
	const decs::light::ContainerConfig containerConfig{
		.EntityChunkSize = 1000,
		.ArchetypeChunkSize = 200,
	};

	decs::light::Container container = { containerConfig };

	decs::light::Entity e = container.CreateEntity();
	e.AddComponent<TestComponent>();
	e.AddComponent<float>();
	e.AddComponent<double>();

	decs::light::Entity e2 = container.Spawn(e);
}

void Test::EntityCreatePerformanceTest()
{
	size_t entityCounter = 0;

	const uint32_t testCount = 4;
	const uint32_t entityCount = 65536;

	decs::light::ContainerConfig config{
		.EntityChunkSize = 10000,
		.ArchetypeChunkSize = 100,
	};

	double finalAvarage = 0;
	double finalEntityAvarage = 0;

	decs::light::Container container{ config };

	decs::LightComponentTypeGroup<Position, TestComponent> comps{};
	decs::TagTypeGroup<float, int> tags{};
	std::tuple<int, bool> filters{ 1, false };

	decs::light::EntitySpawner<decltype(comps), decltype(tags), decltype(filters)> entitySpawner{ &container };
	entitySpawner.SetFilters(filters);

	size_t testCounter = 0;

	auto perfTest = [&] ()
	{

		double sum = 0;
		for (uint32_t testIdx = 0; testIdx < testCount; testIdx++)
		{
			MeasureTimer timer(true);
			{
				/*entitySpawner.Spawn(entityCount, [] (Position& pos, TestComponent& test)
				{

				});*/

				/*for (size_t i = 0; i < entityCount; i++)
				{
					auto e = container.CreateEntity();
					e.AddTag<float>();
					e.AddTag<int>();
					e.AddFilter<int>(1);
					e.AddFilter<bool>(false);
					e.AddComponent<Position>();
					e.AddComponent<TestComponent>();
				}*/

				//for (uint32_t i = 0; i < entityCount; i++)
				//{
				//	//container.CreateEntity(comps,/* tags, filters,*/ [] (Position& pos, TestComponent& test)
				//	//{

				//	//});
				//	/*container.CreateEntity(comps, [] (Position& pos, TestComponent& test)
				//	{

				//	});*/
				//}

				container.CreateEntities(comps, tags, entityCount, [] (Position& pos, TestComponent& test)
				{

				});

				/*container.CreateEntities(comps, tags, filters, entityCount, [] (Position& pos, TestComponent& test)
				{

				});*/

				/*container.CreateEntities(comps, entityCount, [] (Position& pos, TestComponent& test)
				{

				});*/
			}
			sum += timer.ElapsedAsMilisecond();

			container.Clear();
		}

		double avarage = sum / testCount;
		double entityAvarageTime = avarage / entityCount;

		std::cout << "Creating " << entityCount << " entities -> " << avarage << " ms (entity avarage time " << entityAvarageTime * 1000. << "us)\n";

		if (testCounter > 0)
		{
			finalAvarage += avarage;
			finalEntityAvarage += entityAvarageTime;
		}
		testCounter++;

	};

	uint32_t finalTestCount = 100;
	for (uint32_t i = 0; i < finalTestCount; i++)
	{
		perfTest();
	}

	double validTestCount = (finalTestCount - 1.);
	double finalEntitiesCreationTime = finalAvarage / validTestCount;
	double finalSingleEntityCreationTime = finalEntityAvarage / validTestCount;

	std::cout << "Final avarage " << entityCount << " entity creation time " << finalEntitiesCreationTime << " ms\n";
	std::cout << "Final avarage single entity creation time " << finalSingleEntityCreationTime * 1000. << " us (" << finalSingleEntityCreationTime << "ms)\n";
	std::cout << entityCounter << "\n";
}

void Test::FilterTest()
{
	using FloatTag = decs::tag<float>;

	decs::light::Container container{};


	decs::TagTypeGroup<float, int> group{};

	decs::light::Entity e = container.CreateEntity();


	e.AddComponent<float>(14.0f);
	e.AddComponent<double>(21.);
	e.AddTag<FloatTag>();
	e.RemoveFilter<TestEntityFilter>();

	e.AddFilter<TestEntityFilter>(TestEntityFilter(1));
	e.AddFilter<float>(1.f);

	{
		decs::LightComponentTypeGroup<Position, TestComponent> comps{};
		decs::TagTypeGroup<float, int> tags{};
		std::tuple<int, bool> filters{};

		auto entity = container.CreateEntity(comps, tags, filters, [] (Position& pos, TestComponent& test)
		{

		});

		bool hasFiltes = entity.HasFilters<int, bool>();
		DECS_ASSERT(hasFiltes, "Must have filters");
	}


	auto [f, i, d] = e.GetComponents<float, int, double>();
	if (f && d && !i)
	{
		PrintLine("Expected pointers values!");
	}

	auto e2 = container.Spawn(e);
	e2.SetFilter<TestEntityFilter>(2);

	decs::light::Query<decs::filter<TestEntityFilter>> queryWithFilter(&container);

	queryWithFilter.ForEach([] (const TestEntityFilter& filter)
	{
		std::cout << "Filter value: " << filter.Data << "\n";
	});

}

void Test::QueryManagerTest()
{
	std::unique_ptr<decs::light::Container> container = std::make_unique<decs::light::Container>();;

	{
		decs::light::Query<TestComponent> query{ container.get() };
		decs::light::MultiQuery<TestComponent> multiQuery{};
		multiQuery.AddContainer(container.get());

		auto testFunc = [] (TestComponent& component)
		{
			PrintLine("Test component!");
		};


		auto callQueriesForEach = [&] ()
		{
			PrintLine("Query::ForEach");
			query.ForEach(testFunc);
			PrintLine("MultiQuery::ForEach");
			multiQuery.ForEach(testFunc);
		};

		callQueriesForEach();

		decs::light::Entity e = container->CreateEntity();
		e.AddComponent<float>();
		e.AddComponent<TestComponent>();

		callQueriesForEach();

	}


	container.reset();
}

void Test::RemovingArchetypesTest()
{
	decs::light::Container ecs{};

	decs::light::ArchetypeDestroyState state{};
	decs::light::ArchetypeDestroyConfig config{
		.m_MaxArchetypesToCheck = 1000000,
		.m_MaxArchetypesDestroy = 100000,
		.m_bDestroyOnlyArchetypesWithFilters = true,
	};

	decs::light::Query<float> query{ &ecs };
	query.With<TestEntityFilter>();

	{
		auto e = ecs.CreateEntity();

		e.AddComponent<float>();
		e.SetFilter<TestEntityFilter>(TestEntityFilter(10));
		e.AddComponent<TestEntityFilter>();

		e.AddComponent<int>();
		e.AddComponent<double>();
		e.AddTag<float>();
		e.AddTag<decs::tag<int>>();

		bool bHasTag1 = e.HasTag<float>();
		bool bHasTag2 = e.HasTag<decs::tag<float>>();

		if (e.HasTags<float, decs::tag<int>, bool>())
		{
			PrintLine("Has tags");
		}

		e.RemoveTag<float>();
		e.RemoveTag<decs::tag<int>>();

		query.ForEach([] (float f)
		{
			PrintLine("Float iteration");
		});

		e.Destroy();
	}

	ecs.TryDestroyArchetypes(state, config);

	query.ForEach([] (float f)
	{
		PrintLine("Float iteration 2");
	});

	constexpr size_t idx = decs::type_index_v<int, float, bool, double, int, uint32_t>;

}

void Test::ObserversTest()
{
	decs::light::Container container{};

	container.AddComponentCreateObserver<Position>([] (const decs::light::Entity& entity, Position& pos)
	{
		PrintLine("Position create observer");
	});
	container.AddComponentDestroyObserver<Position>([] (const decs::light::Entity& entity, Position pos)
	{
		PrintLine("Position destroy observer");
	});
	container.AddComponentSetObserver<Position>([] (const decs::light::Entity& entity, Position pos)
	{
		PrintLine("Position set observer");
	});

	container.AddFilterAddObserver<TestEntityFilter>([] (const decs::light::Entity& entity, const TestEntityFilter& pos)
	{
		PrintLine("TestEntityFilter add observer");
	});
	container.AddFilterRemoveObserver<TestEntityFilter>([] (const decs::light::Entity& entity, const TestEntityFilter pos)
	{
		PrintLine("TestEntityFilter remove observer");
	});
	container.AddFilterChangeObserver<TestEntityFilter>([] (const decs::light::Entity& entity, TestEntityFilter oldValue, TestEntityFilter newValue)
	{
		PrintLine("TestEntityFilter change observer");
	});


	decs::light::Entity e = container.CreateEntity();
	e.AddComponent<Position>(Position(0, 0));
	e.SetComponent(Position(1, 1));
	e.RemoveComponent<Position>();

	e.AddFilter<TestEntityFilter>(TestEntityFilter(1));
	e.SetFilter<TestEntityFilter>(TestEntityFilter(2));
	e.RemoveFilter<TestEntityFilter>();

	e.AddFilter_NoObserver<TestEntityFilter>(TestEntityFilter(1));
	e.SetFilter_NoObserver<TestEntityFilter>(TestEntityFilter(2));
	e.RemoveFilter_NoObserver<TestEntityFilter>();

	e.Destroy();
}

void Test::SettingComponents()
{
	decs::light::Container container{};

	container.AddComponentSetObserver<Position>([] (const decs::light::Entity& entity, Position& pos)
	{
		PrintLine("Position setted!");
	});

	decs::light::Entity e = container.CreateEntity();
	e.SetComponent(Position());

	e.AddComponent<Position>(1.f, 1.f);

	e.SetComponent(Position(1.f, 1.f));
	e.SetComponent(Position(1.f, 2.f));

	e.SetComponent_NoObserver(Position(3.f, 3.f));

}

END_NAMESPACE
