#pragma once

#include <iostream>
#include "Application.h"

#include "Oeuvre/Scripting/ScriptEngine.h"

void Test()
{
	ScriptEngine::Init();
}

extern std::unique_ptr<Application> Oeuvre::CreateApplication();

int main()
{
	std::cout << "Choose a graphics API: 1) DirectX11 2) DirectX12 3) Vulkan 4) OpenGL\n";

	unsigned chosenApi = 0;
	std::cin >> chosenApi;

	RendererAPI::API api = (RendererAPI::API)chosenApi;
	RendererAPI::SetAPI(api);

	auto app = Oeuvre::CreateApplication();
	try
	{
		app->Run();
	}
	catch (std::exception e)
	{
		std::cerr << e.what();
		return EXIT_FAILURE;
	}

	//Test();

	return EXIT_SUCCESS;
}

 
