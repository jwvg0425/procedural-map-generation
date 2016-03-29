#include "../src/pmg.h"

int main()
{
	pmg::BSP generator(100, 100, 6, 5, 10);

	generator.createMap();

	generator.toTextFile("test.txt");

	return 0;
}