#include "pch.h"

using namespace std;

struct Obj
{
};
using ObjWeakPtr = weak_ptr<Obj>;
using ObjPtr = shared_ptr<Obj>;

class Cache : public enable_shared_from_this<Cache>
{
public:
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
	mutable unordered_map<string, ObjWeakPtr> m_items;
};

struct DataSource
{
};
using DataSourcePtr = shared_ptr<DataSource>;
using DataSourceWeakPtr = weak_ptr<DataSource>;

struct Data
{
	Data(DataSourcePtr dataSrc)
		: m_dataSrc(move(dataSrc))
	{
	}

private:
	DataSourcePtr m_dataSrc;
};
using DataPtr = shared_ptr<Data>;
using DataWeakPtr = weak_ptr<Data>;

class WeakCache : public enable_shared_from_this<WeakCache>
{
public:
	DataPtr GetData(const DataSourcePtr& dataSrc) const
	{
		Key key{ dataSrc };
		DataPtr data;
		if (auto it = m_items.find(key); it != m_items.end())
		{
			data = it->second.lock();
			assert(data);
		}

		if (!data)
		{
			data.reset(new Data(std::move(dataSrc)),
				[weakSelf = weak_from_this(), key](Data* data) {
					if (auto self = weakSelf.lock())
					{
						self->m_items.erase(key);
					}
					delete data;
				});
			m_items.emplace(key, data);
		}

		return data;
	}

private:
	struct Key
	{
		Key(const DataSourcePtr& p)
			: hash(std::hash<DataSourcePtr>()(p))
			, rawPtr(p.get())
			, wptr(p)
		{
		}
		bool operator==(const Key& rhs) const noexcept
		{
			return rawPtr == rhs.rawPtr;
		}
		size_t hash;
		DataSource* rawPtr;
		DataSourceWeakPtr wptr;
	};

	struct KeyHasher
	{
		size_t operator()(const Key& key) const noexcept
		{
			return key.hash;
		}
	};

	mutable std::unordered_map<Key, DataWeakPtr, KeyHasher> m_items;
};

SCENARIO("Weak cache access")
{
	GIVEN("A weak cache and some data sources")
	{
		auto cache = make_shared<WeakCache>();
		auto ds1 = make_shared<DataSource>();
		auto ds2 = make_shared<DataSource>();

		WHEN("data is retrieved from cache using the same data source")
		{
			auto d1 = cache->GetData(ds1);
			auto d1_1 = cache->GetData(ds1);
			THEN("the same data is returned")
			{
				CHECK(d1 == d1_1);
				AND_THEN("data source is not expired until its data is in use")
				{
					weak_ptr wds1{ ds1 };
					ds1.reset();
					d1.reset();
					CHECK_FALSE(wds1.expired());
					d1_1.reset();
					CHECK(wds1.expired());
				}
			}
		}
		WHEN("data is retrieved from cache using different data sources")
		{
			auto d1 = cache->GetData(ds1);
			auto d2 = cache->GetData(ds2);
			THEN("different data are returned")
			{
				CHECK(d1 != d2);
			}
		}
		WHEN("data is added to cache")
		{
			auto d1 = cache->GetData(ds1);
			THEN("it is not removed from key")
			{
				d1.reset();
			}
		}
	}
}

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
