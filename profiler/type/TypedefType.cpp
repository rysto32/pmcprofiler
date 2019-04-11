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

#include "TypedefType.h"

#include "HashUtil.h"
#include "TypeVisitor.h"

TypedefType::TypedefType(SharedString name, const TargetType &type)
  : TargetType(name, type.GetSize()),
    type(type)
{

}

bool
TypedefType::operator==(const TargetType &other) const
{
	return other.EqualsTypedef(this);
}

bool
TypedefType::ShallowEquals(const TargetType & other) const
{
	return other.ShallowEqualsTypedef(this);
}

bool
TypedefType::ShallowEqualsTypedef(const TypedefType * other) const
{
	return GetName() == other->GetName() && type.ShallowEquals(other->type);
}

bool TypedefType::EqualsTypedef(const TypedefType *other) const
{
	return GetName() == other->GetName() && type == other->type;
}

size_t TypedefType::Hash() const
{
	return hash_combine(type.Hash(), GetName());
}

void
TypedefType::Accept(TypeVisitor & v) const
{
	v.Visit(*this);
}
