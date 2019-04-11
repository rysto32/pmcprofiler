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

#ifndef STRUCTTYPE_H
#define STRUCTTYPE_H

#include "TargetType.h"

#include <vector>

class StructType : public TargetType
{
private:
	struct StructMember {
		SharedString name;
		size_t offset;
		size_t bitOffset;
		size_t bitSize;
		const TargetType &type;

		StructMember(SharedString n, size_t o, size_t bitOff,
		    size_t bitSz, const TargetType & t)
		  : name(n),
		    offset(o),
		    bitOffset(bitOff),
		    bitSize(bitSz),
		    type(t)
		{
		}

		bool operator==(const StructMember &) const;

		inline bool operator!=(const StructMember & other) const
		{
			return !(*this == other);
		}
	};
	typedef std::vector<StructMember> MemberList;

	MemberList members;

public:
	StructType(SharedString name, size_t size);

	void AddMember(SharedString name, size_t offset, const TargetType &type);

	bool operator==(const TargetType & other) const override final;
	bool EqualsStruct(const StructType * other) const override final;
	size_t Hash() const override final;

	void Accept(TypeVisitor & ) const override;
};

#endif // STRUCTTYPE_H
