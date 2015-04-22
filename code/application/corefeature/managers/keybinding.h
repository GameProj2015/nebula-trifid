#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreFeature::KeyBindingManager
    
    With the KeyBindingManager you will be able to make your input code look the same but still be able to change what keys and buttons that will trigger which event.
	Before creating the KeyBindingManager the BaseGameFeatureUnit singleton has to be created because this class uses the mouse and keyboard.
	Mouse and key bindings can be loaded from an xml or be added manually by your game.
	The possible keys and mouse buttons you can use is defined at the bottom of keybindmanager.cc file.

	Example of an xml file to load key bindings:
	<Keybinding Category="Combat" Name="Fire" Key="Left Mouse Button"/>
	<Keybinding Category="Movement" Name="Forward" Key="W"/>
	<Keybinding Category="Movement" Name="Back" Key="S"/>
*/
#include "core/singleton.h"
#include "io/xmlreader.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "uielement.h"
#include "uilayout.h"

namespace CoreFeature
{
	class Keybinding : public Core::RefCounted
	{
		__DeclareClass(CoreFeature::Keybinding);
	public:
		Keybinding();
		virtual ~Keybinding();
		void setKeyName(Util::String keyName);
		void setActionName(Util::String actionName);
		void setKey(Input::Key::Code key);
		void setMouseButton(Input::MouseButton::Code mouseButton);
		void setIsKey(bool isKey);
		Util::String getKeyName();
		Util::String getActionName();
		Input::Key::Code getKey();
		Input::MouseButton::Code getMouseButton();
		bool getIsKey();
	private:
		Util::String keyName;
		Util::String actionName;
		Input::Key::Code key;
		Input::MouseButton::Code mouseButton;
		bool isKey;
	};
};