// Copyright (c) 2018 Ryan Stone.
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

#include "UnionType.h"

#include "HashUtil.h"
#include "TypeVisitor.h"

UnionType::UnionType(SharedString tag, size_t size)
  : TargetType("union " + *tag, size)
{
}

void
UnionType::AddMember(SharedString name, size_t, const TargetType & t)
{
	fprintf(stderr, "%s: Add member %s of type %s (%p)\n",
	    GetName()->c_str(), name->c_str(), t.GetName()->c_str(),
	    &t);
	members.emplace_back(name, t);
}

bool
UnionType::EqualsUnion(const UnionType *other) const
{
	return members == other->members;
}

bool
UnionType::operator ==(const TargetType & other) const
{
	return other.EqualsUnion(this);
}

size_t
UnionType::Hash() const
{
	size_t val = 0;

	for (const auto & member : members) {
		val = hash_combine(val, member.name);
		val = hash_combine(val, member.type);
	}

	return val;
}

bool
UnionType::UnionMember::operator==(const UnionMember & other) const
{
	return name == other.name && type.ShallowEquals(other.type);
}

void
UnionType::Accept(TypeVisitor & v) const
{
	v.Visit(*this);
}
