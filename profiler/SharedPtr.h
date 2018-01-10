// Copyright (c) 2017 Ryan Stone.  All rights reserved.
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

#ifndef SHAREDPTR_H
#define SHAREDPTR_H

#include <assert.h>
#include <string>
#include <unordered_map>

template <typename T>
class SharedPtr
{
private:
	struct RefValue
	{
		T value;
		int count;

		template <typename... Args>
		RefValue(Args &&... args)
		  : value(std::forward<Args>(args)...), count(1)
		{
		}

		RefValue(T && t)
		  : value(std::move(t)), count(1)
		{
		}
	};

	RefValue *value;

	void Copy(const SharedPtr<T> &other)
	{
		Drop();

		value = other.value;
		if (value != NULL) {
			assert(value->count > 0);
			++value->count;
		}
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

	explicit SharedPtr(RefValue *v)
	  : value(v)
	{
	}

public:
	SharedPtr()
	  : value(NULL)
	{
	}

	SharedPtr(const SharedPtr<T> &other)
	  : value(NULL)
	{
		Copy(other);
	}

	SharedPtr(SharedPtr<T> &&other) noexcept
	  : value(other.value)
	{
		other.value = NULL;
	}

	template <typename... Args>
	static SharedPtr<T> make(Args &&... args)
	{
		return SharedPtr<T>(new RefValue(args...));
	}

	static SharedPtr<T> make(T && t)
	{
		return SharedPtr<T>(new RefValue(std::move(t)));
	}

	~SharedPtr()
	{
		Drop();
	}

	T &operator*()
	{
		return value->value;
	}

	T *operator->()
	{
		return &value->value;
	}

	const T &operator*() const
	{
		return value->value;
	}

	const T *operator->() const
	{
		return &value->value;
	}

	void clear()
	{
		Drop();
	}

	bool operator==(const SharedPtr<T> &other) const
	{
		return value == other.value;
	}

	SharedPtr<T> &operator=(const SharedPtr<T> &other)
	{
		Copy(other);
		return *this;
	}

	SharedPtr<T> &operator=(SharedPtr<T> &&other)
	{
		Drop();
		value = other.value;
		other.value = NULL;
		return *this;
	}

	T *get()
	{
		return &value->value;
	}

	const T * get() const
	{
		return &value->value;
	}

	operator bool() const
	{
		return value != NULL;
	}
};

#endif
