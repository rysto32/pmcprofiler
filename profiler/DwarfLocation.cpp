// Copyright (c) 2014 Sandvine Incorporated.  All rights reserved.
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

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include "DwarfLocation.h"

#include <err.h>

DwarfLocation::DwarfLocation(const std::string &file, const std::string &func,
    u_int line)
  : m_file(file),
    m_func(func),
    m_lineno(line),
    m_needsDebug(false)
{
}

DwarfLocation::DwarfLocation(const std::string &file, const std::string &func)
  : m_file(file),
    m_func(func),
    m_lineno(0),
    m_needsDebug(true)
{
}

const std::string &
DwarfLocation::GetFile() const
{

	return (m_file);
}

const std::string &
DwarfLocation::GetFunc() const
{

	return (m_func);
}

u_int
DwarfLocation::GetLineNumber() const
{

	return (m_lineno);
}

bool
DwarfLocation::NeedsDebug() const
{

	return (m_needsDebug);
}

void
DwarfLocation::SetDebug(const std::string &file, u_int line)
{

	m_file = file;
	m_lineno = line;
	m_needsDebug = false;
}

