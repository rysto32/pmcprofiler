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

class DwarfDieList;

class DwarfDie
{
	Dwarf_Debug dwarf;
	Dwarf_Die die;
	bool ownDie;


	DwarfDie(Dwarf_Debug dwarf, Dwarf_Die die) noexcept
	  : dwarf(dwarf), die(die), ownDie(die != nullptr)
	{
	}

	void Release()
	{
		if (ownDie)
			dwarf_dealloc(dwarf, die, DW_DLA_DIE);
	}

	friend class DwarfDieList;

public:
	DwarfDie()
	  : die(nullptr), ownDie(false)
	{
	}

	~DwarfDie()
	{
		Release();
	}

	DwarfDie(const DwarfDie &) = delete;

	DwarfDie(DwarfDie && other) noexcept
	  : dwarf(other.dwarf), die(other.die), ownDie(other.ownDie)
	{
		other.ownDie = false;
	}

	DwarfDie & operator=(const DwarfDie&) = delete;

	DwarfDie & operator=(DwarfDie && other) noexcept
	{
		Release();

		dwarf = other.dwarf;
		die = other.die;
		ownDie = other.ownDie;

		other.ownDie = false;

		return *this;
	}

	bool operator==(const DwarfDie & other) const
	{
		return dwarf == other.dwarf && die == other.die;
	}

	operator bool() const
	{
		return die != nullptr;
	}

	bool operator!() const
	{
		return !bool(*this);
	}

	Dwarf_Die operator*() const
	{
		return die;
	}

	void AdvanceToSibling();
	DwarfDie GetSibling() const;

	static DwarfDie OffDie(Dwarf_Debug dwarf, Dwarf_Off ref);
	static DwarfDie GetCuDie(Dwarf_Debug dwarf);
};

#endif
