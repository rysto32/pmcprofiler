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

#include "PointerType.h"

#include "HashUtil.h"

PointerType::PointerType(const TargetType &t, size_t ptrSize)
  : TargetType(*t.GetName() + "*", ptrSize),
    pointeeType(t)
{
}

bool
PointerType::EqualsPointer(const PointerType *other) const
{
	return pointeeType == other->pointeeType;
}

bool
PointerType::ShallowEquals(const TargetType & other) const
{
	return other.ShallowEqualsPointer(this);
}

bool
PointerType::ShallowEqualsPointer(const PointerType *other) const
{
	return &pointeeType == &other->pointeeType;
}

bool
PointerType::operator ==(const TargetType & other) const
{
	return other.EqualsPointer(this);
}

size_t
PointerType::Hash() const
{
	// Hash based on the name of the pointed-to type.  This prevents
	// infinite recursion when hashing a self-referential type.
	return hash_combine(1, pointeeType.GetName());
}
