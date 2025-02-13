#pragma once
#include <memory>
#include "../../Platform/Windows/WindowsWindow.h"
#include "Oeuvre/Renderer/Renderer.h"
#include "Oeuvre/Events/WindowEvents.h"
#include "Oeuvre/Events/MouseEvents.h"
#include "Oeuvre/Events/KeyEvents.h"

namespace Oeuvre 
{
	class Application
	{
	public:
		Application();
		virtual ~Application();
		virtual void Run();

		virtual Window& GetWindow() { return *m_Window; }		
	protected:
		virtual bool Init();
		virtual void RenderLoop();
		virtual void Cleanup();

		virtual void OnWindowEvent(const Event<WindowEvents>& e);
		virtual void OnMouseEvent(const Event<MouseEvents>& e);
		virtual void OnKeyEvent(const Event<KeyEvents>& e);
	protected:
		bool m_Running = true;	
		std::shared_ptr<Window> m_Window;
		std::unique_ptr<Renderer> m_Renderer;
	};

	std::unique_ptr<Application> CreateApplication();
}



