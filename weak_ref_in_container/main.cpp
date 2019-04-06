#include <memory>
#include <unordered_map>

using namespace std;

struct Obj
{
};


class Cache : enable_shared_from_this<Cache>
{
public:
	using ObjPtr = shared_ptr<Obj>;
	ObjPtr GetItemById(const string& key) const
	{
		ObjPtr obj;
		if (auto item = m_items.find(key);
			item != m_items.end())
		{
			obj = item->second.lock();
		}

		if (!obj)
		{
			obj.reset(new Obj(), [key, self = shared_from_this()](Obj* p) {
				self->m_items.erase(key);
				delete p;
			});
			m_items.insert_or_assign(key, obj);
		}
	}
private:
	using ObjWeakPtr = weak_ptr<Obj>;
	mutable unordered_map<string, ObjWeakPtr> m_items;
};

int main()
{
	return 0;
}
