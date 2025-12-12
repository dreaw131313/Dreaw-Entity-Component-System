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
		PrintLine("TestComponent::TestComponent");
	}

	TestComponent(const TestComponent& other)
	{
		PrintLine("TestComponent::TestComponent(const TestComponent&)");
	}

	~TestComponent()
	{
		PrintLine("TestComponent::~TestComponent");
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

int main()
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

	decs::Entity prefab{};
	{
		decs::ContainerConfig containerConfig{
			.EntityChunkSize = 1000,
			.DefaultComponentChunkSize = 200,
			.ArchetypeChunkSize = 200,
		};
		decs::Container container = { containerConfig };
		observerManager.FillContainerObservers(container);

		prefab = container.CreateEntity();

		if (prefab)
		{

		}

		decs::ConstEntity constPrefab = prefab;
		if (constPrefab)
		{

		}

		prefab.AddComponent<TestComponent>();


		prefab.AddComponent<Renderer>();
		prefab.AddTag<FloatTag>();
		prefab.AddTag<IntTag>();
		prefab.AddTag<BoolTag>();

		prefab.AddComponent<Position>(10.f, 10.f);
		auto position = prefab.GetComponent<const Position>();

		//prefab.RemoveComponent<Renderer>();
		prefab.RemoveComponent_If<Renderer>([](const Renderer& renderer)
		{
			return true;
		});
		prefab.RemoveTag<IntTag>();

		auto comp = prefab.GetComponent<TestComponent>();

		container.Spawn(prefab, 10, true);

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

	return 0;
}
