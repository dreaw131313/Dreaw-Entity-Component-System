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

			uint64_t queryEntityCount = query.GetEntityCount();

			std::vector<QueryType::BatchIterator> iterators{};
			query.CreateBatchIterators(iterators, 10, 3);

			/*
			PrintLine();
			for (auto& it : iterators)
			{
				it.ForEach(testFunc);
			}
			PrintLine();
			for (auto& it : iterators)
			{
				it.ForEach_IngoreEntityActiveState(testFunc);
			}
			*/
		}

		{
			using QueryType = decs::MultiQuery<const TestComponent>;
			QueryType multiQuery{};
			multiQuery.WithAny<FloatTag, IntTag>();
			multiQuery.AddContainer(&container);

			PrintLine("ForEach");
			multiQuery.ForEach(testFunc);
			PrintLine("ForEach With Entity");
			multiQuery.ForEach(testFuncWithEntity);
			PrintLine("ForEach Safe");
			multiQuery.ForEach_Safe(testFunc);
			PrintLine("ForEach With Entity Safe");
			multiQuery.ForEach_Safe(testFuncWithEntity);
			PrintLine("ForEachBackward");
			multiQuery.ForEachBackward(testFunc);
			PrintLine("ForEachBackward With Entity");
			multiQuery.ForEachBackward(testFuncWithEntity);
			PrintLine("ForEachBackward Safe");
			multiQuery.ForEachBackward_Safe(testFunc);
			PrintLine("ForEachBackward With Entity Safe");
			multiQuery.ForEachBackward_Safe(testFuncWithEntity);
			PrintLine("ForEach_IngoreEntityActiveState");
			multiQuery.ForEach_IngoreEntityActiveState(testFunc);
			PrintLine("ForEach_IngoreEntityActiveState With Entity");
			multiQuery.ForEach_IngoreEntityActiveState(testFuncWithEntity);

			/*std::vector<QueryType::BatchIterator> iterators{};
			multiQuery.CreateBatchIteratorsWithMaxNumberPerBatch(iterators, 7);

			PrintLine();
			for (auto& it : iterators)
			{
				it.ForEach(testFunc);
			}

			PrintLine();
			for (auto& it : iterators)
			{
				it.ForEach_IngoreEntityActiveState(testFunc);
			}*/
		}

	}

	return 0;
}
