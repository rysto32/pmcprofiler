// Copyright (c) 2017 Ryan Stone.  All rights reserved.
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

#ifndef DWARFCOMPILEUNITDIE_H
#define DWARFCOMPILEUNITDIE_H

#include "DwarfCompileUnitParams.h"
#include "DwarfDie.h"
#include "ProfilerTypes.h"
#include "SharedPtr.h"

class Callframe;

class DwarfCompileUnitDie
{
private:
	DwarfDie die;
	SharedPtr<DwarfCompileUnitParams> params;
	TargetAddr baseAddr;

public:
	DwarfCompileUnitDie(DwarfDie &&die, SharedPtr<DwarfCompileUnitParams> params);

	DwarfCompileUnitDie(DwarfCompileUnitDie &&) = default;
	DwarfCompileUnitDie(const DwarfCompileUnitDie &) = delete;

	DwarfCompileUnitDie & operator=(DwarfCompileUnitDie &&) = default;
	DwarfCompileUnitDie & operator=(const DwarfCompileUnitDie &) = delete;

	Dwarf_Die GetDie() const
	{
		return *die;
	}

	TargetAddr GetBaseAddr() const
	{
		return baseAddr;
	}

	const DwarfCompileUnitParams & GetParams() const
	{
		return *params;
	}

	SharedPtr<DwarfCompileUnitDie> GetSibling() const;
};

#endif
