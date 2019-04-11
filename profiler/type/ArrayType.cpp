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

#include "ArrayType.h"

#include "HashUtil.h"
#include "TypeVisitor.h"

#include <sstream>

SharedString
ArrayType::GetName(const TargetType &type, size_t members)
{
	std::ostringstream str;

	str << *type.GetName() << "[" << members << "]";

	return str.str();
}

ArrayType::ArrayType(const TargetType &type, size_t members)
  : TargetType(GetName(type, members), type.GetSize() * members),
    type(type),
    numMembers(members)
{

}

#if 0
void
ArrayType::AddAccess(size_t offset, size_t size, size_t numSamples)
{
	if (members.empty()) {
		AddUnknownOffset(numSamples);
		return;
	}

	size_t memberTypeSize = members.at(0)->GetSize();
	size_t index = offset / memberTypeSize;
	size_t memberOff = offset % memberTypeSize;

	while (size > 0 && index < members.size()) {
		size_t memberSize = std::min(memberTypeSize - memberOff, size);
		members.at(index)->AddAccess(memberOff, memberSize, numSamples);

		size -= memberSize;
		index++;
		memberOff = 0;
	}

	if (size != 0) {
		AddUnknownOffset(numSamples);
		return;
	}
}
#endif

bool
ArrayType::operator ==(const TargetType & other) const
{
	return other.EqualsArray(this);
}

bool
ArrayType::EqualsArray(const ArrayType *other) const
{
	return type == other->type && numMembers == other->numMembers;
}

bool
ArrayType::ShallowEquals(const TargetType & other) const
{
	return other.ShallowEqualsArray(this);
}

bool
ArrayType::ShallowEqualsArray(const ArrayType * other) const
{
	return type.ShallowEquals(other->type)
	    && numMembers == other->numMembers;
}

size_t
ArrayType::Hash() const
{
	return hash_combine(numMembers, type);
}

void
ArrayType::Accept(TypeVisitor & v)
{
	v.Visit(*this);
}
