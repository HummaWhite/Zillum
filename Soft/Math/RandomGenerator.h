#ifndef RANDOM_GENERATOR_H
#define RANDOM_GENERATOR_H

#include <iostream>
#include <ctime>  

class RandomGenerator
{
public:
	float get()  
	{  
		seed = (a * seed + c) & 0xFFFFFFFFFFFFLL;  
		unsigned int x = seed >> 16;  
		return  ((float)x / (float)m);  
	}

	float get(float _min, float _max)
	{
		return _min + get() * (_max - _min);
	}

	void srand(unsigned int i)  
	{  
		seed  = (((long long int)i) << 16) | rand();  
	}

private:
	const static unsigned long long m;
	const static unsigned long long c;
	const static unsigned long long a;
	static unsigned long long seed;
};

unsigned long long RandomGenerator::seed = 1;
const unsigned long long RandomGenerator::m = 0x100000000LL;
const unsigned long long RandomGenerator::c = 0xB16;
const unsigned long long RandomGenerator::a = 0x5DEECE66DLL;

#endif
