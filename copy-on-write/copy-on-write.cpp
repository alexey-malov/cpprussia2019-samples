// copy-on-write.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "pch.h"
#include "Cow.h"

struct Base
{
	virtual ~Base() = default;
	virtual std::shared_ptr<Base> Clone() const = 0;
	virtual void SetValue(int value) = 0;
	virtual int GetValue() const = 0;
};

struct Derived : Base
{
	void SetValue(int value) override
	{
		m_val = value;
	}

	int GetValue() const override
	{
		return m_val;
	}

	std::shared_ptr<Base> Clone() const override
	{
		return std::make_shared<Derived>(*this);
	}

private:
	int m_val = 0;
};

struct Shape
{
	virtual ~Shape() = default;
	virtual std::unique_ptr<Shape> clone() const = 0;
	virtual double getArea() const = 0;
};

struct Circle : Shape
{
	explicit Circle(double radius)
		: m_radius(radius)
	{
	}

	std::unique_ptr<Shape> clone() const override
	{
		return std::make_unique<Circle>(*this);
	}

	double getArea() const override
	{
		return M_PI * m_radius * m_radius;
	}

private:
	double m_radius;
};

struct Square : Shape
{
	explicit Square(double size)
		: m_size(size)
	{
	}

	std::unique_ptr<Shape> clone() const override
	{
		return std::make_unique<Square>(*this);
	}

	double getArea() const override
	{
		return m_size * m_size;
	}

private:
	double m_size;
};

template <typename T>
struct Clonable
{
	static std::shared_ptr<T> clone(const T& v)
	{
		return v.clone();
	}
};

template <typename Type, template <class T> class Cloner, typename... Args>
auto MakeCow(Args&&... args)
{
	return Cow<Type, Cloner<Type>>(std::forward<Args>(args)...);
}

int main()
{
	{
		Cow<std::vector<int>> v1(100000u);
		auto v2 = v1;
		v2--->push_back(42);
	}

	{
		Cow<Base> v1(std::make_unique<Derived>());
		auto v2 = v1;
		v2--->SetValue(1);
	}

	{
		Cow<Circle, Clonable<Circle>> circle(10.0);
		Cow<Square, Clonable<Shape>> square(5.0);
		Cow<Shape, Clonable<Shape>> shape = circle;
		shape = square;
		shape = circle;

		shape = MakeCow<Circle, Clonable>(10.0);
	}
}
