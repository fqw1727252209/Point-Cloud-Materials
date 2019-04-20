// GesturePanzerVor00类 手势PanzerVor的第一步状态

#pragma once

class GesturePanzerVor00 : public GestureStateBase{
public:
    // GesturePanzerVor00
    // 析构函数
    virtual ~GesturePanzerVor00(){};
    // 获取手势名称
    wchar_t* GetName(){ return L"GesturePanzerVor00"; };
    // 刷新 需求: 关节状态
    bool Update(GestureDataRequired* data){
        return false;
    }
};