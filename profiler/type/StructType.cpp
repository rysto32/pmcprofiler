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

#include "StructType.h"

#include "HashUtil.h"
#include "MapUtil.h"

StructType::StructType(SharedString tag, size_t size)
  : TargetType("struct " + *tag, size)
{
}

#if 0
void
StructType::AddAccess(size_t offset, size_t size, size_t samples)
{
	auto it = LastSmallerThan(members, offset);

	while (size > 0 && it != members.end()) {
		TargetType * member = it->second;
		size_t memberOff;
		if (offset < it->first)
			memberOff = offset - it->first;
		else
			memberOff = 0;
		size_t memberSize = std::min(size, member->GetSize());
		member->AddAccess(memberOff, memberSize, samples);

		++it;
		size -= memberSize;
	}

	if (size > 0) {
		AddUnknownOffset(samples);
		return;
	}
}
#endif

void
StructType::AddMember(SharedString member, size_t offset, const TargetType &type)
{
	fprintf(stderr, "%s: Add member %s of type %s (%p) at offset %zx\n",
	    GetName()->c_str(), member->c_str(), type.GetName()->c_str(),
	    &type, offset);
#ifndef NDEBUG
	if (!members.empty()) {
		const StructMember & mem = members.back();

		assert ((mem.offset + mem.type.GetSize()) <= offset);
	}
#endif
	members.emplace_back(member, offset, type);
}

bool
StructType::operator==(const TargetType & other) const
{
	return other.EqualsStruct(this);
}

bool
StructType::EqualsStruct(const StructType *other) const
{
	auto myIt = members.begin();
	auto otIt = other->members.begin();

	while (myIt != members.end() && otIt != other->members.end()) {
		if (*myIt != *otIt)
			return false;

		++myIt;
		++otIt;
	}

	if (myIt != members.end() || otIt != other->members.end())
		return false;

	return true;
}

size_t
StructType::Hash() const
{
	size_t val = 0;

	for (auto & member : members) {
		val = hash_combine(val, member.name);
		val = hash_combine(val, member.offset);
		val = hash_combine(val, member.bitSize);
		val = hash_combine(val, member.bitOffset);
		val = hash_combine(val, member.type);
	}

	return val;
}

bool
StructType::StructMember::operator ==(const StructMember & other) const
{
	return name == other.name && offset == other.offset &&
	    type.ShallowEquals(other.type);
}
