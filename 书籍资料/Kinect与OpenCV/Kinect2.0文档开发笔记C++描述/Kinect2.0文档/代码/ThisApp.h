// ThisApp类 本程序的抽象

#pragma once

class ThisApp
{
public:
	// 构造函数
	ThisApp(){};
	// 析构函数
	~ThisApp(){};
	// 初始化
	HRESULT Initialize(HINSTANCE hInstance, int nCmdShow);
	// 消息循环
	void RunMessageLoop();
private:
	// 窗口过程函数
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
private:
	// 窗口句柄
	HWND			m_hwnd = nullptr;
	// 渲染器
	ImageRenderer	m_ImagaRenderer;
};