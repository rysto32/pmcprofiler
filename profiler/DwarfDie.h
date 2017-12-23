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

#ifndef DWARFDIE_H
#define DWARFDIE_H

#include <libdwarf.h>

class DwarfDie
{
	Dwarf_Debug dwarf;
	Dwarf_Die die;


	DwarfDie(Dwarf_Debug dwarf, Dwarf_Die die) noexcept
	  : dwarf(dwarf), die(die)
	{
	}

	void Release()
	{
		if (die != DW_DLV_BADADDR)
			dwarf_dealloc(dwarf, die, DW_DLA_DIE);
	}

public:
	DwarfDie()
	  : die(DW_DLV_BADADDR)
	{
	}

	~DwarfDie()
	{
		Release();
	}

	DwarfDie(const DwarfDie &) = delete;

	DwarfDie(DwarfDie && other) noexcept
	  : dwarf(other.dwarf), die(other.die)
	{
		other.die = DW_DLV_BADADDR;
	}

	DwarfDie & operator=(const DwarfDie&) = delete;

	DwarfDie & operator=(DwarfDie && other) noexcept
	{
		Release();

		dwarf = other.dwarf;
		die = other.die;

		other.die = DW_DLV_BADADDR;

		return *this;
	}

	operator bool() const
	{
		return die != DW_DLV_BADADDR;
	}

	bool operator!() const
	{
		return !this->operator bool();
	}

	const Dwarf_Die &operator*() const
	{
		return die;
	}

	static DwarfDie OffDie(Dwarf_Debug dwarf, Dwarf_Off ref)
	{
		Dwarf_Die die;
		Dwarf_Error derr;
		int error;

		error = dwarf_offdie(dwarf, ref, &die, &derr);
		if (error != 0)
			return DwarfDie();
		else
			return DwarfDie(dwarf, die);
	}
};

#endif
