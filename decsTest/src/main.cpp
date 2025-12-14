#include <iostream>
#include <format>

#include "decs/decs.h"

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

void NormalTest()
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

		decs::ComponentTypeGroup<TestComponent, Position> componetns{};
		decs::TagTypeGroup<FloatTag, IntTag, BoolTag> tags{};

		auto initFunc = [](const decs::Entity& e, TestComponent& component, Position& position)
		{
			PrintLine("Init from helepr create entity func!");
		};

		decs::Entity e = prefabContainer.CreateEntity(componetns, tags, true, initFunc);
	}

	{
		decs::Container container = { containerConfig };
		observerManager.FillContainerObservers(container);

		container.Spawn(prefab, true);
		container.Spawn(prefab, 9, true);

		uint32_t counter = 0;
		auto testFunc = [&](const TestComponent& test)
		{
			PrintLine("Test func!");
		};
		auto testFuncWithEntity = [&](const decs::Entity& entity, const TestComponent& test)
		{
			PrintLine(std::format("Entity: {0} TestComponent", entity.GetID()));
		};


		if (false)
		{
			using QueryType = decs::Query< const TestComponent>;
			QueryType query(&container);

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

		{
			decs::TypeGroup<int, float> t{};

			auto st = t;

			using QueryType = decs::MultiQuery<const TestComponent>;
			QueryType query{};
			query.WithAny<FloatTag, IntTag>();
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
