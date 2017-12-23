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

#ifndef DWARFARRAY_H
#define DWARFARRAY_H

#include <libdwarf.h>

template <typename ArrType, typename Deleter>
class DwarfArray
{
protected:
	struct Array
	{
		ArrType *buffer;
		Dwarf_Signed count;

		Array(ArrType *a, Dwarf_Signed c)
		  : buffer(a), count(c)
		{
		}
	};
private:
	Dwarf_Debug dwarf;
	Array array;

protected:
	DwarfArray(Dwarf_Debug dwarf, Array arr)
	  : dwarf(dwarf), array(arr)
	{
	}

public:
	DwarfArray() = delete;
	DwarfArray(const DwarfArray<ArrType, Deleter> &) = delete;
	DwarfArray(DwarfArray<ArrType, Deleter> &&) = delete;
	DwarfArray & operator=(const DwarfArray<ArrType, Deleter> &) = delete;
	DwarfArray & operator=(DwarfArray<ArrType, Deleter> &&) = delete;

	~DwarfArray()
	{
		if (array.buffer != nullptr) {
			Deleter del;
			del(dwarf, array.buffer, array.count);
		}
	}

	class const_iterator
	{
	private:
		const DwarfArray<ArrType, Deleter> *parent;
		Dwarf_Signed index;

		friend class DwarfArray<ArrType, Deleter>;

		const_iterator(const DwarfArray<ArrType, Deleter> *p, int i)
		  : parent(p), index(i)
		{
		}

	public:
		const_iterator()
		  : parent(nullptr)
		{
		}

		const ArrType & operator*() const
		{
			return (parent->array.buffer[index]);
		}

		const ArrType & operator->() const
		{
			return (parent->array.buffer[index]);
		}

		const_iterator &operator++()
		{
			index++;
			return (*this);
		}

		bool operator==(const const_iterator &rhs) const
		{
			if (parent != rhs.parent)
				return (false);
			return (index == rhs.index);
		}

		bool operator!=(const const_iterator &rhs) const
		{
			return !(*this == rhs);
		}
	};

	const_iterator begin() const
	{
		return const_iterator(this, 0);
	}

	const_iterator end() const
	{
		return const_iterator(this, array.count);
	}
};

#endif
