#include "ovpch.h"
#include "d3dUtil.h"

#include <fstream>
#include "Oeuvre/Utils/HelperMacros.h"
#include <dxcapi.h>

#pragma comment(lib, "dxcompiler.lib")

using Microsoft::WRL::ComPtr;

ComPtr<ID3DBlob> d3dUtil::LoadBinary(const std::wstring& filename)
{
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}

ComPtr<ID3DBlob> d3dUtil::CompileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint, const std::string& target)
{
   /* UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    HRESULT hr = S_OK;

    ComPtr<ID3DBlob> byteCode = nullptr;
    ComPtr<ID3DBlob> errors;
    hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

    if (errors != nullptr)
        OutputDebugStringA((char*)errors->GetBufferPointer());

    ThrowIfFailed(hr);*/
    ComPtr<IDxcCompiler3> pCompiler;
    DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));

    ComPtr<IDxcUtils> pUtils;
    DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));

    ComPtr<IDxcCompilerArgs> pArgs;

    std::wstring entrypointW = { entrypoint.begin(), entrypoint.end() };
    std::wstring targetW = { target.begin(), target.end() };

    pUtils->BuildArguments(filename.c_str(), entrypointW.c_str(), targetW.c_str(), nullptr, 0, nullptr, 0, pArgs.GetAddressOf());

    ComPtr<IDxcIncludeHandler> pIncludeHandler;
    pUtils->CreateDefaultIncludeHandler(pIncludeHandler.GetAddressOf());

    std::ifstream fin(filename, std::ios::binary);

    std::vector<char> fileContent;

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    fileContent.resize(size);

    fin.read(fileContent.data(), size);
    fin.close();

    DxcBuffer dxcBuffer{};
    dxcBuffer.Ptr = fileContent.data();
    dxcBuffer.Size = fileContent.size();
    dxcBuffer.Encoding = 0;

    ComPtr<IDxcResult> pResult;
    pCompiler->Compile(&dxcBuffer, pArgs->GetArguments(), pArgs->GetCount(), pIncludeHandler.Get(), IID_PPV_ARGS(&pResult));
   
    HRESULT hrStatus;
    pResult->GetStatus(&hrStatus);

    ComPtr<IDxcBlob> pShaderBlob;
    if (SUCCEEDED(hrStatus)) {
        
        pResult->GetResult(&pShaderBlob);
        // Use pShaderBlob (e.g., write to file or pass to graphics API)
    }
    else {
        ComPtr<IDxcBlobEncoding> pErrors;
        pResult->GetErrorBuffer(&pErrors);
        // Handle compilation errors (e.g., print pErrors->GetStringPointer())
        std::wstring errorW = (const wchar_t*)pErrors->GetBufferPointer();
        std::string error = { errorW.begin(), errorW.end() };
        std::string fileName = { filename.begin(), filename.end() };

        OV_CORE_ERROR("Failed to compile D3D12 Shader from path \"{}\". Error string: {}", fileName, error);
    }

    ComPtr<ID3DBlob> blob = (ID3DBlob*)pShaderBlob.Get();

    return blob;
}

ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    // Create the actual default buffer resource.
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // In order to copy CPU memory data into our default buffer, we need to create
    // an intermediate upload heap.
    heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    // Describe the data we want to copy into the default buffer.
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
    // will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
    // the intermediate upload heap data will be copied to mBuffer.
    auto transition = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &transition);
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
    transition = CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    cmdList->ResourceBarrier(1, &transition);

    // Note: uploadBuffer has to be kept alive after the above function calls because
    // the command list has not been executed yet that performs the actual copy.
    // The caller can Release the uploadBuffer after it knows the copy has been executed.


    return defaultBuffer;
}
