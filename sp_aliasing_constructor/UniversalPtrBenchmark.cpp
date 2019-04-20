#include "pch.h"
#include "UniversalPtrBenchmark.h"

//std::minstd_rand0 rnd(static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count()));

unsigned int seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());

unsigned* TestPtr(unsigned* p)
{
	*p += seed;
	return p;
}

std::unique_ptr<unsigned> TestPtr(std::unique_ptr<unsigned> p)
{
	*p += seed;
	return p;
}

std::shared_ptr<unsigned> TestPtr(std::shared_ptr<unsigned> p)
{
	*p += seed;
	return p;
}

UniversalPtr<unsigned> TestPtr(UniversalPtr<unsigned> p)
{
	*p += seed;
	return p;
}
