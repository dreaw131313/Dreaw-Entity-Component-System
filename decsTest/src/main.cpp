#include <iostream>

#include "Test/NormalTest.h"
#include "LightTest/LightTest.h"

int main()
{
	Light::Test{}.Run();
	std::cout << "\n";
	Normal::Test{}.Run();

	return 0;
}
