#pragma once

#include <glm/glm.hpp>
#include <map>
#include <string>
#include <vector>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#define NOMINMAX
#include "Animation.h"
#include "Bone.h"

namespace Oeuvre
{
	class Animator
	{
	public:
		Animator(Animation* animation)
		{
			m_CurrentTime = 0.0;
			m_CurrentAnimation = animation;

			m_FinalBoneMatrices.reserve(100);

			for (int i = 0; i < 100; i++)
				m_FinalBoneMatrices.push_back(glm::mat4(1.0f));
		}

		void UpdateAnimation(float dt)
		{
			m_DeltaTime = dt;
			if (m_CurrentAnimation)
			{
				m_CurrentTime += m_CurrentAnimation->GetTicksPerSecond() * dt;
				m_CurrentTime = fmod(m_CurrentTime, m_CurrentAnimation->GetDuration());
				CalculateBoneTransform(m_CurrentAnimation, &m_CurrentAnimation->GetRootNode(), glm::mat4(1.0f), "nothingToIgnore");
			}
		}

		void ChangeAnimation(Animation* pAnimation, bool resetTime)
		{
			m_CurrentAnimation = pAnimation;
			if (resetTime)
				m_CurrentTime = 0.0f;
		}

		void CalculateBoneTransform(Animation* animation, const AssimpNodeData* node, const glm::mat4& parentTransform, const std::string& boneToIgnore)
		{
			const std::string& nodeName = node->name;

			if (nodeName == boneToIgnore)
				return;

			glm::mat4 nodeTransform = node->transformation;

			Bone* Bone = animation->FindBoneInMap(nodeName);

			if (Bone)
			{
				Bone->Update(m_CurrentTime);
				nodeTransform = Bone->GetLocalTransform();
			}

			glm::mat4 globalTransformation = parentTransform * nodeTransform;

			auto& boneInfoMap = animation->GetBoneIDMap();
			if (boneInfoMap.find(nodeName) != boneInfoMap.end())
			{
				int index = boneInfoMap[nodeName].id;
				glm::mat4 offset = boneInfoMap[nodeName].offset;
				m_FinalBoneMatrices[index] = globalTransformation * offset;
			}

			for (int i = 0; i < node->childrenCount; i++)
				CalculateBoneTransform(animation, &node->children[i], globalTransformation, boneToIgnore);
		}

		const std::vector<glm::mat4>& GetFinalBoneMatrices()
		{
			return m_FinalBoneMatrices;
		}

		void BlendTwoAnimations(Animation* pBaseAnimation, Animation* pLayeredAnimation, float blendFactor, float deltaTime)
		{
			// Speed multipliers to correctly transition from one animation to another
			float a = 1.0f;
			float b = pBaseAnimation->GetDuration() / pLayeredAnimation->GetDuration();
			const float animSpeedMultiplierUp = (1.0f - blendFactor) * a + b * blendFactor; // Lerp

			a = pLayeredAnimation->GetDuration() / pBaseAnimation->GetDuration();
			b = 1.0f;
			const float animSpeedMultiplierDown = (1.0f - blendFactor) * a + b * blendFactor; // Lerp

			// Current time of each animation, "scaled" by the above speed multiplier variables
			//currentTimeBase = 0.0f;
			m_CurrentTimeBase += pBaseAnimation->GetTicksPerSecond() * deltaTime * animSpeedMultiplierUp;
			m_CurrentTimeBase = fmod(m_CurrentTimeBase, pBaseAnimation->GetDuration());

			//currentTimeLayered = 0.0f;
			m_CurrentTimeLayered += pLayeredAnimation->GetTicksPerSecond() * deltaTime * animSpeedMultiplierDown;
			m_CurrentTimeLayered = fmod(m_CurrentTimeLayered, pLayeredAnimation->GetDuration());

			CalculateBlendedBoneTransform(pBaseAnimation, &pBaseAnimation->GetRootNode(), pLayeredAnimation, &pLayeredAnimation->GetRootNode(), m_CurrentTimeBase, m_CurrentTimeLayered, glm::mat4(1.0f), blendFactor);
		}

		glm::mat4 GetGlobalBoneTransform(Animation* animation, const std::string boneName)
		{
			glm::mat4 nodeTransform = glm::mat4(1.f);

			std::string bones[] = {
				"RootNode",
				"Armature",
				"mixamorig:Hips",
				"mixamorig:Spine",
				"mixamorig:Spine1",
				"mixamorig:Spine2",
				"mixamorig:Neck",
				"mixamorig:Head"
			};

			for (int i = 0; i < 8; i++)
			{
				Bone* Bone = animation->FindBoneInMap(bones[i]);
				if (Bone)
				{
					Bone->Update(m_CurrentTime);
					nodeTransform = nodeTransform * Bone->GetLocalTransform();

					if (boneName == Bone->GetBoneName())
						break;
				}
			}

			return nodeTransform;
		}

		void CalculateBlendedPerBoneBoneTransform(Animation* baseAnimation, Animation* layeredAnimation, const AssimpNodeData* node, const AssimpNodeData* nodeLayered, const glm::mat4& parentTransform, std::string boneName)
		{
			std::string bones[] = {
				"RootNode",
				"Armature",
				"mixamorig:Hips",
				"mixamorig:Spine",
				"mixamorig:Spine1",
				"mixamorig:Spine2",
				"mixamorig:Neck",
				"mixamorig:Head"
			};

			CalculateBoneTransform(baseAnimation, node, glm::mat4(1.f), boneName);

			const std::string& nodeLayeredName = nodeLayered->name;

			glm::mat4 nodeTransform = nodeLayered->transformation;

			Bone* Bone = layeredAnimation->FindBone(nodeLayeredName);

			if (Bone)
			{
				Bone->Update(m_CurrentTime);
				nodeTransform = Bone->GetLocalTransform();
			}

			glm::mat4 globalTransformation = parentTransform * nodeTransform;

			auto boneInfoMap = baseAnimation->GetBoneIDMap();
			if (boneInfoMap.find(nodeLayeredName) != boneInfoMap.end())
			{
				int index = boneInfoMap[nodeLayeredName].id;
				glm::mat4 offset = boneInfoMap[nodeLayeredName].offset;
				m_FinalBoneMatrices[index] = globalTransformation * offset;
			}


			for (int i = 0; i < nodeLayered->childrenCount; i++)
				CalculateBlendedPerBoneBoneTransform(baseAnimation, layeredAnimation, &node->children[i], &nodeLayered->children[i], globalTransformation, boneName);
		}

		void BlendTwoAnimationsPerBone(Animation* pBaseAnimation, Animation* pLayeredAnimation, std::string boneName, float deltaTime, float yaw, float pitch, glm::vec3 pitchAxis, float shootingTime, glm::mat4& boneTransform)
		{
			if (!pBaseAnimation || !pLayeredAnimation)
				return;

			auto minDur = glm::min(pBaseAnimation->GetDuration(), pLayeredAnimation->GetDuration());

			m_CurrentTime += pLayeredAnimation->GetTicksPerSecond() * deltaTime;
			if (shootingTime > 0.f)
				m_CurrentTime = shootingTime * pLayeredAnimation->GetTicksPerSecond();
			m_CurrentTime = fmod(m_CurrentTime, minDur);

			auto baseNode = pBaseAnimation->GetRootNode();
			auto layeredNode = pLayeredAnimation->GetRootNode();

			AssimpNodeData* node = pLayeredAnimation->FindNode(boneName);

			glm::mat4 nodeTransform = GetGlobalBoneTransform(pBaseAnimation, "mixamorig:Spine");

			if (pitch <= 0.f)
				pitch = 360.f - pitch;		

			nodeTransform *= glm::rotate(glm::mat4(1.f), glm::radians(pitch), pitchAxis);

			//boneTransform = nodeTransform;

			CalculateBoneTransform(pBaseAnimation, &baseNode, glm::mat4(1.f), boneName);
			CalculateBoneTransform(pLayeredAnimation, node, nodeTransform, "nothingToIgnore");

			boneTransform = GetGlobalBoneTransform(pBaseAnimation, "mixamorig:Neck");
		}

		// Recursive function that sets interpolated bone matrices in the 'm_FinalBoneMatrices' vector
		void CalculateBlendedBoneTransform(
			Animation* pAnimationBase, const AssimpNodeData* node,
			Animation* pAnimationLayer, const AssimpNodeData* nodeLayered,
			const float currentTimeBase, const float currentTimeLayered,
			const glm::mat4& parentTransform,
			const float blendFactor)
		{
			const std::string& nodeName = node->name;

			glm::mat4 nodeTransform = node->transformation;
			Bone* pBone = pAnimationBase->FindBoneInMap(nodeName);
			if (pBone)
			{
				pBone->Update(currentTimeBase);
				nodeTransform = pBone->GetLocalTransform();
			}

			glm::mat4 layeredNodeTransform = nodeLayered->transformation;
			pBone = pAnimationLayer->FindBoneInMap(nodeName);
			if (pBone)
			{
				pBone->Update(currentTimeLayered);
				layeredNodeTransform = pBone->GetLocalTransform();
			}

			// Blend two matrices
			const glm::quat rot0 = glm::quat_cast(nodeTransform);
			const glm::quat rot1 = glm::quat_cast(layeredNodeTransform);
			const glm::quat finalRot = glm::slerp(rot0, rot1, blendFactor);
			glm::mat4 blendedMat = glm::mat4_cast(finalRot);
			blendedMat[3] = (1.0f - blendFactor) * nodeTransform[3] + layeredNodeTransform[3] * blendFactor;

			glm::mat4 globalTransformation = parentTransform * blendedMat;

			const auto& boneInfoMap = pAnimationBase->GetBoneInfoMap();
			if (boneInfoMap.find(nodeName) != boneInfoMap.end())
			{
				const int index = boneInfoMap.at(nodeName).id;
				const glm::mat4& offset = boneInfoMap.at(nodeName).offset;

				m_FinalBoneMatrices[index] = globalTransformation * offset;
			}

			for (size_t i = 0; i < node->children.size() && i < nodeLayered->children.size(); ++i)
			{
				CalculateBlendedBoneTransform(pAnimationBase, &node->children[i], pAnimationLayer, &nodeLayered->children[i], currentTimeBase, currentTimeLayered, globalTransformation, blendFactor);
			}
		}

		float m_CurrentTime = 0.f;
		float m_DeltaTime = 0.f;

		float m_CurrentTimeBase = 0.f;
		float m_CurrentTimeLayered = 0.f;

	private:
		std::vector<glm::mat4> m_FinalBoneMatrices;
		Animation* m_CurrentAnimation;
	};
}

