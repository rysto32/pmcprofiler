
#ifndef SHARED_STRING_H
#define SHARED_STRING_H

#include <memory>
#include <string>

class SharedString
{
	std::shared_ptr<std::string> value;
	static std::shared_ptr<std::string> NULL_STR;

public:
	SharedString()
	  : value(NULL_STR)
	{
	}

	SharedString(const std::string &str)
	  : value(new std::string(str))
	{
	}

	SharedString(const char *str)
	  : value(new std::string(str))
	{
	}

	const std::string &operator*() const
	{
		return *value;
	}

	const std::shared_ptr<std::string>& operator->() const
	{
		return value;
	}

	bool operator==(const SharedString &other) const
	{
		if (value == other.value)
			return true;
		return *value == *other.value;
	}
};

#endif
