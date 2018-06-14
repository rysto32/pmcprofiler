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

#include "DefaultBufferSampleFactory.h"

#include "ArrayType.h"
#include "PaddingType.h"
#include "PointerType.h"
#include "StructType.h"
#include "SubroutineType.h"
#include "TargetType.h"
#include "TypedefType.h"
#include "UnionType.h"

#include "DwarfCompileUnitParams.h"
#include "DwarfDie.h"
#include "DwarfDieList.h"
#include "DwarfException.h"
#include "DwarfLocList.h"

#include  "BufferSample.h"

#include <dwarf.h>
#include <err.h>

DefaultBufferSampleFactory::DefaultBufferSampleFactory()
  : voidType("void", 1),
    defaultSample(std::make_unique<BufferSample>(voidType))
{

}

DefaultBufferSampleFactory::~DefaultBufferSampleFactory()
{

}

BufferSample *
DefaultBufferSampleFactory::GetSample(Dwarf_Debug dwarf, const DwarfCompileUnitParams &params, const DwarfDie &typeDie)
{
	const TargetType & type = BuildType(dwarf, params, typeDie);

	auto it = sampleMap.find(&type);
	if (!it->second) {
		it->second = std::make_unique<BufferSample>(type);
	}

	return it->second.get();
}

BufferSample *
DefaultBufferSampleFactory::GetUnknownSample()
{
	return defaultSample.get();
}

const TargetType &
DefaultBufferSampleFactory::BuildType(Dwarf_Debug dwarf, const DwarfCompileUnitParams & params, const DwarfDie & type)
{
	DwarfDieOffset dieOff = type.GetOffset();

	auto it = dieMap.find(dieOff);
	if (it != dieMap.end()) {
		return *it->second;
	}

	std::unique_ptr<TargetType> newPtr = BuildTypeFromDwarf(dwarf, params, type);

	// We initialize with a null sample because we're not sure whether we're ever
	// going to need one (we might only be instantiating this type because it's embedded
	// in another type).  We'll allocate one in GetSample() on-demand.
	auto inserted = sampleMap.insert(std::make_pair(newPtr.get(), nullptr));

	// If the insert succeeded then hold on to the unique_ptr to ensure that it's not
	// freed when we return.
	if (inserted.second) {
		typeList.push_back(std::move(newPtr));
	}

	const TargetType * insertType = inserted.first->first;
	dieMap.insert(std::make_pair(dieOff, insertType));
	return *insertType;
}

std::unique_ptr<TargetType>
DefaultBufferSampleFactory::BuildTypeFromDwarf(Dwarf_Debug dwarf, const DwarfCompileUnitParams & params, const DwarfDie &type)
{
	auto tag = type.GetTag();

	switch (tag) {
	case DW_TAG_array_type:
		return BuildTypeFromArray(dwarf, params, type);
	case DW_TAG_base_type:
	case DW_TAG_enumeration_type: // enums can be treated as ints for our purposes
		return BuildTypeFromBaseType(dwarf, params, type);
	case DW_TAG_const_type:
	case DW_TAG_volatile_type:
		return BuildTypeFromQualifier(dwarf, params, type);
	case DW_TAG_pointer_type:
		return BuiltTypeFromPointerType(dwarf, params, type);
	case DW_TAG_structure_type:
		return BuildTypeFromStruct(dwarf, params, type);
	case DW_TAG_subroutine_type:
		return BuildTypeFromSubroutine(dwarf, params, type);
	case DW_TAG_typedef:
		return BuildTypeFromTypedef(dwarf, params, type);
	case DW_TAG_union_type:
		return BuildTypeFromUnion(dwarf, params, type);
	default:
		errx(1, "Unrecognized tag %d", tag);
	}
}

DwarfDie
DefaultBufferSampleFactory::GetTypeDie(Dwarf_Debug dwarf, const DwarfDie &die)
{
	Dwarf_Attribute subtypeAttr;
	Dwarf_Error derr;

	int error = dwarf_attr(*die, DW_AT_type, &subtypeAttr, &derr);
	if (error != 0)
		return DwarfDie();

	Dwarf_Off subtypeOff;
	error = dwarf_global_formref(subtypeAttr, &subtypeOff, &derr);
	if (error != 0)
		throw DwarfException("dwarf_global_formref failed");

	return DwarfDie::OffDie(dwarf, subtypeOff);
}

std::unique_ptr<TargetType>
DefaultBufferSampleFactory::BuildTypeFromArray(Dwarf_Debug dwarf, const DwarfCompileUnitParams & params, const DwarfDie &type)
{
	DwarfDie subdie = GetTypeDie(dwarf, type);
	if (!subdie)
		throw DwarfException("Could not get DIE of type");

	DwarfDieList list(dwarf, *subdie);

	auto listIt = list.begin();
	if (listIt == list.end())
		throw DwarfException("Expected child die");

	const DwarfDie & numElemsDie = listIt.Get();
	++listIt;
	if (listIt != list.end())
		errx(1, "Expected only one child");

	if (numElemsDie.GetTag() != DW_TAG_subrange_type) {
		errx(1, "Expected subrange die");
	}

	int lowerBound = numElemsDie.GetUnsignedAttr(DW_AT_lower_bound);
	if (lowerBound != 0) {
		errx(1, "Array starts at unexpected index %d", lowerBound);
	}

	size_t numElems = numElemsDie.GetUnsignedAttr(DW_AT_count);

	return std::make_unique<ArrayType>(BuildType(dwarf, params, subdie), numElems);
}

std::unique_ptr<TargetType>
DefaultBufferSampleFactory::BuildTypeFromBaseType(Dwarf_Debug dwarf, const DwarfCompileUnitParams & params, const DwarfDie &type)
{
	SharedString typeName = type.GetNameAttr();
	size_t len = type.GetUnsignedAttr(DW_AT_byte_size);

	return std::make_unique<BasicType>(typeName, len);
}

std::unique_ptr<TargetType>
DefaultBufferSampleFactory::BuildTypeFromQualifier(Dwarf_Debug dwarf,
    const DwarfCompileUnitParams & params, const DwarfDie &type)
{
	DwarfDie subdie = GetTypeDie(dwarf, type);
	if (!subdie)
		throw DwarfException("Could not get DIE of type");

	// We just strip const from the type as we don't care.
	return BuildTypeFromDwarf(dwarf, params, subdie);
}

size_t
DefaultBufferSampleFactory::GetBlockOffset(Dwarf_Debug dwarf, Dwarf_Attribute attr)
{
	Dwarf_Block *block;
	Dwarf_Error derr;
	Dwarf_Unsigned i;
	int error;

	error = dwarf_formblock(attr, &block, &derr);
	if (error != 0)
		throw DwarfException("dwarf_formblock failed");

	// XXX libdwarf doesn't offer any API for parsing a block, so it has to be done manually
	const uint8_t * bytes = reinterpret_cast<const uint8_t*>(block->bl_data);
	if (block->bl_len < 2)
		throw DwarfException("Can't parse dwarf block");

	if (bytes[0] != DW_OP_plus_uconst)
		throw DwarfException("Unexpected operator");

	// Decode the ule128 representing the offset
	size_t offset = 0;
	for (i = 1; i < block->bl_len; ++i) {
		offset |= ((size_t)(bytes[i] & 0x7f)) << (7 * (i - 1));
		if ((bytes[i] & 0x80) == 0)
			break;
	}

	if (i != block->bl_len - 1) {
		fprintf(stderr, "i=%ld bl_len=%ld\n", i, block->bl_len);
		//throw DwarfException("unparsed bytes");
	}

	return offset;
}

size_t
DefaultBufferSampleFactory::GetMemberOffset(Dwarf_Debug dwarf, const DwarfDie & member)
{
	Dwarf_Attribute attr;
	Dwarf_Half form;
	Dwarf_Error derr;

	int error = dwarf_attr(*member, DW_AT_data_member_location, &attr, &derr);
	if (error != 0) {
		if (member.HasAttr(DW_AT_data_bit_offset))
			errx(1, "Unhandled data_bit_offset attribute");

		// DWARF4 5.5.6:
		// The member entry corresponding to a data member that is
		// defined in a structure, union or class may have either a
		// DW_AT_data_member_location attribute or a DW_AT_data_bit_offset
		// attribute. If the beginning of the data member is the same
		// as the beginning of the containing entity then neither
		// attribute is required.
		return (0);
	}

	error = dwarf_whatform(attr, &form, &derr);
	if (error != 0)
		throw DwarfException("dwarf_whatform failed");

	switch (form) {
	case DW_FORM_block:
	case DW_FORM_block1:
	case DW_FORM_block2:
	case DW_FORM_block4:
		// llvm inexplicably uses a block expression to represent
		// a simple offset rather than an unsigned value.  *sigh*
		return GetBlockOffset(dwarf, attr);
	default:
		return member.GetUnsignedAttr(DW_AT_data_member_location);
	}
}

template <typename T>
std::unique_ptr<T>
DefaultBufferSampleFactory::BuildStructuredType(Dwarf_Debug dwarf, const DwarfCompileUnitParams & params, const DwarfDie &type)
{
	SharedString tag;

	if (type.HasNameAttr()) {
		tag = type.GetNameAttr();
	} else {
		// anonymous struct
		tag = "";
	}

	if (!type.HasAttr(DW_AT_byte_size)) {
		// Type is forward declared, so it can't be deferenced.  Use a size of 0.
		return std::make_unique<T>(tag, 0);
	}

	size_t len = type.GetUnsignedAttr(DW_AT_byte_size);

	auto ptr = std::make_unique<T>(tag, len);

	dieMap.insert(std::make_pair(type.GetOffset(), ptr.get()));

	DwarfDieList list(dwarf, *type);
	for (auto it = list.begin(); it != list.end(); ++it) {
		const DwarfDie & member = it.Get();

		if (member.GetTag() != DW_TAG_member)
			continue;

		SharedString memberName = member.GetNameAttr();
		size_t memberOff = GetMemberOffset(dwarf, member);
		DwarfDie memberTypeDie = GetTypeDie(dwarf, member);
		const TargetType & memberType = BuildType(dwarf, params, memberTypeDie);

		ptr->AddMember(memberName, memberOff, memberType);
	}

	return std::move(ptr);
}

std::unique_ptr<TargetType>
DefaultBufferSampleFactory::BuildTypeFromStruct(Dwarf_Debug dwarf, const DwarfCompileUnitParams & params, const DwarfDie &type)
{
	return BuildStructuredType<StructType>(dwarf, params, type);
}

std::unique_ptr<TargetType>
DefaultBufferSampleFactory::BuildTypeFromSubroutine(Dwarf_Debug dwarf,
    const DwarfCompileUnitParams & params, const DwarfDie &type)
{
	const TargetType * retType;
	DwarfDie retTypeDie = GetTypeDie(dwarf, type);

	if (retTypeDie)
		retType = &BuildType(dwarf, params, retTypeDie);
	else
		retType = nullptr;

	std::vector<const TargetType *> argTypeList;
	DwarfDieList list(dwarf, *type);
	for (auto it = list.begin(); it != list.end(); ++it) {
		const DwarfDie & paramDie = it.Get();

		if (paramDie.GetTag() != DW_TAG_formal_parameter)
			continue;

		DwarfDie argTypeDie = GetTypeDie(dwarf, paramDie);
		argTypeList.push_back(&BuildType(dwarf, params, argTypeDie));
	}

	return std::make_unique<SubroutineType>(retType, std::move(argTypeList), params.GetPointerSize());
}

std::unique_ptr<TargetType>
DefaultBufferSampleFactory::BuildTypeFromTypedef(Dwarf_Debug dwarf, const DwarfCompileUnitParams & params, const DwarfDie &type)
{
	DwarfDie subtype = GetTypeDie(dwarf, type);
	SharedString name = type.GetNameAttr();

	return std::make_unique<TypedefType>(name, BuildType(dwarf, params, subtype));
}

std::unique_ptr<TargetType>
DefaultBufferSampleFactory::BuildTypeFromUnion(Dwarf_Debug dwarf, const DwarfCompileUnitParams & params, const DwarfDie &type)
{
	return BuildStructuredType<UnionType>(dwarf, params, type);
}

std::unique_ptr<TargetType>
DefaultBufferSampleFactory::BuiltTypeFromPointerType(Dwarf_Debug dwarf, const DwarfCompileUnitParams & params, const DwarfDie &type)
{
	DwarfDie pointeeDie = GetTypeDie(dwarf, type);

	const TargetType *pointee;
	if (pointeeDie) {
		pointee = &BuildType(dwarf, params, pointeeDie);
	} else {
		pointee = GetVoidType();
	}
	return std::make_unique<PointerType>(*pointee, params.GetPointerSize());
}

size_t
DefaultBufferSampleFactory::TargetTypePtrHash::operator ()(const TargetType * t) const
{
	return t->Hash();
}

bool
DefaultBufferSampleFactory::TargetTypePtrEq::operator()(const TargetType *l, const TargetType *r) const
{
	return *l == *r;
}
