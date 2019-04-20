// GestureBase类 手势基类 并包含其他手势头文件

#pragma once

// 设置时间API 保证效率不用函数
#define GESTURE_GET_TIME(time) time = ::GetTickCount()


// 手势必要数据
struct GestureDataRequired{
    union{
        // 关节位置
        Joint               jt[JointType_Count];
        // 关节方向
        JointOrientation    jo[JointType_Count];
    };
    // 左手状态
    HandState   left_hand;
    // 右手状态
    HandState   right_hand;
};

// 手势类型
enum class GestureType{
    TYPE_NONE =0, // 型0
    TYPE_STATE, // 表示状态的手势
    TYPE_GROUP, // 表示状态组的手势 
};

// GestureBase类
class GestureBase{
public:
    // 虚 析构函数
    virtual ~GestureBase(){};
    // 纯虚 刷新函数
    // 输入GestureDataRequired 具体需要子类自行定义
    // 返回true表示手势触发成功false则失败
    virtual bool Update(GestureDataRequired*) = 0;
    // 获取类型
    virtual GestureType GetType() = 0;
    // 获取手势名称
    virtual wchar_t* GetName() = 0;
};


// 包含子类
#include "GestureState.h"
#include "GesturePanzerVor00.h"


#include "GestureGroup.h"