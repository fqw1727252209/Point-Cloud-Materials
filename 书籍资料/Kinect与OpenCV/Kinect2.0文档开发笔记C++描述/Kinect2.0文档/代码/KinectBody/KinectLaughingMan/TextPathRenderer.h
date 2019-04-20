// ImageRenderer类 主管图形图像渲染

#pragma once

// 文本路径渲染器用参数
struct PathTextDrawingContext{
    ID2D1RenderTarget*          pRenderTarget;
    ID2D1Geometry*              pGeometry;
    ID2D1Brush*                 pBrush;
};


// 文本路径渲染器
class PathTextRenderer : public IDWriteTextRenderer{
public:
    // 构造函数(设备无关像素大小)
    PathTextRenderer(FLOAT pixelsPerDip);
public:
    // 创建文本路径渲染器
    static void CreatePathTextRenderer(FLOAT pixelsPerDip, PathTextRenderer **textRenderer);
    // 刻画字形
    STDMETHOD(DrawGlyphRun)(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        _In_ DWRITE_GLYPH_RUN const* glyphRun,
        _In_ DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        _In_opt_ IUnknown* clientDrawingEffect
        ) override;
    // 刻画下滑线
    STDMETHOD(DrawUnderline)(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ DWRITE_UNDERLINE const* underline,
        _In_opt_ IUnknown* clientDrawingEffect
        ) override;
    // 刻画单删除线
    STDMETHOD(DrawStrikethrough)(
        _In_opt_ void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        _In_ DWRITE_STRIKETHROUGH const* strikethrough,
        _In_opt_ IUnknown* clientDrawingEffect
        ) override;
    // 刻画内联对象
    STDMETHOD(DrawInlineObject)(
        _In_opt_ void* clientDrawingContext,
        FLOAT originX,
        FLOAT originY,
        IDWriteInlineObject* inlineObject,
        BOOL isSideways,
        BOOL isRightToLeft,
        _In_opt_ IUnknown* clientDrawingEffect
        ) override;
    // 检查像素不可用
    STDMETHOD(IsPixelSnappingDisabled)(
        _In_opt_ void* clientDrawingContext,
        _Out_ BOOL* isDisabled
        ) override;
    // 获取当前转变
    STDMETHOD(GetCurrentTransform)(
        _In_opt_ void* clientDrawingContext,
        _Out_ DWRITE_MATRIX* transform
        ) override;
    // 获取设备无关像素大小
    STDMETHOD(GetPixelsPerDip)(
        _In_opt_ void* clientDrawingContext,
        _Out_ FLOAT* pixelsPerDip
        ) override;
    // QueryInterface
    STDMETHOD(QueryInterface)(
        REFIID riid,
        _Outptr_ void** object
        ) override;
    // AddRef
    STDMETHOD_(ULONG, AddRef)() override;
    // Release
    STDMETHOD_(ULONG, Release)() override;
private:
    // 设备无关像素大小
    FLOAT m_pixelsPerDip;
    // 引用计数
    UINT m_ref;
};