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

#ifndef MEMORYOFFSET_H
#define MEMORYOFFSET_H

class MemoryOffset
{
public:
	enum AccessType { UNDEFINED, LOAD, STORE };

private:
	int reg;
	AccessType type;
	size_t offset;

public:
	MemoryOffset()
	  : reg(0), type(UNDEFINED), offset(0)
	{
	}

	MemoryOffset(int reg, int32_t offset, AccessType type)
	  : reg(reg), type(type), offset(offset)
	{
	}

	int GetReg() const
	{
		return (reg);
	}

	int GetOffset() const
	{
		return (offset);
	}

	size_t GetAccessSize() const
	{
		// XXX
		return 1;
	}

	AccessType GetType() const
	{
		return (type);
	}

	bool IsDefined() const
	{
		return (type != UNDEFINED);
	}

	const char * GetTypeString() const
	{
		return (type == LOAD ? "load" : "store");
	}
};

#endif // MEMORYOFFSET_H
