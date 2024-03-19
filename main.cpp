#include <cstdio>

#include <vector>
#include <fstream>
#include <chrono>

#include <DirectXMath.h>
#include <d3d11.h>
#include <wrl/client.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <stb_image.h>

#include <imgui.h>
#include <backends/imgui_impl_dx11.h>
#include <backends/imgui_impl_glfw.h>


#include "mesh.h"

//DirectX handles
HWND hWnd;
Microsoft::WRL::ComPtr<ID3D11Device> device;
Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;
D3D_FEATURE_LEVEL featurelevel;
D3D11_VIEWPORT viewport;
Microsoft::WRL::ComPtr<ID3D11RenderTargetView> backBufferView;
Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader;
Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader;
Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;
Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;
Microsoft::WRL::ComPtr<ID3D11Buffer> lightingBuffer;

#define MAX_LIGHTS 64

int width = 1600;
int height = 900;
float rotY = 0.0000f;
float dtms = 0.0f;

// Camera
DirectX::XMVECTOR m_Position = {0.0f, 0.0f, 0.0f};
DirectX::XMVECTOR m_LookDirection = {0.0f, 0.0f, 1.0f};
DirectX::XMVECTOR m_UpDirection = {0.0f, 1.0f, 0.0f};
float m_Angle = 90.0f;

// SRT Shape
DirectX::XMFLOAT3 scaling = { 1.0f, 1.0f, 1.0f };
DirectX::XMFLOAT3 rotation = { 0.0f, 0.0f, 0.0f };
DirectX::XMFLOAT3 translation = { 0.0f, 0.0f, 0.0f };

bool InitDirectX(HWND hwnd, Microsoft::WRL::ComPtr<ID3D11Device>& pDevice, Microsoft::WRL::ComPtr<ID3D11DeviceContext>& pContext, Microsoft::WRL::ComPtr<IDXGISwapChain>& pSwapChain, D3D_FEATURE_LEVEL& featureLevel);

void CreateVertexShader(const char* path, Microsoft::WRL::ComPtr<ID3D11VertexShader>& shader);
void CreatePixelShader(const char* path, Microsoft::WRL::ComPtr<ID3D11PixelShader>& shader);

void clear(float r, float g, float b, float a);

//struct Vertex {
//    float x, y, z;
//};
//
//std::vector<Vertex> geometry = {
//    {-0.5f, -0.5f, 0.0f},
//    {0.0f, 0.5f, 0.0f},
//    {0.5f, -0.5f, 0.0f},
//};

struct CBufferPerFrame {
    float totalTime;
    DirectX::XMFLOAT3 eyePos;
    DirectX::XMFLOAT4X4 worldMatrix;
    DirectX::XMFLOAT4X4 perspective;
    DirectX::XMFLOAT4 ambient;
    DirectX::XMFLOAT4 diffuse;
    DirectX::XMFLOAT4 specular; 
    DirectX::XMFLOAT4 fogColour;
    float specularPower;
    float fogStart;
    float fogRange;
    float pad0;
} cbuffer;

struct DirectionalLight {
    DirectX::XMFLOAT4 colour;
    DirectX::XMFLOAT3 direction;
    float pad_0;
};

struct PointLight {
    DirectX::XMFLOAT4 colour;
    DirectX::XMFLOAT3 position;
    float radius;
};

struct SpotLight
{
    DirectX::XMFLOAT4 colour;
    DirectX::XMFLOAT3 position;
    float radius;
    DirectX::XMFLOAT3 direction;
    float innerRadius;
    float fallofRadius;
    float Pad0, Pad1, Pad2;
};

struct CBufferLighting {
    DirectionalLight directionalLight;
    PointLight pointLight[MAX_LIGHTS];
    SpotLight spotLights[MAX_LIGHTS];
} lBuffer;

int main()
{
	printf("hello world\n");

	glfwInit();

    auto startTime = std::chrono::high_resolution_clock::now();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(width, height, "Lighting / Shadows Test", nullptr, nullptr);

	hWnd = glfwGetWin32Window(window);

    cbuffer.totalTime = 0.0f;
    cbuffer.ambient = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
    cbuffer.diffuse = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    cbuffer.specular = DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    cbuffer.specularPower = 255.0f;
    cbuffer.eyePos = DirectX::XMFLOAT3(0, 0, -10);

    lBuffer.directionalLight.direction = { -0.5, -0.5, 0.5 };
    lBuffer.directionalLight.colour = { 1.0f, 1.0f, 1.0f, 0.5f };

	//Init Application
	InitDirectX(hWnd, device, context, swapchain, featurelevel);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    //ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX11_Init(device.Get(), context.Get());
    ImGui_ImplGlfw_InitForOther(window, true);

    CreateVertexShader("vs_main.cso", vertexShader);
    CreatePixelShader("ps_main.cso", pixelShader);

    Cube cube;
    cube.Create(device);

    //D3D11_BUFFER_DESC desc = {};
    //desc.ByteWidth = sizeof(Vertex) * geometry.size();
    //desc.Usage = D3D11_USAGE_DEFAULT;
    //desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    //desc.CPUAccessFlags = 0;
    //desc.MiscFlags = 0;
    //desc.StructureByteStride = 0;
    //
    //D3D11_SUBRESOURCE_DATA bufferData = {};
    //bufferData.pSysMem = geometry.data();
    //
    //device->CreateBuffer(&desc, &bufferData, vertexBuffer.ReleaseAndGetAddressOf());
    //
    //D3D11_BUFFER_DESC index = {};
    //index.ByteWidth = sizeof(uint32_t) * indices.size();
    //index.Usage = D3D11_USAGE_DEFAULT;
    //index.BindFlags = D3D11_BIND_INDEX_BUFFER;
    //index.CPUAccessFlags = 0;
    //index.MiscFlags = 0;
    //index.StructureByteStride = 0;
    //
    //D3D11_SUBRESOURCE_DATA indexBufferData = {};
    //indexBufferData.pSysMem = indices.data();
    //
    //device->CreateBuffer(&index, &indexBufferData, indexBuffer.ReleaseAndGetAddressOf());

    XMStoreFloat4x4(&cbuffer.worldMatrix, DirectX::XMMatrixIdentity());
	
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(CBufferPerFrame);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    cbDesc.MiscFlags = 0;
    cbDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA cbData = {};
    cbData.pSysMem = &cbuffer;

    device->CreateBuffer(&cbDesc, &cbData, constantBuffer.ReleaseAndGetAddressOf());

    D3D11_BUFFER_DESC lcbDesc = {};
    lcbDesc.ByteWidth = sizeof(CBufferLighting);
    lcbDesc.Usage = D3D11_USAGE_DEFAULT;
    lcbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    lcbDesc.CPUAccessFlags = 0; //D3D11_CPU_ACCESS_WRITE;
    lcbDesc.MiscFlags = 0;
    lcbDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA lcbData = {};
    lcbData.pSysMem = &lBuffer;

    device->CreateBuffer(&lcbDesc, &lcbData, lightingBuffer.ReleaseAndGetAddressOf());

    while (!glfwWindowShouldClose(window))
	{
        dtms = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - startTime).count() / 1000000000.0f;
        printf("dtms: %f\r", dtms);
        cbuffer.totalTime += dtms;
        startTime = std::chrono::high_resolution_clock::now();

		glfwPollEvents();
        rotY += 0.1f * dtms;
        DirectX::XMStoreFloat4x4(&cbuffer.worldMatrix, 
            DirectX::XMMatrixMultiply(
            DirectX::XMMatrixScaling(scaling.x, scaling.y, scaling.z),
                DirectX::XMMatrixMultiply(
                    DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z),
                    DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z))));

        DirectX::XMStoreFloat4x4(&cbuffer.perspective,
            DirectX::XMMatrixMultiply(
                DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&cbuffer.eyePos), m_LookDirection, m_UpDirection),
                DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(m_Angle), (float)width / (float)height, 0.01f, 100000.0f)));

        clear(0, 0, 0, 0);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        bool demo = true;
        ImGui::ShowDemoWindow(&demo);

        ImGui::Begin("Constant Buffer Parameters");
        ImGui::LabelText("Application Time", "%f", cbuffer.totalTime);

        ImGui::DragFloat3("Eye Position", (float*)&cbuffer.eyePos, 0.1f);
        ImGui::InputFloat4("World Row 0", ((float*)&cbuffer.worldMatrix) + 0);
        ImGui::InputFloat4("World Row 1", ((float*)&cbuffer.worldMatrix) + 4);
        ImGui::InputFloat4("World Row 2", ((float*)&cbuffer.worldMatrix) + 8);
        ImGui::InputFloat4("World Row 3", ((float*)&cbuffer.worldMatrix) + 12);

        ImGui::ColorEdit4("Ambient", (float*)&cbuffer.ambient, ImGuiColorEditFlags_DisplayHex);
        ImGui::ColorEdit4("Diffuse", (float*)&cbuffer.diffuse, ImGuiColorEditFlags_DisplayHex);
        ImGui::ColorEdit4("Specular", (float*)&cbuffer.specular, ImGuiColorEditFlags_DisplayHex);
        ImGui::DragFloat("Specular Power", (float*)&cbuffer.specularPower);

        ImGui::End();

        ImGui::Begin("Lights");

        ImGui::BeginChild("DirectionalLights");
        ImGui::DragFloat3("Directional", (float*)&lBuffer.directionalLight.direction, 0.1f, -1.0f, 1.0f);
        ImGui::ColorEdit4("Colour", (float*)&lBuffer.directionalLight.colour, ImGuiColorEditFlags_DisplayHex);
        ImGui::EndChild();

        ImGui::End();
        
        ImGui::BeginListBox("Point Lights");
        for (int i = 0; i < MAX_LIGHTS; i++)
        {
            ImGui::PushID(i);
            ImGui::LabelText("Point Light", "[%d]", i);
            ImGui::DragFloat3("Position", (float*)&lBuffer.pointLight[i].position);
            ImGui::ColorEdit4("Colour", (float*)&lBuffer.pointLight[i].colour, ImGuiColorEditFlags_DisplayHex);
            ImGui::DragFloat("Radius", (float*)&lBuffer.pointLight[i].radius);
            ImGui::PopID();
        }
        ImGui::EndListBox();

        ImGui::BeginListBox("Spot Lights");
        for (int i = 0; i < MAX_LIGHTS; i++)
        {
            ImGui::PushID(i);
            ImGui::LabelText("Spot Light", "[%d]", i);
            ImGui::DragFloat3("Position", (float*)&lBuffer.spotLights[i].position);
            ImGui::ColorEdit4("Colour", (float*)&lBuffer.spotLights[i].colour, ImGuiColorEditFlags_DisplayHex);
            ImGui::DragFloat("Radius", (float*)&lBuffer.spotLights[i].radius);
            ImGui::DragFloat3("Direction", (float*)&lBuffer.spotLights[i].direction);
            ImGui::DragFloat("InnerRadius", (float*)&lBuffer.spotLights[i].innerRadius);
            ImGui::DragFloat("fallofRadius", (float*)&lBuffer.spotLights[i].fallofRadius);
            ImGui::PopID();
        }
        ImGui::EndListBox();

        ImGui::Begin("Shape Matrix");
        ImGui::DragFloat3("Translation", (float*)&translation, 0.1f);
        ImGui::DragFloat3("Rotation", (float*)&rotation, 0.1f);
        ImGui::DragFloat3("Scaling", (float*)&scaling, 0.1f);

        ImGui::End();

        ImGui::Begin("Fog");
        ImGui::ColorEdit4("Colour", (float*)&cbuffer.fogColour, ImGuiColorEditFlags_DisplayHex);
        ImGui::DragFloat("Start", (float*)&cbuffer.fogStart);
        ImGui::DragFloat("Range", (float*)&cbuffer.fogRange);

        ImGui::End();

        D3D11_MAPPED_SUBRESOURCE subresource = {};
        context->Map(constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subresource);
        memcpy(subresource.pData, &cbuffer, sizeof(cbuffer));
        context->Unmap(constantBuffer.Get(), 0);

        context->UpdateSubresource(lightingBuffer.Get(), 0, NULL, &lBuffer, 0, 0);

        context->VSSetShader(vertexShader.Get(), nullptr, 0);
        context->PSSetShader(pixelShader.Get(), nullptr, 0);

        context->IASetInputLayout(inputLayout.Get());
        unsigned int strides = sizeof(DirectX::XMFLOAT3);
        unsigned int offsets = 0;

        context->IASetVertexBuffers(0, 1, cube.m_PositionsBuffer.GetAddressOf(), &strides, &offsets);
        context->IASetVertexBuffers(1, 1, cube.m_NormalsBuffer.GetAddressOf(), &strides, &offsets);

        strides = sizeof(DirectX::XMFLOAT2);
        context->IASetVertexBuffers(2, 1, cube.m_TexCoordsBuffer.GetAddressOf(), &strides, &offsets);

        context->IASetIndexBuffer(cube.m_IndiciesBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

        context->VSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
        context->PSSetConstantBuffers(0, 1, constantBuffer.GetAddressOf());
        context->PSSetConstantBuffers(1, 1, lightingBuffer.GetAddressOf());

        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        //context->Draw(8, 0);
        context->DrawIndexed(cube.IndexCount(), 0, 0);

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        swapchain->Present(0, 0);
	}

	glfwTerminate();

	return 0;
}

bool InitDirectX(HWND hwnd, Microsoft::WRL::ComPtr<ID3D11Device>& pDevice, Microsoft::WRL::ComPtr<ID3D11DeviceContext>& pContext, Microsoft::WRL::ComPtr<IDXGISwapChain>& pSwapChain, D3D_FEATURE_LEVEL& featureLevel)
{
	int flags = 0;
#ifdef _DEBUG
	flags != D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };

	D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, 0, flags, featureLevels, ARRAYSIZE(featureLevels), D3D11_SDK_VERSION, pDevice.ReleaseAndGetAddressOf(), &featureLevel, pContext.ReleaseAndGetAddressOf());

    //Populate the Swap-chain desc
    DXGI_MODE_DESC bd = {  };
    {
        bd.Width = width;
        bd.Height = height;
        bd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        bd.RefreshRate.Numerator = 60;
        bd.RefreshRate.Denominator = 1;
        bd.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        bd.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    }

    DXGI_SWAP_CHAIN_DESC sd = {};
    {
        sd.BufferDesc = bd;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;
        sd.OutputWindow = hwnd;
        sd.Windowed = true;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        sd.Flags = 0;

        sd.SampleDesc.Count = 1;

        sd.SampleDesc.Quality = 0;
    }

    //Create the swapchain
    IDXGIDevice* dxgiDevice = 0;
    pDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

    IDXGIAdapter* dxgiAdapter = 0;
    dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);

    IDXGIFactory* dxgiFactory = 0;
    dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory);

    dxgiFactory->CreateSwapChain(dxgiDevice, &sd, pSwapChain.ReleaseAndGetAddressOf());

    dxgiFactory->Release();
    dxgiAdapter->Release();
    dxgiDevice->Release();

    D3D11_TEXTURE2D_DESC backBufferDesc = {};
    ID3D11Texture2D* backBuffer = {};
    pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    backBuffer->GetDesc(&backBufferDesc);

    pDevice->CreateRenderTargetView(backBuffer, nullptr, backBufferView.ReleaseAndGetAddressOf());

    backBuffer->Release();

    {
        D3D11_TEXTURE2D_DESC dsd = {};
        dsd.Width = width; //depthStencilResolution.x
        dsd.Height = height; //depthStencilResolution.y
        dsd.MipLevels = 1;
        dsd.ArraySize = 1;
        dsd.Format = DXGI_FORMAT_R32_TYPELESS;
        dsd.SampleDesc.Count = 1;
        dsd.SampleDesc.Quality = 0;
        dsd.Usage = D3D11_USAGE_DEFAULT;
        dsd.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
        dsd.CPUAccessFlags = 0;
        dsd.MiscFlags = 0;

        ID3D11Texture2D* depthStencilBuffer;
        pDevice->CreateTexture2D(&dsd, nullptr, &depthStencilBuffer);

        D3D11_DEPTH_STENCIL_VIEW_DESC dsvd = {};
        dsvd.Format = DXGI_FORMAT_D32_FLOAT;
        dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
        dsvd.Texture2D.MipSlice = 0;
        dsvd.Flags = 0;

        pDevice->CreateDepthStencilView(depthStencilBuffer, &dsvd, depthStencilView.GetAddressOf());


        D3D11_SHADER_RESOURCE_VIEW_DESC srvd = {};
        srvd.Format = DXGI_FORMAT_R32_FLOAT;
        srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;
        srvd.Texture2D.MostDetailedMip = 0;
        srvd.Texture2D.MipLevels = 1;

        Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> depthStencilTex;
        pDevice->CreateShaderResourceView(depthStencilBuffer, &srvd, depthStencilTex.ReleaseAndGetAddressOf());

        depthStencilBuffer->Release();
    }

    //Bind views to the Output Merger and set the viewport
    pContext->OMSetRenderTargets(1, backBufferView.GetAddressOf(), depthStencilView.Get());

    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = static_cast<FLOAT>(width);
    viewport.Height = static_cast<FLOAT>(height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    pContext->RSSetViewports(1, &viewport);

	return true;
}

void CreateVertexShader(const char* path, Microsoft::WRL::ComPtr<ID3D11VertexShader>& shader)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file!\n");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    device->CreateVertexShader(buffer.data(), buffer.size(), nullptr, shader.ReleaseAndGetAddressOf());

    D3D11_INPUT_ELEMENT_DESC descriptorLayout[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    device->CreateInputLayout(descriptorLayout, ARRAYSIZE(descriptorLayout), buffer.data(), buffer.size(), inputLayout.ReleaseAndGetAddressOf());
}

void CreatePixelShader(const char* path, Microsoft::WRL::ComPtr<ID3D11PixelShader>& shader)
{
    std::ifstream file(path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file!\n");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    device->CreatePixelShader(buffer.data(), buffer.size(), nullptr, shader.ReleaseAndGetAddressOf());
}


void clear(float r, float g, float b, float a)
{
    float clearColour[] = {
    0.0, 0.0, 0.0, 0.0

    };

    context->ClearRenderTargetView(backBufferView.Get(), clearColour);
    context->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0, 0x0);
}
