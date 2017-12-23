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

#ifndef DWARFDIELIST_H
#define DWARFDIELIST_H

#include <libdwarf.h>

class DwarfDieList
{
private:
	Dwarf_Debug dwarf;
	Dwarf_Die parent;

public:
	DwarfDieList(Dwarf_Debug dwarf, Dwarf_Die die = nullptr)
	  : dwarf(dwarf), parent(die)
	{
	}

	DwarfDieList(const DwarfDieList &) = delete;
	DwarfDieList & operator=(const DwarfDieList &) = delete;

	DwarfDieList(DwarfDieList &&) noexcept = default;
	DwarfDieList & operator=(DwarfDieList &&) = default;

	class const_iterator
	{
	private:
		Dwarf_Debug dwarf;
		Dwarf_Die die;

		friend class DwarfDieList;

		const_iterator(Dwarf_Debug dwarf, Dwarf_Die die)
		  : dwarf(dwarf), die(die)
		{
		}

	public:
		const_iterator()
		  : dwarf(nullptr), die(nullptr)
		{
		}

		const_iterator(const_iterator && other) noexcept
		  : dwarf(other.dwarf), die(other.die)
		{
			other.die = nullptr;
		}

		const_iterator(const const_iterator &) = delete;

		~const_iterator()
		{
			if (die != nullptr)
				dwarf_dealloc(dwarf, die, DW_DLA_DIE);
		}

		const_iterator & operator=(const_iterator && other)
		{
			dwarf = other.dwarf;
			die = other.die;
			other.die = nullptr;
			return *this;
		}

		const_iterator & operator=(const const_iterator &) = delete;

		const Dwarf_Die & operator*() const
		{
			return (die);
		}

		const_iterator &operator++()
		{
			Dwarf_Die last_die = die;
			Dwarf_Error derr;
			int error;

			error = dwarf_siblingof(dwarf, last_die, &die, &derr);
			if (last_die != nullptr)
				dwarf_dealloc(dwarf, last_die, DW_DLA_DIE);

			if (error != DW_DLV_OK)
				die = nullptr;
			return (*this);
		}

		bool operator==(const const_iterator &rhs) const
		{
			if (dwarf != rhs.dwarf)
				return (false);
			return (die == rhs.die);
		}

		bool operator!=(const const_iterator &rhs) const
		{
			return !(*this == rhs);
		}

	};

	const_iterator begin() const
	{
		Dwarf_Die child;
		int error;
		Dwarf_Error derr;

		if (!parent)
			return end();

		error = dwarf_child(parent, &child, &derr);
		if (error != DW_DLV_OK)
			child = nullptr;

		return (const_iterator(dwarf, child));
	}

	const_iterator end() const
	{
		return (const_iterator(dwarf, nullptr));
	}
};


#endif
