#include <iostream>

#include "Test/NormalTest.h"
#include "LightTest/LightTest.h"

int main()
{
	{
		std::cout << "/////////////////////////////////////" << "\n";
		std::cout << "///////// LIGHT ECS TEST ////////////" << "\n";
		std::cout << "/////////////////////////////////////" << "\n";
		Light::Test{}.PerformanceTest();
	}

	{
		std::cout << "///////////////////////////////////////////" << "\n";
		std::cout << "///////// NORMAL ECS TEST ////////////" << "\n";
		std::cout << "///////////////////////////////////////////" << "\n";
		Normal::Test{}.PerformanceTest();
	}

	return 0;
}
