// Copyright (c) 2019 Ryan Stone.
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

#ifndef TYPE_PRINTER_VISITOR
#define TYPE_PRINTER_VISITOR

#include "TypeVisitor.h"

#include <stdio.h>
#include <string>
#include <vector>

class BufferSample;
class TargetType;

class TypePrinterVisitor : public TypeVisitor
{
private:
	FILE *outfile;
	const BufferSample &sample;
	size_t curOffset;
	std::vector<std::string> fieldNameStack;
	bool firstVisit;
	bool firstStruct;

	std::string GetFieldNamePrefix() const;

	void PrintBaseType(const TargetType &);

public:
	TypePrinterVisitor(FILE *outfile, const BufferSample &sample);

	virtual void Visit(const ArrayType &) final override;
	virtual void Visit(const BasicType &) final override;
	virtual void Visit(const PaddingType &) final override;
	virtual void Visit(const PointerType &) final override;
	virtual void Visit(const StructType &) final override;
	virtual void Visit(const SubroutineType &) final override;
	virtual void Visit(const TypedefType &) final override;
	virtual void Visit(const UnionType &) final override;
};

#endif
