#pragma once
#include <string>
#include <functional>
#include <map>
#include <vector>

namespace Oeuvre
{
	template <typename T>
	class Event
	{
	protected:
		T m_Type;
		std::string m_Name;
		// bool m_Handled = false;
	public:
		Event() = default;
		Event(T type, const std::string& name = "") : m_Type(type), m_Name(name) {}
		virtual ~Event() {}
		inline const T GetType() const { return m_Type; }

		template <typename EventType>
		inline EventType ToType() const
		{
			return static_cast<const EventType&>(*this);
		}

		inline const std::string& GetName() const { return m_Name; }
		//virtual bool Handled() const { return m_Handled; }

		bool Handled = false;
	};

	template <typename T>
	class EventDispatcher
	{
	private:
		using Func = std::function<void(const Event<T>&)>;
		std::map<T, std::vector<Func>> m_Listeners;
	public:
		size_t AddListener(T type, const Func& func)
		{
			m_Listeners[type].push_back(func);
			return m_Listeners[type].size() - 1;
		}

		void RemoveListener(T type, size_t index)
		{
			m_Listeners[type].erase(m_Listeners[type].begin() + index);
		}

		void SendEvent(const Event<T>& event)
		{
			if (m_Listeners.find(event.GetType()) == m_Listeners.end())
				return;

			for (auto&& listener : m_Listeners.at(event.GetType()))
			{
				//if (!event.Handled()) listener(event);
				if (!event.Handled) listener(event);
			}
		}
	};
}



