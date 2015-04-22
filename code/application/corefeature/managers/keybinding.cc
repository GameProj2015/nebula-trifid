#include "stdneb.h"
#include "corefeature/managers/keybinding.h"
#include "uilayout.h"
#include "uifeatureunit.h"
#include "io/ioserver.h"
#include "coregraphics/displaydevice.h"
#include "graphicsfeatureunit.h"
#include "keybindingmanager.h"
#include "util/array.h"

namespace CoreFeature
{
	__ImplementClass(CoreFeature::Keybinding, 'KYBI', Core::RefCounted);

	Keybinding::Keybinding()
	{
	}

	Keybinding::~Keybinding()
	{
	}

	void Keybinding::setKeyName(Util::String keyName)
	{
		this->keyName = keyName;
	}

	void Keybinding::setActionName(Util::String actionName)
	{
		this->actionName = actionName;
	}

	void Keybinding::setKey(Input::Key::Code key)
	{
		this->key = key;
	}

	void Keybinding::setMouseButton(Input::MouseButton::Code mouseButton)
	{
		this->mouseButton = mouseButton;
	}

	Util::String Keybinding::getKeyName()
	{
		return keyName;
	}

	Util::String Keybinding::getActionName()
	{
		return actionName;
	}

	Input::Key::Code Keybinding::getKey()
	{
		return key;
	}

	Input::MouseButton::Code Keybinding::getMouseButton()
	{
		return mouseButton;
	}
};