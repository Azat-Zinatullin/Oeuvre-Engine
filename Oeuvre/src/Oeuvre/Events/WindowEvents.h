#pragma once

#include "Event.h"

namespace Oeuvre
{
	enum class WindowEvents
	{
		WindowResize,
		WindowClose
	};

	class WindowResizeEvent : public Event<WindowEvents>
	{
	public:
		WindowResizeEvent() : Event<WindowEvents>(WindowEvents::WindowResize, "WindowResize") {}
		virtual ~WindowResizeEvent() {}
		int width = 0, height = 0;
	};

	class WindowCloseEvent : public Event<WindowEvents>
	{
	public:
		WindowCloseEvent() : Event<WindowEvents>(WindowEvents::WindowClose, "WindowClose") {}
		virtual ~WindowCloseEvent() {}
	};
}