#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <wrl/client.h>

class Mesh {
public:
	Mesh() {

	}

    uint64_t IndexCount() const
    {
        return m_Indicies.size();
    }
	
	void Create(Microsoft::WRL::ComPtr<ID3D11Device>& device) {
		CreateBuffer(device, sizeof(DirectX::XMFLOAT3) * m_Positions.size(), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, (void**) m_Positions.data(), m_PositionsBuffer);
        CreateBuffer(device, sizeof(DirectX::XMFLOAT3) * m_Normals.size(), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, (void**)m_Normals.data(), m_NormalsBuffer);
        CreateBuffer(device, sizeof(DirectX::XMFLOAT3) * m_TexCoords.size(), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, (void**)m_TexCoords.data(), m_TexCoordsBuffer);
        CreateBuffer(device, sizeof(DirectX::XMFLOAT3) * m_Indicies.size(), D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, (void**)m_Indicies.data(), m_IndiciesBuffer);
	}

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_PositionsBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_NormalsBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_TexCoordsBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_IndiciesBuffer;


protected:
	void CreateBuffer(Microsoft::WRL::ComPtr<ID3D11Device>& device, const uint64_t byteWidth, const D3D11_USAGE usage, const uint32_t blindFlags, const uint32_t cpuAccessFlags, void** pData, Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer)
	{
		D3D11_BUFFER_DESC desc = {};
		desc.ByteWidth = byteWidth;
		desc.Usage = usage;
		desc.BindFlags = blindFlags;
		desc.CPUAccessFlags = cpuAccessFlags;
		desc.MiscFlags = 0;
		desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA bufferData = {};
		bufferData.pSysMem = pData;

		device->CreateBuffer(&desc, &bufferData, buffer.ReleaseAndGetAddressOf());
	}


	std::vector<DirectX::XMFLOAT3> m_Positions;
	std::vector<DirectX::XMFLOAT3> m_Normals;
	std::vector<DirectX::XMFLOAT2> m_TexCoords;
	std::vector<uint32_t> m_Indicies;


};

class Cube : public Mesh
{
public:
    Cube() {
        m_Positions = {
           {-1.0f, -1.0f, -1.0f},
           {1.0f, -1.0f, -1.0f},
           {1.0f, 1.0f, -1.0f},
           {-1.0f, 1.0f, -1.0f},
           {-1.0f, -1.0f, 1.0f},
           {1.0f, -1.0f, 1.0f},
           {1.0f,1.0f, 1.0f},
           {-1.0f, 1.0f, 1.0f},
        };

        m_Normals = {
            { 0.57    , 0.57      , 0.57} ,
            { -0.57    , 0.57     , 0.57} ,
            { -0.57   , -0.57     , 0.57} ,
            { 0.57   , -0.57      , 0.57} ,
            { 0.57    , 0.57      , -0.57} ,
            { -0.57    , 0.57     , -0.57} ,
            { -0.57   , -0.57     , -0.57} ,
            { 0.57   , -0.57      , -0.57}
        };

        m_TexCoords = {

        };

        m_Indicies= {
            0, 1, 3,
            3, 1, 2,
            1, 5, 2,
            2, 5, 6,
            5, 4, 6,
            6, 4, 7,
            4, 0, 7,
            7, 0, 3,
            3, 2, 7,
            7, 2, 6,
            4, 5, 0,
            0, 5, 1
        };
    }
};