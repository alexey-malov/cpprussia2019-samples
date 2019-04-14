#include "pch.h"

using namespace std;

class Obj
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

/*
class WeakCache : public enable_shared_from_this<WeakCache>
{
public:
	DataPtr GetData(const DataSourcePtr& dataSrc) const
	{
		DataPtr data;
		if (auto it = m_items.find(dataSrc); it != m_items.end())
		{
			data = it->second.lock();
			assert(data);
		}

		if (!data)
		{
			data.reset(new Data(dataSrc),
				[weakSelf = weak_from_this(), dataSrc](Data* data) {
					if (auto self = weakSelf.lock())
					{
						self->m_items.erase(dataSrc);
					}
					delete data;
				});
			m_items.emplace(dataSrc, data);
		}

		return data;
	}

private:
	mutable std::unordered_map<DataSourcePtr, DataWeakPtr> m_items;
};
*/

template <typename Key, typename Val, typename Hasher = hash<Key>, typename KeyEq = equal_to<Key>>
class CacheT : public enable_shared_from_this<CacheT<Key, Val>>
{
public:
	using MyType = CacheT<Key, Val, Hasher, KeyEq>;
	using ValuePtr = shared_ptr<Val>;
	using ValueWeakPtr = weak_ptr<Val>;
	using CacheCleaner = function<void(const Key& key)>;
	using ValueFactory = function<ValuePtr(const Key& key, CacheCleaner d)>;

	CacheT(ValueFactory valueFactory)
		: m_valueFactory(move(valueFactory))
	{
	}

	ValuePtr GetValue(const Key& key) const
	{
		ValuePtr value;
		if (auto it = m_items.find(key); it != m_items.end())
		{
			value = it->second.lock();
		}

		if (!value)
		{
			value = m_valueFactory(key,
				[weakSelf = MyType::weak_from_this()](const auto& key) mutable {
					if (auto self = weakSelf.lock())
						self->m_items.erase(key);
					weakSelf.reset();
				});
			m_items.emplace(key, value);
		}
		return value;
	}

private:
	mutable std::unordered_map<Key, ValueWeakPtr> m_items;
	ValueFactory m_valueFactory;
};

class WeakCache : public CacheT<DataSourcePtr, Data>
{
public:
	WeakCache()
		: CacheT([](const DataSourcePtr& key, auto&& cleaner) {
			return shared_ptr<Data>(new Data(key), [cleaner = move(cleaner), key](Data* d) {
				cleaner(key);
				delete d;
			});
		})
	{
	}
};

namespace detail
{

} // namespace detail

class DestructionObservable
{
public:
	template <typename Handler>
	void AddDestructionHandler(Handler&& handler) const
	{
		m_destructionObservers.emplace_back(forward<Handler>(handler));
	}

protected:
	~DestructionObservable() noexcept
	{
		for (auto&& handler : m_destructionObservers)
		{
			try
			{
				handler();
			}
			catch (...)
			{
			}
		}
	}

private:
	mutable vector<function<void()>> m_destructionObservers;
};

/*
template <typename T>
class SharedPtr
{
public:
	constexpr SharedPtr() noexcept = default;

	constexpr SharedPtr(nullptr_t) noexcept
	{
	}

	SharedPtr(SharedPtr&&) = default;

	template <typename U>
	explicit SharedPtr(U* u)
		: m_wrapper(u)
	{
	}

	template <class U>
	SharedPtr& operator=(const SharedPtr<U>& rhs) noexcept
	{
		SharedPtr(rhs).swap(*this);
		return *this;
	}

	SharedPtr& operator=(const SharedPtr&) noexcept = default;

	template <class U>
	SharedPtr& operator=(SharedPtr<U>&& rhs) noexcept
	{
		SharedPtr(move(rhs)).swap(*this);
		return *this;
	}

	SharedPtr& operator=(SharedPtr&&) noexcept = default;

	template <class _Ux,
		class _Dx>
	SharedPtr& operator=(unique_ptr<_Ux, _Dx>&& _Right)
	{ // move from unique_ptr
			SharedPtr(_STD move(_Right)).swap(*this);
		return (*this);
	}


	void swap(SharedPtr& other)
	{
		m_wrapper.swap(other.m_wrapper);
	}


private:
	template <typename T, typename... Args>
	friend SharedPtr<T> MakeShared(Args&&... args);

	void SetWrapper(std::shared_ptr<detail::Wrapper<T>> w)
	{
		m_wrapper = move(w);
	}
	std::shared_ptr<detail::Wrapper<T>> m_wrapper;
};

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args)
{
	auto wrapper = make_shared<detail::Wrapper<T>>(forward<Args>(args)...);
	SharedPtr<T> p;
	p.SetWrapper(move(wrapper));
	return p;
}
*/

template <typename Key, typename Val>
class WeakMap : public enable_shared_from_this<WeakMap<Key, Val>>
{
public:
	using MyType = WeakMap<Key, Val>;
	using KeyPtr = shared_ptr<const Key>;

	optional<Val> TryGetValue(const KeyPtr& key) const
	{
		if (auto it = m_items.find(key.get()); it != m_items.end())
		{
			return it->second;
		}
		return nullopt;
	}

	template <typename V>
	void SetValue(const KeyPtr& key, V&& value)
	{
		WeakKey wkey{ key.get() };
		if (auto it = m_items.find(wkey); it != m_items.end())
		{
			it->second = value;
		}
		else
		{
			WeakMapAddDestructionHandler(*key,
				[weakSelf = MyType::weak_from_this(), wkey]() mutable {
					if (auto self = weakSelf.lock())
						self->m_items.erase(wkey);
					weakSelf.reset();
				});

			m_items.emplace(wkey, forward<V>(value));
		}
	}

private:
	struct WeakKey
	{
		WeakKey(const Key* key)
			: key(key)
			, hash(std::hash<const Key*>()(key))
		{
		}
		bool operator==(const WeakKey& rhs) const
		{
			return key == rhs.key;
		}
		const Key* key;
		size_t hash;
	};

	struct Hasher
	{
		size_t operator()(const WeakKey& key) const
		{
			return key.hash;
		}
	};
	mutable unordered_map<WeakKey, Val, Hasher> m_items;
};

struct FooObservable : DestructionObservable
{
};

template <typename Handler>
void WeakMapAddDestructionHandler(const DestructionObservable& d, Handler&& h)
{
	d.AddDestructionHandler(forward<Handler>(h));
}

SCENARIO("Weak key in a map")
{
	GIVEN("a WeakMap")
	{
		auto wm = make_shared<WeakMap<FooObservable, int>>();

		auto k = make_shared<FooObservable>();
		CHECK(!wm->TryGetValue(k));

		wm->SetValue(k, 42);
		CHECK(wm->TryGetValue(k).value_or(0) == 42);
	}
}

SCENARIO("Template cache test")
{
	CacheT<string, string> cache([](const string& key, auto&& cleaner) {
		return shared_ptr<string>(new string("value for:" + key),
			[cleaner = std::move(cleaner), key](string* s) {
				cleaner(key);
				delete s;
			});
	});
	CHECK(*cache.GetValue("one") == "value for:one");
}

SCENARIO("Weak cache access")
{
	GIVEN("A weak cache and some data sources")
	{
		auto cache = make_shared<WeakCache>();
		auto ds1 = make_shared<DataSource>();
		auto ds2 = make_shared<DataSource>();

		WHEN("data is retrieved from cache using the same data source")
		{
			auto d1 = cache->GetValue(ds1);
			auto d1_1 = cache->GetValue(ds1);
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
			auto d1 = cache->GetValue(ds1);
			auto d2 = cache->GetValue(ds2);
			THEN("different data are returned")
			{
				CHECK(d1 != d2);
			}
		}
		WHEN("data is added to cache")
		{
			auto d1 = cache->GetValue(ds1);
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
