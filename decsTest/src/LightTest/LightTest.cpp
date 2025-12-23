#include "LightTest.h"

#include <iostream>
#include <format>

#include "decs/decs.h"


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

		TestComponent()
		{
			PrintLine("TestComponent constructor");
		}

		TestComponent(const TestComponent& other)
		{
			PrintLine("TestComponent copy consructor");
		}

		TestComponent(TestComponent&& other) noexcept
		{
			PrintLine("TestComponent move constructor");
		}

		~TestComponent()
		{
			PrintLine("TestComponent destructor");
		}

		TestComponent& operator=(const TestComponent& other)
		{
			PrintLine("TestComponent copy assignment");
			return *this;
		}

		TestComponent& operator=(TestComponent&& other) noexcept
		{
			PrintLine("TestComponent move assignment");
			return *this;
		}
	};

	struct Renderer
	{
	public:
		double mesh;
	};

	using Entity = decs::light::Entity;
	using Container = decs::light::Container;
	template<decs::TLightComponentConcept... TComps>
	using Query = decs::light::Query<TComps...>;
	template<decs::TLightComponentConcept... TComps>
	using MultiQuery = decs::light::MultiQuery<TComps...>;

	void Test::Run()
	{
		ComponentCreationTest();
	}

	void Test::IterationTest()
	{
		using FloatTag = decs::tag<float>;
		using IntTag = decs::tag<int>;
		using DoubleTag = decs::tag<double>;
		using BoolTag = decs::tag<bool>;

		const decs::light::ContainerConfig containerConfig{
			.EntityChunkSize = 1000,
			.ArchetypeChunkSize = 200,
		};

		Container container = { containerConfig };

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
			decs::LightComponentTypeGroup<TestComponent, Renderer, Position> componetns{};
			decs::TagTypeGroup<FloatTag, IntTag, BoolTag> tags{};

			auto initFunc = [](const Entity& e, TestComponent& component, Renderer& renderer, Position& position)
			{
				PrintLine("Init from helepr create entity func!");
			};

			container.CreateEntities(componetns, tags, 10, initFunc);
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
	}

	void Test::ComponentCreationTest()
	{
		const decs::light::ContainerConfig containerConfig{
			.EntityChunkSize = 1000,
			.ArchetypeChunkSize = 200,
		};

		Container container = { containerConfig };

		Entity e = container.CreateEntity();
		e.AddComponent<TestComponent>();
		e.AddComponent<float>();
		e.AddComponent<double>();

		Entity e2 = container.Spawn(e);
	}

}