/* Copyright (C) 2003-2015 LiveCode Ltd.

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

#include "prefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "globals.h"

#include "handler.h"
#include "scriptpt.h"
#include "newobj.h"
#include "chunk.h"
#include "param.h"
#include "mcerror.h"
#include "property.h"
#include "object.h"
#include "button.h"
#include "field.h"
#include "image.h"
#include "card.h"
#include "eps.h"
#include "graphic.h"
#include "group.h"
#include "scrolbar.h"
#include "player.h"
#include "aclip.h"
#include "vclip.h"
#include "stack.h"
#include "dispatch.h"
#include "stacklst.h"
#include "sellst.h"
#include "undolst.h"
#include "util.h"
#include "date.h"
#include "printer.h"
#include "debug.h"
#include "cmds.h"
#include "mode.h"
#include "osspec.h"
#include "hndlrlst.h"

#include "securemode.h"
#include "syntax.h"
#include "stacksecurity.h"

#include "license.h"

#ifdef _SERVER
#include "srvscript.h"
#endif

#include "exec.h"

MCChoose::~MCChoose()
{
	delete etool;
}

Parse_stat MCChoose::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	sp.skip_token(SP_FACTOR, TT_THE);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add(PE_CHOOSE_NOTOKEN, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_TOOL, te) != PS_NORMAL)
	{
		sp.backup();
		if (sp.parseexp(False, True, &etool) != PS_NORMAL)
		{
			MCperror->add(PE_CHOOSE_BADEXP, sp);
			return PS_ERROR;
		}
		sp.skip_token(SP_TOOL, TT_END);
		return PS_NORMAL;
	}
	else
	{
		littool = (Tool)te->which;
		while (sp.skip_token(SP_TOOL, TT_TOOL) == PS_NORMAL)
			;
		sp.skip_token(SP_TOOL, TT_END);
	}
	return PS_NORMAL;
}

void MCChoose::exec_ctxt(MCExecContext &ctxt)
{
    MCAutoStringRef t_string;
    if (!ctxt . EvalOptionalExprAsStringRef(etool, kMCEmptyString, EE_CHOOSE_BADEXP, &t_string))
        return;
    
    MCInterfaceExecChooseTool(ctxt, *t_string, littool);
}

void MCChoose::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (etool != nil)
		etool -> compile(ctxt);
	else
		MCSyntaxFactoryEvalConstantNil(ctxt);

	MCSyntaxFactoryEvalConstantInt(ctxt, littool);

	MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCInterfaceExecChooseToolMethodInfo, 1);

	MCSyntaxFactoryEndStatement(ctxt);
}

MCConvert::~MCConvert()
{
	delete container;
	delete source;
}

Parse_stat MCConvert::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	MCerrorlock++;
	container = new MCChunk(True);
	MCScriptPoint tsp(sp);
	if (container->parse(sp, False) != PS_NORMAL)
	{
		sp = tsp;
		MCerrorlock--;
		delete container;
		container = NULL;
		if (sp.parseexp(False, True, &source) != PS_NORMAL)
		{
			MCperror->add
			(PE_CONVERT_NOCONTAINER, sp);
			return PS_ERROR;
		}
	}
	else
		MCerrorlock--;
	if (sp.skip_token(SP_FACTOR, TT_FROM) == PS_NORMAL)
	{
		if (parsedtformat(sp, fform, fsform) != PS_NORMAL)
			return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_TO) != PS_NORMAL)
	{
		MCperror->add
		(PE_CONVERT_NOTO, sp);
		return PS_ERROR;
	}
	if (parsedtformat(sp, pform, sform) != PS_NORMAL)
		return PS_ERROR;
	return PS_NORMAL;
}

Parse_stat MCConvert::parsedtformat(MCScriptPoint &sp, Convert_form &firstform,
                                    Convert_form &secondform)
{
	const LT *te;
	Symbol_type type;
	Boolean needformat = True;
	Convert_form localeform = CF_UNDEFINED;
	while (True)
	{
		if (sp.next(type) != PS_NORMAL)
		{
			if (needformat)
			{
				MCperror->add
				(PE_CONVERT_NOFORMAT, sp);
				return PS_ERROR;
			}
			else
				return PS_NORMAL;
		}
		if (sp.lookup(SP_CONVERT, te) != PS_NORMAL)
		{
			if (needformat)
			{
				MCperror->add
				(PE_CONVERT_NOTFORMAT, sp);
				return PS_ERROR;
			}
			else
			{
				sp.backup();
				break;
			}
		}
		switch (te->which)
		{
		case CF_ABBREVIATED:
		case CF_SHORT:
		case CF_LONG:
		case CF_INTERNET:
			if (firstform == CF_UNDEFINED)
				firstform = (Convert_form)te->which;
			else
				secondform = (Convert_form)te->which;
			break;
		case CF_SYSTEM:
		case CF_ENGLISH:
			localeform = (Convert_form)te->which;
			break;
		case CF_DATE:
		case CF_TIME:
			if (firstform == CF_UNDEFINED)
				firstform = (Convert_form)(te->which + 1 + localeform);
			else
				if (firstform > CF_INTERNET)
				{
					if (secondform == CF_UNDEFINED)
						secondform = (Convert_form)(te->which + 1 + localeform);
					else
					{
						uint2 dummy = secondform;
						dummy += te->which + localeform;
						secondform = (Convert_form)dummy;
					}
				}
				else
				{
					uint2 dummy = firstform;
					dummy += te->which + localeform;
					firstform = (Convert_form)dummy;
				}
			needformat = False;
			break;
		default:
			firstform = (Convert_form)te->which;
			return PS_NORMAL;
		}
		if (sp.skip_token(SP_FACTOR, TT_BINOP, O_AND) == PS_NORMAL)
		{
			if (needformat)
			{
				MCperror->add
				(PE_CONVERT_BADAND, sp);
				return PS_ERROR;
			}
			else
				needformat = True;
		}
		else
			if (!needformat)
				break;
	}
	return PS_NORMAL;
}

void MCConvert::exec_ctxt(MCExecContext& ctxt)
{
    MCAutoStringRef t_input;
	MCAutoStringRef t_output;
	if (container != NULL)
    {
        if (!ctxt . EvalExprAsStringRef(container, EE_CONVERT_CANTGET, &t_input))
            return;
    }
	else
	{
        if (!ctxt . EvalExprAsStringRef(source, EE_CONVERT_CANTGET, &t_input))
            return;
    }

	if (container == NULL)
        MCDateTimeExecConvertIntoIt(ctxt, *t_input, fform, fsform, pform, sform);
	else
	{
		MCDateTimeExecConvert(ctxt, *t_input, fform, fsform, pform, sform, &t_output);
        container -> set(ctxt, PT_INTO, *t_output);

        if (ctxt . HasError())
            ctxt . LegacyThrow(EE_CONVERT_CANTSET, *t_output);
	}
}

void MCConvert::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);
	
	if (container == nil)
		source -> compile(ctxt);
	else
		container -> compile_inout(ctxt);

	MCSyntaxFactoryEvalConstantInt(ctxt, fform);
	MCSyntaxFactoryEvalConstantInt(ctxt, fsform);
	MCSyntaxFactoryEvalConstantInt(ctxt, pform);
	MCSyntaxFactoryEvalConstantInt(ctxt, sform);
	
	if (container == nil)
		MCSyntaxFactoryExecMethod(ctxt, kMCDateTimeExecConvertIntoItMethodInfo);
	else
		MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCDateTimeExecConvertMethodInfo, 0, 1, 2, 3, 4, 0);
	
	MCSyntaxFactoryEndStatement(ctxt);
}

MCDo::~MCDo()
{
	delete source;
	delete alternatelang;
	delete widget;
}

Parse_stat MCDo::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add(PE_DO_BADEXP, sp);
		return PS_ERROR;
	}
	
	if (sp.skip_token(SP_FACTOR, TT_IN, PT_IN) == PS_NORMAL)
	{
		if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_BROWSER) == PS_NORMAL)
			browser = True;
		else if (sp.skip_token(SP_SUGAR, TT_UNDEFINED, SG_CALLER) == PS_NORMAL)
			caller = true;
		else
		{
			widget = new MCChunk(False);
			if (widget->parse(sp, False) != PS_NORMAL)
			{
				MCperror->add(PE_DO_BADENV, sp);
				return PS_ERROR;
			}
		}
		
		return PS_NORMAL;
	}
	
	if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AS) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, &alternatelang) != PS_NORMAL)
		{
			MCperror->add(PE_DO_BADLANG, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

void MCDo::exec_ctxt(MCExecContext& ctxt)
{
    MCAutoStringRef t_script;
    if (!ctxt . EvalExprAsStringRef(source, EE_DO_BADEXP, &t_script))
        return;
    
	if (widget)
	{
		MCObject *t_object;
		uint32_t t_parid;
		if (!widget->getobj(ctxt, t_object, t_parid, True) || t_object->gettype() != CT_WIDGET)
		{
			ctxt.LegacyThrow(EE_DO_BADWIDGETEXP);
			return;
		}
		
		MCInterfaceExecDoInWidget(ctxt, *t_script, (MCWidget*)t_object);
		return;
	}
	
    if (browser)
    {
        MCLegacyExecDoInBrowser(ctxt, *t_script);
        return;        
    }
    
    if (alternatelang != NULL)
	{
        MCAutoStringRef t_language;
        if (!ctxt . EvalExprAsStringRef(alternatelang, EE_DO_BADLANG, &t_language))
            return;
        
        MCScriptingExecDoAsAlternateLanguage(ctxt, *t_script, *t_language);
        return;
	}
    
    if (debug)
	{
		MCDebuggingExecDebugDo(ctxt, *t_script, line, pos);
        return;
	}
    
    // AL-2014-11-17: [[ Bug 14044 ]] Do in caller not implemented
    if (caller)
    {
        MCEngineExecDoInCaller(ctxt, *t_script, line, pos);
        return;
    }

	MCEngineExecDo(ctxt, *t_script, line, pos);
}

void MCDo::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	source -> compile(ctxt);

	if (widget != nil)
	{
		widget->compile(ctxt);
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecDoInWidgetMethodInfo);
	}
	else if (browser)
		MCSyntaxFactoryExecMethod(ctxt, kMCLegacyExecDoInBrowserMethodInfo);
	else if (alternatelang != nil)
	{
		alternatelang -> compile(ctxt);
		MCSyntaxFactoryExecMethod(ctxt, kMCScriptingExecDoAsAlternateLanguageMethodInfo);
	}
	else 
	{
		MCSyntaxFactoryEvalConstantInt(ctxt, line);
		MCSyntaxFactoryEvalConstantInt(ctxt, pos);

		if (debug)
			MCSyntaxFactoryExecMethod(ctxt, kMCDebuggingExecDebugDoMethodInfo);
		else
			MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecDoMethodInfo);
	}

	MCSyntaxFactoryEndStatement(ctxt);
}

MCDoMenu::~MCDoMenu()
{
	delete source;
}

Parse_stat MCDoMenu::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add
		(PE_DOMENU_BADEXP, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCDoMenu::exec_ctxt(MCExecContext& ctxt)
{
    MCAutoStringRef t_option;
    if (!ctxt . EvalExprAsStringRef(source, EE_DOMENU_BADEXP, &t_option))
        return;
    
    MCLegacyExecDoMenu(ctxt, *t_option);
}

void MCDoMenu::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	source-> compile(ctxt);

	MCSyntaxFactoryExecMethod(ctxt, kMCLegacyExecDoMenuMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
}

MCEdit::~MCEdit()
{
	delete target;
    delete m_at;
}

Parse_stat MCEdit::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	sp.skip_token(SP_FACTOR, TT_THE);
	if (sp.skip_token(SP_FACTOR, TT_PROPERTY) != PS_NORMAL)
	{
		MCperror->add(PE_EDIT_NOSCRIPT, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_OF) != PS_NORMAL)
	{
		MCperror->add(PE_EDIT_NOOF, sp);
		return PS_ERROR;
	}
	target = new MCChunk(False);
	if (target->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add(PE_EDIT_NOTARGET, sp);
		return PS_ERROR;
	}
    // MERG 2013-9-13: [[ EditScriptChunk ]] Added line and column
    if (sp.skip_token(SP_FACTOR, TT_PREP, PT_AT) == PS_NORMAL)
	{
		if (sp.parseexp(False, True, &m_at) != PS_NORMAL)
        {
            MCperror->add(PE_EDIT_NOAT, sp);
            return PS_ERROR;
        }
	}
	return PS_NORMAL;
}

void MCEdit::exec_ctxt(MCExecContext& ctxt)
{
	MCObject *optr;
    uint4 parid;
    if (!target->getobj(ctxt, optr, parid, True))
    {
        ctxt . LegacyThrow(EE_EDIT_BADTARGET);
        return;
    }

    // MERG 2013-9-13: [[ EditScriptChunk ]] Added at expression that's passed through as a second parameter to editScript
    MCAutoStringRef t_at;
    if (!ctxt . EvalOptionalExprAsNullableStringRef(m_at, EE_EDIT_BADAT, &t_at))
        return;

    MCIdeExecEditScriptOfObject(ctxt, optr, *t_at);
}

void MCEdit::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);
	
	target -> compile_object_ptr(ctxt);

	MCSyntaxFactoryExecMethod(ctxt, kMCIdeExecEditScriptOfObjectMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
}

MCFind::~MCFind()
{
	delete tofind;
	delete field;
}

Parse_stat MCFind::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);

	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_FIND_NOSTRING, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_FIND, te) == PS_NORMAL)
		mode = (Find_mode)te->which;
	else
		sp.backup();
	if (sp.parseexp(False, True, &tofind) != PS_NORMAL)
	{
		MCperror->add
		(PE_FIND_BADSTRING, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_IN) == PS_NORMAL)
	{
		field = new MCChunk(False);
		if (field->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add
			(PE_FIND_BADFIELD, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

void MCFind::exec_ctxt(MCExecContext& ctxt)
{
    MCAutoStringRef t_needle;
    if (!ctxt . EvalExprAsStringRef(tofind, EE_FIND_BADSTRING, &t_needle))
        return;

    MCInterfaceExecFind(ctxt, mode, *t_needle, field);
    
    // SN-2014-03-21: [[ Bug 11949 ]] 'find' shouldn't throw an error on a failure
    // but MCInterfaceExecFind would cause the context to be set on error if finding fails
    ctxt . IgnoreLastError();
}

void MCFind::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	MCSyntaxFactoryEvalConstantInt(ctxt, mode);
	tofind -> compile(ctxt);
	field -> compile(ctxt);

	MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecFindMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
}

MCGet::~MCGet()
{
	delete value;
}

Parse_stat MCGet::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &value) != PS_NORMAL)
	{
		MCperror->add
		(PE_GET_BADEXP, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCGet::exec_ctxt(MCExecContext& ctxt)
{
    MCExecValue t_value;
    if (!ctxt . EvaluateExpression(value, EE_GET_BADEXP, t_value))
        return;

    MCEngineExecGet(ctxt, t_value);
}

void MCGet::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	value -> compile(ctxt);

	MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecGetMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
}

MCMarking::~MCMarking()
{
	delete where;
	delete tofind;
	delete field;
	delete card;
}

Parse_stat MCMarking::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);

	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_MARK_NOCARDS, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_MARK, te) != PS_NORMAL)
	{
		sp.backup();
		card = new MCChunk(False);
		if (card->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add
			(PE_MARK_NOTCARDS, sp);
			return PS_ERROR;
		}
		return PS_NORMAL;
	}
	if (te->which == MC_ALL)
	{
		if (sp.skip_token(SP_MARK, TT_UNDEFINED, MC_CARDS) != PS_NORMAL)
		{
			MCperror->add
			(PE_MARK_NOCARDS, sp);
			return PS_ERROR;
		}
		return PS_NORMAL;
	}
	if (te->which != MC_CARDS)
	{
		MCperror->add
		(PE_MARK_NOCARDS, sp);
		return PS_ERROR;
	}
if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_MARK_NOBYORWHERE, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_MARK,te) != PS_NORMAL
	    || (te->which != MC_BY && te->which != MC_WHERE))
	{
		MCperror->add
		(PE_MARK_NOTBYORWHERE, sp);
		return PS_ERROR;
	}
	if (te->which == MC_WHERE)
	{
		if (sp.parseexp(False, True, &where) != PS_NORMAL)
		{
			MCperror->add
			(PE_MARK_BADWHEREEXP, sp);
			return PS_ERROR;
		}
		return PS_NORMAL;
	}
	if (sp.skip_token(SP_MARK, TT_UNDEFINED, MC_FINDING) != PS_NORMAL)
	{
		MCperror->add
		(PE_MARK_NOFINDING, sp);
		return PS_ERROR;
	}
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_MARK_NOSTRING, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_FIND, te) == PS_NORMAL)
		mode = (Find_mode)te->which;
	else
		sp.backup();
	if (sp.parseexp(False, True, &tofind) != PS_NORMAL)
	{
		MCperror->add
		(PE_MARK_BADSTRING, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_IN) == PS_NORMAL)
	{
		field = new MCChunk(False);
		if (field->parse(sp, False) != PS_NORMAL)
		{
			MCperror->add
			(PE_MARK_BADFIELD, sp);
			return PS_ERROR;
		}
	}
	return PS_NORMAL;
}

void MCMarking::exec_ctxt(MCExecContext &ctxt)
{
    if (card != NULL)
	{
        MCObjectPtr t_object;
        if (!card->getobj(ctxt, t_object, True)
            || t_object . object -> gettype() != CT_CARD)
		{
            ctxt . LegacyThrow(EE_MARK_BADCARD);
            return;
        }

        if (mark)
            MCInterfaceExecMarkCard(ctxt, t_object);
        else
            MCInterfaceExecUnmarkCard(ctxt, t_object);
	}
    // SN-2014-03-21 [[ Bug 11950 ]]: 'mark' shouldn't throw an error when failing to mark when card is nil
    // Any error set is discarded in the end of this block - unless is was triggered by a bad string.
    else
    {
        if (tofind == nil)
        {
            if (mark)
            {
                if (where != nil)
                    MCInterfaceExecMarkCardsConditional(ctxt, where);
                else
                    MCInterfaceExecMarkAllCards(ctxt);
            }
            else
            {
                if (where != nil)
                    MCInterfaceExecUnmarkCardsConditional(ctxt, where);
                else
                    MCInterfaceExecUnmarkAllCards(ctxt);
            }
        }
        else
        {
            MCAutoStringRef t_needle;
            
            if (!ctxt . EvalExprAsStringRef(tofind, EE_MARK_BADSTRING, &t_needle))
                return;
            
            if (mark)
                MCInterfaceExecMarkFind(ctxt, mode, *t_needle, field);
            else
                MCInterfaceExecUnmarkFind(ctxt, mode, *t_needle, field);
        }
        
        ctxt . IgnoreLastError();
    }
}

MCPut::~MCPut()
{
	delete source;
	delete dest;
	
	// cookie
	delete name;
	delete domain;
	delete path;
	delete expires;
}
		
// put [ unicode | binary ] <expr>
// put [ unicode ] ( content | markup ) <expr>
// put [ new ] header <expr>
// put [ secure ] [ httponly ] cookie <name> [ for <path> ] [ on <domain> ] to <value> [ until <expiry> ]
// put <expr> ( into | after | before ) message [ box ]
// put <expr> ( into | after | before ) <chunk>
//
Parse_stat MCPut::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	
	// IM-2011-08-22: [[ SERVER ]] Add support for new put variant.
	// Parse: put [ secure ] [ httponly ] cookie <name> [ for path ] [ on domain ] with <value> [ until expires ]
	if (sp . skip_token(SP_SERVER, TT_SERVER, SK_SECURE) == PS_NORMAL)
		is_secure = true;
	
	if (sp . skip_token(SP_SERVER, TT_SERVER, SK_HTTPONLY) == PS_NORMAL)
		is_httponly = true;
	
	if (sp . skip_token(SP_SERVER, TT_PREP, PT_COOKIE) == PS_NORMAL)
	{
		prep = PT_COOKIE;
		if (sp . parseexp(True, False, &name) != PS_NORMAL)
		{
			MCperror->add(PE_PUT_BADEXP, sp);
			return PS_ERROR;
		}
		if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_FOR) == PS_NORMAL)
		{
			if (sp . parseexp(True, False, &path) != PS_NORMAL)
			{
				MCperror->add(PE_PUT_BADEXP, sp);
				return PS_ERROR;
			}
		}
		if (sp . skip_token(SP_FACTOR, TT_OF, PT_ON) == PS_NORMAL)
		{
			if (sp . parseexp(True, False, &domain) != PS_NORMAL)
			{
				MCperror->add(PE_PUT_BADEXP, sp);
				return PS_ERROR;
			}
		}
		sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH);
		if (sp . parseexp(True, False, &source) != PS_NORMAL)
		{
			MCperror->add(PE_PUT_BADEXP, sp);
			return PS_ERROR;
		}
		if (sp . skip_token(SP_REPEAT, TT_UNDEFINED, RF_UNTIL) == PS_NORMAL)
		{
			if (sp . parseexp(True, False, &expires) != PS_NORMAL)
			{
				MCperror->add(PE_PUT_BADEXP, sp);
				return PS_ERROR;
			}
		}
		
		return PS_NORMAL;
	}
	
	if (is_secure || is_httponly)
	{
		MCperror->add(PE_PUT_BADPREP, sp);
		return PS_ERROR;
	}
	
	// MW-2011-06-22: [[ SERVER ]] Add support for new put variant.
	// Parse: put new header <expr>
	if (sp.skip_token(SP_SERVER, TT_SERVER, SK_NEW) == PS_NORMAL)
	{
		if (sp.skip_token(SP_SERVER, TT_PREP, PT_HEADER) == PS_NORMAL)
		{
			prep = PT_NEW_HEADER;
			if (sp . parseexp(False, True, &source) != PS_NORMAL)
			{
				MCperror->add(PE_PUT_BADEXP, sp);
				return PS_ERROR;
			}
			
			return PS_NORMAL;
		}
		else
			sp.backup();			
	}

	// MW-2012-02-23: [[ UnicodePut ]] Store whether 'unicode' was present
	//   in the ast.
	if (sp . skip_token(SP_SERVER, TT_SERVER, SK_UNICODE) == PS_NORMAL)
		is_unicode = true;

	// MW-2011-06-22: [[ SERVER ]] Add support for new put variant.
	// Parse: put [ unicode ] ( header | content | markup ) <expr>	
	if (sp.next(type) == PS_NORMAL)
	{
		if (type == ST_ID && sp.lookup(SP_SERVER, te) == PS_NORMAL && te -> type == TT_PREP)
		{
			prep = (Preposition_type)te -> which;
			if (is_unicode && (prep == PT_HEADER || prep == PT_BINARY))
			{
				MCperror->add(PE_PUT_BADPREP, sp);
				return PS_ERROR;
			}
			
			if (sp . parseexp(False, True, &source) != PS_NORMAL)
			{
				MCperror->add(PE_PUT_BADEXP, sp);
				return PS_ERROR;
			}
			
			return PS_NORMAL;
		}
		else
			sp.backup();
	}

	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add(PE_PUT_BADEXP, sp);
		return PS_ERROR;
	}
	
	if (sp.next(type) != PS_NORMAL)
		return PS_NORMAL;
		
	if (sp.lookup(SP_FACTOR, te) != PS_NORMAL || te->type != TT_PREP)
	{
		sp.backup();
		return PS_NORMAL;
	}
	prep = (Preposition_type)te->which;
	if (prep != PT_BEFORE && prep != PT_INTO && prep != PT_AFTER)
	{
		MCperror->add(PE_PUT_BADPREP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_MESSAGE) == PS_NORMAL)
	{
		sp.skip_token(SP_SHOW, TT_UNDEFINED, SO_MESSAGE); // "box"
		return PS_NORMAL;
	}
	dest = new MCChunk(True);
	if (dest->parse(sp, False) != PS_NORMAL)
	{
		MCperror->add(PE_PUT_BADCHUNK, sp);
		return PS_ERROR;
	}

	MCVarref *t_src_ref, *t_dst_ref;
	t_src_ref = source -> getrootvarref();
	t_dst_ref = dest -> getrootvarref();
	overlap = t_src_ref != NULL && t_dst_ref != NULL && t_src_ref -> rootmatches(t_dst_ref);

	return PS_NORMAL;
}

void MCPut::exec_ctxt(MCExecContext& ctxt)
{
    
    MCExecValue t_value;
    if (!ctxt . EvaluateExpression(source, EE_PUT_BADEXP, t_value))
        return;
	
    if (dest != nil)
    {
//        MCAutoValueRef t_valueref;
//        MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value, kMCExecValueTypeValueRef, &(&t_valueref));
//        dest -> set(ctxt, prep, *t_valueref, is_unicode);
        dest -> set(ctxt, prep, t_value, is_unicode);
	}
    else
	{        
        if (ctxt . HasError())
            return;
        
		MCAutoValueRef t_val;
		if (is_unicode && (prep == PT_UNDEFINED || prep == PT_CONTENT || prep == PT_MARKUP))
        {
            MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value, kMCExecValueTypeDataRef, &(&t_val));
			if (ctxt . HasError())
			{
                ctxt . LegacyThrow(EE_CHUNK_CANTSETDEST);
				return;
			}
        }
		else
        {
            MCExecTypeConvertAndReleaseAlways(ctxt, t_value . type, &t_value, kMCExecValueTypeStringRef, &(&t_val));
            if (ctxt . HasError())
			{
                ctxt . LegacyThrow(EE_CHUNK_CANTSETDEST);
				return;
			}
        }
		
		// Defined for convenience
		MCStringRef t_string = (MCStringRef)*t_val;
		MCDataRef t_data = (MCDataRef)*t_val;
		
		if (prep == PT_COOKIE)
		{
            MCAutoStringRef t_name;
            if (!ctxt . EvalOptionalExprAsStringRef(name, kMCEmptyString, EE_PUT_CANTSETINTO, &t_name))
                return;
						
			uinteger_t t_expires;
            if (!ctxt . EvalOptionalExprAsUInt(expires, 0, EE_PUT_CANTSETINTO, t_expires))
                return;
						
			MCAutoStringRef t_path;            
			if (!ctxt . EvalOptionalExprAsStringRef(path, kMCEmptyString, EE_PUT_CANTSETINTO, &t_path))
                return;			
			
			MCAutoStringRef t_domain;
			if (!ctxt . EvalOptionalExprAsStringRef(domain, kMCEmptyString, EE_PUT_CANTSETINTO, &t_domain))
                return;
			
			MCServerExecPutCookie(ctxt, *t_name, t_string, t_expires, *t_path, *t_domain, is_secure, is_httponly);
		}
		else if (prep == PT_UNDEFINED)
		{
			if (is_unicode)
				MCEngineExecPutOutputUnicode(ctxt, t_data);
			else
				MCEngineExecPutOutput(ctxt, t_string);
		}
		else if (prep == PT_INTO || prep == PT_AFTER || prep == PT_BEFORE)
			MCIdeExecPutIntoMessage(ctxt, t_string, prep);
		else if (prep == PT_HEADER || prep == PT_NEW_HEADER)
			MCServerExecPutHeader(ctxt, t_string, prep == PT_NEW_HEADER);
		else if (prep == PT_CONTENT)
		{
			if (is_unicode)
				MCServerExecPutContentUnicode(ctxt, t_data);
			else
				MCServerExecPutContent(ctxt, t_string);
		}
		else if (prep == PT_MARKUP)
		{
			if (is_unicode)
				MCServerExecPutMarkupUnicode(ctxt, t_data);
			else
				MCServerExecPutMarkup(ctxt, t_string);
		}
		else if (prep == PT_BINARY)
            MCServerExecPutBinaryOutput(ctxt, t_data);
	}
}

void MCPut::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (dest != nil)
	{
		source -> compile(ctxt);
		MCSyntaxFactoryEvalConstantInt(ctxt, prep);
		dest -> compile_out(ctxt);
		MCSyntaxFactoryEvalConstantBool(ctxt, is_unicode);
		
		MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCEngineExecPutIntoVariableMethodInfo, 0, 1, 2);
		MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCNetworkExecPutIntoUrlMethodInfo, 0, 1, 2);
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecPutIntoFieldMethodInfo);
		MCSyntaxFactoryExecMethodWithArgs(ctxt, kMCInterfaceExecPutIntoObjectMethodInfo, 0, 1, 2);	
	}
	else if (prep == PT_COOKIE)
	{
		name -> compile(ctxt);
		source -> compile(ctxt);
			
		if (expires != nil)
			expires -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantUInt(ctxt, 0);
		
		if (path != nil)
			path -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);
	
		if (domain != nil)
			domain -> compile(ctxt);
		else
			MCSyntaxFactoryEvalConstantNil(ctxt);

		MCSyntaxFactoryEvalConstantBool(ctxt, is_secure);
		MCSyntaxFactoryEvalConstantBool(ctxt, is_httponly);

		MCSyntaxFactoryExecMethod(ctxt, kMCServerExecPutCookieMethodInfo);
	}
	else 
	{
		source -> compile(ctxt);

		switch (prep)
		{
		case PT_UNDEFINED:
			MCSyntaxFactoryEvalConstantBool(ctxt, is_unicode);
			MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecPutOutputMethodInfo);
			break;
		case PT_INTO:
		case PT_AFTER:
		case PT_BEFORE:
			MCSyntaxFactoryEvalConstantInt(ctxt, prep);
			MCSyntaxFactoryExecMethod(ctxt, kMCIdeExecPutIntoMessageMethodInfo);
			break;
		case PT_HEADER:
		case PT_NEW_HEADER:
			MCSyntaxFactoryEvalConstantBool(ctxt, prep == PT_NEW_HEADER);
			MCSyntaxFactoryExecMethod(ctxt, kMCServerExecPutHeaderMethodInfo);
			break;
		case PT_CONTENT:
			MCSyntaxFactoryEvalConstantBool(ctxt, is_unicode);
			MCSyntaxFactoryExecMethod(ctxt, kMCServerExecPutContentMethodInfo);
			break;
		case PT_MARKUP:
			MCSyntaxFactoryEvalConstantBool(ctxt, is_unicode);
			MCSyntaxFactoryExecMethod(ctxt, kMCServerExecPutMarkupMethodInfo);
			break;
		case PT_BINARY:
			MCSyntaxFactoryExecMethod(ctxt, kMCServerExecPutBinaryOutputMethodInfo);
			break;
		default:
			break;
		}
	}

	MCSyntaxFactoryEndStatement(ctxt);
}

MCQuit::~MCQuit()
{
	delete retcode;
}

Parse_stat MCQuit::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	MCerrorlock++;
	sp.parseexp(False, True, &retcode);
	MCerrorlock--;
	return PS_NORMAL;
}

void MCQuit::exec_ctxt(MCExecContext& ctxt)
{
    integer_t t_retcode;
    if (!ctxt . EvalOptionalExprAsInt(retcode, 0, EE_UNDEFINED, t_retcode))
        return;
    
    MCEngineExecQuit(ctxt, t_retcode);
}

void MCQuit::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (retcode != nil)
		retcode -> compile(ctxt);
	else
		MCSyntaxFactoryEvalConstantInt(ctxt, 0);

	MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecQuitMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
}

Parse_stat MCReset::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);
	sp.skip_token(SP_FACTOR, TT_THE);
	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_CHOOSE_NOTOKEN, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_RESET, te) != PS_NORMAL)
	{
		MCperror->add
		(PE_RESET_NOTYPE, sp);
		return PS_ERROR;
	}
	which = (Reset_type)te->which;
	return PS_NORMAL;
}

void MCReset::exec_ctxt(MCExecContext& ctxt)
{
    switch (which)
	{
		case RT_CURSORS:
			MCInterfaceExecResetCursors(ctxt);
			break;
		case RT_PAINT:
			MCGraphicsExecResetPaint(ctxt);
			break;
		case RT_PRINTING:
			MCPrintingExecResetPrinting(ctxt);
			break;
		default:
			MCInterfaceExecResetTemplate(ctxt, which);
		break;
	}

	return;
}

void MCReset::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	switch (which)
	{
	case RT_CURSORS:
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecResetCursorsMethodInfo);
		break;

	case RT_PAINT:
		MCSyntaxFactoryExecMethod(ctxt, kMCGraphicsExecResetPaintMethodInfo);
		break;

	case RT_PRINTING:
		MCSyntaxFactoryExecMethod(ctxt, kMCPrintingExecResetPrintingMethodInfo);
		break;

	default:
		MCSyntaxFactoryEvalConstantInt(ctxt, which);
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecResetTemplateMethodInfo);
		break;
	}

	MCSyntaxFactoryEndStatement(ctxt);
}

MCReturn::~MCReturn()
{
	delete source;
	delete url;
	delete var;
}

Parse_stat MCReturn::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.parseexp(False, True, &source) != PS_NORMAL)
	{
		MCperror->add
		(PE_RETURN_BADEXP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
	{
		if (sp.skip_token(SP_FACTOR, TT_CHUNK, CT_URL))
		{
			// MW-2011-06-22: [[ SERVER ]] Update to use SP findvar method to take into account
			//   execution outwith a handler.
			Symbol_type type;
			if (sp.next(type) != PS_NORMAL || sp.findvar(sp.gettoken_nameref(), &var) != PS_NORMAL)
				sp.backup();
			else
				var->parsearray(sp);
		}
		if (var == NULL)
		{
			sp.skip_token(SP_FACTOR, TT_FUNCTION, F_CACHED_URLS);
			if (sp.parseexp(False, True, &url) != PS_NORMAL)
			{
				MCperror->add
				(PE_RETURN_BADEXP, sp);
				return PS_ERROR;
			}
		}
	}
	return PS_NORMAL;
}


// MW-2007-07-03: [[ Bug 4570 ]] - Using the return command now causes a
//   RETURN_HANDLER status rather than EXIT_HANDLER. This is used to not
//   clear the result in this case. (see MCHandler::exec).
void MCReturn::exec_ctxt(MCExecContext &ctxt)
{
	MCAutoValueRef t_result;

    if (!ctxt . EvalExprAsValueRef(source, EE_RETURN_BADEXP, &t_result))
        return;
	
	if (url != nil)
	{
		MCAutoValueRef t_url_result;
        if (!ctxt . EvalExprAsValueRef(url, EE_RETURN_BADEXP, &t_url_result))
            return;

		MCNetworkExecReturnValueAndUrlResult(ctxt, *t_result, *t_url_result);
	}
	else if (var != nil)
	{
		MCNetworkExecReturnValueAndUrlResultFromVar(ctxt, *t_result, var);
	}
	else
	{
		MCEngineExecReturnValue(ctxt, *t_result);
	}
	
	if (!ctxt . HasError())
        ctxt . SetIsReturnHandler();
}

uint4 MCReturn::linecount()
{
	return 0;
}

void MCReturn::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	source -> compile(ctxt);

	if (url != nil)
	{
		url -> compile(ctxt);
		MCSyntaxFactoryExecMethod(ctxt, kMCNetworkExecReturnValueAndUrlResultMethodInfo);
	}
	else if (var != nil)
	{
		MCSyntaxFactoryEvalConstant(ctxt, var);
		MCSyntaxFactoryExecMethod(ctxt, kMCNetworkExecReturnValueAndUrlResultFromVarMethodInfo);
	}
	else
		MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecReturnValueMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
}

Parse_stat MCScriptError::parse(MCScriptPoint &sp)
{
	return PS_ERROR; // catch on/function/getprop/setprop
}

MCSet::~MCSet()
{
	delete target;
	delete value;
}

Parse_stat MCSet::parse(MCScriptPoint &sp)
{
	initpoint(sp);
	if (sp.skip_token(SP_FACTOR, TT_THE) == PS_ERROR)
	{
		MCperror->add(PE_SET_NOTHE, sp);
		return PS_ERROR;
	}
	target = new MCProperty;
	if (target->parse(sp, True) != PS_NORMAL)
	{
		MCperror->add(PE_SET_NOPROP, sp);
		return PS_ERROR;
	}
	if (sp.skip_token(SP_FACTOR, TT_TO) != PS_NORMAL)
	{
		MCperror->add(PE_SET_NOTO, sp);
		return PS_ERROR;
	}
	if (sp.parseexp(False, True, &value) != PS_NORMAL)
	{
		MCperror->add(PE_SET_BADEXP, sp);
		return PS_ERROR;
	}
	return PS_NORMAL;
}

void MCSet::exec_ctxt(MCExecContext& ctxt)
{
    MCAutoValueRef t_value;
    if (!ctxt . EvalExprAsValueRef(value, EE_SET_BADEXP, &t_value))
        return;
    
    ctxt . SetTheResultToEmpty();
    MCEngineExecSet(ctxt, target, *t_value);
}

/*void MCSet::compile(MCSyntaxFactoryRef ctxt)
{
	
}*/

MCSort::~MCSort()
{
	delete of;
	delete by;
}

Parse_stat MCSort::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);

	while (True)
	{
		if (sp.next(type) != PS_NORMAL)
		{
			if (of == NULL && chunktype == CT_FIELD)
			{
				MCperror->add
				(PE_SORT_NOTARGET, sp);
				return PS_ERROR;
			}
			else
				break;
		}
		if (sp.lookup(SP_SORT, te) == PS_NORMAL)
		{
			switch (te->which)
			{
			case ST_OF:
				of = new MCChunk(True);
				if (of->parse(sp, False) != PS_NORMAL)
				{
					MCperror->add
					(PE_SORT_BADTARGET, sp);
					return PS_ERROR;
				}
				break;
			case ST_BY:
				if (chunktype == CT_FIELD && of == NULL)
					chunktype = CT_CARD;
				if (sp.parseexp(False, True, &by) != PS_NORMAL)
				{
					MCperror->add
					(PE_SORT_BADEXPRESSION, sp);
					return PS_ERROR;
				}
				if (of == NULL && chunktype == CT_FIELD)
				{
					MCperror->add
					(PE_SORT_NOTARGET, sp);
					return PS_ERROR;
				}
				return PS_NORMAL;
			case ST_LINES:
				chunktype = CT_LINE;
				break;
			case ST_ITEMS:
				chunktype = CT_ITEM;
				break;
			case ST_MARKED:
				chunktype = CT_MARKED;
				break;
			case ST_CARDS:
				if (chunktype != CT_MARKED)
					chunktype = CT_CARD;
				break;
			case ST_TEXT:
            case ST_BINARY:
			case ST_NUMERIC:
			case ST_INTERNATIONAL:
			case ST_DATETIME:
				format = (Sort_type)te->which;
				break;
			case ST_ASCENDING:
			case ST_DESCENDING:
				direction = (Sort_type)te->which;
				break;
			}
		}
		else
		{
			sp.backup();
			if (of == NULL)
			{
				of = new MCChunk(True);
				if (of->parse(sp, False) != PS_NORMAL)
				{
					MCperror->add
					(PE_SORT_BADTARGET, sp);
					return PS_ERROR;
				}
			}
			else
				break;
		}
	}
	return PS_NORMAL;
}

void MCSort::exec_ctxt(MCExecContext& ctxt)
{
    
    MCObjectPtr t_object;
    MCAutoStringRef t_target;
    
    // SN-2014-03-21: [[ Bug 11953 ]] sort card does not work
    t_object . object = nil;
    t_object . part_id = 0;
    
	if (of != NULL)
	{
		MCerrorlock++;
        of->getoptionalobj(ctxt, t_object, False);
		if (t_object . object == nil || t_object . object->gettype() == CT_BUTTON)
		{
			MCerrorlock--;

            if (!ctxt . EvalExprAsStringRef(of, EE_SORT_BADTARGET, &t_target))
                return;
		}
		else
        {
			MCerrorlock--;
        }
		if (t_object . object != nil && t_object . object->gettype() > CT_GROUP && chunktype <= CT_GROUP)
			chunktype = CT_LINE;
    }
    // SN-2015-04-01: [[ Bug 14885 ]] Make sure that the default stack is used
    //  if none is specified.
    else
        t_object . object = MCdefaultstackptr;
    
	if (chunktype == CT_CARD || chunktype == CT_MARKED)
    {
        if (t_object . object == nil ||
            t_object . object -> gettype() != CT_STACK)
		{
            ctxt . LegacyThrow(EE_SORT_CANTSORT);
			return;
		}
        
        MCInterfaceExecSortCardsOfStack(ctxt, (MCStack *)t_object . object, direction == ST_ASCENDING, format, by, chunktype == CT_MARKED);
    }
	else if (t_object . object == nil || t_object . object->gettype() == CT_BUTTON)
	{
        MCStringRef t_sorted_target;

        if (*t_target == nil)
            t_sorted_target = MCValueRetain(kMCEmptyString);
        else
            t_sorted_target = MCValueRetain(*t_target);

        MCInterfaceExecSortContainer(ctxt, t_sorted_target, chunktype, direction == ST_ASCENDING, format, by);
        if (!ctxt . HasError())
            of -> set(ctxt, PT_INTO, t_sorted_target);

        MCValueRelease(t_sorted_target);
	}
	else
	{
		if (t_object . object->gettype() != CT_FIELD || !of->notextchunks())
		{
            ctxt . LegacyThrow(EE_SORT_CANTSORT);
			return;
		}
		MCInterfaceExecSortField(ctxt, t_object, chunktype, direction == ST_ASCENDING, format, by);
	}
}

void MCSort::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (chunktype == CT_CARD || chunktype == CT_MARKED)
	{
		of -> compile_object_ptr(ctxt);
		MCSyntaxFactoryEvalConstantBool(ctxt, direction == ST_ASCENDING);
		MCSyntaxFactoryEvalConstantInt(ctxt, format);
		by -> compile(ctxt);
		MCSyntaxFactoryEvalConstantBool(ctxt, chunktype == CT_MARKED);
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecSortCardsOfStackMethodInfo);
	}
	else
	{
		of -> compile_inout(ctxt);

		if (chunktype <= CT_GROUP)
			MCSyntaxFactoryEvalConstantInt(ctxt, CT_LINE);
		else
			MCSyntaxFactoryEvalConstantInt(ctxt, chunktype);
			
		MCSyntaxFactoryEvalConstantBool(ctxt, direction == ST_ASCENDING);
		MCSyntaxFactoryEvalConstantInt(ctxt, format);
		by -> compile(ctxt);

		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecSortFieldMethodInfo);
		MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecSortContainerMethodInfo);
	}

	MCSyntaxFactoryEndStatement(ctxt);
}

MCWait::~MCWait()
{
	delete duration;
}

Parse_stat MCWait::parse(MCScriptPoint &sp)
{
	Symbol_type type;
	const LT *te;

	initpoint(sp);

	if (sp.next(type) != PS_NORMAL)
	{
		MCperror->add
		(PE_WAIT_NODURATION, sp);
		return PS_ERROR;
	}
	if (sp.lookup(SP_REPEAT, te) == PS_NORMAL)
		condition = (Repeat_form)te->which;
	else
	{
		condition = RF_FOR;
		sp.backup();
	}

	if (sp.skip_token(SP_MOVE, TT_UNDEFINED, MM_MESSAGES) == PS_NORMAL)
		messages = True;
	else
	{
		if (sp.parseexp(False, True, &duration) != PS_NORMAL)
		{
			MCperror->add
			(PE_WAIT_BADCOND, sp);
			return PS_ERROR;
		}
		if (condition == RF_FOR)
		{
			if (sp.skip_token(SP_FACTOR, TT_FUNCTION, F_MILLISECS) == PS_NORMAL)
				units = F_MILLISECS;
			else if (sp.skip_token(SP_FACTOR, TT_FUNCTION, F_SECONDS) == PS_NORMAL
			         || sp.skip_token(SP_FACTOR, TT_CHUNK, CT_SECOND) == PS_NORMAL)
				units = F_SECONDS;
			else if (sp.skip_token(SP_FACTOR, TT_FUNCTION, F_TICKS) == PS_NORMAL)
				units = F_TICKS;
			else
				units = F_TICKS;
		}
		if (sp.skip_token(SP_REPEAT, TT_UNDEFINED, RF_WITH) == PS_NORMAL)
		{
			sp.skip_token(SP_MOVE, TT_UNDEFINED, MM_MESSAGES);
			messages = True;
		}
	}
	return PS_NORMAL;
}

void MCWait::exec_ctxt(MCExecContext& ctxt)
{
    if (duration == NULL)
		MCEngineExecWaitFor(ctxt, MCmaxwait, F_UNDEFINED, messages == True);

    else
	{
		switch (condition)
		{
            case RF_FOR:
            {
                double t_delay;
                if (!ctxt . EvalExprAsDouble(duration, EE_WAIT_BADEXP, t_delay))
                    return;
                
                MCEngineExecWaitFor(ctxt, t_delay, units, messages == True);
                break;
            }
            case RF_UNTIL:
                MCEngineExecWaitUntil(ctxt, duration, messages == True);
                break;
            case RF_WHILE:
                MCEngineExecWaitWhile(ctxt, duration, messages == True);
                break;
            default:
                return;
		}
	}
}

void MCWait::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	if (duration == nil)
	{
		MCSyntaxFactoryEvalConstantDouble(ctxt, MCmaxwait);
		MCSyntaxFactoryEvalConstantInt(ctxt, F_UNDEFINED);
		MCSyntaxFactoryEvalConstantBool(ctxt, messages == True);

		MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecWaitForMethodInfo);
	}
	else
	{
		duration -> compile(ctxt);

		switch (condition)
		{
		case RF_FOR:
			MCSyntaxFactoryEvalConstantInt(ctxt, units);
			MCSyntaxFactoryEvalConstantBool(ctxt, messages == True);
			MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecWaitForMethodInfo);
			break;

		case RF_UNTIL:
			MCSyntaxFactoryEvalConstantBool(ctxt, messages == True);
			MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecWaitUntilMethodInfo);
			break;

		case RF_WHILE:
			MCSyntaxFactoryEvalConstantBool(ctxt, messages == True);
			MCSyntaxFactoryExecMethod(ctxt, kMCEngineExecWaitWhileMethodInfo);
			break;

		default:
			break;
		}
	}
}

MCInclude::~MCInclude(void)
{
	delete filename;
}

Parse_stat MCInclude::parse(MCScriptPoint& sp)
{
	initpoint(sp);
	
	if (sp . parseexp(False, True, &filename) != PS_NORMAL)
	{
		MCperror -> add(PE_INCLUDE_BADFILENAME, sp);
		return PS_ERROR;
	}
	
	return PS_NORMAL;
}

void MCInclude::exec_ctxt(MCExecContext& ctxt)
{	
    MCAutoStringRef t_filename;
    if (!ctxt . EvalExprAsStringRef(filename, EE_INCLUDE_BADFILENAME, &t_filename))
        return;
    
    MCServerExecInclude(ctxt, *t_filename, is_require);
}

void MCInclude::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	filename -> compile(ctxt);
	MCSyntaxFactoryEvalConstantBool(ctxt, is_require);

	MCSyntaxFactoryExecMethod(ctxt, kMCServerExecIncludeMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
}

MCEcho::~MCEcho(void)
{
	if (data != nil)
		MCValueRelease(data);
}

Parse_stat MCEcho::parse(MCScriptPoint& sp)
{
	initpoint(sp);
	data = MCValueRetain(sp . gettoken_stringref());
	return PS_NORMAL;
}

void MCEcho::exec_ctxt(MCExecContext& ctxt)
{
	MCServerExecEcho(ctxt, data);
	return;
}

void MCEcho::compile(MCSyntaxFactoryRef ctxt)
{
	MCSyntaxFactoryBeginStatement(ctxt, line, pos);

	MCSyntaxFactoryEvalConstant(ctxt, data);

	MCSyntaxFactoryExecMethod(ctxt, kMCServerExecEchoMethodInfo);

	MCSyntaxFactoryEndStatement(ctxt);
}

MCResolveImage::~MCResolveImage(void)
{
    delete m_relative_object;
    delete m_id_or_name;
}

Parse_stat MCResolveImage::parse(MCScriptPoint &p_sp)
{
    Parse_stat t_stat;
    t_stat = PS_NORMAL;
    
    if (t_stat == PS_NORMAL)
        t_stat =  p_sp.skip_token(SP_FACTOR, TT_CHUNK, CT_IMAGE);
	
	// Parse the optional 'id' token
    m_is_id = (PS_NORMAL == p_sp . skip_token(SP_FACTOR, TT_PROPERTY, P_ID));
    
    // Parse the id_or_name expression
    if (t_stat == PS_NORMAL)
        t_stat = p_sp . parseexp(False, True, &m_id_or_name);
    
    if (t_stat != PS_NORMAL)
    {
        MCperror->add
        (PE_RESOLVE_BADIMAGE, p_sp);
        return PS_ERROR;
    }
    
    // Parse the 'relative to' tokens
    if (t_stat == PS_NORMAL)
        t_stat = p_sp . skip_token(SP_FACTOR, TT_TO, PT_RELATIVE);
    
    if (t_stat == PS_NORMAL)
        t_stat = p_sp . skip_token(SP_FACTOR, TT_TO, PT_TO);
    
    // Parse the target object clause
    if (t_stat == PS_NORMAL)
    {
        m_relative_object = new MCChunk(false);
        t_stat = m_relative_object -> parse(p_sp, False);
    }
    else
    {
        MCperror->add
        (PE_RESOLVE_BADOBJECT, p_sp);
        return PS_ERROR;
    }
    return t_stat;
}

void MCResolveImage::exec_ctxt(MCExecContext &ctxt)
{
    
    uint4 t_part_id;
    MCObject *t_relative_object;


    if (!m_relative_object -> getobj(ctxt, t_relative_object, t_part_id, True))
    {
            ctxt . Throw();
            return;
    }

    if (m_is_id)
    {
        uinteger_t t_id;
        if (!ctxt . EvalExprAsUInt(m_id_or_name, EE_RESOLVE_IMG_BADEXP, t_id))
        return;

        MCInterfaceExecResolveImageById(ctxt, t_relative_object, t_id);
    }
    else
    {
        MCAutoStringRef t_name;

        if (!ctxt . EvalExprAsStringRef(m_id_or_name, EE_RESOLVE_IMG_BADEXP, &t_name))
        return;

        MCInterfaceExecResolveImageByName(ctxt, t_relative_object, *t_name);
    }
}

void MCResolveImage::compile(MCSyntaxFactoryRef ctxt)
{
    MCSyntaxFactoryBeginStatement(ctxt, line, pos);
    
    m_relative_object -> compile_object_ptr(ctxt);
    m_id_or_name -> compile(ctxt);
    
    if (m_is_id)
        MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecResolveImageByIdMethodInfo);
    else
        MCSyntaxFactoryExecMethod(ctxt, kMCInterfaceExecResolveImageByNameMethodInfo);
    
    MCSyntaxFactoryEndStatement(ctxt);
}

////////////////////////////////////////////////////////////////////////////////
//
//  MW-2013-11-14: [[ AssertCmd ]] Implementation of 'assert' command.
//

MCAssertCmd::~MCAssertCmd(void)
{
	delete m_expr;
}

// assert <expr>
// assert true <expr>
// assert false <expr>
// assert success <expr>
// assert failure <expr>
Parse_stat MCAssertCmd::parse(MCScriptPoint& sp)
{
	initpoint(sp);
	
	// See if there is a type token
	MCScriptPoint temp_sp(sp);
	if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_TRUE) == PS_NORMAL)
		m_type = TYPE_TRUE;
	else if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_FALSE) == PS_NORMAL)
		m_type = TYPE_FALSE;
	else if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_SUCCESS) == PS_NORMAL)
		m_type = TYPE_SUCCESS;
	else if (sp . skip_token(SP_SUGAR, TT_UNDEFINED, SG_FAILURE) == PS_NORMAL)
		m_type = TYPE_FAILURE;
	else
		m_type = TYPE_NONE;
	
	// Now try to parse an expression
	if (sp.parseexp(False, True, &m_expr) == PS_NORMAL)
		return PS_NORMAL;

	// Now if we are not of NONE type, then backup and try for just an
	// expression (TYPE_NONE).
	if (m_type != TYPE_NONE)
	{
		MCperror -> clear();
		sp = temp_sp;
	}
	
	// Parse the expression again (if not NONE, otherwise we already have
	// a badexpr error to report).
	if (m_type == TYPE_NONE ||
		sp.parseexp(False, True, &m_expr) != PS_NORMAL)
	{
		MCperror -> add(PE_ASSERT_BADEXPR, sp);
		return PS_ERROR;
	}
	
	// We must be of type none.
	m_type = TYPE_NONE;
	
	return PS_NORMAL;
}

void MCAssertCmd::exec_ctxt(MCExecContext& ctxt)
{
    
	bool t_success, t_result;
    t_success = ctxt . EvalExprAsNonStrictBool(m_expr, EE_UNDEFINED, t_result);
    
    if (!t_success)
        ctxt . IgnoreLastError();
    
    MCDebuggingExecAssert(ctxt, m_type, t_success, t_result);
}

void MCAssertCmd::compile(MCSyntaxFactoryRef ctxt)
{

}
