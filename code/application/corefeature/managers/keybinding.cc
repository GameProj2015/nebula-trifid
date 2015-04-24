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
		isKey = true;
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
		isKey = true;
	}

	void Keybinding::setMouseButton(Input::MouseButton::Code mouseButton)
	{
		this->mouseButton = mouseButton;
		isKey = false;
	}

	void Keybinding::setIsKey(bool isKey)
	{
		this->isKey = isKey;
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
		if(isKey)
		{
			return key;
		}
		return Input::Key::InvalidKey;
	}

	Input::MouseButton::Code Keybinding::getMouseButton()
	{
		if(!isKey)
		{
			return mouseButton;
		}
		return Input::MouseButton::InvalidMouseButton;
	}

	bool Keybinding::getIsKey()
	{
		return isKey;
	}
};