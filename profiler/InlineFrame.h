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

#ifndef INLINEFRAME_H
#define INLINEFRAME_H

#include "SharedString.h"
#include "ProfilerTypes.h"

class InlineFrame
{
	SharedString file;
	SharedString func;
	SharedString demangledFunc;
	TargetAddr offset;
	int codeLine;
	int funcLine;

public:
	InlineFrame(SharedString file, SharedString func,
	    SharedString demangled, TargetAddr off,
	    int codeLine, int funcLine)
	  : file(file), func(func), demangledFunc(demangled),
	    offset(off), codeLine(codeLine), funcLine(funcLine)
	{
	}

	SharedString getFile() const
	{

		return (file);
	}

	SharedString getFunc() const
	{

		return (func);
	}

	SharedString getDemangled() const
	{

		return (demangledFunc);
	}

	TargetAddr getOffset() const
	{
		return offset;
	}

	int getCodeLine() const
	{

		return (codeLine);
	}

	int getFuncLine() const
	{

		return (funcLine);
	}
};

#endif
