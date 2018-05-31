// Copyright (c) 2009-2014 Sandvine Incorporated.  All rights reserved.
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

#ifndef STRINGCHAIN_H
#define STRINGCHAIN_H

#include <functional>
#include <vector>

#include "SharedString.h"

class StringChain
{
	typedef std::vector<SharedString> CallchainVector;

	CallchainVector vec;
	mutable size_t hash_value;
	mutable bool hash_valid;

public:
	typedef CallchainVector::const_iterator iterator;

	StringChain()
	  : hash_valid(false)
	{
	}

	size_t hash() const
	{
		if (hash_valid)
			return hash_value;

		size_t val = 0;
		for (const auto & str : vec)
			val = hash_combine(val, *str);

		hash_value = val;
		hash_valid = true;

		return val;
	}

	void push_back(const SharedString& t)
	{
		vec.push_back(t);
		hash_valid = false;
	}

	void pop_back()
	{
		vec.pop_back();
		hash_valid = false;
	}

	const SharedString& back()
	{
		return vec.back();
	}

	bool operator==(const StringChain & other) const
	{
		return vec == other.vec;
	}

	struct Hasher
	{
		size_t operator()(const StringChain & v) const
		{
			return v.hash();
		}
	};

	iterator begin()
	{
		return vec.begin();
	}

	iterator end()
	{
		return vec.end();
	}
};

#endif
