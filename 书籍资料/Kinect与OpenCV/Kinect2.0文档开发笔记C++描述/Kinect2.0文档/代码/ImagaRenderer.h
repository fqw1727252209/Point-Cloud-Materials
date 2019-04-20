// ImageRender类 主管图形图像渲染

#pragma once

typedef std::map<std::wstring, ID2D1Bitmap*> BitmapCacheMap;

class ImageRenderer{
public:
	// 构造函数
	ImageRenderer();
	// 析构函数
	~ImageRenderer();
	// 渲染
	HRESULT OnRender();
	// 设置窗口句柄
	void SetHwnd(HWND hwnd){ m_hwnd = hwnd; }
	// 返回初始化结果
	operator HRESULT() const{ return m_hrInit; }
private:
	// 获取图片
	// bitmapName	[in] : 文件名
	// 返回: NULL表示失败 其余的则为位图的指针
	ID2D1Bitmap* GetBitmap(std::wstring& bitmapName);
	// 创建设备无关资源
	HRESULT CreateDeviceIndependentResources();
	// 创建设备有关资源
	HRESULT CreateDeviceResources();
	// 丢弃设备有关资源
	void DiscardDeviceResources();
	// 从文件读取位图
	HRESULT LoadBitmapFromFile(ID2D1RenderTarget*, IWICImagingFactory *, PCWSTR uri, UINT, UINT, ID2D1Bitmap **);
private:
	// 初始化结果
	HRESULT								m_hrInit;
	// 窗口句柄
	HWND								m_hwnd;
	// D2D 工厂
	ID2D1Factory*						m_pD2DFactory;
	// WIC 工厂
	IWICImagingFactory*					m_pWICFactory;
	// DWrite工厂
	IDWriteFactory*						m_pDWriteFactory;
	// 正文文本渲染格式
	IDWriteTextFormat*					m_pTextFormatMain;
	// D2D渲染目标
	ID2D1HwndRenderTarget*				m_pRenderTarget;
	// 图像缓存
	BitmapCacheMap						m_mapBitmapCache;
};