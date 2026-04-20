#include <iostream>

#include "Test/NormalTest.h"
#include "LightTest/LightTest.h"

int main()
{

	//Normal::Test{}.Run();
	{
		std::cout << "\n";
		std::cout << "/////////////////////////////////////" << "\n";
		std::cout << "///////// LIGHT ECS TEST ////////////" << "\n";
		std::cout << "/////////////////////////////////////" << "\n";
		Light::Test{}.Run();
	}

	//{
	//	std::cout << "///////////////////////////////////////////" << "\n";
	//	std::cout << "///////// PERFORMANCE ECS TEST ////////////" << "\n";
	//	std::cout << "///////////////////////////////////////////" << "\n";
	//	Normal::Test{}.PerformanceTest();
	//}

	return 0;
}
