// shared_ptr_custom_deleter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"

struct Foo
{
};

using namespace std;

int main()
{
	shared_ptr<Foo> foo(new Foo(), [](Foo* p) {
		delete p;
		cout << "Foo has been deleted\n";
	});
}
