#include "SceneHierarchyPanel.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <glm/gtc/type_ptr.hpp>

#include "Oeuvre/Utils/PlatformUtils.h"
#include "Oeuvre/Scripting/ScriptEngine.h"

#include <type_traits>

#include "ImGuizmo.h"
#include <glm/gtx/matrix_decompose.hpp>

#include "Oeuvre/Renderer/ResourceManager.h"

namespace Oeuvre
{
	SceneHierarchyPanel::SceneHierarchyPanel(const std::shared_ptr<Scene>& context)
	{
		SetContext(context);
	}

	void SceneHierarchyPanel::SetContext(const std::shared_ptr<Scene>& context)
	{
		m_Context = context;
		m_SelectionContext = {};
	}

	void SceneHierarchyPanel::OnImGuiRender(bool disableInputs)
	{
		int windowFlags = 0;

		if (disableInputs)
			windowFlags |= ImGuiWindowFlags_NoInputs;
		else
			windowFlags &= ~ImGuiWindowFlags_NoInputs;

		ImGui::Begin("Scene Hierarchy", nullptr, windowFlags);

		if (m_Context)
		{
			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
				m_SelectionContext = {};

			// Right-click on blank space
			if (ImGui::BeginPopupContextWindow(0, 1))
			{
				if (ImGui::MenuItem("Create Empty Entity"))
					m_Context->CreateEntity("Empty Entity");

				ImGui::EndPopup();
			}

			//for (auto entityID : m_EdContext->m_Registry.view<entt::entity>())
			//{
				//Entity entity{ entityID , m_EdContext.get() };
			Entity entity = m_Context->TryGetEntityByTag("Scene");
			DrawEntityNode(entity);
			//}
		}
		ImGui::End();

		ImGui::Begin("Properties", nullptr, windowFlags);
		if (m_SelectionContext)
		{
			DrawComponents(m_SelectionContext);
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::SetSelectedEntity(Entity entity)
	{
		m_SelectionContext = entity;
	}

	void SceneHierarchyPanel::DrawEntityNode(Entity entity)
	{
		auto& tag = entity.GetComponent<TagComponent>().Tag;
		auto& relComp = entity.GetComponent<RelationshipComponent>();

		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;

		if (relComp.First == entt::null)
		{
			flags |= ImGuiTreeNodeFlags_Leaf;
			flags |= ImGuiTreeNodeFlags_Bullet;
		}

		bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, tag.c_str());
		if (ImGui::BeginDragDropSource())
		{
			// Set the payload (e.g., an ID or data representing the node)
			uint32_t node_id = entity; // Example payload
			ImGui::SetDragDropPayload("NODE_DRAG_PAYLOAD", &node_id, sizeof(uint32_t));

			// Optional: Draw a preview of the dragged item
			ImGui::Text(tag.c_str());

			ImGui::EndDragDropSource();
		}
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("NODE_DRAG_PAYLOAD"))
			{
				// Payload accepted, retrieve data
				uint32_t dropped_node_id = *(const uint32_t*)payload->Data;
				entt::entity dropped_entity = (entt::entity)dropped_node_id;
				if (dropped_entity != entity)
				{
					Entity droppedEnt = { dropped_entity, m_Context.get() };
					droppedEnt.AddParent(entity);
				}
			}
			ImGui::EndDragDropTarget();
		}


		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (opened)
		{
			auto curr = relComp.First;

			while (curr != entt::null) {
				Entity ent = { curr, m_Context.get() };
				curr = ent.GetComponent<RelationshipComponent>().Next;
				DrawEntityNode(ent);
			}

			//ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			//bool opened = ImGui::TreeNodeEx((void*)9817239, flags, tag.c_str());
			//if (opened)
			//	ImGui::TreePop();
			ImGui::TreePop();
		}

		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (m_SelectionContext == entity)
				m_SelectionContext = {};
		}
	}

	static bool DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		bool valueChanged = false;

		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
		{
			values.x = resetValue;
			valueChanged = true;
		}		
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		if (ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.3f"))
		{
			valueChanged = true;
		}			
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
		{
			values.y = resetValue;
			valueChanged = true;
		}		
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		if (ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.3f"))
		{
			valueChanged = true;
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
		{
			values.z = resetValue;
			valueChanged = true;
		}		
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		if (ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.3f"))
		{
			valueChanged = true;
		}
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();

		return valueChanged;
	}

	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Entity entity, UIFunction uiFunction)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
		if (entity.HasComponent<T>())
		{
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
			ImGui::PopStyleVar(
			);
			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			bool removeComponent = false;
			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove component"))
					removeComponent = true;

				ImGui::EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}

			if (removeComponent)
			{
				if (!std::is_same<T, TransformComponent>::value)
					entity.RemoveComponent<T>();
			}
		}
	}

	void SceneHierarchyPanel::DrawComponents(Entity entity)
	{
		if (entity.HasComponent<TagComponent>())
		{
			auto& tag = entity.GetComponent<TagComponent>().Tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strncpy_s(buffer, sizeof(buffer), tag.c_str(), sizeof(buffer));
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
			{
				tag = std::string(buffer);
			}
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(-1);

		if (ImGui::Button("Add Component"))
			ImGui::OpenPopup("AddComponent");

		if (ImGui::BeginPopup("AddComponent"))
		{
			DisplayAddComponentEntry<CameraComponent>("Camera");
			DisplayAddComponentEntry<MeshComponent>("Mesh");
			if (m_Context->GetAllEntitiesWith<PointLightComponent>().size() < 16)
				DisplayAddComponentEntry<PointLightComponent>("PointLight");
			DisplayAddComponentEntry<ScriptComponent>("Script");

			ImGui::EndPopup();
		}

		ImGui::PopItemWidth();

		DrawComponent<TransformComponent>("Transform", entity, [this, entity](auto& component)
			{
				bool valueChanged = false;
				valueChanged |= DrawVec3Control("Translation", component.Translation);
				valueChanged |= DrawVec3Control("Rotation", component.Rotation);
				valueChanged |= DrawVec3Control("Scale", component.Scale, 1.0f);

			});

		DrawComponent<CameraComponent>("Camera", entity, [](auto& component)
			{
				ImGui::SliderFloat("Movement Speed", &component.MovementSpeed, 10.f, 100.f);
				component.m_Camera.GetMovementSpeed() = component.MovementSpeed;
				ImGui::Checkbox("Primary", &component.Primary);
			});

		DrawComponent<ScriptComponent>("Script", entity, [](auto& component)
			{
				bool scriptExists = ScriptEngine::EntityClassExists(component.ClassName);

				if (!scriptExists)
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.2f, 0.3f, 1.f));

				ImGui::InputText("Class", &component.ClassName);

				if (!scriptExists)
					ImGui::PopStyleColor();
			});

		DrawComponent<MeshComponent>("Mesh", entity, [&entity](auto& component)
			{
				if (ImGui::Button("F"))
				{
					auto filePath = FileDialogs::OpenFile("All Files\0*.*\0\0");
					component.FilePath = filePath;
				}
				ImGui::SameLine();
				ImGui::InputText("FilePath", &component.FilePath);
				if (ImGui::Button("A"))
				{
					auto filePath = FileDialogs::OpenFile("All Files\0*.*\0\0");
					component.AlbedoTexturePath = filePath;

				}
				ImGui::SameLine();
				ImGui::InputText("AlbedoPath", &component.AlbedoTexturePath);
				if (ImGui::Button("N"))
				{
					auto filePath = FileDialogs::OpenFile("All Files\0*.*\0\0");
					component.NormalTexturePath = filePath;
				}
				ImGui::SameLine();
				ImGui::InputText("NormalPath", &component.NormalTexturePath);
				ImGui::SameLine();
				ImGui::Checkbox("Use", &component.UseNormalMap);
				if (component.UseNormalMap)
				{
					ImGui::Checkbox("InvertNormalG", &component.InvertNormalG);
				}
				if (ImGui::Button("R"))
				{
					auto filePath = FileDialogs::OpenFile("All Files\0*.*\0\0");
					component.RoughnessTexturePath = filePath;
				}
				ImGui::SameLine();
				ImGui::InputText("RoughnessPath", &component.RoughnessTexturePath);
				if (ImGui::Button("M"))
				{
					auto filePath = FileDialogs::OpenFile("All Files\0*.*\0\0");
					component.MetallicTexturePath = filePath;
				}
				ImGui::SameLine();
				ImGui::InputText("MetallicPath", &component.MetallicTexturePath);
				if (ImGui::Checkbox("Combined Textures", &component.CombinedTextures))
				{
					if (component.m_Model && component.m_Model->IsLoaded())
					{
						component.m_Model->GetUseCombinedTextures() = component.CombinedTextures;
					}				
				}
				if (ImGui::Button("Load"))
				{
					ResourceManager::AddPendingModelEntity(entity.GetUUID());
				}
				ImGui::SliderFloat("Emission", &component.Emission, 0.f, 50.f);
				ImGui::ColorEdit3("Color", (float*)&component.Color);
				ImGui::SliderFloat("Roughness", &component.Roughness, 0.f, 1.f);
				ImGui::SliderFloat("Metallic", &component.Metallic, 0.f, 1.f);
			});

		DrawComponent<PointLightComponent>("PointLight", entity, [](auto& component)
			{
				ImGui::SliderFloat("Range", &component.Range, 1.f, 100.f);
				ImGui::SliderFloat("Intensity", &component.Intensity, 0.1f, 20.f);
				ImGui::ColorEdit3("Color", (float*)&component.Color);
			});

	}

	template<typename T>
	void SceneHierarchyPanel::DisplayAddComponentEntry(const std::string& entryName) {
		if (!m_SelectionContext.HasComponent<T>())
		{
			if (ImGui::MenuItem(entryName.c_str()))
			{
				m_SelectionContext.AddComponent<T>();
				ImGui::CloseCurrentPopup();
			}
		}
	}
}


