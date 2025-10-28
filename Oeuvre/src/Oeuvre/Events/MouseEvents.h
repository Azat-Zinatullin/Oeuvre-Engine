#pragma once
#include "Event.h"

namespace Oeuvre
{
	enum MouseEvents
	{
		MouseMoved,
		MouseButtonDown,
		MouseButtonUp,
		MouseScroll
	};

	class MouseMovedEvent : public Event<MouseEvents>
	{
	public:
		MouseMovedEvent() : Event<MouseEvents>(MouseEvents::MouseMoved, "MouseMoved") {}
		virtual ~MouseMovedEvent() {}
		int x = -1, y = -1;
	};

	class MouseButtonDownEvent : public Event<MouseEvents>
	{
	public:
		MouseButtonDownEvent() : Event<MouseEvents>(MouseEvents::MouseButtonDown, "MouseButtonDown") {}
		virtual ~MouseButtonDownEvent() {}
		int button = -1;
	};

	class MouseButtonUpEvent : public Event<MouseEvents>
	{
	public:
		MouseButtonUpEvent() : Event<MouseEvents>(MouseEvents::MouseButtonUp, "MouseButtonUp") {}
		virtual ~MouseButtonUpEvent() {}
		int button = -1;
	};

	class MouseScrollEvent : public Event<MouseEvents>
	{
	public:
		MouseScrollEvent() : Event<MouseEvents>(MouseEvents::MouseScroll, "MouseScroll") {}
		float xoffset = 0;
		float yoffset = 0;
	};
}