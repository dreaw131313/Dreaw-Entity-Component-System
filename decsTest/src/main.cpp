#include <iostream>

#include "decs/decs.h"


class C1
{
public:
	float X;
	float Y;

public:
	C1(float x, float y) :X(x), Y(y)
	{

	}
};

template<typename... Components>
class Job
{
	using ViewType = decs::View<Components...>;
	using Iterator = typename ViewType::BatchIterator;

public:
	virtual void Update(Components&... args) = 0;

	inline void UpdateInternal(Iterator& it)
	{
		it.ForEach([this] (Components&... args)
		{
			Update(std::forward<Components&>(args)...);
		});
	}
};
template<typename... Components>
class Job<decs::Entity, Components...>
{
	using ViewType = decs::View<Components...>;
	using Iterator = typename ViewType::BatchIterator;

public:
	virtual void Update(decs::Entity& entity, Components&... args) = 0;

	inline void UpdateInternal(Iterator& it)
	{
		it.ForEachWithEntity([this] (decs::Entity& entity, Components&... args)
		{
			Update(entity, std::forward<Components&>(args)...);
		});
	}
};

template<typename... Components>
class Job<const decs::Entity, Components...>
{
	using ViewType = decs::View<Components...>;
	using Iterator = typename ViewType::BatchIterator;

public:
	virtual void Update(const decs::Entity& entity, Components&... args) = 0;

	inline void UpdateInternal(Iterator& it)
	{
		it.ForEachWithEntity([this] (decs::Entity& entity, Components&... args)
		{
			Update(entity, std::forward<Components&>(args)...);
		});
	}
};


class CustomJob : public Job<C1>
{
public:
	virtual void Update(C1& c1)  override
	{
		std::cout << "Custom job" << std::endl;
	}
};
class CustomJobWithEntity : public Job<const decs::Entity, C1>
{
public:
	virtual void Update(const decs::Entity& entity, C1& c1)  override
	{
		std::cout << "Custom job with entity" << std::endl;
	}
};

int main()
{
	std::cout << "Hello world" << std::endl;

	decs::Container container = {};

	decs::Entity entity = container.CreateEntity();
	entity.AddComponent<C1>(1.f, 2.f);

	using ViewType = decs::View<C1>;
	decs::View<C1> view = { container };

	view.ForEach([] (C1& c)
	{
		c.X += 1;
	std::cout << "Hello world" << std::endl;
	});

	CustomJob job = {};
	std::vector<ViewType::BatchIterator> iterators;
	view.CreateBatchIterators(iterators, 10, 1000);
	for (auto& it : iterators)
	{
		job.UpdateInternal(it);
	}

	CustomJobWithEntity jobWithEntity = {};
	for (auto& it : iterators)
	{
		jobWithEntity.UpdateInternal(it);
	}


	std::cin.get();
	return 0;
}