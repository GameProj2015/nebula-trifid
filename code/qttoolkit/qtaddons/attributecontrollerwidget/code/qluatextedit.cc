//------------------------------------------------------------------------------
//  qluatextedit.cc
//  (C) 2013-2015 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "qluatextedit.h"
#include "game/entity.h"

using namespace Util;

namespace QtAttributeControllerAddon
{

//------------------------------------------------------------------------------
/**
*/
QLuaTextEdit::QLuaTextEdit(QWidget *parent):
QPlainTextEdit(parent)
{
	this->setTabStopWidth(16);
	highLight = new QLuaSyntaxHighlighter(document());
}

//------------------------------------------------------------------------------
/**
*/
QLuaTextEdit::~QLuaTextEdit()
{
	delete highLight;
}
}