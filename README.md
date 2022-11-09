# Dreaw-Entity-Component-System
**Dreaw-Entity-Component-System** in short **decs** it is simple ecs static library written in **C++** which I am developing for educational purposes and for use in my own game engine<br/>

## About decs
* Requires C++ 17.
* Every class in **decs** is in "decs" namespace
* Mainly use standard library, but also is using hash maps from https://github.com/skarupke/flat_hash_map repository. Which map is used can be changed in Core.h file by simple typdef.

## How to use **decs**
### Creating and storing entities and components
All entites and components are stored in class decs::Container which is giving access for method for creating and destroying entiteis and components.


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
    decs::Entitiy entity = container.CreateEntity();
    // using entity member function :
    entity.AddComponent<Component1>(1.f,2.f);
    entity.AddComponent<Component2>(3.f,4.f);
    entity.RemoveComponent<Component2>();
    entity.Destroy()
    
    // or container functions:
    decs::Entitiy entity2 = container.CreateEntity();
    container.AddComponent<Component1>(entity2, 1.f, 2.f);
    container.AddComponent<Component2>(entity2, 1.f, 2.f);
    container.RemoveComponent<Component2>(entity2);
    container.DestroyEntity(entity2);
  }
```
