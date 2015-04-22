#include "stdneb.h"
#include "corefeature/managers/keybindingmanager.h"
#include "io/ioserver.h"
#include "input/inputserver.h"

namespace CoreFeature
{
	__ImplementClass(CoreFeature::KeyBindingManager, 'KBMG', Core::RefCounted);
	__ImplementSingleton(CoreFeature::KeyBindingManager);


	KeyBindingManager::KeyBindingManager()
	{
		__ConstructSingleton;
		setAllKeyNames();
		setAllMouseKeyNames();
		Input::InputServer* inputServer = Input::InputServer::Instance();
		keyboard = inputServer->GetDefaultKeyboard();
		mouse  = inputServer->GetDefaultMouse();
	}

	KeyBindingManager::~KeyBindingManager()
	{
		__DestructSingleton;
	}

	//------------------------------------------------------------------------------
	/**
		Adds one key binding to the list of key bindings.
	*/
	void KeyBindingManager::AddKeyBinding(const Util::String& name, const Input::Key::Code keyCode)
	{
		Ptr<CoreFeature::Keybinding> keybinding = CoreFeature::Keybinding::Create();
		keybinding->setActionName(name);
		keybinding->setKeyName(GetNameForKeyCode(keyCode));
		keybinding->setKey(keyCode);
		this->keyBindings.Append(keybinding);
	}

	//------------------------------------------------------------------------------
	/**
		Adds one mouse key binding to the list of key bindings. This will also update the mouse binding if it already exists.
	*/
	void KeyBindingManager::AddMouseBinding(const Util::String& name, const Input::MouseButton::Code mouseButton)
	{
		Ptr<CoreFeature::Keybinding> keybinding = CoreFeature::Keybinding::Create();
		keybinding->setActionName(name);
		keybinding->setKeyName(GetNameForMouseButtonCode(mouseButton));
		keybinding->setMouseButton(mouseButton);
		this->keyBindings.Append(keybinding);
	}

	//------------------------------------------------------------------------------
	/**
		Removes one key or mouse button binding to the lists of bindings.
	*/
	void KeyBindingManager::UpdateKeyBinding(const Util::String& name, const Input::Key::Code keyCode)
	{
		for(int i = 0; i < keyBindings.Size(); i++)
		{
			if(keyBindings[i]->getActionName() == name)
			{
				keyBindings[i]->setKey(keyCode);
				keyBindings[i]->setKeyName(GetNameForKeyCode(keyCode));
			}
		}
	}

	//------------------------------------------------------------------------------
	/**
		Removes one key or mouse button binding to the lists of bindings.
	*/
	void KeyBindingManager::UpdateMouseBinding(const Util::String& name, const Input::MouseButton::Code mouseButton)
	{
		for(int i = 0; i < keyBindings.Size(); i++)
		{
			if(keyBindings[i]->getActionName() == name)
			{
				keyBindings[i]->setMouseButton(mouseButton);
				keyBindings[i]->setKeyName(GetNameForMouseButtonCode(mouseButton));
			}
		}
	}

	//------------------------------------------------------------------------------
	/**
		Removes one key or mouse button binding to the lists of bindings.
	*/
	void KeyBindingManager::RemoveBinding(const Util::String& name)
	{
		for(int i = 0; i < keyBindings.Size(); i++)
		{
			if(keyBindings[i]->getActionName() == name)
			{
				keyBindings.EraseIndex(i);
				return;
			}
		}
	}

	//------------------------------------------------------------------------------
	/**
		Loads all key bindings that exists in the XML file to the list of key bindings.
	*/
	void KeyBindingManager::AddKeyBindingsFromXML(const IO::URI& path)
	{
		if (IO::IoServer::Instance()->FileExists(path))
		{
			Ptr<IO::XmlReader> reader = GetXML(path);
			if (reader->Open())
			{
				if (reader->HasNode("/Keybinding"))
				{
					reader->SetToFirstChild();
					{
						do
						{
							Util::String name = reader->GetString("Name");
							Util::String keyName = reader->GetString("Key");

							Ptr<CoreFeature::Keybinding> keybinding = CoreFeature::Keybinding::Create();
							keybinding->setKeyName(keyName);
							keybinding->setActionName(name);
							if(keyNames.Contains(keyName))
							{
								Input::Key::Code key = keyNames[keyName];
								keybinding->setKey(key);
							}
							if(mouseNames.Contains(keyName))
							{
								Input::MouseButton::Code mouseButton = mouseNames[keyName];
								keybinding->setMouseButton(mouseButton);
							}
							keyBindings.Append(keybinding);
						} while (reader->SetToNextChild());
					}
				}
				reader->Close();
			}
		}
	}

	//------------------------------------------------------------------------------
	/**
		Returns the key code the the desired key binding name. Has to be a key.
	*/
	Input::Key::Code KeyBindingManager::GetKey(Util::String name)
	{
		for(int i = 0; i < keyBindings.Size(); i++)
		{
			if(keyBindings[i]->getActionName() == name)
			{
				return keyBindings[i]->getKey();
			}
		}
		return Input::Key::Code::InvalidKey;
	}

	//------------------------------------------------------------------------------
	/**
		Returns the key code the the desired mouse binding name. Has to be a mouse button.
	*/
	Input::MouseButton::Code KeyBindingManager::GetMouseButton(Util::String name)
	{
		for(int i = 0; i < keyBindings.Size(); i++)
		{
			if(keyBindings[i]->getActionName() == name)
			{
				return keyBindings[i]->getMouseButton();
			}
		}
		return Input::MouseButton::Code::InvalidMouseButton;
	}

	//------------------------------------------------------------------------------
	/**
		Returns true if the key or mouse button with the desired name is pressed down.
	*/
	bool KeyBindingManager::IsPressed(Util::String name)
	{
		Input::Key::Code key = GetKey(name);
		if(key != Input::Key::Code::InvalidKey)
		{
			if (keyboard->KeyPressed(key))
			{
				return true;
			}
		}
		Input::MouseButton::Code mouseButton = GetMouseButton(name);
		if(mouseButton != Input::MouseButton::Code::InvalidMouseButton)
		{
			if (mouse->ButtonPressed(mouseButton))
			{
				return true;
			}
		}
		return false;
	}

	//------------------------------------------------------------------------------
	/**
		Returns true if the key or mouse button with the desired name was pressed down this frame.
	*/
	bool KeyBindingManager::IsDown(Util::String name)
	{
		Input::Key::Code key = GetKey(name);
		if(key != Input::Key::Code::InvalidKey)
		{
			if (keyboard->KeyDown(key))
			{
				return true;
			}
		}
		Input::MouseButton::Code mouseButton = GetMouseButton(name);
		if(mouseButton != Input::MouseButton::Code::InvalidMouseButton)
		{
			if (mouse->ButtonDown(mouseButton))
			{
				return true;
			}
		}
		return false;
	}

	//------------------------------------------------------------------------------
	/**
		Returns true if the key or mouse button with the desired name was released this frame.
	*/
	bool KeyBindingManager::IsUp(Util::String name)
	{
		Input::Key::Code key = GetKey(name);
		if(key != Input::Key::Code::InvalidKey)
		{
			if (keyboard->KeyUp(key))
			{
				return true;
			}
		}
		Input::MouseButton::Code mouseButton = GetMouseButton(name);
		if(mouseButton != Input::MouseButton::Code::InvalidMouseButton)
		{
			if (mouse->ButtonUp(mouseButton))
			{
				return true;
			}
		}
		return false;
	}

	//------------------------------------------------------------------------------
	/**
		Returns the entire dictionary of key bindings.
	*/
	Util::Array<Ptr<CoreFeature::Keybinding>> KeyBindingManager::GetKeyBindings()
	{
		return keyBindings;
	}

	//------------------------------------------------------------------------------
	/**
		Returns the entire dictionary of possible key binding names.
	*/
	Util::Dictionary<Util::String, Input::Key::Code> KeyBindingManager::GetKeyNames()
	{
		return keyNames;
	}

	//------------------------------------------------------------------------------
	/**
		Returns the entire dictionary of possible mouse key binding names.
	*/
	Util::Dictionary<Util::String, Input::MouseButton::Code> KeyBindingManager::GetMouseNames()
	{
		return mouseNames;
	}

	//------------------------------------------------------------------------------
	/**
		Returns the name of the key.
	*/
	Util::String KeyBindingManager::GetNameForKeyCode(Input::Key::Code key)
	{
		Util::Array<Input::Key::Code> keyCodeArray = keyNames.ValuesAsArray();
		return keyNames.KeyAtIndex(keyCodeArray.FindIndex(key));
	}

	//------------------------------------------------------------------------------
	/**
		Returns the name of the mouse button.
	*/
	Util::String KeyBindingManager::GetNameForMouseButtonCode(Input::MouseButton::Code mouseButton)
	{
		Util::Array<Input::MouseButton::Code> keyCodeArray = mouseNames.ValuesAsArray();
		return mouseNames.KeyAtIndex(keyCodeArray.FindIndex(mouseButton));
	}

	//------------------------------------------------------------------------------
	/**
		Clears the map with key bindings.
	*/
	void KeyBindingManager::ClearBindings()
	{
		keyBindings.Clear();
	}

	//------------------------------------------------------------------------------
	/**
		Returns the XML at the given place.
	*/
	Ptr<IO::XmlReader> KeyBindingManager::GetXML(const IO::URI& path)
	{
		Ptr<IO::Stream> stream = IO::IoServer::Instance()->CreateStream(path);
		stream->SetAccessMode(IO::Stream::ReadAccess);
		Ptr<IO::XmlReader> reader = IO::XmlReader::Create();
		reader->SetStream(stream);
		return reader;
	}

	//------------------------------------------------------------------------------
	/**
		Adds all the key names to the keyNames dictionary.
	*/
	void KeyBindingManager::setAllKeyNames()
	{
		keyNames.Add("Back", Input::Key::Code::Back);
		keyNames.Add("Tab", Input::Key::Code::Tab);
		keyNames.Add("Clear", Input::Key::Code::Clear);
		keyNames.Add("Return", Input::Key::Code::Return);
		keyNames.Add("Shift", Input::Key::Code::Shift);
		keyNames.Add("Control", Input::Key::Code::Control);
		keyNames.Add("Menu", Input::Key::Code::Menu);
		keyNames.Add("Pause", Input::Key::Code::Pause);
		keyNames.Add("Caps Lock", Input::Key::Code::Capital);
		keyNames.Add("Escape", Input::Key::Code::Escape);
		keyNames.Add("Convert", Input::Key::Code::Convert);
		keyNames.Add("Non Convert", Input::Key::Code::NonConvert);
		keyNames.Add("Accept", Input::Key::Code::Accept);
		keyNames.Add("Mode Change", Input::Key::Code::ModeChange);
		keyNames.Add("Space", Input::Key::Code::Space);
		keyNames.Add("Prior", Input::Key::Code::Prior);
		keyNames.Add("Next", Input::Key::Code::Next);
		keyNames.Add("End", Input::Key::Code::End);
		keyNames.Add("Home", Input::Key::Code::Home);
		keyNames.Add("Left", Input::Key::Code::Left);
		keyNames.Add("Right", Input::Key::Code::Right);
		keyNames.Add("Up", Input::Key::Code::Up);
		keyNames.Add("Down", Input::Key::Code::Down);
		keyNames.Add("Select", Input::Key::Code::Select);
		keyNames.Add("Print", Input::Key::Code::Print);
		keyNames.Add("Execute", Input::Key::Code::Execute);
		keyNames.Add("Snapshot", Input::Key::Code::Snapshot);
		keyNames.Add("Insert", Input::Key::Code::Insert);
		keyNames.Add("Delete", Input::Key::Code::Delete);
		keyNames.Add("Help", Input::Key::Code::Help);
		keyNames.Add("Left Windows", Input::Key::Code::LeftWindows);
		keyNames.Add("Right Windows", Input::Key::Code::RightWindows);
		keyNames.Add("Apps", Input::Key::Code::Apps);
		keyNames.Add("Sleep", Input::Key::Code::Sleep);
		keyNames.Add("NumPad0", Input::Key::Code::NumPad0);
		keyNames.Add("NumPad1", Input::Key::Code::NumPad1);
		keyNames.Add("NumPad2", Input::Key::Code::NumPad2);
		keyNames.Add("NumPad3", Input::Key::Code::NumPad3);
		keyNames.Add("NumPad4", Input::Key::Code::NumPad4);
		keyNames.Add("NumPad5", Input::Key::Code::NumPad5);
		keyNames.Add("NumPad6", Input::Key::Code::NumPad6);
		keyNames.Add("NumPad7", Input::Key::Code::NumPad7);
		keyNames.Add("NumPad8", Input::Key::Code::NumPad8);
		keyNames.Add("NumPad9", Input::Key::Code::NumPad9);
		keyNames.Add("Multiply", Input::Key::Code::Multiply);
		keyNames.Add("Add", Input::Key::Code::Add);
		keyNames.Add("Subtract", Input::Key::Code::Subtract);
		keyNames.Add("Separator", Input::Key::Code::Separator);
		keyNames.Add("Decimal", Input::Key::Code::Decimal);
		keyNames.Add("Divide", Input::Key::Code::Divide);
		keyNames.Add("F1", Input::Key::Code::F1);
		keyNames.Add("F2", Input::Key::Code::F2);
		keyNames.Add("F3", Input::Key::Code::F3);
		keyNames.Add("F4", Input::Key::Code::F4);
		keyNames.Add("F5", Input::Key::Code::F5);
		keyNames.Add("F6", Input::Key::Code::F6);
		keyNames.Add("F7", Input::Key::Code::F7);
		keyNames.Add("F8", Input::Key::Code::F8);
		keyNames.Add("F9", Input::Key::Code::F9);
		keyNames.Add("F10", Input::Key::Code::F10);
		keyNames.Add("F11", Input::Key::Code::F11);
		keyNames.Add("F12", Input::Key::Code::F12);
		keyNames.Add("F13", Input::Key::Code::F13);
		keyNames.Add("F14", Input::Key::Code::F14);
		keyNames.Add("F15", Input::Key::Code::F15);
		keyNames.Add("F16", Input::Key::Code::F16);
		keyNames.Add("F17", Input::Key::Code::F17);
		keyNames.Add("F18", Input::Key::Code::F18);
		keyNames.Add("F19", Input::Key::Code::F19);
		keyNames.Add("F20", Input::Key::Code::F20);
		keyNames.Add("F21", Input::Key::Code::F21);
		keyNames.Add("F22", Input::Key::Code::F22);
		keyNames.Add("F23", Input::Key::Code::F23);
		keyNames.Add("F24", Input::Key::Code::F24);
		keyNames.Add("NumLock", Input::Key::Code::NumLock);
		keyNames.Add("Scroll", Input::Key::Code::Scroll);
		keyNames.Add("Semicolon", Input::Key::Code::Semicolon);
		keyNames.Add("Slash", Input::Key::Code::Slash);
		keyNames.Add("Tilde", Input::Key::Code::Tilde);
		keyNames.Add("Left Bracket", Input::Key::Code::LeftBracket);
		keyNames.Add("Right Bracket", Input::Key::Code::RightBracket);
		keyNames.Add("BackSlash", Input::Key::Code::BackSlash);
		keyNames.Add("Quote", Input::Key::Code::Quote);
		keyNames.Add("Comma", Input::Key::Code::Comma);
		keyNames.Add("Underbar", Input::Key::Code::Underbar);
		keyNames.Add("Period", Input::Key::Code::Period);
		keyNames.Add("Equality", Input::Key::Code::Equality);
		keyNames.Add("Left Shift", Input::Key::Code::LeftShift);
		keyNames.Add("Right Shift", Input::Key::Code::RightShift);
		keyNames.Add("Left Control", Input::Key::Code::LeftControl);
		keyNames.Add("Right Control", Input::Key::Code::RightControl);
		keyNames.Add("Left Menu", Input::Key::Code::LeftMenu);
		keyNames.Add("Right Menu", Input::Key::Code::RightMenu);
		keyNames.Add("Browser Back", Input::Key::Code::BrowserBack);
		keyNames.Add("Browser Forward", Input::Key::Code::BrowserForward);
		keyNames.Add("Browser Refresh", Input::Key::Code::BrowserRefresh);
		keyNames.Add("Browser Stop", Input::Key::Code::BrowserStop);
		keyNames.Add("Browser Search", Input::Key::Code::BrowserSearch);
		keyNames.Add("Browser Favorites", Input::Key::Code::BrowserFavorites);
		keyNames.Add("Browser Home", Input::Key::Code::BrowserHome);
		keyNames.Add("Mute Volume", Input::Key::Code::VolumeMute);
		keyNames.Add("Volume Down", Input::Key::Code::VolumeDown);
		keyNames.Add("Volume Up", Input::Key::Code::VolumeUp);
		keyNames.Add("Next Track", Input::Key::Code::MediaNextTrack);
		keyNames.Add("Prev Track", Input::Key::Code::MediaPrevTrack);
		keyNames.Add("Stop", Input::Key::Code::MediaStop);
		keyNames.Add("Play/Pause", Input::Key::Code::MediaPlayPause);
		keyNames.Add("Launch Mail", Input::Key::Code::LaunchMail);
		keyNames.Add("Launch Media Select", Input::Key::Code::LaunchMediaSelect);
		keyNames.Add("Launch App1", Input::Key::Code::LaunchApp1);
		keyNames.Add("Launch App2", Input::Key::Code::LaunchApp2);
		keyNames.Add("0", Input::Key::Code::Key0);
		keyNames.Add("1", Input::Key::Code::Key1);
		keyNames.Add("2", Input::Key::Code::Key2);
		keyNames.Add("3", Input::Key::Code::Key3);
		keyNames.Add("4", Input::Key::Code::Key4);
		keyNames.Add("5", Input::Key::Code::Key5);
		keyNames.Add("6", Input::Key::Code::Key6);
		keyNames.Add("7", Input::Key::Code::Key7);
		keyNames.Add("8", Input::Key::Code::Key8);
		keyNames.Add("9", Input::Key::Code::Key9);
		keyNames.Add("A", Input::Key::Code::A);
		keyNames.Add("B", Input::Key::Code::B);
		keyNames.Add("C", Input::Key::Code::C);
		keyNames.Add("D", Input::Key::Code::D);
		keyNames.Add("E", Input::Key::Code::E);
		keyNames.Add("F", Input::Key::Code::F);
		keyNames.Add("G", Input::Key::Code::G);
		keyNames.Add("H", Input::Key::Code::H);
		keyNames.Add("I", Input::Key::Code::I);
		keyNames.Add("J", Input::Key::Code::J);
		keyNames.Add("K", Input::Key::Code::K);
		keyNames.Add("L", Input::Key::Code::L);
		keyNames.Add("M", Input::Key::Code::M);
		keyNames.Add("N", Input::Key::Code::N);
		keyNames.Add("O", Input::Key::Code::O);
		keyNames.Add("P", Input::Key::Code::P);
		keyNames.Add("Q", Input::Key::Code::Q);
		keyNames.Add("R", Input::Key::Code::R);
		keyNames.Add("S", Input::Key::Code::S);
		keyNames.Add("T", Input::Key::Code::T);
		keyNames.Add("U", Input::Key::Code::U);
		keyNames.Add("V", Input::Key::Code::V);
		keyNames.Add("W", Input::Key::Code::W);
		keyNames.Add("X", Input::Key::Code::X);
		keyNames.Add("Y", Input::Key::Code::Y);
		keyNames.Add("Z", Input::Key::Code::Z);
	}

	//------------------------------------------------------------------------------
	/**
		Adds all the mouse button names to the mouseNames dictionary.
	*/
	void KeyBindingManager::setAllMouseKeyNames()
	{
		mouseNames.Add("Left Mouse Button", Input::MouseButton::Code::LeftButton);
		mouseNames.Add("Right Mouse Button", Input::MouseButton::Code::RightButton);
		mouseNames.Add("Middle Mouse Button", Input::MouseButton::Code::MiddleButton);
	}
};