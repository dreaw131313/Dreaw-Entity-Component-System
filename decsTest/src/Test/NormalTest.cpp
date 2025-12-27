#include "NormalTest.h"
#include <iostream>
#include <format>

#include "decs/decs.h"

#include "MeasureTimer.h"

namespace Normal
{

	void PrintLine(std::string message = "")
	{
		std::cout << message << "\n";
	}


	struct Position : public decs::EntityComponent
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

	struct TestComponent : public decs::EntityComponent
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

	struct Renderer :public decs::EntityComponent
	{
	public:
		double mesh;
	};


	class TestComponetObserver :
		public decs::CreateComponentObserver<TestComponent>,
		public decs::DestroyComponentObserver<TestComponent>,
		public decs::EnableComponentObserver<TestComponent>,
		public decs::DisableComponentObserver<TestComponent>
	{
	public:

		// Inherited via CreateComponentObserver
		void OnCreateComponent(TestComponent& component, const decs::Entity& entity) override
		{
			component.GetEntity().AddComponent<Position>();

		}

		// Inherited via DestroyComponentObserver
		void OnDestroyComponent(TestComponent& component, const decs::Entity& entity) override
		{
		}


		// Inherited via EnableComponentObserver
		void OnEnableComponent(TestComponent& component, const decs::Entity& entity) override
		{
		}


		// Inherited via DisableComponentObserver
		void OnDisableComponent(TestComponent& component, const decs::Entity& entity) override
		{
		}
	};

	void Test::Run()
	{
		using FloatTag = decs::tag<float>;
		using IntTag = decs::tag<int>;
		using DoubleTag = decs::tag<double>;
		using BoolTag = decs::tag<bool>;

		TestComponetObserver testComponentObserver = {};

		decs::ObserversManager observerManager = {};
		{
			observerManager.SetComponentObservers(&testComponentObserver, &testComponentObserver, &testComponentObserver, &testComponentObserver);
		}

		{
			const decs::ContainerConfig containerConfig{
				.EntityChunkSize = 1000,
				.DefaultComponentChunkSize = 200,
				.ArchetypeChunkSize = 200,
			};

			decs::Container container = { containerConfig };
			observerManager.FillContainerObservers(container);

			/*container.Spawn(prefab, true);
			container.Spawn(prefab, 9, true);*/

			{
				auto e = container.CreateEntity();
				e.AddComponent_NoObserver<TestComponent>();

				container.InvokeEntitesOnCreateListeners();
			}

			{
				decs::ComponentTypeGroup<TestComponent, Renderer, Position> componetns{};
				decs::TagTypeGroup<FloatTag, IntTag, BoolTag> tags{};

				auto initFunc = [](const decs::Entity& e, TestComponent& component, Renderer& renderer, Position& position)
				{
					PrintLine("Init from helepr create entity func!");
				};

				container.CreateEntities(componetns, tags, 10, true, initFunc);
			}

			/*{
			decs::ComponentTypeGroup<TestComponent, Renderer, Position> comps{};
			decs::TagTypeGroup<FloatTag, IntTag, BoolTag> tags{};

			auto entityInit = [](const decs::Entity& e, TestComponent& component, Renderer& renderer, Position& position)
			{
			PrintLine("Only entity created!");
			};

			decs::Entity newEntity = container.CreateEntity(comps, tags, true, entityInit);
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
				PrintLine("ForEach_IngoreEntityActiveState");
				query.ForEach_IngoreEntityActiveState(testFunc);
				PrintLine("ForEach_IngoreEntityActiveState With Entity");
				query.ForEach_IngoreEntityActiveState(testFuncWithEntity);

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
					PrintLine("BatchIterator::ForEach_IngoreEntityActiveState");
					for (auto& it : iterators)
					{
						it.ForEach_IngoreEntityActiveState(testFunc);
					}
					PrintLine("BatchIterator::ForEach_IngoreEntityActiveState With Entity");
					for (auto& it : iterators)
					{
						it.ForEach_IngoreEntityActiveState(testFuncWithEntity);
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
				query.ForEach_IngoreEntityActiveState(testFunc);
				PrintLine("ForEach_IngoreEntityActiveState With Entity");
				query.ForEach_IngoreEntityActiveState(testFuncWithEntity);

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
					PrintLine("BatchIterator::ForEach_IngoreEntityActiveState");
					for (auto& it : iterators)
					{
						it.ForEach_IngoreEntityActiveState(testFunc);
					}
					PrintLine("BatchIterator::ForEach_IngoreEntityActiveState With Entity");
					for (auto& it : iterators)
					{
						it.ForEach_IngoreEntityActiveState(testFuncWithEntity);
					}
				}
			}
		}
	}

	void Test::PerformanceTest()
	{
		const uint32_t testCount = 100;
		const uint32_t entityCount = 4096;

		decs::ContainerConfig config{
			.EntityChunkSize = 10000,
			.DefaultComponentChunkSize = 1000,
			.ArchetypeChunkSize = 100,
		};

		double finalAvarage = 0;

		auto perfTest = [&](
			uint32_t entityChunkSize,
			uint32_t componentChunkSize
			)
		{
			decs::Container container{config};
			double sum = 0;

			for (uint32_t testIdx = 0; testIdx < testCount; testIdx++)
			{
				MeasureTimer timer(true);
				{
					decs::ComponentTypeGroup<Position, TestComponent> comps{};


					container.CreateEntities_NoObserver(comps, entityCount,true, [](auto, auto) {});

					/*for (uint32_t i = 0; i < entityCount; i++)
					{
					}*/
				}
				sum += timer.ElapsedAsMilisecond();
			}

			double avarage = sum / testCount;

			finalAvarage += avarage;
			std::cout << "Creating " << entityCount << " entities -> " << avarage << " ms\n";
		};

		uint32_t finalTestCount = 100;
		for (uint32_t i = 0; i < finalTestCount ; i++)
		{
			perfTest(10000, 1000);
		}

		std::cout << "Final avarage time " << finalAvarage / finalTestCount << " ms\n";

		finalAvarage = 0;
		for (uint32_t i = 0; i < finalTestCount ; i++)
		{
			perfTest(10000, 1000);
		}

		std::cout << "Final avarage time " << finalAvarage / finalTestCount << " ms\n";


	}
}