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

		Position(float x, float y) : X(x), Y(y)
		{
		}

		void TestFunc(int& i)
		{
			PrintLine("Is working");
			//i += 1;
		}

		void ECS_OnConstruct(const decs::Entity& e)
		{
			//PrintLine("Position on construct!");
		}

	};

	struct TestComponent : public decs::EntityComponent
	{
	public:
		int table[50];

	public:
		void ECS_OnConstruct(const decs::Entity& e)
		{
			//PrintLine("TestComponent on construct!");
		}
	};

	struct HeavyDataComponent : public decs::EntityComponent
	{
	public:
		char array[1024];

	};

	struct Renderer :public decs::EntityComponent
	{
	public:
		double mesh;
		char table[200];
	};


	class TestComponentObserver
	{
	public:
		void OnCreateComponent(const decs::Entity& entity, TestComponent& component)
		{
			PrintLine("TestComponent Create");
		}

		void OnDestroyComponent(const decs::Entity& entity, TestComponent& component)
		{
			PrintLine("TestComponent Destroy");
		}

		void OnEnableComponent(const decs::Entity& entity, TestComponent& component)
		{
			PrintLine("TestComponent Enable");
		}

		void OnDisableComponent(const decs::Entity& entity, TestComponent& component)
		{
			PrintLine("TestComponent Disable");
		}
	};

	class EntityObserver
	{
	public:
		void OnCreateEntity(const decs::Entity& entity)
		{
			PrintLine("Entity created");
		}
		void OnDestroyEntity(const decs::Entity& entity)
		{
			PrintLine("Entity destroyed");
		}
		void OnEnableEntity(const decs::Entity& entity)
		{
			PrintLine("Entity enabled");
		}
		void OnDisableEntity(const decs::Entity& entity)
		{
			PrintLine("Entity disabled");
		}
	};

	void Test::Run()
	{
		std::cout << "///////////////////////////////////////////" << "\n";
		std::cout << "///////// NORMAL ECS TEST ////////////" << "\n";
		std::cout << "///////////////////////////////////////////" << "\n";

		//QueryIterationTest();
		EntityCreatePerformanceTest();
		//ObserversTest();
	}

	void Test::QueryIterationTest()
	{
		using FloatTag = decs::tag<float>;
		using IntTag = decs::tag<int>;
		using DoubleTag = decs::tag<double>;
		using BoolTag = decs::tag<bool>;

		{
			const decs::ContainerConfig containerConfig{
				.EntityChunkSize = 1000,
				.DefaultComponentChunkSize = 200,
				.ArchetypeChunkSize = 200,
			};

			decs::Container container = { containerConfig };

			{
				decs::ComponentTypeGroup<TestComponent, Renderer, Position> componetns{};
				decs::TagTypeGroup<FloatTag, IntTag, BoolTag> tags{};

				auto initFunc = [] (const decs::Entity& e, TestComponent& component, Renderer& renderer, Position& position)
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
			auto testFunc = [&] (const TestComponent& test)
			{
				PrintLine("Test func!");
			};
			auto testFuncWithEntity = [&] (const decs::Entity& entity, const TestComponent& test)
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
					decs::ecsVector<QueryType::BatchIterator> iterators{};
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
					decs::ecsVector<QueryType::BatchIterator> iterators{};
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

			{
				using EntityQuery = decs::MultiQuery<>;

				EntityQuery query{};
				query.AddContainer(&container, true);

				query.ForEach([] ()
				{
					PrintLine("Empty query iteration");
				});

				query.ForEach([] (const decs::Entity& e)
				{
					PrintLine("Empty query iteration with entity");
				});
			}

			container.InvokeEntitesOnDestroyListeners();
		}
	}

	void Test::EntityCreatePerformanceTest()
	{
		const uint32_t testCount = 30;
		const uint32_t entityCount = 100000;

		decs::ContainerConfig config{
			.EntityChunkSize = 10000,
			.DefaultComponentChunkSize = 10000,
			.ArchetypeChunkSize = 100,
		};

		double finalAvarage = 0;
		double finalEntityAvarage = 0;

		decs::Container container{ config };

		decs::ComponentTypeGroup<Position, TestComponent> comps{};
		decs::TagTypeGroup<float, int> tags{};

		size_t testCounter = 0;

		auto perfTest = [&] ()
		{

			double sum = 0;
			for (uint32_t testIdx = 0; testIdx < testCount; testIdx++)
			{
				MeasureTimer timer(true);
				{
					/*for (size_t i = 0; i < entityCount; i++)
					{
						auto e = container.CreateEntity();
						e.AddTag<float>();
						e.AddTag<int>();
						auto position = e.AddComponent<Position>();
						auto testComponent = e.AddComponent<TestComponent>();
					}*/

					//for (uint32_t i = 0; i < entityCount; i++)
					//{
					//	//container.CreateEntity(comps,/* tags, filters,*/ [] (Position& pos, TestComponent& test)
					//	//{

					//	//});
					//	container.CreateEntity(comps, true, [] (Position& pos, TestComponent& test)
					//	{

					//	});
					//}

					/*container.CreateEntities_NoObservers(comps,tags, entityCount, true, [] (Position& pos, TestComponent& test)
					{

					});*/
					container.CreateEntities(comps, tags, entityCount, true, [] (Position& pos, TestComponent& test)
					{

					});

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
			//std::cout << "Lookup table size " << container.GetEntityDataLookupTableSize() << "\n";

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
	}


	void Test::ObserversTest()
	{

		TestComponentObserver testComponentObserver{};
		EntityObserver entityObserver{};

		decs::Container container{};

		 /*{
			auto createObserver = [] (const decs::Entity& e, TestComponent& comp)
			{
				PrintLine("Create observer 1");
			};

			auto createObserver2 = [] (const decs::Entity& e, TestComponent& comp)
			{
				PrintLine("Create observer 2");
			};

			auto destroyObserver = [] (const decs::Entity& e, TestComponent& comp)
			{
				PrintLine("Destroy observer");
			};

			container.AddComponentObserver<TestComponent>(decs::EComponentObserver::Create, createObserver);
			container.AddComponentObserver<TestComponent>(decs::EComponentObserver::Create, createObserver2, -1);
			container.AddComponentObserver<TestComponent>(decs::EComponentObserver::Destroy, destroyObserver);
		}*/


		decs::ObserversManager observersManager{};
		observersManager.AddContainer(&container);

		observersManager.AddObserver<TestComponent, TestComponentObserver>(&testComponentObserver, 0);
		observersManager.AddEntityObserver(&entityObserver);

		auto entity = container.CreateEntity(true);
		entity.AddComponent_NoObserver<TestComponent>();

		container.InvokeEntitesOnCreateListeners();

		//observersManager.RemoveContainer(&container);

		//observersManager.RemoveObserver<TestComponentObserver>(&testComponentObserver);
		//observersManager.RemoveObserver<EntityObserver>(&entityObserver);

		observersManager.RemoveObserver<TestComponentObserver>();
		observersManager.RemoveObserver<EntityObserver>();

		//container.InvokeEntitesOnDestroyListeners();

		entity.Destroy();
	}
}