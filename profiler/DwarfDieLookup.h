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

#ifndef DWARFDIELOOKUP_H
#define DWARFDIELOOKUP_H

#include <map>

#include "DwarfDie.h"
#include "MapUtil.h"
#include "ProfilerTypes.h"
#include "SharedPtr.h"

class DwarfDieLookup
{
private:
	struct MapValue
	{
		TargetAddr low;
		TargetAddr high;
		SharedPtr<DwarfDie> die;

		MapValue() = delete;
		MapValue(TargetAddr l, TargetAddr h, SharedPtr<DwarfDie> &&d)
		  : low(l), high(h), die(d)
		{
		}

		MapValue(const MapValue &) = delete;
		MapValue(MapValue &&) = default;

		MapValue &operator=(const MapValue &) = delete;
		MapValue &operator=(MapValue &&) = default;

		bool Contains(TargetAddr a) const
		{
			return low <= a && a < high;
		}
	};

	typedef std::map<TargetAddr, MapValue> DieMap;

public:
	class const_iterator
	{
	private:
		DieMap::const_iterator it;

		explicit const_iterator(DieMap::const_iterator i)
		  : it(i)
		{
		}

		friend class DwarfDieLookup;

	public:
		const_iterator()
		{
		}

		const_iterator &operator++()
		{
			++it;
			return *this;
		}

		SharedPtr<DwarfDie> operator*()
		{
			return it->second.die;
		}

		const SharedPtr<DwarfDie> *operator->()
		{
			return &it->second.die;
		}

		bool operator==(const const_iterator & other) const
		{
			return it == other.it;
		}

		bool operator!=(const const_iterator & other) const
		{
			return !(*this == other);
		}
	};

private:
	DieMap map;

public:
	DwarfDieLookup()
	{
	}

	DwarfDieLookup(const DwarfDieLookup &) = delete;
	DwarfDieLookup(DwarfDieLookup &&) = delete;

	DwarfDieLookup &operator=(const DwarfDieLookup&) = delete;
	DwarfDieLookup &operator=(DwarfDieLookup &&) = delete;

	void insert(TargetAddr l, TargetAddr h, SharedPtr<DwarfDie> die)
	{
		map.insert(std::make_pair(l, MapValue(l, h, std::move(die))));
	}

	const_iterator Lookup(TargetAddr a) const
	{
		return const_iterator(LastSmallerThan(map, a));
	}

	const_iterator end() const
	{
		return const_iterator(map.end());
	}

	/*void FindRange(FrameMap::const_iterator &it, const FrameMap::const_iterator end)
	{
		if (map.empty())
			return;

		const MapValue & back = map.rbegin()->second;
		while (it != end) {
			if (!back.Contains(it->second->getOffset()))
				return;

			++it;
		}
	}*/
};

#endif
