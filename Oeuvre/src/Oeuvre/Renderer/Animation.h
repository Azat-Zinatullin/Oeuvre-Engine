#pragma once

#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Bone.h"
#include <functional>
#include "AnimData.h"
#include "Model.h"

namespace Oeuvre
{
	struct AssimpNodeData
	{
		glm::mat4 transformation;
		std::string name;
		int childrenCount;
		std::vector<AssimpNodeData> children;
		glm::mat4 sceneRootTransform;
	};

	class Animation
	{
	public:
		Animation() = default;

		Animation(const std::string& animationPath, Model* model)
		{
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate | aiProcess_FindInvalidData);
			assert(scene && scene->mRootNode);
			auto animation = scene->mAnimations[0];
			m_Duration = animation->mDuration;
			m_TicksPerSecond = animation->mTicksPerSecond;
			aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
			m_globalTransformation = globalTransformation;
			//globalTransformation = globalTransformation.Inverse();     ///////////////
			ReadHierarchyData(m_RootNode, scene->mRootNode);
			ReadMissingBones(animation, *model);
			m_Model = model;

			PopulateMapOfNodes(&m_RootNode);
			PopulateMapOfBones();
		}

		~Animation()
		{
		}

		void PopulateMapOfNodes(AssimpNodeData* node)
		{
			m_NodesMap[node->name] = node;
			for (auto& child : node->children)
			{
				PopulateMapOfNodes(&child);
			}
		}

		void PopulateMapOfBones()
		{
			for (auto& bone : m_Bones)
			{
				m_BonesMap[bone.GetBoneName()] = &bone;
			}
		}

		Bone* FindBoneInMap(const std::string& name)
		{
			auto iterator = m_BonesMap.find(name); 
			if (iterator != m_BonesMap.end())
				return iterator->second;
			else
				return nullptr;
		}

		Bone* FindBone(const std::string& name)
		{
			auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
				[&](const Bone& Bone)
				{
					return Bone.GetBoneName() == name;
				}
			);
			if (iter == m_Bones.end()) return nullptr;
			else return &(*iter);
		}

		AssimpNodeData* FindNode(const std::string& nodeName)
		{
			auto iterator = m_NodesMap.find(nodeName);
			if (iterator != m_NodesMap.end())
				return iterator->second;
			else
				return nullptr;
		}

		AssimpNodeData* FindNode(AssimpNodeData* parentNode, const std::string nodeName)
		{
			if (parentNode == nullptr) return nullptr;
			if (nodeName == parentNode->name) {
				return parentNode;
			}

			AssimpNodeData* result = nullptr;
			for (int i = 0; i < parentNode->childrenCount; i++)
			{
				result = FindNode(&parentNode->children[i], nodeName);
				if (result != nullptr)
					break;
			}
			return result;
		}

		inline float GetTicksPerSecond() { return m_TicksPerSecond; }
		inline float GetDuration() { return m_Duration; }
		inline const AssimpNodeData& GetRootNode() { return m_RootNode; }
		inline std::unordered_map<std::string, BoneInfo>& GetBoneIDMap()
		{
			return m_BoneInfoMap;
		}

		auto& GetBoneInfoMap() { return m_Model->GetBoneInfoMap(); }

	private:
		void ReadMissingBones(const aiAnimation* animation, Model& model)
		{
			int size = animation->mNumChannels;

			auto& boneInfoMap = model.GetBoneInfoMap();//getting m_BoneInfoMap from Model class
			int& boneCount = model.GetBoneCount(); //getting the m_BoneCounter from Model class

			//reading channels(bones engaged in an animation and their keyframes)
			for (int i = 0; i < size; i++)
			{
				auto channel = animation->mChannels[i];
				std::string boneName = channel->mNodeName.data;

				if (boneInfoMap.find(boneName) == boneInfoMap.end())
				{
					boneInfoMap[boneName].id = boneCount;
					boneCount++;
				}
				m_Bones.push_back(Bone(channel->mNodeName.data,
					boneInfoMap[channel->mNodeName.data].id, channel));
			}

			m_BoneInfoMap = boneInfoMap;
		}

		void ReadHierarchyData(AssimpNodeData& dest, const aiNode* src)
		{
			assert(src);

			dest.name = src->mName.data;
			dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
			dest.childrenCount = src->mNumChildren;
			dest.sceneRootTransform = AssimpGLMHelpers::ConvertMatrixToGLMFormat(m_globalTransformation);  /////////

			for (int i = 0; i < src->mNumChildren; i++)
			{
				AssimpNodeData newData;
				ReadHierarchyData(newData, src->mChildren[i]);
				dest.children.push_back(newData);
			}
		}

		Model* m_Model = nullptr;

		float m_Duration;
		int m_TicksPerSecond;
		std::vector<Bone> m_Bones;
		AssimpNodeData m_RootNode;
		std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;

		std::unordered_map<std::string, AssimpNodeData*> m_NodesMap;
		std::unordered_map<std::string, Bone*> m_BonesMap;

		aiMatrix4x4 m_globalTransformation;
	};

}

