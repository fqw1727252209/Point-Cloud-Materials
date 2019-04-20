#include "stdafx.h"
#include "included.h"

// IDWritePixelSnapping::GetCurrentTransform 获取的单位矩阵
const DWRITE_MATRIX identityTransform = { 1, 0, 0, 1, 0, 0 };

// <PathTextRenderer>创建文本路径渲染器
void PathTextRenderer::CreatePathTextRenderer(FLOAT pixelsPerDip, PathTextRenderer **ppTextRenderer){
    *ppTextRenderer = nullptr;
    PathTextRenderer *newRenderer = new PathTextRenderer(pixelsPerDip);
    newRenderer->AddRef();
    *ppTextRenderer = newRenderer;
    newRenderer = nullptr;
}

// <PathTextRenderer> 构造函数
PathTextRenderer::PathTextRenderer(FLOAT pixelsPerDip) :m_pixelsPerDip(pixelsPerDip), m_ref(0){
}

// 刻画字形
HRESULT PathTextRenderer::DrawGlyphRun(
    void* pClientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_MEASURING_MODE measuringMode,
    DWRITE_GLYPH_RUN const* glyphRun,
    DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
    IUnknown* clientDrawingEffect
    )
{
    HRESULT hr = S_OK;
    // 检查可用性
    if (!pClientDrawingContext) return hr;
    // 获取参数
    PathTextDrawingContext* pDC = reinterpret_cast<PathTextDrawingContext*>(pClientDrawingContext);
    // 获取当前远转换矩阵
    D2D1_MATRIX_3X2_F originalTransform;
    pDC->pRenderTarget->GetTransform(&originalTransform);
    // 计算集合体长度
    FLOAT maxLength = 0.f;
    if (SUCCEEDED(hr)){
        hr = pDC->pGeometry->ComputeLength(nullptr, &maxLength);
    }
    // 设置一个我们可以修改的局部字形
    DWRITE_GLYPH_RUN partialGlyphRun = *glyphRun;
    // 检查是从左至右还是从右至左
    BOOL leftToRight = (glyphRun->bidiLevel % 2 == 0);
    // 沿着路径设置初始长度
    FLOAT length = baselineOriginX;
    // 在当前字形集群设置第一个字形的索引位置
    UINT firstGlyphIdx = 0;
    // 主循环
    while (firstGlyphIdx < glyphRun->glyphCount){
        // 计算字形在这个集群的数量和集群的总宽度
        UINT numGlyphsInCluster = 0;
        UINT i = firstGlyphIdx;
        FLOAT clusterWidth = 0;

        while (glyphRunDescription->clusterMap[i] == glyphRunDescription->clusterMap[firstGlyphIdx] && i < glyphRun->glyphCount){
            clusterWidth += glyphRun->glyphAdvances[i];
            i++;
            numGlyphsInCluster++;
        }

        // 沿着路径计算集群的中点
        FLOAT midpoint = leftToRight ? (length + (clusterWidth / 2)) : (length - (clusterWidth / 2));

        // 在路径内渲染这个集群
        if (midpoint < maxLength && SUCCEEDED(hr)) {
            // 计算中点偏移值
            D2D1_POINT_2F offset;
            D2D1_POINT_2F tangent;
            hr = pDC->pGeometry->ComputePointAtLength(midpoint, D2D1::IdentityMatrix(), &offset, &tangent);

            D2D1_MATRIX_3X2_F rotation = D2D1::Matrix3x2F(
                tangent.x,
                tangent.y,
                -tangent.y,
                tangent.x,
                (offset.x * (1.0f - tangent.x) + offset.y * tangent.y),
                (offset.y * (1.0f - tangent.x) - offset.x * tangent.y)
                );

            // 在切线方向创建渲染矩阵
            D2D1_MATRIX_3X2_F translation = leftToRight ?
                D2D1::Matrix3x2F::Translation(-clusterWidth / 2, 0) : // LTR --> nudge it left
                D2D1::Matrix3x2F::Translation(clusterWidth / 2, 0); // RTL --> nudge it right

            // 适用转换矩阵
            pDC->pRenderTarget->SetTransform(translation * rotation * originalTransform);

            // 渲染字形
            partialGlyphRun.glyphCount = numGlyphsInCluster;
            pDC->pRenderTarget->DrawGlyphRun(
                D2D1::Point2F(offset.x, offset.y),
                &partialGlyphRun,
                pDC->pBrush
                );
        }


        // 推进到下一个
        length = leftToRight ? (length + clusterWidth) : (length - clusterWidth);
        partialGlyphRun.glyphIndices += numGlyphsInCluster;
        partialGlyphRun.glyphAdvances += numGlyphsInCluster;

        if (partialGlyphRun.glyphOffsets != nullptr)
        {
            partialGlyphRun.glyphOffsets += numGlyphsInCluster;
        }

        firstGlyphIdx += numGlyphsInCluster;
    }

    // 复原转换矩阵
    pDC->pRenderTarget->SetTransform(originalTransform);

    return S_OK;
}

// 刻画下划线
HRESULT PathTextRenderer::DrawUnderline(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    _In_ DWRITE_UNDERLINE const* underline,
    _In_opt_ IUnknown* clientDrawingEffect
    )
{
    // 不需要就懒得写
    return E_NOTIMPL;
}

// 刻画单删除线
HRESULT PathTextRenderer::DrawStrikethrough(
    _In_opt_ void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    _In_ DWRITE_STRIKETHROUGH const* strikethrough,
    _In_opt_ IUnknown* clientDrawingEffect
    )
{
    // 不需要就懒得写
    return E_NOTIMPL;
}

// 刻画内联对象
HRESULT PathTextRenderer::DrawInlineObject(
    _In_opt_ void* clientDrawingContext,
    FLOAT originX,
    FLOAT originY,
    IDWriteInlineObject* inlineObject,
    BOOL isSideways,
    BOOL isRightToLeft,
    _In_opt_ IUnknown* clientDrawingEffect
    )
{
    // 不需要就懒得写
    return E_NOTIMPL;
}

//
// IDWritePixelSnapping 方法
//
HRESULT PathTextRenderer::IsPixelSnappingDisabled(
    _In_opt_ void* clientDrawingContext,
    _Out_ BOOL* isDisabled
    )
{
    *isDisabled = FALSE;
    return S_OK;
}

HRESULT PathTextRenderer::GetCurrentTransform(
    _In_opt_ void* clientDrawingContext,
    _Out_ DWRITE_MATRIX* transform
    )
{
    *transform = identityTransform;
    return S_OK;
}

HRESULT PathTextRenderer::GetPixelsPerDip(
    _In_opt_ void* clientDrawingContext,
    _Out_ FLOAT* pixelsPerDip
    )
{
    *pixelsPerDip = m_pixelsPerDip;
    return S_OK;
}

// IUnknown 方法
HRESULT PathTextRenderer::QueryInterface(
    REFIID riid,
    _Outptr_ void** object
    )
{
    *object = nullptr;
    return E_NOTIMPL;
}

ULONG PathTextRenderer::AddRef()
{
    m_ref++;

    return m_ref;
}

ULONG PathTextRenderer::Release()
{
    m_ref--;

    if (m_ref == 0)
    {
        delete this;
        return 0;
    }

    return m_ref;
}
