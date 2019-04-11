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

#include "SubroutineType.h"

#include "HashUtil.h"
#include "TypeVisitor.h"

#include <sstream>

SubroutineType::SubroutineType(const TargetType *retType, std::vector<const TargetType *> &&args, size_t ptrSize)
  : TargetType(BuildName(retType, args), ptrSize),
    returnType(retType),
    argTypes(args)
{

}

SharedString
SubroutineType::BuildName(const TargetType *returnType, const std::vector<const TargetType *> &argTypes)
{
	std::ostringstream name;

	if (returnType == nullptr)
		name << "void";
	else
		name << *returnType->GetName();

	name << " ()(";
	if (argTypes.empty())
		name << "void";
	else {
		const char * sep = "";
		for (const TargetType * a : argTypes) {
			name << sep << *a->GetName();
			sep = ", ";
		}
	}
	name << ")";

	return name.str();
}

bool
SubroutineType::operator==(const TargetType & other) const
{
	return other.EqualsSubroutine(this);
}

bool
SubroutineType::EqualsSubroutine(const SubroutineType *other) const
{
	if ((returnType != nullptr && other->returnType == nullptr) ||
	    (returnType == nullptr && other->returnType != nullptr)) {
		return false;
	}

	if (returnType != nullptr) {
		if (!returnType->ShallowEquals(*other->returnType))
			return false;
	} else {
		assert (other->returnType == nullptr);
	}

	if (argTypes.size() != other->argTypes.size())
		return false;

	auto myIt = argTypes.begin();
	auto otIt = other->argTypes.begin();

	while (myIt != argTypes.end() && otIt != other->argTypes.end()) {
		if (!(*myIt)->ShallowEquals(**otIt))
			return false;
		++myIt;
		++otIt;
	}

	return true;
}

size_t
SubroutineType::Hash() const
{
	size_t val;

	if (returnType != nullptr)
		val = returnType->Hash();
	else
		val = 0;

	for (const TargetType * t : argTypes) {
		val = hash_combine(val, *t);
	}

	return val;
}

void
SubroutineType::Accept(TypeVisitor & v)
{
	v.Visit(*this);
}
