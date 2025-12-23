#include <iostream>

#include "Test/NormalTest.h"
#include "LightTest/LightTest.h"

int main()
{
	/*
	std::cout << "//////////////////////////////////////" << "\n";
	std::cout << "///////// NORMAL ECS TEST ////////////" << "\n";
	std::cout << "//////////////////////////////////////" << "\n";

	Normal::Test{}.Run();
	*/

	std::cout << "\n";
	std::cout << "/////////////////////////////////////" << "\n";
	std::cout << "///////// LIGHT ECS TEST ////////////" << "\n";
	std::cout << "/////////////////////////////////////" << "\n";

	Light::Test{}.Run();

	return 0;
}
