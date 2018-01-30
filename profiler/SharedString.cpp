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

#include "SharedString.h"

// To avoid static initialization/deinitialization issues, we make the
// intern_map a pointer that is allocated on first use and never
// deallocated.
SharedString::InternMap *
SharedString::GetInternMap()
{
	static InternMap *intern_map;

	if (intern_map == NULL)
		intern_map = new InternMap;
	return intern_map;
}

SharedString::StringValue *
SharedString::Intern(const char *str)
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

void
SharedString::Destroy(StringValue *str)
{
	GetInternMap()->erase(str->str);
	//printf("String '%s' destroy\n", str->str.c_str());
	delete str;
}
