#pragma once
#include "Data/Defs.h"
#include "Rendering/D3DCore.h"

#include <vector>

namespace Haboob
{
  template<typename VertexT> class Mesh
  {
    public:
		Mesh() = default;
    
		void draw(ID3D11DeviceContext* context);
		void useBuffers(ID3D11DeviceContext* context, D3D_PRIMITIVE_TOPOLOGY primitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		protected:
    HRESULT buildBuffers(ID3D11Device* device, const std::vector<VertexT>& vertices, const std::vector<ULong>& indices);

    private:
    ComPtr<ID3D11Buffer> vertexBuffer;
    ComPtr<ID3D11Buffer> indexBuffer;
		UINT indexCount = 0;
  };

	template<typename VertexT>
	inline void Mesh<VertexT>::draw(ID3D11DeviceContext* context)
	{
		context->DrawIndexed(indexCount, 0, 0);
	}

	template<typename VertexT>
	inline void Mesh<VertexT>::useBuffers(ID3D11DeviceContext* context, D3D_PRIMITIVE_TOPOLOGY primitiveType)
	{
		static const UINT vertexStride = sizeof(VertexT);
		static const UINT vertexOffset = 0;

		context->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), &vertexStride, &vertexOffset);
		context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		context->IASetPrimitiveTopology(primitiveType);
	}

	template<typename VertexT>
	inline HRESULT Mesh<VertexT>::buildBuffers(ID3D11Device* device, const std::vector<VertexT>& vertices, const std::vector<ULong>& indices)
	{
		HRESULT result = S_OK;

		// Create the vertex buffer
		{
			D3D11_BUFFER_DESC vertexBufferDesc;
			D3D11_SUBRESOURCE_DATA vertexBufferSubDesc;
			ZeroMemory(&vertexBufferDesc, sizeof(D3D11_BUFFER_DESC));
			ZeroMemory(&vertexBufferSubDesc, sizeof(D3D11_SUBRESOURCE_DATA));

			vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			vertexBufferDesc.ByteWidth = vertices.size() * sizeof(VertexT);
			vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

			vertexBufferSubDesc.pSysMem = (void*)vertices.data();

			result = device->CreateBuffer(&vertexBufferDesc, &vertexBufferSubDesc, this->vertexBuffer.GetAddressOf());
		}
		Firebreak(result);

		// Create the index buffer
		{
			D3D11_BUFFER_DESC indexBufferDesc;
			D3D11_SUBRESOURCE_DATA indexBufferSubDesc;
			ZeroMemory(&indexBufferDesc, sizeof(D3D11_BUFFER_DESC));
			ZeroMemory(&indexBufferSubDesc, sizeof(D3D11_SUBRESOURCE_DATA));

			indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			indexBufferDesc.ByteWidth = indices.size() * sizeof(ULong);
			indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

			indexBufferSubDesc.pSysMem = (void*)indices.data();

			result = device->CreateBuffer(&indexBufferDesc, &indexBufferSubDesc, this->indexBuffer.GetAddressOf());
		}

		indexCount = UINT(indices.size());

    return result;
  }
}