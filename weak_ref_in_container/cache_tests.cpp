#include "pch.h"

using namespace std;

struct Obj
{
};

class Cache : public enable_shared_from_this<Cache>
{
public:
	using ObjPtr = shared_ptr<Obj>;
	ObjPtr GetObjectById(const string& key) const
	{
		ObjPtr obj;
		if (auto item = m_items.find(key);
			item != m_items.end())
		{
			obj = item->second.lock();
		}

		if (!obj)
		{
			obj.reset(new Obj(), [key, weakSelf = weak_from_this()](Obj* p) {
				if (auto self = weakSelf.lock())
				{
					self->m_items.erase(key);
				}
				delete p;
			});
			m_items.insert_or_assign(key, obj);
		}

		return obj;
	}

private:
	using ObjWeakPtr = weak_ptr<Obj>;
	mutable unordered_map<string, ObjWeakPtr> m_items;
};

TEST_CASE("Cache access")
{
	auto cache = make_shared<Cache>();
	auto obj1 = cache->GetObjectById("obj1"s);
	auto obj2 = cache->GetObjectById("obj2"s);
	auto obj1alias = cache->GetObjectById("obj1"s);
	CHECK(obj1 != obj2);
	CHECK(obj1alias == obj1);
	weak_ptr wobj1{ obj1 };
	obj1.reset();
	CHECK(!wobj1.expired());
	obj1alias.reset();
	CHECK(wobj1.expired());
}