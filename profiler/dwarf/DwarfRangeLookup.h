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

class Callframe;

template <typename T>
class DwarfRangeLookup
{
public:
	class MapValue
	{
	private:
		TargetAddr low;
		TargetAddr high;
		SharedPtr<T> value;
		std::vector<Callframe *> pendingFrames;

	public:
		MapValue() = delete;
		MapValue(TargetAddr l, TargetAddr h, SharedPtr<T> &&d)
		  : low(l), high(h), value(d)
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

		void AddFrame(Callframe *frame)
		{
			pendingFrames.push_back(frame);
		}

		const std::vector<Callframe*> & GetFrames() const
		{
			return pendingFrames;
		}

		const T & GetValue() const
		{
			return *value;
		}

		TargetAddr GetLow() const
		{
			return low;
		}

		TargetAddr GetHigh() const
		{
			return high;
		}
	};

	typedef typename std::map<TargetAddr, MapValue> DieMap;
	typedef typename DieMap::iterator iterator;

private:
	DieMap map;

public:
	DwarfRangeLookup()
	{
	}

	DwarfRangeLookup(const DwarfRangeLookup &) = delete;
	DwarfRangeLookup(DwarfRangeLookup &&) = delete;

	DwarfRangeLookup &operator=(const DwarfRangeLookup&) = delete;
	DwarfRangeLookup &operator=(DwarfRangeLookup &&) = delete;

	void insert(TargetAddr l, TargetAddr h, SharedPtr<T> die)
	{
		map.insert(std::make_pair(l, MapValue(l, h, std::move(die))));
	}

	iterator Lookup(TargetAddr a)
	{
		return LastSmallerThan(map, a);
	}

	iterator begin()
	{
		return map.begin();
	}

	iterator end()
	{
		return map.end();
	}

	void clear()
	{
		map.clear();
	}
};

#endif
