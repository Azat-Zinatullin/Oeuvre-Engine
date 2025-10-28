#pragma once

#include <string>
#include <memory>

namespace Oeuvre
{
	enum class TextureFormat
	{
		None = 0,
		R8G8B8A8_UNORM,
		B8G8R8A8_UNORM,
		R16G16B16A16_FLOAT,
		R32G32B32A32_FLOAT,
		R16G16_FLOAT,
		R16G16_UINT,
		R16G16_UNORM,
		R32_FLOAT,
		R32_UINT,
		R16_FLOAT,
		R16_UINT,
		R16_UNORM,
		R8_UINT,
		R8_UNORM,
		D32_FLOAT,
		D24_UNORM_S8_UINT
	};

	struct TextureDesc
	{
		uint32_t Width = 1;
		uint32_t Height = 1;
		TextureFormat Format = TextureFormat::R8G8B8A8_UNORM;
		bool GenerateMips = true;
	};

	class ITexture
	{
	public:
		virtual ~ITexture() = default;

		virtual const TextureDesc& GetDesc() const = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;
		virtual uint32_t GetRendererID() const = 0;

		virtual const std::string& GetPath() const = 0;

		virtual void SetData(void* data, uint32_t size) = 0;

		virtual void Bind(uint32_t slot) = 0;

		virtual bool IsLoaded() const = 0;

		virtual bool operator==(const ITexture& other) const = 0;
	protected:
		virtual bool Load() = 0;

	public:
		static std::shared_ptr<ITexture> Create(const TextureDesc& desc);
		static std::shared_ptr<ITexture> Create(const std::string& path);
	};
}