#include "ContentBrowserPanel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

namespace Oeuvre
{
	ContentBrowserPanel::ContentBrowserPanel()
		: m_BaseDirectory("../resources"), m_CurrentDirectory(m_BaseDirectory)
	{
		m_DirectoryIcon = ITexture::Create("../resources/textures/DirectoryIcon.png");
		m_FileIcon = ITexture::Create("../resources/textures/FileIcon.png");
	}

	void ContentBrowserPanel::OnImGuiRender(bool disableInputs)
	{
		int windowFlags = 0;

		if (disableInputs)
			windowFlags |= ImGuiWindowFlags_NoInputs;
		else
			windowFlags &= ~ImGuiWindowFlags_NoInputs;

		ImGui::Begin("Content Browser", nullptr, windowFlags);

		if (m_CurrentDirectory != std::filesystem::path(m_BaseDirectory))
		{
			if (ImGui::Button("<-"))
			{
				m_CurrentDirectory = m_CurrentDirectory.parent_path();
			}
		}

		static float padding = 16.0f;
		static float thumbnailSize = 128.0f;
		float cellSize = thumbnailSize + padding;

		float panelWidth = ImGui::GetContentRegionAvail().x;
		int columnCount = (int)(panelWidth / cellSize);
		if (columnCount < 1)
			columnCount = 1;

		ImGui::Columns(columnCount, 0, false);

		for (auto& directoryEntry : std::filesystem::directory_iterator(m_CurrentDirectory))
		{
			const auto& path = directoryEntry.path();
			std::string filenameString = path.filename().string();

			ImGui::PushID(filenameString.c_str());
			std::shared_ptr<ITexture> icon = directoryEntry.is_directory() ? m_DirectoryIcon : m_FileIcon;
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton(filenameString.c_str(), (ImTextureID)(intptr_t)((DX11Texture2D*)icon.get())->GetSRV(), {thumbnailSize, thumbnailSize});

			if (ImGui::BeginDragDropSource())
			{
				std::filesystem::path relativePath(path);
				std::wstring itemPathWStr = relativePath.c_str();
				std::string itemPathStr = { itemPathWStr.begin(), itemPathWStr.end() };
				ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", itemPathStr.c_str(), (strlen(itemPathStr.c_str()) + 1) * sizeof(char));
				ImGui::EndDragDropSource();
			}

			ImGui::PopStyleColor();
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
			{
				if (directoryEntry.is_directory())
					m_CurrentDirectory /= path.filename();

			}
			ImGui::TextWrapped(filenameString.c_str());

			ImGui::NextColumn();

			ImGui::PopID();
		}

		ImGui::Columns(1);

		ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
		ImGui::SliderFloat("Padding", &padding, 0, 32);

		// TODO: status bar
		ImGui::End();
	}
}


