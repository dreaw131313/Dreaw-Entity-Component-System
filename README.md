# Dreaw-Entity-Component-System
**Dreaw-Entity-Component-System** in short **decs** it is simple ecs static library written in **C++** which I am developing for educational purposes and for use in my own game engine<br/>

## About decs
* Requires C++ 17.
* Every class in **decs** is in "decs" namespace
* Mainly use standard library, but also is using hash maps from https://github.com/skarupke/flat_hash_map repository. Which map is used can be changed in Core.h file by simple typdef.

## How to use **decs**
### Creating and storing entities and components
All entites and components are stored in class decs::Container which is giving access for method for creating and destroying entiteis and components. Component classes do not need to inherit from any class. Base types like int, float etc. can also be componenets.

```C++

	class Component1
	{
	public:
		float X = 0;
		float Y = 0;

	public:
		Component1()
		{

		}

		Component1(const float& x, const float& y) :
			X(x), Y(y)
		{

		}
	};

	class Component2
	{
	public:
		float X = 0;
		float Y = 0;

	public:
		Component2()
		{

		}

		Component2(const float& x, const float& y) :
			X(x), Y(y)
		{

		}

	};

  int main()
  {
    decs::Container container = {};
    decs::Entity entity1 = container.CreateEntity();
    // using entity member function :
    Component1* c1 = entity1.AddComponent<Component1>(1.f,2.f);
    Component2* = entity1.AddComponent<Component2>(3.f,4.f);
    entity1.RemoveComponent<Component2>();
    entity1.Destroy()
    
    // or container functions:
    decs::Entity entity2 = container.CreateEntity();
    container.AddComponent<Component1>(entity2, 1.f, 2.f);
    container.AddComponent<Component2>(entity2, 1.f, 2.f);
    container.RemoveComponent<Component2>(entity2);
    container.DestroyEntity(entity2);
  }
```

Like in most of ecs system, in **decs** entity can have only one component of given type. If component of the same type will be added twice AddComponent function will return pointer to firstly created component.<br/>

###decs::Entity class
decs::Entity class is represented by ID and reference to decs::Container in which was created.<br/>

Entites can also be spawned which is a little faster than creating them by regular method. To spawn entity you need to use one of **Spawn()** methods from decs::Container class.
```C++

	Entity Spawn(const Entity& prefab, const bool& isActive = true);
	bool Spawn(const Entity& prefab, std::vector<Entity>& spawnedEntities, const uint64_t& spawnCount, const bool& areActive = true);
	bool Spawn(const Entity& prefab, const uint64_t& spawnCount, const bool& areActive = true);

```
As prefab parameter can be used entity from any decs::Container, but if it this container is not in sync* with container in which we are trying spawn, **Spawn** method will return false or invalid **decs::Entity** object.<br/>

*Containers are in sync when they contains internal containers for components of the same type (syncing containers will be implemented in near future)

### Iterating over entites


