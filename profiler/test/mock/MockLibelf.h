// Copyright (c) 2018 Ryan Stone.  All rights reserved.
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

#ifndef MOCK_MOCK_LIBELF_H
#define MOCK_MOCK_LIBELF_H

#include <gmock/gmock.h>
#include <gelf.h>
#include <libelf.h>

class MockLibelf
{
public:
	MOCK_METHOD3(elf_begin, Elf *(int, Elf_Cmd, Elf *));
	MOCK_METHOD2(elf_getphdrnum, int(Elf *, size_t *));
	MOCK_METHOD3(gelf_getphdr, GElf_Phdr *(Elf *, int, GElf_Phdr*));
	MOCK_METHOD1(elf_end, int (Elf *));
};

testing::StrictMock<MockLibelf> libelf;

Elf *
elf_begin(int fd, Elf_Cmd cmd, Elf *ar)
{
	return libelf.elf_begin(fd, cmd, ar);
}

int
elf_getphdrnum(Elf *elf, size_t *phnum)
{
	return libelf.elf_getphdrnum(elf, phnum);
}

GElf_Phdr *
gelf_getphdr(Elf *elf, int index, GElf_Phdr *dst)
{
	return libelf.gelf_getphdr(elf, index, dst);
}

int
elf_end(Elf *elf)
{
	return libelf.elf_end(elf);
}

#endif
