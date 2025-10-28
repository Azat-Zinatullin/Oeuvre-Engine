#pragma once

#include "WindowEvents.h"
#include "MouseEvents.h"
#include "KeyEvents.h"

#include <memory>

namespace Oeuvre
{
	class EventHandler
	{
	public:
		EventHandler() : WindowEventDispatcher(), MouseEventDispatcher(), KeyEventDispatcher() {}
		static EventHandler* GetInstance();

		EventDispatcher<WindowEvents> WindowEventDispatcher;
		EventDispatcher<MouseEvents> MouseEventDispatcher;
		EventDispatcher<KeyEvents> KeyEventDispatcher;
	private:
		static std::unique_ptr<EventHandler> s_Instance;
	};

#define ADD_WINDOW_EVENT_LISTENER(eventType, func, arg) EventHandler::GetInstance()->WindowEventDispatcher.AddListener(eventType, std::bind(&func, arg, std::placeholders::_1));
#define ADD_MOUSE_EVENT_LISTENER(eventType, func, arg) EventHandler::GetInstance()->MouseEventDispatcher.AddListener(eventType, std::bind(&func, arg, std::placeholders::_1));
#define ADD_KEY_EVENT_LISTENER(eventType, func, arg) EventHandler::GetInstance()->KeyEventDispatcher.AddListener(eventType, std::bind(&func, arg, std::placeholders::_1));

#define REMOVE_WINDOW_EVENT_LISTENER(eventType, index) EventHandler::GetInstance()->WindowEventDispatcher.RemoveListener(eventType, index);
#define REMOVE_MOUSE_EVENT_LISTENER(eventType, index) EventHandler::GetInstance()->MouseEventDispatcher.RemoveListener(eventType, index);
#define REMOVE_KEY_EVENT_LISTENER(eventType, index) EventHandler::GetInstance()->KeyEventDispatcher.RemoveListener(eventType, index);

#define SEND_WINDOW_EVENT(_event) EventHandler::GetInstance()->WindowEventDispatcher.SendEvent(_event);
#define SEND_MOUSE_EVENT(_event) EventHandler::GetInstance()->MouseEventDispatcher.SendEvent(_event);
#define SEND_KEY_EVENT(_event) EventHandler::GetInstance()->KeyEventDispatcher.SendEvent(_event);
}