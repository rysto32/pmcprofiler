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

#ifndef DEFAULTBUFFERSAMPLEFACTORY_H
#define DEFAULTBUFFERSAMPLEFACTORY_H

#include "BufferSampleFactory.h"

#include "BasicType.h"
#include "DwarfUtil.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class DefaultBufferSampleFactory : public BufferSampleFactory
{
private:
	struct TargetTypePtrHash
	{
		size_t operator()(const TargetType * t) const;
	};

	struct TargetTypePtrEq
	{
		bool operator()(const TargetType *l, const TargetType *r) const;
	};
	typedef std::unordered_map<DwarfDieOffset, const TargetType*> DieTypeMap;
	typedef std::unordered_map<const TargetType *, std::unique_ptr<BufferSample>, TargetTypePtrHash, TargetTypePtrEq> SampleMap;
	typedef std::vector<std::unique_ptr<TargetType>> TypeList;

	DieTypeMap dieMap;
	SampleMap sampleMap;
	TypeList typeList;

	BasicType voidType;
	std::unique_ptr<BufferSample> defaultSample;

	const TargetType & BuildType(Dwarf_Debug dwarf, const DwarfCompileUnitParams & params, const DwarfDie & type);
	std::unique_ptr<TargetType> BuildTypeFromDwarf(Dwarf_Debug dwarf, const DwarfCompileUnitParams & params, const DwarfDie &type);

	std::unique_ptr<TargetType> BuildTypeFromArray(Dwarf_Debug dwarf,
	    const DwarfCompileUnitParams & params, const DwarfDie &type);
	std::unique_ptr<TargetType> BuildTypeFromBaseType(Dwarf_Debug dwarf,
	    const DwarfCompileUnitParams & params, const DwarfDie &type);
	std::unique_ptr<TargetType> BuildTypeFromQualifier(Dwarf_Debug dwarf,
	     const DwarfCompileUnitParams & params, const DwarfDie &type);
	std::unique_ptr<TargetType> BuiltTypeFromPointerType(Dwarf_Debug dwarf,
	     const DwarfCompileUnitParams & params, const DwarfDie &type);
	std::unique_ptr<TargetType> BuildTypeFromStruct(Dwarf_Debug dwarf,
	    const DwarfCompileUnitParams & params, const DwarfDie &type);
	std::unique_ptr<TargetType> BuildTypeFromSubroutine(Dwarf_Debug dwarf,
	    const DwarfCompileUnitParams & params, const DwarfDie &type);
	std::unique_ptr<TargetType> BuildTypeFromTypedef(Dwarf_Debug dwarf,
	    const DwarfCompileUnitParams & params, const DwarfDie &type);
	std::unique_ptr<TargetType> BuildTypeFromUnion(Dwarf_Debug dwarf,
	    const DwarfCompileUnitParams & params, const DwarfDie &type);

	template <typename T>
	std::unique_ptr<T> BuildStructuredType(Dwarf_Debug dwarf, const DwarfCompileUnitParams & params, const DwarfDie &type);

	DwarfDie GetTypeDie(Dwarf_Debug dwarf, const DwarfDie &die);
	size_t GetMemberOffset(Dwarf_Debug dwarf, const DwarfDie & member);
	size_t GetBlockOffset(Dwarf_Debug dwarf, Dwarf_Attribute attr);

public:
	DefaultBufferSampleFactory();
	~DefaultBufferSampleFactory();

	DefaultBufferSampleFactory(const DefaultBufferSampleFactory &) = delete;
	DefaultBufferSampleFactory(DefaultBufferSampleFactory &&) = delete;

	const DefaultBufferSampleFactory &operator=(const DefaultBufferSampleFactory &) = delete;
	DefaultBufferSampleFactory && operator=(DefaultBufferSampleFactory &&) = delete;

	BufferSample * GetSample(Dwarf_Debug dwarf, const DwarfCompileUnitParams & params, const DwarfDie & type) final override;
	BufferSample * GetUnknownSample() final override;
};

#endif // DEFAULTDWARFTYPEFACTORY_H
