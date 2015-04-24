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
#include "game/manager.h"
#include "input/gamepad.h"
#include "io/xmlreader.h"
#include "input/keyboard.h"
#include "input/mouse.h"
#include "corefeature/managers/keybinding.h"

namespace CoreFeature
{
	class KeyBindingManager : public Core::RefCounted
	{
		__DeclareClass(CoreFeature::KeyBindingManager);
		__DeclareSingleton(CoreFeature::KeyBindingManager);
	public:
		KeyBindingManager();
		virtual ~KeyBindingManager();
		//Load a set of key and mouse bindings from an xml file
		virtual void AddKeyBindingsFromXML(const IO::URI& path);
		//Add one single key binding
		virtual void AddKeyBinding(const Util::String& name, const Input::Key::Code keyCode);
		//Add one single mouse key binding
		virtual void AddMouseBinding(const Util::String& name, const Input::MouseButton::Code keyCode);
		//Updates one single key binding
		virtual void UpdateKeyBinding(const Util::String& name, const Input::Key::Code keyCode);
		//Updates one single mouse button binding
		virtual void UpdateMouseBinding(const Util::String& name, const Input::MouseButton::Code keyCode);
		//Removes one single key or mouse binding
		virtual void RemoveBinding(const Util::String& name);
		//Returns the key code the the desired key binding name
		virtual Input::Key::Code GetKey(Util::String name);
		//Returns the key code the the desired key binding name
		virtual Input::MouseButton::Code GetMouseButton(Util::String name);
		//Returns true if the key or mouse button with the desired name is pressed down
		virtual bool IsPressed(Util::String name);
		//Returns true if the key or mouse button with the desired name was pressed down this frame
		virtual bool IsDown(Util::String name);
		//Returns true if the key or mouse button with the desired name was released this frame
		virtual bool IsUp(Util::String name);
		//Returns all the key bindings
		virtual Util::Array<Ptr<CoreFeature::Keybinding>> GetKeyBindings();
		//Returns all the possible key binding names
		virtual Util::Dictionary<Util::String, Input::Key::Code> GetKeyNames();
		//Returns all the possible mouse key binding names
		virtual Util::Dictionary<Util::String, Input::MouseButton::Code> GetMouseNames();
		//Returns all the name of the key
		virtual Util::String GetNameForKeyCode(Input::Key::Code key);
		//Returns all the name of the mouse button
		virtual Util::String GetNameForMouseButtonCode(Input::MouseButton::Code mouseButton);
		//Deletes all key bindings
		virtual void ClearBindings();
	protected:
		Util::Array<Ptr<CoreFeature::Keybinding>> keyBindings;
		//A list of all possible key inputs
		Util::Dictionary<Util::String, Input::Key::Code> keyNames;
		//A list of all possible mouse inputs
		Util::Dictionary<Util::String, Input::MouseButton::Code> mouseNames;
		Ptr<Input::Keyboard> keyboard;
		Ptr<Input::Mouse> mouse;
	private:
		Ptr<IO::XmlReader> GetXML(const IO::URI& path);
		void setAllKeyNames();
		void setAllMouseKeyNames();
	};
};