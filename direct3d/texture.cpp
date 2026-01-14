/*==============================================================================

   テクスチャ管理[texture.cpp]
                                                         Author : Natsume Shidara
                                                         Date   : 2025/06/13
--------------------------------------------------------------------------------

==============================================================================*/


#include "texture.h"
#include <string>
#include "WICTextureLoader11.h"
#include "direct3d.h"

using namespace DirectX;


static constexpr int TEXTURE_MAX = 8192; // テクスチャ管理最大数

struct Texture
{
    std::wstring filename;
    unsigned int width;
    unsigned int height;
    ID3D11Resource* pTexture = nullptr;
    ID3D11ShaderResourceView* pTextureView = nullptr;
};

static Texture g_Textures[TEXTURE_MAX]{};

static int g_SetTextureIndex = -1;


// 注意！初期化で外部から設定されるもの。Release不要。
static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

void Texture_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    for (Texture& t : g_Textures)
    {
        t.pTexture = nullptr;
    }

    g_SetTextureIndex = -1;

    g_pDevice = pDevice;
    g_pContext = pContext;
}

void Texture_Finalize(void) { Texture_AllRelease(); }
int Texture_Load(const wchar_t* pFilename)
{
    // すでに読み込んだfileは読み込まない
    for (int i = 0; i < TEXTURE_MAX; i++)
    {
        if (g_Textures[i].filename == pFilename)
        {
            return i;
        }
    }

    // 空いている管理領域を探す
    for (int i = 0; i < TEXTURE_MAX; i++)
    {

        if (g_Textures[i].pTexture)
            continue; // 使用中

        // テクスチャの読み込み
        HRESULT hr;

        hr = CreateWICTextureFromFile(g_pDevice, g_pContext, pFilename, &g_Textures[i].pTexture, &g_Textures[i].pTextureView);

        if (FAILED(hr))
        {
            std::wstring errorMsg = L"テクスチャの読み込みに失敗しました\n指定されたパス:";
            errorMsg += pFilename;
            MessageBoxW(nullptr, errorMsg.c_str(), pFilename, MB_OK | MB_ICONERROR);

            return -1;
        }

        ID3D11Texture2D* pTexture = (ID3D11Texture2D*)g_Textures[i].pTexture;
        D3D11_TEXTURE2D_DESC t2desc;
        pTexture->GetDesc(&t2desc);
        g_Textures[i].width = t2desc.Width;
        g_Textures[i].height = t2desc.Height;

        

        g_Textures[i].filename = pFilename;
        return i;
    }

    return -1;
}

void Texture_AllRelease()
{
    for (Texture& t : g_Textures)
    {
        t.filename.clear();
        SAFE_RELEASE(t.pTextureView);
        SAFE_RELEASE(t.pTexture);
    }
}

void Texture_SetTexture(int texid, int slot)
{
    if (texid < 0) return;

    //g_SetTextureIndex = texid;

    // テクスチャ設定
    g_pContext->PSSetShaderResources(slot, 1, &g_Textures[texid].pTextureView);
}

int Texture_Width(int texid)
{
    if (texid < 0)
        return 0;

    return g_Textures[texid].width;
}
int Texture_Height(int texid)
{
    if (texid < 0)
        return 0;

    return g_Textures[texid].height;
}
