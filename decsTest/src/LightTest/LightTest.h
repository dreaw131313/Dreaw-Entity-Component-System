#pragma once

namespace Light
{

	class Test
	{
	public:
		void Run();
	private:

		void IterationTest();

		void ComponentCreationTest();

		void EntityCreatePerformanceTest();

		void FilterTest();

		void QueryManagerTest();

		void RemovingArchetypesTest();

		void ObserversTest();
	};

}