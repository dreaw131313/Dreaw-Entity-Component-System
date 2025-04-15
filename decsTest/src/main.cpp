#include <iostream>
#include <format>

#include "decs/decs.h"

void PrintLine(std::string message = "")
{
	std::cout << message << "\n";
}


struct Position : public decs::ComponentBase
{
public:
	float X = 0;
	float Y = 0;

public:
	Position()
	{
		PrintLine("Position constructior");
	}

	Position(float x, float y) : X(x), Y(y)
	{
		PrintLine("Position constructior");
	}

	void TestFunc(int& i)
	{
		PrintLine("Is working");
		//i += 1;
	}

protected:
	void OnPreCreate(const decs::Entity& entity) override final
	{
		PrintLine(std::format("Position X: {0}, Y: {1}", X, Y));
	}
};

struct TestComponent : public decs::ComponentBase
{
public:
	int table[10];

	TestComponent()
	{
		std::cout << "TestComponent constructor " << this << "\n";
	}

	~TestComponent()
	{
		std::cout << "TestComponent destructor " << this << "\n";
	}

};

struct Renderer :public decs::ComponentBase
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
		PrintLine("TestComponent on create");
	}

	// Inherited via DestroyComponentObserver
	void OnDestroyComponent(TestComponent& component, const decs::Entity& entity) override
	{
		PrintLine("TestComponent on destroy");
	}


	// Inherited via EnableComponentObserver
	void OnEnableComponent(TestComponent& component, const decs::Entity& entity) override
	{
		//PrintLine("TestComponent on enable");

		std::cout << "TestComponent on enable " << &component << "\n";
	}


	// Inherited via DisableComponentObserver
	void OnDisableComponent(TestComponent& component, const decs::Entity& entity) override
	{
		//PrintLine("TestComponent on disable");

		std::cout << "TestComponent on disable " << &component << "\n";
	}
};

int main()
{
	TestComponetObserver testComponentObserver = {};

	decs::ObserversManager observerManager = {};
	{
		observerManager.SetComponentObservers(&testComponentObserver, &testComponentObserver, &testComponentObserver, &testComponentObserver);
	}

	decs::Container container = {};
	decs::Container secondContainer = {};

	observerManager.FillContainerObservers(container);

	{
		auto entity = container.CreateEntity(false);
		auto test = entity.AddComponent<TestComponent>();

		entity.SetActive(true);
		entity.SetActive(false);

		entity.Destroy();
	}


	return 0;
}
