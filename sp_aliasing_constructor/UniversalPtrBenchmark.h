#pragma once
#include "UniversalPtr.h"

unsigned* TestPtr(unsigned* p);
std::unique_ptr<unsigned> TestPtr(std::unique_ptr<unsigned> p);
std::shared_ptr<unsigned> TestPtr(std::shared_ptr<unsigned> p);
UniversalPtr<unsigned> TestPtr(UniversalPtr<unsigned> p);