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

#ifndef TARGETTYPE_H
#define TARGETTYPE_H

#include "SharedString.h"

class ArrayType;
class BasicType;
class PaddingType;
class PointerType;
class StructType;
class SubroutineType;
class TypedefType;
class UnionType;

class TargetType
{
private:
	SharedString name;
	size_t size;

public:
	TargetType(SharedString name, size_t size)
	  : name(name), size(size)
	{
	}

	virtual ~TargetType() = default;

	TargetType(const TargetType &) = delete;
	TargetType(TargetType &&) = delete;

	const TargetType & operator=(const TargetType &) = delete;
	TargetType && operator=(TargetType &&) = delete;

	SharedString GetName() const
	{
		return name;
	}

	size_t GetSize() const
	{
		return size;
	}

	virtual bool EqualsArray(const ArrayType *) const;
	virtual bool EqualsBasic(const BasicType *) const;
	virtual bool EqualsPadding(const PaddingType *) const;
	virtual bool EqualsPointer(const PointerType *) const;
	virtual bool EqualsTypedef(const TypedefType *) const;
	virtual bool EqualsStruct(const StructType *) const;
	virtual bool EqualsSubroutine(const SubroutineType *) const;
	virtual bool EqualsUnion(const UnionType *) const;

	virtual bool ShallowEquals(const TargetType &) const;
	virtual bool ShallowEqualsPointer(const PointerType *) const;

	virtual bool operator==(const TargetType& other) const = 0;

	bool operator!= (const TargetType & other) const
	{
		return !(*this == other);
	}

	virtual size_t Hash() const = 0;
};

namespace std
{
template <>
struct hash<TargetType>
{
	size_t operator()(const TargetType & t) const
	{
		return t.Hash();
	}
};
}

#endif // TARGETTYPE_H
