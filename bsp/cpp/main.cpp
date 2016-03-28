#include "bsp.h"

int main()
{
	pmg::BSP bsp(100, 100, 6, 10, 20);

	bsp.createMap();

	bsp.toTextFile("test.txt");

	return 0;
}