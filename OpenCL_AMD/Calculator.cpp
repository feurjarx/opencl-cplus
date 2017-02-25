#include "stdafx.h"
#include "Calculator.h"
#include <malloc.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <omp.h>

using namespace std;

/*
	Params: 
		sizeMemory	- размер выделяемой памяти в байтах, 
		isRandom	- заполнение случайными числами (true) или нулями (false)
		constraint  - ограничение random'а
*/
Calculator::Calculator(int sizeMemory, double &interval, int constraint) {
	double startTime;
	startTime = omp_get_wtime();

	this->lengthNumberSet = sizeMemory / sizeof(big);
	this->numberSet = new big[this->lengthNumberSet];

	#pragma omp parallel for
	for (int i = 0; i < this->lengthNumberSet; i++) {
		numberSet[i] = rand() % constraint + 1;
	}

	interval = omp_get_wtime() - startTime;
}

Calculator::Calculator(big* _array, int N) {
	this->lengthNumberSet = N;
	this->numberSet = _array;
}

big* Calculator::getNumberSet() {
	return this->numberSet;
}

void Calculator::setNumberSet(big* _array, int N) {
	this->lengthNumberSet = N;
	this->numberSet = _array;
}

big _gcd(big a, big b) { 
	for (;;)
    {
        if (a == 0) return b;
        b %= a;
        if (b == 0) return a;
        a %= b;
    }
}

big _lcm(big a, big b) {// ~ O(log_{a/a%b}(ab)
	big temp = _gcd(a, b);
	return temp ? (a / temp * b) : 0;
}

big Calculator::lcmThroughCycle() { // O(log_{a/a%b}(ab)) * O(n)
	big result = 1;
	for (int i = 0; i < this->lengthNumberSet; i++) {
		result = _lcm(result, this->numberSet[i]);
	}
	return result;
}
