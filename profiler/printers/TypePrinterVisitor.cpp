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

#include "TypePrinterVisitor.h"

#include "BufferSample.h"

#include "ArrayType.h"
#include "BasicType.h"
#include "PaddingType.h"
#include "PointerType.h"
#include "StructType.h"
#include "SubroutineType.h"
#include "TargetType.h"
#include "TypedefType.h"
#include "UnionType.h"

#include <sstream>

TypePrinterVisitor::TypePrinterVisitor(FILE *f, const BufferSample &s)
  : outfile(f), sample(s), curOffset(0), firstVisit(true), firstStruct(true)
{

}

std::string
TypePrinterVisitor::GetFieldNamePrefix() const
{
	std::ostringstream prefix;

	for (auto & n : fieldNameStack) {
		prefix << n;
	}

	return prefix.str();
}

void
TypePrinterVisitor::PrintBaseType(const TargetType &t)
{
	const std::string prefix = GetFieldNamePrefix();
	size_t numSamples = sample.GetNumSamples(curOffset, t.GetSize());
	double percent =  numSamples * 100.0 / sample.GetTotalSamples();

	fprintf(outfile, "\t%-16s%-16s (%zd bytes)\t%zd samples\t%6.2f%%\t\n",
	    prefix.c_str(), t.GetName()->c_str(),
	    t.GetSize(), numSamples, percent);
}

void
TypePrinterVisitor::Visit(const ArrayType &t)
{
	firstVisit = false;
	size_t num = t.GetNumMembers();
	const auto & memberType = t.GetMemberType();

	for (size_t i = 0; i < num; ++i) {
		std::ostringstream index;
		index << "[" << i << "]";
		fieldNameStack.push_back(index.str());

		memberType.Accept(*this);

		fieldNameStack.pop_back();
		curOffset += memberType.GetSize();
	}
}

void
TypePrinterVisitor::Visit(const BasicType &t)
{
	firstVisit = false;
	PrintBaseType(t);
}

void
TypePrinterVisitor::Visit(const PaddingType &t)
{
	firstVisit = false;
	PrintBaseType(t);
}

void
TypePrinterVisitor::Visit(const PointerType &t)
{
	if (firstVisit) {
		firstVisit = false;
		t.GetPointeeType().Accept(*this);
	} else {
		PrintBaseType(t);
	}
}

void
TypePrinterVisitor::Visit(const StructType &t)
{
	size_t startOffset = curOffset;

	if (firstStruct) {
		firstStruct = false;
		size_t numMembers = t.GetNumMembers();

		for (size_t i = 0; i < numMembers; ++i) {
			const TargetType & memberType = t.GetMemberType(i);
			SharedString memberName = t.GetMemberName(i);

			std::ostringstream prefix;
			prefix << ".";
			if (memberName->empty()) {
				prefix << "<anon>";
			} else {
				prefix << *memberName;
			}
			fieldNameStack.push_back(prefix.str());

			curOffset = startOffset + t.GetMemberOffset(i);

			memberType.Accept(*this);

			fieldNameStack.pop_back();
		}
	} else {
		PrintBaseType(t);
	}

	curOffset = startOffset + t.GetSize();
}

void
TypePrinterVisitor::Visit(const SubroutineType &t)
{
	firstVisit = false;
	PrintBaseType(t);
}

void
TypePrinterVisitor::Visit(const TypedefType &t)
{
	firstVisit = false;
	t.GetUnderlyingType().Accept(*this);
}

void
TypePrinterVisitor::Visit(const UnionType &t)
{
	firstVisit = false;
	PrintBaseType(t);
}


