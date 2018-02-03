// Copyright (c) 2015 Ryan Stone.  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
// OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
// SUCH DAMAGE.

#ifndef SHARED_STRING_H
#define SHARED_STRING_H

#include <assert.h>
#include <functional>
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

	static InternMap *GetInternMap();
	static StringValue *Intern(const char *str);
	static void Destroy(StringValue *str);

	void Init(const char *str)
	{
		value = Intern(str);
	}

	void Copy(const SharedString &other)
	{
		Drop();

		value = other.value;
		assert(value->count > 0);
		++value->count;
	}

	void Drop()
	{
		if (value == NULL)
			return;

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

	void clear()
	{
		*this = "";
	}

	bool operator==(const SharedString &other) const
	{
		// Becase we intern strings we can do a simple pointer compare
		return value == other.value;
	}

	bool operator!=(const SharedString &other) const
	{
		return !(*this == other);
	}

	SharedString &operator=(const SharedString &other)
	{
		Copy(other);
		return *this;
	}

	SharedString &operator=(SharedString &&other)
	{
		Drop();
		value = other.value;
		other.value = NULL;
		return *this;
	}
};

namespace std
{
	template <>
	struct hash<SharedString>
	{
		size_t operator()(const SharedString & str) const
		{
			return std::hash<std::string>()(*str);
		}
	};
}

#endif
