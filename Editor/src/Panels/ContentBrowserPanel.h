#pragma once

#include "Platform/DirectX11/DX11Texture2D.h"

#include <filesystem>

namespace Oeuvre {

	class ContentBrowserPanel
	{
	public:
		ContentBrowserPanel();

		void OnImGuiRender(bool disableInputs);
	private:
		std::filesystem::path m_BaseDirectory;
		std::filesystem::path m_CurrentDirectory;

		std::shared_ptr<ITexture> m_DirectoryIcon;
		std::shared_ptr<ITexture> m_FileIcon;
	};

}