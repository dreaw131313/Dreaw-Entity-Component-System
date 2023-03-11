# Dreaw-Entity-Component-Systems
**Dreaw-Entity-Component-Systems** in short **decs** it is simple archetype based ECS library written in **C++**.<br/>

## About decs
* Requires C++ 17
* Mainly standard library is used, but hash maps from https://github.com/skarupke/flat_hash_map repository are also used. Which map is used can be changed in Core.h file by changing line with: ``` using ecsMap = std::unordered_map<Key, Value>; ```
* Can store components with stable and unstable pointers


## How to use **decs**
To start using decs, copy the **decs** folder to your project and include the header file **decs.h**.

### Creating and storing entities and components
All entites and components are stored in class **decs::Container**.<br/>
Component classes do not need to inherit from any class. Base types like int, float etc. (except bool) can also be components.<br/>

By default components stored in **decs::Container** do not have **stable memory addresses**, but it can be enforced by using template **decs::Stable< ComponentType >** instead of only **ComponentType**.<br/>

```cpp
class Component1
{
public:
	float X = 0;
	float Y = 0;
	
	Component1();
	Component1(const float& x, const float& y);
};

class Component2
{
public:
	float X = 0;
	float Y = 0;
	
	Component2();
	Component2(const float& x, const float& y);
};

int main()
{
	decs::Container container = {};
	decs::Entity entity1 = container.CreateEntity();
	// using entity member function :
	Component1* c1 = entity1.AddComponent<Component1>(1.f,2.f);
	Component2* c2 = entity1.AddComponent<decs::Stable<Component2>>(3.f,4.f);
	entity1.RemoveComponent<Component1>();
	entity1.RemoveComponent<decs::Stable<Component2>>();
	entity1.Destroy()
	
	return 0;
}
```

Like in most of ecs systems, in **decs** entity can have only one component of given type. If component of the same type will be added twice AddComponent function will return pointer to component created earlier.<br/>

### Entity class
decs::Entity class is represented by ID and reference to decs::Container in which was created.<br/>

Entites can also be spawned which is a little faster than creating them by regular method. To spawn entity you need to use one of **Spawn()** methods from decs::Container class.
```cpp
Entity Spawn(const Entity& prefab, const bool& isActive = true);

bool Spawn(
	const Entity& prefab, 
	std::vector<Entity*>& spawnedEntities, 
	const uint64_t& spawnCount, 
	const bool& areActive = true
	);
	
bool Spawn(const Entity& prefab, const uint64_t& spawnCount, const bool& areActive = true);
```
As prefab parameter can be used entity from any decs::Container.

Entities can also be activated or deactivated. Deactivated entities will not be iterated. Activating and deactivating is performed by functions:
```cpp
decs::Container container = {};
decs::Entity entity = container.CreateEntity();
entity.SetActive(true);
bool isActive = entity.IsActive();
```

### Iterating over entites
**decs::Query<typename... Components>** object serves to iterating over entities. It has 3 methods for iteration:
```cpp
// Iterates over entities in archetypes from first to last
template<typename Callable
Query::ForEachForward(Callable&& func);

// Iterates over entities in archetypes form last to first
template<typename Callable
Query::ForEachBackward(Callable&& func);

// It works exacly like ForEachBackward
template<typename Callable
Query::ForEach(Callable&& func);

```

```cpp
decs::Container container = {}; 

// this query can iterate over all entities which contains components passed as template parameters
decs::Query<Component1, decs::Stable<Component2>> query = { container }; 

query.ForEach([](Component1& c1, Component2& c2)
{
	// doing stuff with components
});

// if during iteration entity reference is needed, decs::Entity can be placed as first parameter of passed callable to ForEach function 
query.ForEach([](decs::Entity& e, Component1& c1, Component2& c2)
{
	// doing stuff with components and entity
});
```
It is also possible to query for more complex queries by using the member methods of the **decs::Query** class:
```cpp
template<typename... ComponentsTypes>
Query& Without(); // Entities in query will not have all components from ComponetsTypes parameters list
```
```cpp
template<typename... ComponentsTypes>
Query& WithAnyFrom(); // Entities in query will have at least one of component from ComponentTypes parameters list
```
```cpp
template<typename... ComponentsTypes>
Query& With(); // Entities in query will have all components from ComponentTypes parameters list
```

Creating Query with this methods can look like:
```cpp
decs::Query<Component1, decs::Stable<Component2>, Component3> query = { container };
query.Without<Component4, Component5>().WithAnyFrom<Component6, Component7>().With<Component8, Component9>();

query.ForEach([](Component1& c1, Component2& c2, Component3& c3)
{
	// doing stuff with components
});

query.ForEach([](decs::Entity& e, Component1& c1, Component2& c2, Component3& c3)
{
	// doing stuff with components and entity
});
```

During iteration with methods **ForEach** and **ForEachBackward** is possible:
* create new entites. 
* adding and removing components from currently iterated entity
* destroying currently iterated entity

Things like:
* destroying other entites
* adding and removing component from entities different than currently iterated entity

are undefined behavior.

During Iteration with **ForEachForward** method component setups of existing entities can not be changed and existing entities cannot be destroyed. New entities can be created, them component setup can be edited and destroy them.

**Query** can be used to iterate from multiple threads. To be able to iterate from multiple threads, first we need create batch iterators from **Query** with method:
```cpp
void CreateBatchIterators(std::vector<BatchIterator>& iterators, uint64_t desiredBatchesCount, uint64_t minBatchSize);
```
* **desiredBatchesCount** - number of maximum batch iterators which can be created for this query
* **minBatchSize** is minimal number of enttites in each iterator. 

If number of entities in query is less than **desiredBatchesCount** * **minBatchSize** then function will generate smaller number of iterators witch will contain max **minBatchSize** count of entities.

```cpp
using QueryType = decs::Query<Component1>;
QueryType query = { container };
std::vector<QueryType::BatchIterator> iterators;
query.CreateBatchIterators(iterators, 1000, 8);
for (auto& it : iterators)
{
	it.ForEach(lambda); // can be called from diffrent threads
}
```
During iteration over **Query::BatchIterator** with **ForEach** method, the same rules applay as when iterating with **ForEachForward** method of **Query** class.


