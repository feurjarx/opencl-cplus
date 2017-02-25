#include <iostream>
#include <numeric>
#include <omp.h>
#include <time.h>
#include <limits>


#pragma once
using namespace std;

typedef unsigned __int64 big;

class Calculator
{
private:
	big* numberSet;
public:
	int lengthNumberSet;
	Calculator(int sizeMemory, double &interval, int constraint = 1000);
	Calculator(big* _array, int N);

	big* Calculator::getNumberSet();

	void Calculator::setNumberSet(big* _array, int N);

	big Calculator::lcmThroughCycle();

	~Calculator(void);
};

