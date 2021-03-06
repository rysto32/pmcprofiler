// Copyright (c) 2015 Sandvine Incorporated.  All rights reserved.
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
//
// $FreeBSD$

#ifndef MAP_UTIL_H
#define MAP_UTIL_H

#include <map>

template <typename K, typename Map, typename Iterator>
Iterator
LastSmallerThanImpl(Map &map, K addr)
{
	Iterator it = map.upper_bound(addr);

	if (it == map.begin())
		return (map.end());
	--it;
	return (it);
}

template <typename K, typename V, typename C, typename A>
typename std::map<K, V, C, A>::iterator
LastSmallerThan(std::map<K, V, C, A> &map, K addr)
{

	return (LastSmallerThanImpl<K, std::map<K, V, C, A>,
	    typename std::map<K, V, C, A>::iterator>(map, addr));
}

template <typename K, typename V, typename C, typename A>
typename std::map<K, V, C, A>::const_iterator
LastSmallerThan(const std::map<K, V, C, A> &map, K addr)
{

	return (LastSmallerThanImpl<K, const std::map<K, V, C, A>,
	    typename std::map<K, V, C, A>::const_iterator>(map, addr));
}

#endif
