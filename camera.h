#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <vector>
#include <wrl/client.h>


class Camera
{
public:
	void View()
	{

	};
private:
	DirectX::XMFLOAT3 m_Position;
	DirectX::XMFLOAT3 m_LookDirection;
	DirectX::XMFLOAT3 m_UpDirection;
	float m_Angle;

	DirectX::XMMATRIX m_View;
};