#pragma once

#include <iostream>
#include "Application.h"

extern std::unique_ptr<Application> Oeuvre::CreateApplication();

int main()
{
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
	return EXIT_SUCCESS;
}

 
