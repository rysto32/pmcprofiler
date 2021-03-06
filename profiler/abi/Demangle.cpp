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

#include "Demangle.h"

#include "ProfilerTypes.h"

#include <cxxabi.h>

SharedString
Demangle(SharedString name)
{
	char *demangled;
	char *dst, *src;
	int angle_count;
	size_t len;
	int status;

	/*
	 * abi::__cxa_demangle doesn't work on all non-mangled symbol names,
	 * so do a hacky test for a mangled name before trying to demangle it.
	 */
	if (name->substr(0, 2) != "_Z")
		return (name);

	len = 0;
	demangled = (abi::__cxa_demangle(name->c_str(), NULL, &len, &status));

	if (demangled == NULL)
		return (name);

	// If template arguments are included in the output, it tends to be
	// so long that functions are unreadable.  By default, filter out
	// the template arguments.
	if (!g_includeTemplates) {
		dst = demangled;
		src = demangled;
		angle_count = 0;
		while (*src != '\0') {
			if (*src == '<')
				angle_count++;
			else if (*src == '>')
				angle_count--;
			else if (angle_count == 0) {
				if (dst != src)
					*dst = *src;
				dst++;
			}
			src++;
		}
		*dst = '\0';
	}

	SharedString shared(demangled);
	free(demangled);

	return (shared);
}
