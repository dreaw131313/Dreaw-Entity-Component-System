#include <iostream>

#include "Test/NormalTest.h"
#include "LightTest/LightTest.h"

int main()
{
	{
		std::cout << "\n";
		std::cout << "/////////////////////////////////////" << "\n";
		std::cout << "///////// LIGHT ECS TEST ////////////" << "\n";
		std::cout << "/////////////////////////////////////" << "\n";
		Light::Test{}.PerformanceTest();
		//Light::Test{}.PerformanceTest();
	}

	/*{
		std::cout << "///////////////////////////////////////////" << "\n";
		std::cout << "///////// PERFORMANCE ECS TEST ////////////" << "\n";
		std::cout << "///////////////////////////////////////////" << "\n";
		Normal::Test{}.Run();
	}*/

	return 0;
}
