
#ifndef SHARED_STRING_H
#define SHARED_STRING_H

#include <assert.h>
#include <memory>
#include <string>

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

	static SharedString NULL_STR;

	void Init(const char *str)
	{
		Drop();
		value = new StringValue(str);
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
			delete value;
		value = NULL;
	}

public:
	SharedString()
	  : value(NULL)
	{
		Copy(NULL_STR);
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
};

#endif
