#include "LightTest.h"

#include <iostream>
#include <format>

#include "decs/decs.h"

#include "MeasureTimer.h"

namespace Light
{

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

	};

	struct Renderer
	{
	public:
		double mesh;
	};

	using Entity = decs::light::Entity;
	using ECSContainer = decs::light::Container;
	template<decs::TLightComponentConcept... TComps>
	using Query = decs::light::Query<TComps...>;
	template<decs::TLightComponentConcept... TComps>
	using MultiQuery = decs::light::MultiQuery<TComps...>;

	void Test::IterationTest()
	{
		using FloatFilter = decs::filter<float>;

		using FloatTag = decs::tag<float>;
		using IntTag = decs::tag<int>;
		using DoubleTag = decs::tag<double>;
		using BoolTag = decs::tag<bool>;

		const decs::light::ContainerConfig containerConfig{
			.EntityChunkSize = 1000,
			.ArchetypeChunkSize = 200,
		};

		ECSContainer container = { containerConfig };

		/*{
		Entity prefab = container.CreateEntity();

		prefab.AddComponent<TestComponent>();
		prefab.AddComponent<Renderer>();
		prefab.AddTag<FloatTag>();
		prefab.AddTag<IntTag>();
		prefab.AddTag<BoolTag>();
		prefab.AddComponent<Position>(10.f, 10.f);

		prefab.AddComponent<float>();

		container.Spawn(prefab, 9);
		}*/

		{
			using ComponentTypeGroup = decs::LightComponentTypeGroup<TestComponent, Renderer, Position>;
			using TagTypeGroup = decs::TagTypeGroup<FloatTag, IntTag, BoolTag>;

			ComponentTypeGroup componetns{};
			TagTypeGroup tags{};

			auto initFunc = [](const Entity& e, TestComponent& component, Renderer& renderer, Position& position)
			{
				PrintLine("Init from entity spawner!");
			};


			decs::light::EntitySpawner<ComponentTypeGroup, TagTypeGroup> spawner{ &container };
			spawner.Spawn(10, initFunc);

			decs::light::EntitySpawner<ComponentTypeGroup> spawner2{ &container };
			spawner.Spawn(initFunc);

			// container.CreateEntities(componetns, 10, initFunc);
			// container.CreateEntity(componetns, initFunc);
		}

		/*{
		Entity e = container.CreateEntity();
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
		decs::LightComponentTypeGroup<TestComponent, Renderer, Position> comps{};
		decs::TagTypeGroup<FloatTag, IntTag, BoolTag> tags{};

		auto entityInit = [](const Entity& e, TestComponent& component, Renderer& renderer, Position& position)
		{
		PrintLine("Only entity created!");
		};

		Entity newEntity = container.CreateEntity(comps, tags, entityInit);
		}*/

		uint32_t counter = 0;
		auto testFunc = [&](const TestComponent& test)
		{
			PrintLine("Test func!");
		};
		auto testFuncWithEntity = [&](const Entity& entity, const TestComponent& test)
		{
			PrintLine(std::format("Entity: {0} TestComponent", entity.GetID()));
		};

		auto forEachArchetypeFunc = [](std::span<const TestComponent> components)
		{
			PrintLine(std::format("{0} component count", components.size()));
		};

		if (true)
		{
			using QueryType = Query<const TestComponent>;
			QueryType query(&container);
			//query.With< Renderer, Position, FloatTag, IntTag, BoolTag>();


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

			using QueryType = MultiQuery<const TestComponent>;
			QueryType query{};
			query.With< Renderer, Position, FloatTag, IntTag, BoolTag>();
			query.AddContainer(&container);

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

		{
			using EntityQuery = Query<>;

			EntityQuery query{ &container };
			query.Without<Position>();

			query.ForEach([]()
			{
				PrintLine("Empty query iteration");
			});

			query.ForEach([](const Entity& e)
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

		ECSContainer container = { containerConfig };

		Entity e = container.CreateEntity();
		e.AddComponent<TestComponent>();
		e.AddComponent<float>();
		e.AddComponent<double>();

		Entity e2 = container.Spawn(e);
	}


	void Test::PerformanceTest()
	{
		const uint32_t testCount = 1;
		const uint32_t entityCount = 16384;

		decs::light::ContainerConfig config{
			.EntityChunkSize = 10000,
			.ArchetypeChunkSize = 100,
		};

		double finalAvarage = 0;

		decs::light::Container container{ config };

		using SpawnerComponentTypes = decs::LightComponentTypeGroup<Position, TestComponent>;
		using SpawnerComponentTags = decs::TagTypeGroup<>;
		decs::light::EntitySpawner<SpawnerComponentTypes> entitySpawner{ &container };

		/*MeasureTimer reserveSpaceTimer(true);
		{
			entitySpawner.ReserveSpaceInArchetype(entityCount);
		}
		double reserveSpaceTime = reserveSpaceTimer.ElapsedAsMilisecond();
		PrintLine(std::format("Rerve space time: {0} ms", reserveSpaceTime));*/

		auto perfTest = [&]()
		{
			decs::LightComponentTypeGroup<Position, TestComponent> comps{};

			double sum = 0;
			for (uint32_t testIdx = 0; testIdx < testCount; testIdx++)
			{
				MeasureTimer timer(true);
				{
					entitySpawner.Spawn(entityCount, [](Position& pos, TestComponent& test)
					{

					});

					/*for (uint32_t i = 0; i < entityCount; i++)
					{
						container.CreateEntity(comps, [](Position& pos, TestComponent& test)
						{

						});
					}*/
				}
				sum += timer.ElapsedAsMilisecond();

				container.Clear();
			}

			double avarage = sum / testCount;

			finalAvarage += avarage;
			std::cout << "Creating " << entityCount << " entities -> " << avarage << " ms\n";
		};

		uint32_t finalTestCount = 100;
		for (uint32_t i = 0; i < finalTestCount; i++)
		{
			perfTest();
		}

		std::cout << "Final avarage time " << finalAvarage / finalTestCount << " ms\n";

		finalAvarage = 0;
		for (uint32_t i = 0; i < finalTestCount; i++)
		{
			perfTest();
		}

		std::cout << "Final avarage time " << finalAvarage / finalTestCount << " ms\n";


	}

	struct TestEntityFilter
	{
	public:
		int Data = 0;

	public:
		TestEntityFilter()
		{
			PrintLine("TestEntityFilter constructor");
		}
		TestEntityFilter(int data):
			Data(data)
		{
			PrintLine("TestEntityFilter constructor");
		}

		~TestEntityFilter()
		{
			PrintLine("TestEntityFilter destructor");
		}


		bool operator==(const TestEntityFilter& other)const noexcept = default;
	};
}

template<>
struct std::hash<Light::TestEntityFilter>
{
public:
	std::size_t operator()(const Light::TestEntityFilter& filter) const noexcept
	{
		return std::hash<int>{}(filter.Data);
	}
};

namespace Light
{

	void Test::FilterTest()
	{
		ECSContainer container{};

		Entity e = container.CreateEntity();
		e.AddComponent<float>();
		e.AddComponent<double>();
		e.SetFilter<TestEntityFilter>(TestEntityFilter(10));
		e.SetFilter<TestEntityFilter>(TestEntityFilter(1));

		auto filterGet = e.GetFilter<TestEntityFilter>();
		bool hasFilter = e.HasFilter<TestEntityFilter>();
		hasFilter = e.HasFilter(decs::Type<TestEntityFilter>::ID());
		//hasFilter = e.HasFilter(TestEntityFilter(11));

		e.RemoveFilter<TestEntityFilter>();
	}

}