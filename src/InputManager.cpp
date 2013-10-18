﻿/*=============================================================================
Blobby Volley 2
Copyright (C) 2006 Jonathan Sieber (jonathan_sieber@yahoo.de)
Copyright (C) 2006 Daniel Knobe (daniel-knobe@web.de)

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
=============================================================================*/

/* header include */
#include "InputManager.h"

/* includes */
#include <cassert>
#include <iostream>

#include <SDL2/SDL.h>

#include "UserConfig.h"
#include "IMGUI.h"
#include "utf8.h"

/* implementation */

InputManager* InputManager::mSingleton = 0;

const unsigned int DOUBLE_CLICK_TIME = 200;

InputManager::InputManager()
{
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	SDL_JoystickEventState(SDL_ENABLE);
	JoystickPool::getSingleton().probeJoysticks();
	assert (mSingleton == 0);
	mSingleton = this;
	mRunning = true;

	/// \todo init properly?
//	mLastInputKey.sym = SDLK_UNKNOWN;
	mLastClickTime = 0;
}

InputManager::~InputManager()
{
	JoystickPool::getSingleton().closeJoysticks();
}

InputDevice* InputManager::beginGame(PlayerSide side) const
{
	// Move Mouse to default position
//	SDL_WarpMouse(400, 300);

	std::string prefix;
	if (side == LEFT_PLAYER)
		prefix = "left_blobby_";

	if (side == RIGHT_PLAYER)
		prefix = "right_blobby_";

	UserConfig config;
	///  \todo we need only read only access here!
	config.loadFile("inputconfig.xml");
	// determine which device is to be used
	std::string device = config.getString(prefix + "device");

	// load config for mouse
	if (device == "mouse")
	{
		int jumpbutton = config.getInteger(prefix + "mouse_jumpbutton");
		return new MouseInputDevice(side, jumpbutton);
	}
	// load config for keyboard

	else if (device == "keyboard")
	{
		SDL_Keycode lkey = SDL_GetKeyFromName((config.getString(prefix + "keyboard_left")).c_str());
		SDL_Keycode rkey = SDL_GetKeyFromName((config.getString(prefix + "keyboard_right")).c_str());
		SDL_Keycode jkey = SDL_GetKeyFromName((config.getString(prefix + "keyboard_jump")).c_str());
		return new KeyboardInputDevice(lkey, rkey, jkey);
	}
	// load config for joystick
	else if (device == "joystick")
	{
		JoystickAction laction(config.getString(prefix + "joystick_left"));
		JoystickAction raction(config.getString(prefix + "joystick_right"));
		JoystickAction jaction(config.getString(prefix + "joystick_jump"));
		return new JoystickInputDevice(laction, raction,
								jaction);
	}
	else
		std::cerr << "Error: unknown input device: " << device << std::endl;

	return 0;
}

InputManager* InputManager::getSingleton()
{
	assert(mSingleton);
	return mSingleton;
}

InputManager* InputManager::createInputManager()
{
	return new InputManager();
}

void InputManager::updateInput()
{
	mUp = false;
	mDown = false;
	mLeft = false;
	mRight = false;
	mSelect = false;
	mExit = false;
	mClick = false;
	mMouseWheelUp = false;
	mMouseWheelDown = false;
	mUnclick = false;
	mLastMouseButton = -1;

	mLastActionKey = SDLK_UNKNOWN;
	mLastTextKey = 0;

	mLastJoyAction = "";
	// Init GUI Events for buffered Input
	SDL_PumpEvents();
	SDL_Event event;
	SDL_JoystickUpdate();

	// process all SDL events
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				mRunning = false;
				break;

			case SDL_KEYDOWN:
				mLastActionKey = event.key.keysym.sym;
				switch (event.key.keysym.sym)
				{
					case SDLK_UP:
						mUp = true;
						break;

					case SDLK_DOWN:
						mDown = true;
						break;

					case SDLK_LEFT:
						mLeft = true;
						break;

					case SDLK_RIGHT:
						mRight = true;
						break;

					case SDLK_RETURN:
					case SDLK_SPACE:
						mSelect = true;
						break;

					case SDLK_ESCAPE:
					//case SDLK_BACKSPACE:
						mExit = true;
						break;

					default:
						break;
				}
				break;

			case SDL_TEXTINPUT:
				mLastTextKey = event.text.text;
				break;

			case SDL_MOUSEBUTTONDOWN:
				mLastMouseButton = event.button.button;
				switch (event.button.button)
				{
					case SDL_BUTTON_LEFT:
						mClick = true;

						if(SDL_GetTicks() - mLastClickTime < DOUBLE_CLICK_TIME )
						{
							mDoubleClick = true;
						}

						mLastClickTime = SDL_GetTicks();
						break;
				}
				break;

			case SDL_MOUSEWHEEL:
				if (event.wheel.y < 0) {
					mMouseWheelDown = true;
				}
				else
				{
					mMouseWheelUp = true;
				}
				break;

			case SDL_MOUSEBUTTONUP:
				mUnclick = true;
				break;

			case SDL_JOYBUTTONDOWN:
			{
				JoystickAction joyAction(event.jbutton.which,
					JoystickAction::BUTTON, event.jbutton.button);
				mLastJoyAction = joyAction.toString();
				break;
			}
			case SDL_JOYAXISMOTION:
			{
				if (abs(event.jaxis.value) > 10000)
				{
					int axis = 0;
					if (event.jaxis.value > 0)
						axis = event.jaxis.axis + 1;

					if (event.jaxis.value < 0)
						axis = -(event.jaxis.axis + 1);

					JoystickAction joyAction(event.jaxis.which, JoystickAction::AXIS, axis);
					mLastJoyAction = joyAction.toString();
				}
				break;
			}

	/* This handles the special buttons on the GP2X, this will
	 * have to be renewed with the next GP2X release.
	/// even if we reintroduce this behaviour, we should move this code
	/// elsewhere...
	/// this is input processing not input retrieving/managing!
	#if defined(__arm__) && defined(linux)
			case SDL_JOYBUTTONDOWN:
				switch (event.jbutton.button)
				{
					case 17:
						volume -= 0.15;
					break;
					case 16:
						volume += 0.15;
					break;
				}
				SoundManager::getSingleton().setVolume(volume);
				break;
	#else
	#endif
	*/
		}
	}
}


// GUI-Methods
bool InputManager::up() const
{
	return mUp;
}

bool InputManager::down() const
{
	return mDown;
}

bool InputManager::left() const
{
	return mLeft;
}

bool InputManager::right() const
{
	return mRight;
}

bool InputManager::select() const
{
	return mSelect;
}

bool InputManager::exit() const
{
	return mExit;
}

Vector2 InputManager::position()
{
	SDL_GetMouseState(&mMouseX,&mMouseY);
	return Vector2(mMouseX,mMouseY);
}

bool InputManager::click() const
{
	return mClick;
}

bool InputManager::doubleClick() const
{
	return mDoubleClick;
}


bool InputManager::mouseWheelUp() const
{
	return mMouseWheelUp;
}

bool InputManager::mouseWheelDown() const
{
	return mMouseWheelDown;
}

bool InputManager::unclick() const
{
	return mUnclick;
}

bool InputManager::running() const
{
	return mRunning;
}

std::string InputManager::getLastTextKey()
{
	if (mLastTextKey != 0)
	{
	std::cout << "HALLO" << std::endl;
		return std::string(mLastTextKey);
	}
	else
		return "";
}

std::string InputManager::getLastActionKey()
{
	return SDL_GetKeyName(mLastActionKey);
	/// \todo this is a hack we cannot prevent until SDL 1.3 is out
	/*int i = 0;
	while (mKeyMap[i].keyname != NULL)
	{
		if (mKeyMap[i].key == mLastInputKey.sym)
		{
			return mKeyMap[i].keyname;
		}
		++i;
	}*/
	//return "";
}

std::string InputManager::getLastJoyAction() const
{
	return mLastJoyAction;
}

