#pragma once

#include "Event.h"

namespace Oeuvre
{
	enum class KeyEvents
	{
		KeyDown,
		KeyUp
	};

	class KeyDownEvent : public Event<KeyEvents>
	{
	public:
		KeyDownEvent() : Event<KeyEvents>(KeyEvents::KeyDown, "KeyDownEvent") {}
		virtual ~KeyDownEvent() {}
		int keycode = -1;
	};

	class KeyUpEvent : public Event<KeyEvents>
	{
	public:
		KeyUpEvent() : Event<KeyEvents>(KeyEvents::KeyUp, "KeyUpEvent") {}
		virtual ~KeyUpEvent() {}
		int keycode = -1;
	};
}