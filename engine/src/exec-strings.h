/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
 This file is part of LiveCode.
 
 LiveCode is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License v3 as published by the Free
 Software Foundation.
 
 LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.
 
 You should have received a copy of the GNU General Public License
 along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "regex.h"

////////////////////////////////////////////////////////////////////////////////

// JS-2013-07-01: [[ EnhancedFilter ]] Utility class and descendents handling the
//   regex and wildcard style pattern matchers.
class MCPatternMatcher
{
protected:
	MCStringRef pattern;
    MCStringRef source;
	bool casesensitive;
    bool formsensitive;
public:
	MCPatternMatcher(MCStringRef p, MCStringRef s, bool cs, bool fs)
	{
		pattern = MCValueRetain(p);
        source = MCValueRetain(s);
		casesensitive = cs;
        formsensitive = fs;
	}
	virtual ~MCPatternMatcher();
	virtual bool compile(MCStringRef& r_error) = 0;
	virtual bool match(MCRange p_range) = 0;
};

class MCRegexMatcher : public MCPatternMatcher
{
protected:
	regexp *compiled;
public:
	MCRegexMatcher(MCStringRef p, MCStringRef s, bool cs, bool fs) : MCPatternMatcher(p, s, cs, fs)
	{
		compiled = NULL;
	}
	virtual bool compile(MCStringRef& r_error);
	virtual bool match(MCRange p_range);
};

class MCWildcardMatcher : public MCPatternMatcher
{
    MCBreakIteratorRef pattern_iter;
    MCBreakIteratorRef source_iter;
public:
	MCWildcardMatcher(MCStringRef p, MCStringRef s, bool cs, bool fs) : MCPatternMatcher(p, s, cs, fs)
	{
        MCLocaleBreakIteratorCreate(kMCBasicLocale, kMCBreakIteratorTypeCharacter, pattern_iter);
        MCLocaleBreakIteratorCreate(kMCBasicLocale, kMCBreakIteratorTypeCharacter, source_iter);
        MCLocaleBreakIteratorSetText(pattern_iter, p);
        MCLocaleBreakIteratorSetText(source_iter, s);
	}
    ~MCWildcardMatcher();
	virtual bool compile(MCStringRef& r_error);
	virtual bool match(MCRange p_range);
protected:
	static bool match(const char *s, const char *p, Boolean cs);
};

MCWildcardMatcher::~MCWildcardMatcher()
{
    MCLocaleBreakIteratorRelease(pattern_iter);
    MCLocaleBreakIteratorRelease(source_iter);
}

MCPatternMatcher::~MCPatternMatcher()
{
	MCValueRelease(pattern);
    MCValueRelease(source);
}

////////////////////////////////////////////////////////////////////////////////