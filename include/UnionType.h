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

#ifndef UNIONTYPE_H
#define UNIONTYPE_H

#include "TargetType.h"

#include <vector>

class UnionType : public TargetType
{
	struct UnionMember {
		SharedString name;
		const TargetType &type;

		UnionMember(SharedString n, const TargetType &t)
		  : name(n), type(t)
		{
		}

		bool operator ==(const UnionMember & other) const;
	};

	typedef std::vector<UnionMember> MemberList;

	MemberList members;

public:
	UnionType(SharedString name, size_t size);

	void AddMember(SharedString name, size_t, const TargetType & t);

	bool EqualsUnion(const UnionType *other) const final override;
	bool operator==(const TargetType &) const final override;
	size_t Hash() const final override;

	void Accept(TypeVisitor & ) const override;
};

#endif // UNIONTYPE_H