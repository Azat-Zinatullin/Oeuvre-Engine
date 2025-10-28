#include "ovpch.h"

#include "Application.h"
#include <stdexcept>


namespace Oeuvre
{
	Application::Application()
	{	
		
	}

	Application::~Application()
	{
	}

	void Application::Run()
	{
		if (!Init()) throw std::runtime_error("Failed to initialize Application!\n");
		MainLoop();
	}

	void Application::OnWindowEvent(const Event<WindowEvents>& e)
	{
	}

	void Application::OnMouseEvent(const Event<MouseEvents>& e)
	{
	}

	void Application::OnKeyEvent(const Event<KeyEvents>& e)
	{
	}

	bool Application::Init()
	{
		return true;
	}

	void Application::MainLoop()
	{
	}

	void Application::Cleanup()
	{
	}

}

