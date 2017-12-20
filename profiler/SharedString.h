
#ifndef SHARED_STRING_H
#define SHARED_STRING_H

#include <assert.h>
#include <memory>
#include <string>
#include <unordered_map>

class SharedString
{
	struct StringValue
	{
		std::string str;
		int count;

		StringValue(const char *s)
		  : str(s), count(1)
		{
		}
	};

	StringValue *value;

	typedef std::unordered_map<std::string, StringValue*> InternMap;

	// To avoid static initialization/deinitialization issues, we make the
	// intern_map a pointer that is allocated on first use and never
	// deallocated.
	static InternMap *GetInternMap()
	{
		static InternMap *intern_map;

		if (intern_map == NULL)
			intern_map = new InternMap;
		return intern_map;
	}

	static StringValue *Intern(const char *str)
	{
		InternMap::iterator it = GetInternMap()->find(str);

		if (it != GetInternMap()->end()) {
			//printf("String '%s' interned\n", str);
			it->second->count++;
			return it->second;
		}

		StringValue *val = new StringValue(str);
		GetInternMap()->insert(std::make_pair(val->str, val));
		//printf("String '%s' create\n", str);
		return val;
	}

	static void Destroy(StringValue *str)
	{
		GetInternMap()->erase(str->str);
		//printf("String '%s' destroy\n", str->str.c_str());
		delete str;
	}

	void Init(const char *str)
	{
		Drop();
		value = Intern(str);
// 		fprintf(stderr, "%p: Init SharedString for '%s'; count=%d\n",
// 			value, str, value->count);
	}

	void Copy(const SharedString &other)
	{
		Drop();

		value = other.value;
		assert(value->count > 0);
		++value->count;
// 		fprintf(stderr, "%p: Add ref; count=%d\n",
// 			value, value->count);
	}

	void Drop()
	{
		if (value == NULL)
			return;

// 		fprintf(stderr, "%p: Drop ref; count=%d\n",
// 			value, value->count);
		assert(value->count > 0);
		--value->count;
		if (value->count == 0)
			Destroy(value);
		value = NULL;
	}

public:
	SharedString()
	  : value(NULL)
	{
		Init("");
	}

	SharedString(const std::string &str)
	  : value(NULL)
	{
		Init(str.c_str());
	}

	SharedString(const char *str)
	  : value(NULL)
	{
		Init(str);
	}

	SharedString(const SharedString &other)
	  : value(NULL)
	{
		Copy(other);
	}

	SharedString(SharedString &&other) noexcept
	  : value(other.value)
	{
		other.value = NULL;
	}

	~SharedString()
	{
		Drop();
	}

	const std::string &operator*() const
	{
		return value->str;
	}

	const std::string *operator->() const
	{
		return &value->str;
	}

	bool operator==(const SharedString &other) const
	{
		if (value == other.value)
			return true;
		return value->str == other.value->str;
	}

	SharedString &operator=(const SharedString &other)
	{
		Copy(other);
		return *this;
	}

	SharedString &operator=(SharedString &&other)
	{
		value = other.value;
		other.value = NULL;
		return *this;
	}
};

#endif
