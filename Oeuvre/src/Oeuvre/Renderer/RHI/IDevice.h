#pragma once
#include "IBuffer.h"
#include "ITexture.h"

namespace Oeuvre
{
	using BufferHandle = std::shared_ptr<IBuffer>;
	using TextureHandle = std::shared_ptr<ITexture>;

	class IDevice
	{
	public:
		virtual BufferHandle CreateBuffer(const BufferDesc& desc) = 0;
		virtual TextureHandle CreateTexture(const TextureDesc& desc) = 0;
	};
}




