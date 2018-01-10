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

	void clear()
	{
		*this = "";
	}

	bool operator==(const SharedString &other) const
	{
		// Becase we intern strings we can do a simple pointer compare
		return value == other.value;
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

#endif
