#ifndef _EX_AKBMOTION_H
#define _EX_AKBMOTION_H

#include <vector>
#define I AKB_Interface
#define TOSTRING System::Runtime::InteropServices::Marshal::PtrToStringAnsi
#define TOCHAR (char*)(void*)Marshal::StringToHGlobalAnsi


#ifdef AGMOTION_EXPORTS
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif

#ifdef __cplusplus

extern "C"
{
#endif

#ifdef STATIC_LIB
#define AKB_CALL
#define AKB_API
#else
#ifdef LINUX
#define AKB_CALL
#define AKB_API
#else	
#define AKB_CALL		__stdcall

#ifdef AKB_EXPORTS
#define AKB_API		__declspec(dllexport)
#else
#define AKB_API		__declspec(dllimport)
#endif // MCCL_EXPORTS
#endif
#endif // STATIC_LIB

//以下枚举只适用于CNC运动，其他接口输入轴号还是按照a,b,c轴对应0,1,2轴
	enum AxisRef
	{
		A = 1,
		B = 2,
		C = 4,
		D = 8,
		E = 0x10,
		F = 0x20,
		G = 0x40,
		H = 0x80,
		I = 0x100,
		J = 0x200,
		K = 0x400,
		L = 0x800
	};


	DLL_EXPORT int returninttest();


	/// <summary>
	/// 关闭已经开启的AACommServer，开启当前相对目录下的AACommserver
	/// 如果当前开启的已经是当前目录下的AACommServer，则直接返回
	/// </summary>
	/// <returns></returns>
	DLL_EXPORT int restart_AACommServer();

	/// <summary>
	/// 创建设备对象
	/// </summary>
	/// <param name="controllerType">对象类型   0:AGC301    1:AGD155    2:AGC300    3:AGM800</param>
	/// <returns>返回生成的控制器ID，可以使用该ID来使用后续接口</returns>
	DLL_EXPORT char* CreateController(int controllerType);

	
	/// <summary>
	/// 连接到Agito控制器
	/// </summary>
	/// <param name="conStr">IP地址</param>
	/// <param name="controllerId">控制器ID</param>
    /// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int Connect(char* controllerId,char* conStr);

	/// <summary>
	/// Agito控制器断连
	/// </summary>
	/// <param name="conStr">控制器ID</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int Disconnect(char* controllerId);

	/// <summary>
	/// 获取数字量输入信息
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="bit">读取位数</param>
	/// <param name="value">读取到的值，通过引用传递，返回时将包含读取到的数字量输入值</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int GetDInPort(char* controllerId, int bit, int& value);

	/// <summary>
	/// 获取数字量输出信息
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="bit">读取位数</param>
	/// <param name="value">读取到的值，通过引用传递，返回时将包含读取到的数字量输入值</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int GetDOutPort(char* controllerId, int bit, int& value);

	/// <summary>
	/// 设置数字量输出
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="bit">读取位数</param>
	/// <param name="value">需要设置的值</param>
	/// <returns></returns>
	DLL_EXPORT void SetDOutPort(char* controllerId, int bit, bool value);

	/// <summary>
	/// 获取模拟量输入信息
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="bit">读取位数</param>
	/// <param name="value">读取到的值，通过引用传递，返回时将包含读取到的数字量输入值</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int GetAInPort(char* controllerId, int bit, int& value);

	/// <summary>
	/// 获取模拟量输出信息
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="bit">读取位数</param>
	/// <param name="value">读取到的值，通过引用传递，返回时将包含读取到的数字量输入值</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int GetAOutPort(char* controllerId, int bit, int& value);

	/// <summary>
	/// 设置模拟量输出
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="bit">读取位数</param>
	/// <param name="value">需要设置的值</param>
	/// <returns></returns>
	DLL_EXPORT void SetAOutPort(char* controllerId, int bit, int value);

	/// <summary>
	/// 发送一个指令给控制器并获取应答信息,以整数类型接收
	/// </summary>
	/// <param name="rawStr">发给控制器的指令字符串</param>
	/// <param name="returnValue">控制器回复的应答信息，通过引用传递</param>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int SendCommandString(const char* controllerId, const char* rawStr, int& returnValue);

	/// <summary>
	/// 发送一个指令给控制器并获取应答信息,以字符串类型接收
	/// </summary>
	/// <param name="rawStr">发给控制器的指令字符串</param>
	/// <param name="result">控制器回复的应答信息，通过引用传递</param>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int SendCommandString_str(const char* controllerId, const char* rawStr, char* result, int resultSize);

	/// <summary>
	/// 发送多条指令给控制器并获取应答信息
	/// </summary>
	/// <param name="rawStr">发给控制器的指令字符串</param>
	/// <param name="resStr">控制器回复的应答信息，通过引用传递，多条结果通过">"分割</param>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int SendBulkCommandString(const char* controllerId,const char* rawStr, char* resStr, int resStrSize);

	/// <summary>
	///获取当前控制器的实例是否已连接通讯
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>1：已连接  0：未连接</returns>
	DLL_EXPORT int GetIsConnected(char* controllerId);

	/// <summary>
	/// 最近发生错误的错误代码
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns></returns>
	DLL_EXPORT int GetErrorCode(char* controllerId,int axis);

	/// <summary>
	/// 获取运动的期望速度
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetSpeed(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取用于所有运动的加速度
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetAccel(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取用于所有运动的减速度，也用于发送Stop指令后的减速度
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetDecel(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取当运动被软限位/硬限位停止时，用于紧急减速的减速度
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetEmrgDecel(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取用来选择用于加速或者减速到目标值的时间(jerk)
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetSmoothFact(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// vel[1]是被velfilt 过滤的速度值；vel[2]是原始的读数；
	/// vel[3]是16 个样本的平均速度
	/// 获取速度数组，包含了三种不同形式的速度值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="arrayIndex">数组的下标</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetVel(char* controllerId, int axis, int arrayIndex, int& returnValue);

	/// <summary>
	/// 获取速度误差-速度指令VelRef和实际速度Vel的差值，如果该值超过了MaxVelErr所定义的最大速度误差，电机将被禁用
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetVelErr(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取内部分析器生成的速度指令
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetVelRef(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取电机当前位置
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetPos(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取电机位置误差
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetPosErr(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取内部分析器生成的位置指令，该值被输入到位置控制环中
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetPosRef(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取定义了控制器电机允许的最大速度
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <returns>单位：user units
	/// <param name="value">需要获取的值</param>
	/// </summary></returns>
	DLL_EXPORT int GetMaxVel(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取定义了允许的最大加速度
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetMaxAcc(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 设置运动的期望速度
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar">设置的值</param>
	/// <returns></returns>
	DLL_EXPORT void SetSpeed(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 设置运动的期望加速度
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar">设置的值</param>
	/// <returns></returns>
	DLL_EXPORT void SetAccel(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 设置运动的期望减速度，也用于发送Stop指令后的减速度
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar">设置的值</param>
	/// <returns></returns>
	DLL_EXPORT void SetDecel(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 获取设置用来选择用于加速或者减速到目标值的时间(jerk)
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="Value">设置的值</param>
	/// <returns></returns>
	DLL_EXPORT void SetSmoothFact(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 获取当前回零的状态
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns>详见文档</returns>
	DLL_EXPORT int GetHomingStat(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取表示直到到达目标位置之前的运动进度的状态-如果位置误差小于InTargetTol并维持至少InTargetTime毫秒，则InTargetStat变为表示目标位置已到达的值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns>详见文档</returns>
	DLL_EXPORT int GetInTargetStat(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取用于判断目标位置到达的维持时间-如果位置误差小于InTargetTol并维持至少InTargetTime毫秒，则InTargetStat变为表示目标位置已到达的值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetInTargetTime(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取判断目标位置到达的容差-如果位置误差小于InTargetTol并维持至少InTargetTime毫秒，则InTargetStat变为表示目标位置已到达的值；单位：user units
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetInTargetTol(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取当前运动的状态
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns>详见文档</returns>
	DLL_EXPORT int GetMotionStat(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取当前处于的运动模式
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns>详见文档</returns>
	DLL_EXPORT int GetMotionMode(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取电机使能状态
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值，0 为电机未使能， 1 为电机使能</param>
	/// <returns></returns>
	DLL_EXPORT int GetMotorOnStatus(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取最近完成的运动的持续时间
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="arrayIndex">所要获取的下标</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetMotionSamples(char* controllerId, int axis, int arrayIndex, int& returnValue);


	/// <summary>
	/// 获取该值表示启用的控制回路类型
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns>详见文档</returns>
	DLL_EXPORT int GetOperationMode(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取切换速度的方向
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetSpeedChgDir(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取切换的新速度的值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetSpeedChgNew(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取启用在某个位置自动切换到使用新速度的功能
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetSpeedChgOn(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取切换速度的位置
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetSpeedChgPos(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取状态寄存器
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns>详见文档</returns>
	DLL_EXPORT int GetStatReg(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取记录了导致电机下使能的错误
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns>详见文档</returns>
	DLL_EXPORT int GetConFlt(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取记录了导致电机下使能的错误对应的具体信息
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <returns></returns>
	DLL_EXPORT int GetConFaultMsg(char* controllerId, int axis, char* returnchar);

	/// <summary>
	/// 设置用于判断目标位置到达的维持时间-如果位置误差小于InTargetTol并维持至少InTargetTime毫秒，则InTargetStat变为表示目标位置已到达的值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetInTargetTime(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 设置判断目标位置到达的容差-如果位置误差小于InTargetTol并维持至少InTargetTime毫秒，则InTargetStat变为表示目标位置已到达的值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns>单位： user units</returns>
	DLL_EXPORT void SetInTargetTol(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 设置当前运动的状态
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetMotionMode(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 设置切换速度的方向
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetSpeedChgDir(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 设置切换的新速度的值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetSpeedChgNew(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 设置启用在某个位置自动切换到使用新速度的功能
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetSpeedChgOn(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 设置切换速度的位置
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetSpeedChgPos(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 获取电机类型
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns>详见文档</returns>
	DLL_EXPORT int GetMotorType(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取电机的总电流
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetMotorCurr(char* controllerId, int axis, int& returnValue);


	/// <summary>
	/// 获取PWM模块的时钟频率
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetUserPWMDiv(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取电流闭环回路的PI控制滤波器的比例增益
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetCurrGain(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取电流闭环回路的PI控制滤波器的积分增益
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetCurrKi(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取注入信号的类型
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetInjectType(char* controllerId, int axis, int& returnValue);

	/// <summary>
	///  获取A相占全部PWM百分比的电压值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns>单位为 mA</returns>
	DLL_EXPORT int GetVa(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取B相占全部PWM百分比的电压值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetVb(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取C相占全部PWM百分比的电压值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetVc(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 电机类型
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar">电机类型详见文档 </param>
	/// <returns></returns>
	DLL_EXPORT void SetMotorType(char* controllerId, int axis, int setvar);


	/// <summary>
	/// PWM模块的时钟频率
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetUserPWMDiv(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 电流闭环回路的PI控制滤波器的比例增益
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetCurrGain(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 电流闭环回路的PI控制滤波器的积分增益
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetCurrKi(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 获取位置事件触发的间隔
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetEventGap(char* controllerId, int axis, int& returnValue);

	

	/// <summary>
	/// 获取选择的触发的事件
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetEventSelect(char* controllerId, int axis, int& returnValue);

	

	/// <summary>
	/// 获取位置事件的类型
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns>详见文档</returns>
	DLL_EXPORT int GetEventType(char* controllerId, int axis, int& returnValue);


	/// <summary>
	/// 设置位置事件触发的间隔
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetEventGap(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 设置位置事件的类型
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetEventType(char* controllerId, int axis, int setvar);


	/// <summary>
	/// 获取位置锁存的数量
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetLockCntr(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取位置锁存功能的启用状态
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetLockEn(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取锁存功能的输入信号源
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetLockSrc(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取锁存信号保存的最近一个值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetLockVal(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 设置锁存功能的输入信号源
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetLockSrc(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 设置位置锁存功能的启用状态
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetLockEn(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 获取力反馈的大小
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForce(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取力反馈值达到该阈值时切换为力控模式
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForceAInTh(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取力指令到达目标值后持续的时间
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="arrayIndex"></param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForceCmdHTime(char* controllerId, int axis, int arrayIndex, int& returnValue);

	/// <summary>
	/// 获取力指令的输入源
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForceCmdSrc(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取力指令的数值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="index"></param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForceCmdVal(char* controllerId, int axis, int index, int& returnValue);


	/// <summary>
	/// 获取力反馈的误差
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForceErr(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取力控环的前馈
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForceFFW(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取力控环的比例增益
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForceGain(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取力控环的微分增益
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForceKd(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取力控环的积分增益
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForceKi(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取位置误差达到该阈值时切换为力控模式
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForcePosErrTh(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取力指令
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForceRef(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取应用于力指令的一阶低通滤波器的频率
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForceRefFilt(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取启用滤波器的状态
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForceRelFOn(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取力控环的速度前馈
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetForceVelFFW(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取允许的最大力误差
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetMaxForceErr(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取开环模式的最大力误差
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetMaxForceErrOL(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取有定义为力反馈的模拟输入且值大于该阈值时切换为电流控制模式
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetCurrAInTh(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取电流指令到达目标值后持续的时间
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="index"></param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetCurrCmdHTime(char* controllerId, int axis, int index, int& returnValue);

	/// <summary>
	/// 获取电流指令的斜率
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="index"></param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetCurrCmdSlope(char* controllerId, int axis, int index, int& returnValue);

	/// <summary>
	/// 获取电流指令的输入源
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetCurrCmdSrc(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取电流指令的数值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="index"></param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetCurrCmdVal(char* controllerId, int axis, int index, int& returnValue);

	/// <summary>
	/// 获取当电流参考值比该阈值大或小的时候，切换为电流控制模式
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetCurrCurrTh(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取该值定义了判断阈值的方向
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetCurrCurrThDir(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取位置误差大于该阈值时切换为电流控制模式
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetCurrPosErrTh(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取该值定义了判断阈值的方向
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetPosPosFlag(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 获取当位置阈值大于该值时，切换为位置控制模式
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int GetPosPosTh(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 力反馈值达到该阈值时切换为力控模式
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetForceAInTh(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 力指令到达目标值后持续的时间
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="index"></param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetForceCmdHTime(char* controllerId, int axis, int index, int setvar);

	/// <summary>
	/// 力指令的输入源
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetForceCmdSrc(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 力指令的数值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="index"></param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetForceCmdVal(char* controllerId, int axis, int index, int setvar);


	/// <summary>
	/// 力控环的前馈
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetForceFFW(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 力控环的比例增益
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetForceGain(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 力控环的微分增益
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetForceKd(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 力控环的积分增益
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetForceKi(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 位置误差达到该阈值时切换为力控模式
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetForcePosErrTh(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 应用于力指令的一阶低通滤波器的频率
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetForceRefFilt(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 启用滤波器的状态
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetForceRelFOn(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 力控环的速度前馈
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetForceVelFFW(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 允许的最大力误差
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetMaxForceErr(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 开环模式的最大力误差
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetMaxForceErrOL(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 有定义为力反馈的模拟输入且值大于该阈值时切换为电流控制模式
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetCurrAInTh(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 电流指令到达目标值后持续的时间
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="index"></param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetCurrCmdHTime(char* controllerId, int axis, int index, int setvar);

	/// <summary>
	/// 电流指令的斜率
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="index"></param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetCurrCmdSlope(char* controllerId, int axis, int index, int setvar);

	/// <summary>
	/// 电流指令的输入源
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="setvar"></param>
	/// <returns></returns>
	DLL_EXPORT void SetCurrCmdSrc(char* controllerId, int axis, int setvar);

	/// <summary>
	/// 立即停止当前运动(急停)
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int Abort(char* controllerId, int axis);

	/// <summary>
	/// 电机打开使能
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int MotorOn(char* controllerId, int axis);

	/// <summary>
	/// 电机关闭使能
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int MotorOff(char* controllerId, int axis);

	/// <summary>
	/// 以指定的速度持续运动，除非收到停止指令停下
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="vel">运动的速度</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int Jog(char* controllerId, int axis, int vel);

	/// <summary>
	/// 绝对运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="pos">目标位置</param>
	/// <param name="sync">是否堵塞- true代表不堵塞，false代表堵塞</param>
	/// <param name="timeout">堵塞时间，默认是10s,选择非堵塞可以忽略</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int MoveAbs(char* controllerId, int axis, int pos, bool sync = true, int timeout = 10);

	/// <summary>
	/// 在当前位置与目标位置之间执行来回的重复运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="pos">目标位置</param>
	/// <param name="msDwellTime">每段运动之间的间隔时间</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int MoveAbsRepetitive(char* controllerId, int axis, int pos, int msDwelltime);

	/// <summary>
	/// 读取速度
	/// </summary>
	/// <param name="ControllerID">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value">需要获取的值</param>
	/// <returns></returns>
	DLL_EXPORT int ReadVel(char* controllerId, int axis, int& returnValue);

	/// <summary>
	/// 相对运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="pos">运动的距离</param>
	/// <param name="sync">是否堵塞- true代表不堵塞，false代表堵塞</param>
	/// <param name="timeout">堵塞时间，默认是10s,选择非堵塞可以忽略</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int MoveRel(char* controllerId, int axis, int pos, bool sync = true, int timeout = 10);

	/// <summary>
	/// 在当前位置执行一段指定距离的运动后，再回到该位置点，并重复执行
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="pos">运动的距离</param>
	/// <param name="msDwellTime">每段运动之间的间隔时间</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int MoveRelRepetitive(char* controllerId, int axis, int pos, int msDwelltime);

	/// <summary>
	/// 运动停止
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="sync">是否堵塞- true代表不堵塞，false代表堵塞</param>
	/// <param name="timeout">堵塞时间，默认是10s,选择非堵塞可以忽略</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int Stop(char* controllerId, int axis, bool sync = true, int timeout = 10);

	/// <summary>
	/// 停止重复运动模式的运动； 这是一种特殊的停止，不会在使用后立即停止，而是在当前运动结束后，在运动的起始点停止
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int StopRep(char* controllerId, int axis);

	/// <summary>
	/// 设置当前位置值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="value"></param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int SetPosition(char* controllerId, int axis, int value);


	/// <summary>
	/// 使用文件回零
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="conStr">回零文件地址</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int Home_file(char* controllerId, int axis, const char*  filename);

	/// <summary>
	/// 回零
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">轴号</param>
	/// <param name="sync">是否堵塞- true代表不堵塞，false代表堵塞</param>
	/// <param name="timeout">堵塞时间，默认是10s,选择非堵塞可以忽略</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int Homingon(char* controllerId, int Axis, bool sync = true, int timeout = 10);
	/// <summary>
	/// 获取数值，决定了脉冲被输入的脉冲和方向怎样过滤
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns></returns>
	DLL_EXPORT int GetDInFilt(char* controllerId);

	/// <summary>
	/// 获取数字输入读数的逻辑
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns></returns>
	DLL_EXPORT int GetDInLog(char* controllerId);

	/// <summary>
	/// 获取差分输入读数的逻辑
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns></returns>
	DLL_EXPORT int GetDInLogHigh(char* controllerId);

	/// <summary>
	/// 获取数字输入端口大小的数组，依次定义了输入端口的用途；高16位为轴的定义，低16位为具体用途的定义；具体用途定义参见表格5.14 DinMode
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="arrayIndex">下标</param>
	/// <returns></returns>
	DLL_EXPORT int GetDInMode(char* controllerId, int arrayIndex);

	/// <summary>
	/// 获取差分输入的十进制值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns></returns>
	DLL_EXPORT int GetDInPortHigh(char* controllerId);
	
	/// <summary>
	/// 设置脉冲被输入的脉冲和方向怎样过滤
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="value">设置的值</param>
	/// <returns></returns>
	DLL_EXPORT void SetDInFilt(char* controllerId, int value);
	
	/// <summary>
	/// 获取数字输入的逻辑
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="value">设置的值</param>
	/// <returns></returns>
	DLL_EXPORT void SetDInLog(char* controllerId, int value);
	
	/// <summary>
	/// 设置差分输入读数的逻辑
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="value">设置的值</param>
	/// <returns></returns>
	DLL_EXPORT void SetDInLogHigh(char* controllerId, int value);
	
	/// <summary>
	/// 设置数字输入端口大小的数组，依次定义了输入端口的用途；高16位为轴的定义，低16位为具体用途的定义；
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="arrayIndex">下标</param>
	/// <param name="value">设置的值</param>
	/// <returns></returns>
	DLL_EXPORT void SetDInMode(char* controllerId, int arrayIndex, int value);
	
	/// <summary>
	/// 获取数字输出的逻辑
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns></returns>
	DLL_EXPORT int GetDoutLog(char* controllerId);
	
	/// <summary>
	/// 获取数字输出端口大小的数组，依次定义了输出端口的用途；高16位为轴的定义，低16位为具体用途的定义；具体用途定义参见表格5.15 DoutMode
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="arrayIndex">下标</param>
	/// <returns></returns>
	DLL_EXPORT int GetDOutMode(char* controllerId, int arrayIndex);
	
	/// <summary>
	/// 获取选择输出引脚用于正常的输出功能或是其他的一些功能
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="arrayIndex">下标</param>
	/// <returns></returns>
	DLL_EXPORT int GetDOutSelect(char* controllerId, int arrayIndex);
	
	/// <summary>
	/// 获取模式类型，配置为0时是sink模式，配置为1时是source模式
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns></returns>
	DLL_EXPORT int GetDOutType(char* controllerId);
	
	/// <summary>
	/// 设置数字输入读数的逻辑
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="value">设置的值</param>
	/// <returns></returns>
	DLL_EXPORT void SetDoutLog(char* controllerId, int value);
	
	/// <summary>
	/// 设置数字输出端口大小的数组，依次定义了输出端口的用途；
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="arrayIndex">下标</param>
	/// <param name="value">设置的值</param>
	/// <returns></returns>
	DLL_EXPORT void SetDOutMode(char* controllerId, int arrayIndex, int value);
	
	/// <summary>
	/// 设置选择输出引脚用于正常的输出功能或是其他的一些功能
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="arrayIndex">下标</param>
	/// <param name="value">设置的值</param>
	/// <returns></returns>
	DLL_EXPORT void SetDOutSelect(char* controllerId, int arrayIndex, int value);
	
	/// <summary>
	/// 配置为0时是sink模式，配置为1时是source模式
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="value">设置的值</param>
	/// <returns></returns>
	DLL_EXPORT void SetDOutType(char* controllerId, int value);
	
	/// <summary>
	/// 用于清除特定的输出
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="index">下标</param>
	/// <returns></returns>
	DLL_EXPORT int DOutClearBit(char* controllerId, int index);
	
	/// <summary>
	/// 用于设置特定的输出
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="index">下标</param>
	/// <returns></returns>
	DLL_EXPORT 	int DOutSetBit(char* controllerId, int index);
	
	/// <summary>
	/// 用于切换特定的输出
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="index">下标</param>
	/// <returns></returns>
	DLL_EXPORT 	int DOutToggleBit(char* controllerId, int index);
	
	
	/// <summary>
	///  IO阻塞（判断输入信号）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="bit">判断对应的bit位，从0开始</param>
	/// <param name="value">IO对应值只有0或者1</param>
	/// <param name="Sync">是否异步，true为异步，false为同步</param>
	/// <param name="TimeOut">同步超时，异步忽略</param>
	/// <returns></returns>
	DLL_EXPORT int WriteJudgeInport(char* controllerId, int bit, int value, bool Sync = true, int TimeOut = 10);
	
	/// <summary>
	///  IO阻塞（判断输出信号）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="bit">判断对应的bit位，从0开始</param>
	/// <param name="value">IO对应值只有0或者1</param>
	/// <param name="ASync">是否异步，true为异步，false为同步</param>
	/// <param name="TimeOut">同步超时，异步忽略</param>
	/// <returns></returns>
	DLL_EXPORT 	int WriteJudgeOutport(char* controllerId, int bit, int value, bool Sync = true, int TimeOut = 10);
	
	/// <summary>
	/// 开始执行运动缓冲区中的指令
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT 	int Begin(char* controllerId);
	
	/// <summary>
	/// 清除所有运动缓冲区中的指令
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT 	int ClearBuffer(char* controllerId);
	
	/// <summary>
	/// 添加一段延时到运动缓冲区中
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="ms"></param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT 	int Delay(char* controllerId, int ms);
	
	/// <summary>
	/// 暂停运动缓冲区中指令的执行
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT 	int Pause(char* controllerId);
	
	/// <summary>
	/// 恢复运动缓冲区中指令的执行
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT 	int Resume(char* controllerId);
	
	/// <summary>
	/// 停止执行运动缓冲区中的指令
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int GroupStop(char* controllerId);
	
	/// <summary>
	/// 将设置第一段运动的起始位置的指令添加到运动缓冲区中，如果使用它的话要保证这必须是队列中的第一条运动指令
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="groupAxisMask">轴组中的所有轴</param>
	/// <param name="xPosCurrent">X 轴的当前位置</param>
	/// <param name="yPosCurrent">Y 轴的当前位置</param>
	/// <param name="zPosCurrentint">Z 轴的当前位置</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	//DLL_EXPORT int SetStartPositionsGroup(char* controllerId, AxisRef groupAxisMask, int xPosCurrent, int yPosCurrent, int zPosCurrentint);
	
	/// <summary>
	/// 将设置第一段运动的起始位置的指令添加到运动缓冲区中，如果使用它的话要保证这必须是队列中的第一条运动指令
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="xPosCurrent">X 轴的当前位置</param>
	/// <param name="yPosCurrent">Y 轴的当前位置</param>
	/// <param name="zPosCurrentint"Z 轴的当前位置</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int SetStartPositions(char* controllerId, int xPosCurrent, int yPosCurrent, int zPosCurrentint);
	
	/// <summary>
	/// 将设置后续运动参数的指令添加到运动缓冲区中
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="percentage">速度百分比</param>
	/// <param name="accel">加速度</param>
	/// <param name="decel">减速度</param>
	/// <param name="sfactor">平滑因子</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT int SetMotionProfile(char* controllerId, int percentage, int accel, int decel, int sfactor);
	
	/// <summary>
	/// 将设置成员轴当前位置为新的值的指令添加到运动缓冲区中，在此之前运动的末速度应该为0
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="aPos">设置 A 轴的位置，</param>
	/// <param name="bPos">设置 B 轴的位置</param>
	/// <param name="cPos">设置 C 轴的位置</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT 	int SetCurrPositions(char* controllerId, int aPos, int bPos, int cPos);
	
	/// <summary>
	/// 将设置后续运动的自动角参数的指令添加到运动缓冲区中
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="cornerType">1 为圆弧转角， 2 为持续加速</param>
	/// <param name="radiusMethod">如果 cornerType 是 1， 0 用于指定半径， 1 用于指定最大误差</param>
	/// <param name="radiusOrError">如果 cornerType 是 2，置为 0，否则，如果radiusMethod 为 0，在这里设置半径，如果 radiusMethod 为1，在这里设置最大误差</param>
	/// <param name="axisAccel">如果 cornerType 是 1，置为 0，否则，在这里设置正峰值加速度</param>
	/// <param name="minAngle">小于这个值的两条线将不会生成角，只能设置 0-180度</param>
	/// <param name="accelLimitType">设置 0 为避免转角速度降低，设置 1 为根据每个轴的加速度极限来限制转角的速度</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	DLL_EXPORT 	int SetCornerParams(char* controllerId, int cornerType, int radiusMethod, int radiusOrError, int axisAccel, int minAngle, int accelLimitType);
	
	/// <summary>
	/// 将在两个直线插补运动中插入一个自动转角占位的指令添加到运动缓冲区中，转角将根据"SetCornerParams()"的定义进行实时计算
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axisRef">axisRef -对于两个参与轴的轴引用，使用按位功能“ |” 来添加</param>
	/// <returns>1：执行成功 0：执行失败</returns>
	//DLL_EXPORT int AutoCorner(char* controllerId, AxisRef axisRef);

	/// <summary>
	/// 单次触发PEG事件
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">运动轴</param>
	/// <param name="eventBegPos">事件触发的起始绝对位置</param>
	/// <param name="eventSelect">事件选择</param>
	/// <param name="eventPulseRes">事件脉冲分辨率    0-低(微秒)   1-高(纳秒)   </param>
	/// <param name="eventPulseWid">事件脉冲宽度</param>
	/// <returns></returns>
	DLL_EXPORT void SetSingleEventPEG(char* controllerId, int axis, int eventBegPos, int eventSelect, int eventPulseRes , int  eventPulseWid);

	/// <summary>
	/// 固定间隔PEG事件
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">运动轴</param>
	/// <param name="eventBegPos">事件触发的起始绝对位置</param>
	/// <param name="eventGap">事件间隔距离</param>
	/// <param name="eventEndPos">结束位置</param>
	/// <param name="eventSelect">事件选择</param>
	/// <param name="eventPulseRes">事件脉冲分辨率    0-低(微秒)   1-高(纳秒)   </param>
	/// <param name="eventPulseWid">事件脉冲宽度</param>
	/// <returns></returns>
	DLL_EXPORT void SetEventFixedGapPEG(char* controllerId, int axis, int eventBegPos, int eventGap, int eventEndPos, int eventSelect, int eventPulseRes, int  eventPulseWid);

	/// <summary>
	/// 使能事件触发
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">运动轴</param>
	/// <returns></returns>
	DLL_EXPORT void SetPEGEventEnable(char* controllerId, int axis);

	/// <summary>
	/// 失能事件触发
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="axis">运动轴</param>
	/// <returns></returns>
	DLL_EXPORT void SetPEGEventDisable(char* controllerId, int axis);
	////////////////////////////////////////////////////CNC相关接口////////////////////////////////////////////////////

	/// <summary>
	/// 设置CNC急停减速度
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="value">减速度值</param>
	/// <returns></returns>
	DLL_EXPORT void CNCSetAEmrgDec(char* controllerId, int value);

	/// <summary>
	/// 获取CNC急停减速度
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="value">减速度值</param>
	/// <returns></returns>
	DLL_EXPORT int CNCGetAEmrgDec(char* controllerId, int& value);

	/// <summary>
	/// 设置CNC运动百分比（影响速度和加减速）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="value">百分比值</param>
	/// <returns></returns>
	DLL_EXPORT void CNCSetAPercents(char* controllerId, int value);

	/// <summary>
	/// 获取CNC运动百分比
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="value">百分比值</param>
	/// <returns></returns>
	DLL_EXPORT int CNCGetAPercents(char* controllerId, int& value);

	/// <summary>
	/// 设置CNC编码器比率（用于不同分辨率轴的运动计算）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="value">编码器比率</param>
	/// <returns></returns>
	DLL_EXPORT void CNCSetAEncRatio(char* controllerId, int value);

	/// <summary>
	/// 获取CNC编码器比率
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="value">编码器比率</param>
	/// <returns></returns>
	DLL_EXPORT int CNCGetAEncRatio(char* controllerId, int& value);

	/// <summary>
	/// 获取CNC路径参考位置
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="value">位置</param>
	/// <returns></returns>
	DLL_EXPORT int CNCGetAPosRef(char* controllerId, int& value);

	/// <summary>
	/// 获取CNC路径参考速度（CNCAPosRef的导数）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="value">位置</param>
	/// <returns></returns>
	DLL_EXPORT int CNCGetAdPosRef(char* controllerId, int& value);

	/// <summary>
	/// 暂停CNC运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCPause(char* controllerId );

	/// <summary>
	/// 恢复CNC运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCResume(char* controllerId );

	/// <summary>
	/// 清除CNC运动缓冲区
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCClearBuffer(char* controllerId );

	/// <summary>
	/// 开始CNC运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCBegin(char* controllerId );

	/// <summary>
	/// 停止CNC运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCStop(char* controllerId);

	/// <summary>
	/// 执行三轴绝对线性运动（A、B、C轴）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="aPos">A轴位置</param>
	/// <param name="bPos">B轴位置</param>
	/// <param name="cPos">C轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsolute(char* controllerId, const int* aPos, const int* bPos, const int* cPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行四轴绝对线性运动（A、B、C、D轴）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="aPos">A轴位置</param>
	/// <param name="bPos">B轴位置</param>
	/// <param name="cPos">C轴位置</param>
	/// <param name="dPos">D轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsolute4Axis(char* controllerId, int  aPos, int  bPos, int  cPos, int  dPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行六轴绝对线性运动（A、B、C、D、E、F轴）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="aPos">A轴位置</param>
	/// <param name="bPos">B轴位置</param>
	/// <param name="cPos">C轴位置</param>
	/// <param name="dPos">D轴位置</param>
	/// <param name="ePos">E轴位置</param>
	/// <param name="fPos">F轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsolute6Axis(char* controllerId, int  aPos, int  bPos, int  cPos, int  dPos, int  ePos, int  fPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行多轴绝对线性运动（使用轴掩码指定参与轴）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_groupAxisMask">参与运动的轴</param>
	/// <param name="aPos">A轴位置</param>
	/// <param name="bPos">B轴位置</param>
	/// <param name="cPos">C轴位置</param>
	/// <param name="dPos">D轴位置</param>
	/// <param name="ePos">E轴位置</param>
	/// <param name="fPos">F轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	//DLL_EXPORT bool CNCLinearAbsoluteMultiAxis(char* controllerId, int AxisRef_groupAxisMask, int  aPos, int bPos, int  cPos, int  dPos, int  ePos, int  fPos, int velCruise, int velEnd);
	DLL_EXPORT bool CNCLinearAbsoluteMultiAxis(char* controllerId, int groupAxisMask, int* aPos, int* bPos, int* cPos, int* dPos, int* ePos, int* fPos, int velCruise, int velEnd);
	/// <summary>
	/// 执行十二轴绝对线性运动（A-L轴）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="aPos">A轴位置</param>
	/// <param name="bPos">B轴位置</param>
	/// <param name="cPos">C轴位置</param>
	/// <param name="dPos">D轴位置</param>
	/// <param name="ePos">E轴位置</param>
	/// <param name="fPos">F轴位置</param>
	/// <param name="gPos">G轴位置</param>
	/// <param name="hPos">H轴位置</param>
	/// <param name="iPos">I轴位置</param>
	/// <param name="jPos">J轴位置</param>
	/// <param name="kPos">K轴位置</param>
	/// <param name="lPos">L轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsolute12Axis(char* controllerId, int aPos, int  bPos, int  cPos, int  dPos, int  ePos, int fPos, int gPos, int hPos, int iPos, int jPos, int kPos, int lPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行十二轴绝对线性运动（使用轴掩码指定参与轴）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="controllerId">参与运动的轴</param>
	/// <param name="aPos">A轴位置</param>
	/// <param name="bPos">B轴位置</param>
	/// <param name="cPos">C轴位置</param>
	/// <param name="dPos">D轴位置</param>
	/// <param name="ePos">E轴位置</param>
	/// <param name="fPos">F轴位置</param>
	/// <param name="gPos">G轴位置</param>
	/// <param name="hPos">H轴位置</param>
	/// <param name="iPos">I轴位置</param>
	/// <param name="jPos">J轴位置</param>
	/// <param name="kPos">K轴位置</param>
	/// <param name="lPos">L轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsolute12AxisMask(char* controllerId, int AxisRef_groupAxisMask, int aPos, int bPos, int cPos, int dPos, int ePos, int fPos, int gPos, int hPos, int iPos, int jPos, int kPos, int lPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行T轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="tPos">t轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteT(char* controllerId, int tPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行XT轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="xPos">x轴位置</param>
	/// <param name="tPos">t轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteXT(char* controllerId, int xPos, int tPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行YT轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="yPos">y轴位置</param>
	/// <param name="tPos">t轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteYT(char* controllerId, int yPos, int tPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行ZT轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="zPos">z轴位置</param>
	/// <param name="tPos">t轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteZT(char* controllerId, int zPos, int tPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行XYT轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="xPos">x轴位置</param>
	/// <param name="yPos">y轴位置</param>
	/// <param name="tPos">t轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteXYT(char* controllerId, int xPos, int yPos, int tPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行XZT轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="xPos">x轴位置</param>
	/// <param name="zPos">z轴位置</param>
	/// <param name="tPos">t轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteXZT(char* controllerId, int xPos, int zPos, int tPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行YZT轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="yPos">y轴位置</param>
	/// <param name="zPos">z轴位置</param>
	/// <param name="tPos">t轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteYZT(char* controllerId, int yPos, int zPos, int tPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行XYZT轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="xPos">x轴位置</param>
	/// <param name="yPos">y轴位置</param>
	/// <param name="zPos">z轴位置</param>
	/// <param name="tPos">t轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteXYZT(char* controllerId, int xPos, int yPos, int zPos, int tPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行XT轴圆弧运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="xPos">x轴位置</param>
	/// <param name="tPos">t轴位置</param>
	/// <param name="iPos">i轴位置</param>
	/// <param name="lPos">l轴位置</param>
	/// <param name="dir">运动方向（0：顺时针，1：逆时针）</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCArcXT(char* controllerId,int xPos, int tPos, int iPos, int lPos, int dir, int velCruise, int velEnd);

	/// <summary>
	/// 执行YT轴圆弧运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="yPos">y轴位置</param>
	/// <param name="tPos">t轴位置</param>
	/// <param name="jPos">j轴位置</param>
	/// <param name="lPos">l轴位置</param>
	/// <param name="dir">运动方向（0：顺时针，1：逆时针）</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCArcYT(char* controllerId,int yPos, int tPos, int jPos, int lPos, int dir, int velCruise, int velEnd);

	/// <summary>
	/// 执行ZT轴圆弧运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="zPos">z轴位置</param>
	/// <param name="tPos">t轴位置</param>
	/// <param name="kPos">k轴位置</param>
	/// <param name="lPos">l轴位置</param>
	/// <param name="dir">运动方向（0：顺时针，1：逆时针）</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCArcZT(char* controllerId,int zPos, int tPos, int kPos, int lPos, int dir, int velCruise, int velEnd);

	/// <summary>
	/// 执行通用圆弧运动（使用轴掩码指定参与轴）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_groupAxisMask">参与圆弧运动的两轴</param>
	/// <param name="pos1">1轴位置</param>
	/// <param name="pos2">2轴位置</param>
	/// <param name="centerOffset1">1轴中心偏移</param>
	/// <param name="centerOffset2">2轴中心偏移</param>
	/// <param name="dir">运动方向（0：顺时针，1：逆时针）</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <param name="additionalRevolutions">附加圈数</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCArc(char* controllerId, int AxisRef_groupAxisMask, int pos1, int pos2, int centerOffset1, int centerOffset2, int dir, int velCruise, int velEnd, int additionalRevolutions );

	/// <summary>
	/// 执行X轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="xPos">x轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteX(char* controllerId,int xPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行Y轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="yPos">y轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteY(char* controllerId,int yPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行Z轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="zPos">z轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteZ(char* controllerId,int zPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行XY轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="xPos">x轴位置</param>
	/// <param name="yPos">y轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteXY(char* controllerId,int xPos, int yPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行XZ轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="xPos">x轴位置</param>
	/// <param name="zPos">z轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteXZ(char* controllerId,int xPos, int zPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行YZ轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="yPos">y轴位置</param>
	/// <param name="zPos">z轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteYZ(char* controllerId,int yPos, int zPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行XYZ轴绝对线性运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="xPos">x轴位置</param>
	/// <param name="yPos">y轴位置</param>
	/// <param name="zPos">z轴位置</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCLinearAbsoluteXYZ(char* controllerId, int xPos, int yPos, int zPos, int velCruise, int velEnd);

	/// <summary>
	/// 执行XY轴圆弧运动（单圈）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="xPos">x轴位置</param>
	/// <param name="yPos">y轴位置</param>
	/// <param name="iPos">x轴中心偏移</param>
	/// <param name="jPos">y轴中心偏移</param>
	/// <param name="dir">运动方向（0：顺时针，1：逆时针）</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCArcXY1(char* controllerId,int xPos, int yPos, int iPos, int jPos, int dir, int velCruise, int velEnd);

	/// <summary>
	/// 执行XZ轴圆弧运动（单圈）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="xPos">x轴位置</param>
	/// <param name="zPos">z轴位置</param>
	/// <param name="iPos">x轴中心偏移</param>
	/// <param name="kPos">z轴中心偏移</param>
	/// <param name="dir">运动方向（0：顺时针，1：逆时针）</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCArcXZ1(char* controllerId,int xPos, int zPos, int iPos, int kPos, int dir, int velCruise, int velEnd);

	/// <summary>
	/// 执行YZ轴圆弧运动（单圈）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="yPos">y轴位置</param>
	/// <param name="zPos">z轴位置</param>
	/// <param name="jPos">y轴中心偏移</param>
	/// <param name="kPos">z轴中心偏移</param>
	/// <param name="dir">运动方向（0：顺时针，1：逆时针）</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCArcYZ1(char* controllerId,int yPos, int zPos, int jPos, int kPos, int dir, int velCruise, int velEnd);

	/// <summary>
	/// 执行XY轴圆弧运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="xPos">x轴位置</param>
	/// <param name="yPos">y轴位置</param>
	/// <param name="iPos">x轴中心偏移</param>
	/// <param name="jPos">y轴中心偏移</param>
	/// <param name="dir">运动方向（0：顺时针，1：逆时针）</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <param name="addtionalCycles">附加圈数</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCArcXY2(char* controllerId,int xPos, int yPos, int iPos, int jPos, int dir, int velCruise, int velEnd, int addtionalCycles);

	/// <summary>
	/// 执行XZ轴圆弧运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="xPos">x轴位置</param>
	/// <param name="zPos">z轴位置</param>
	/// <param name="iPos">x轴中心偏移</param>
	/// <param name="kPos">z轴中心偏移</param>
	/// <param name="dir">运动方向（0：顺时针，1：逆时针）</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <param name="addtionalCycles">附加圈数</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCArcXZ2(char* controllerId,int xPos, int zPos, int iPos, int kPos, int dir, int velCruise, int velEnd, int addtionalCycles);

	/// <summary>
	/// 执行YZ轴圆弧运动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="yPos">y轴位置</param>
	/// <param name="zPos">z轴位置</param>
	/// <param name="jPos">y轴中心偏移</param>
	/// <param name="kPos">z轴中心偏移</param>
	/// <param name="dir">运动方向（0：顺时针，1：逆时针）</param>
	/// <param name="velCruise">CNC运动速度</param>
	/// <param name="velEnd">CNC结束速度</param>
	/// <param name="addtionalCycles">附加圈数</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCArcYZ2(char* controllerId,int yPos, int zPos, int jPos, int kPos, int dir, int velCruise, int velEnd, int addtionalCycles);

	/// <summary>
	/// 执行CNC延时操作
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="ms">延时时间</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCDelay(char* controllerId,int ms);

	/// <summary>
	/// 设置CNC运动参数（百分比、加速度、减速度、平滑因子）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="percentage">CNC百分比</param>
	/// <param name="accel">CNC加速度</param>
	/// <param name="decel">CNC减速度</param>
	/// <param name="sfactor">CNC平滑因子</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCSetMotionProfile(char* controllerId, int percentage, int accel, int decel, double sfactor);

	/// <summary>
	/// 设置CNC拐角参数（拐角类型、半径方法、半径/误差、轴加速度、最小角度、加速度限制类型）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="cornerType">CNC拐角类型</param>
	/// <param name="radiusMethod">CNC半径方法</param>
	/// <param name="radiusOrError">CNC半径或误差</param>
	/// <param name="axisAccel">CNC轴加速度</param>
	/// <param name="minAngle">CNC最小角度</param>
	/// <param name="accelLimitType">CNC加速度限制类型</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCSetCornerParams(char* controllerId, int cornerType, int radiusMethod, int radiusOrError, int axisAccel, int minAngle, int accelLimitType);

	/// <summary>
	/// 设置起始位置（使用轴掩码）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_groupAxisMask">轴掩码</param>
	/// <param name="xPosCurrent">X轴当前位置</param>
	/// <param name="yPosCurrent">YX轴当前位置</param>
	/// <param name="zPosCurrent">Z轴当前位置</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCSetStartPositionsAxisRef(char* controllerId, int AxisRef_groupAxisMask, int xPosCurrent, int yPosCurrent, int zPosCurrent);

	/// <summary>
	/// 设置起始位置（A、B、C轴）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="aPos">A轴位置</param>
	/// <param name="bPos">B轴位置</param>
	/// <param name="cPos">C轴位置</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCSetStartPositions(char* controllerId, int  aPos, int  bPos, int  cPos);

	/// <summary>
	/// 写入数字输出端口值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="dOutPortValue">数字输出端口值</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCWriteDOutPort(char* controllerId, int dOutPortValue);

	/// <summary>
	/// 清除轴数字输出位
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_axisRef">轴引用</param>
	/// <param name="bitMask">位掩码</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCGroupDOutClearBit(char* controllerId, int AxisRef_axisRef, int bitMask);

	/// <summary>
	/// 设置轴数字输出位
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_axisRef">轴引用</param>
	/// <param name="bitMask">位掩码</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCGroupDOutSetBit(char* controllerId, int AxisRef_axisRef, int bitMask);

	/// <summary>
	/// 切换轴数字输出位
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_axisRef">轴引用</param>
	/// <param name="bitMask">位掩码</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCGroupDOutToggleBit(char* controllerId, int AxisRef_axisRef, int bitMask);

	/// <summary>
	/// 写入GenData值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="gendataIndex">GenData索引</param>
	/// <param name="gendataValue">GenData值</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCWriteGenData(char* controllerId, int gendataIndex, int gendataValue);

	/// <summary>
	/// 写入UserParam值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_axisRef">轴引用</param>
	/// <param name="userparamIndex">UserParam索引</param>
	/// <param name="userparamValue">UserParam值</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCWriteUserParam(char* controllerId, int AxisRef_axisRef, int userparamIndex, int userparamValue);

	/// <summary>
	/// 等待GenData条件满足
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="gendataIndex">GenData索引</param>
	/// <param name="trigTyp">触发类型</param>
	/// <param name="trigVal">触发值</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCGenDataWait(char* controllerId, int gendataIndex, int trigTyp, int trigVal);

	/// <summary>
	/// 等待UserParam条件满足
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_axisRef">轴引用</param>
	/// <param name="userparamIndex">UserParam索引</param>
	/// <param name="trigTyp">触发类型</param>
	/// <param name="trigVal">触发值</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCUserParamWait(char* controllerId, int AxisRef_axisRef, int userparamIndex, int trigTyp, int trigVal);

	/// <summary>
	/// 设置当前位置（A、B、C轴）
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="aPos">A轴位置</param>
	/// <param name="bPos">B轴位置</param>
	/// <param name="cPos">C轴位置</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCSetCurrPositions(char* controllerId, int aPos, int bPos, int cPos);

	/// <summary>
	/// 自动拐角
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_axisRef">轴引用</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCAutoCorner(char* controllerId, int AxisRef_axisRef);

	/// <summary>
	/// 设置最大速度跳跃参数
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="aMaxVJ">A轴最大速度跳跃</param>
	/// <param name="bMaxVJ">B轴最大速度跳跃</param>
	/// <param name="cMaxVJ">C轴最大速度跳跃</param>
	/// <param name="jumpMode">跳跃模式</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCSetMaxVelJumpParams(char* controllerId, int aMaxVJ, int  bMaxVJ, int  cMaxVJ, int jumpMode);

	/// <summary>
	/// 设置最大加速度参数
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="aMaxAccel">A轴最大加速度</param>
	/// <param name="bMaxAccel">B轴最大加速度</param>
	/// <param name="cMaxAccel">C轴最大加速度</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCSetMaxAccelParams(char* controllerId, int  aMaxAccel, int  bMaxAccel, int  cMaxAccel);

	/// <summary>
	/// 多通道写入GenData值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="gendataIndex">GenData索引</param>
	/// <param name="gendataValue1">GenData值1</param>
	/// <param name="gendataValue2">GenData值2</param>
	/// <param name="gendataValue3">GenData值3</param>
	/// <param name="gendataValue4">GenData值4</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCMultiWriteGenData(char* controllerId, int gendataIndex, int gendataValue1, int gendataValue2, int gendataValue3, int gendataValue4);

	/// <summary>
	/// 多通道写入UserParam值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_axisRef">轴引用</param>
	/// <param name="userparamIndex">UserParam索引</param>
	/// <param name="userparamValue1">UserParam值1</param>
	/// <param name="userparamValue2">UserParam值2</param>
	/// <param name="userparamValue3">UserParam值3</param>
	/// <param name="userparamValue4">UserParam值4</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCMultiWriteUserParam(char* controllerId, int AxisRef_axisRef, int userparamIndex, int userparamValue1, int userparamValue2, int userparamValue3, int userparamValue4);

	/// <summary>
	/// 多通道写入GenData值并等待条件
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="gendataIndex">GenData索引</param>
	/// <param name="gendataValue1">GenData值1</param>
	/// <param name="gendataValue2">GenData值2</param>
	/// <param name="gendataValue3">GenData值3</param>
	/// <param name="gendataValue4">GenData值4</param>
	/// <param name="trigIndex">触发索引</param>
	/// <param name="trigTyp">触发类型</param>
	/// <param name="trigVal">触发值</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCMultiWriteGenDataWait(char* controllerId, int gendataIndex, int gendataValue1, int gendataValue2, int gendataValue3, int gendataValue4, int trigIndex, int trigTyp, int trigVal);

	/// <summary>
	/// 多通道写入GenData值并等待条件
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_axisRef">轴引用</param>
	/// <param name="userparamIndex">UserParam索引</param>
	/// <param name="userparamValue1">UserParam值1</param>
	/// <param name="userparamValue2">UserParam值2</param>
	/// <param name="userparamValue3">UserParam值3</param>
	/// <param name="userparamValue4">UserParam值4</param>
	/// <param name="trigIndex">触发索引</param>
	/// <param name="trigTyp">触发类型</param>
	/// <param name="trigVal">触发值</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCMultiWriteUserParamWait(char* controllerId, int AxisRef_axisRef, int userparamIndex, int userparamValue1, int userparamValue2, int userparamValue3, int userparamValue4, int trigIndex, int trigTyp, int trigVal);

	/// <summary>
	/// 运行来自文件的CNC程序
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="filePath">CNC文件的绝对路径</param>
	/// <returns></returns>
	DLL_EXPORT void CNCRunCNCProgramFromFile(char* controllerId, const char*  filePath);

	/// <summary>
	/// 检查CNC运动是否完成
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCIsCNCCompleted(char* controllerId);

	/// <summary>
	/// 写入指定轴的数字输出端口值
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_axis">轴引用</param>
	/// <param name="dOutPortValue">数字输出端口值</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCWriteDOutPortAxisRef(char* controllerId, int AxisRef_axis, int dOutPortValue);

	/// <summary>
	/// 设置Central-I数字输出位
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_axis">轴引用</param>
	/// <param name="bitMask">位掩码</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCCiGroupDOutSetBit(char* controllerId, int AxisRef_axis, int bitMask);

	/// <summary>
	/// 清除Central-I数字输出位
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_axis">轴引用</param>
	/// <param name="bitMask">位掩码</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCCiGroupDOutClearBit(char* controllerId, int AxisRef_axis, int bitMask);

	/// <summary>
	/// 切换Central-I数字输出位
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="AxisRef_axis">轴引用</param>
	/// <param name="bitMask">位掩码</param>
	/// <returns></returns>
	DLL_EXPORT bool CNCCiGroupDOutToggleBit(char* controllerId, int AxisRef_axis, int bitMask);





	
	/////////////////////////////////////////////光迅接口

	int WriteGenData(const char* controllerId, int index, int value);

	int ReadGenData(const char* controllerId, int index, int& returnValue);

	DLL_EXPORT int writedatatest(char* controllerId, int a,int b);

	DLL_EXPORT int readdatatest(char* controllerId, int& a, int& b);

	DLL_EXPORT int writedatatest2(char* controllerId, int a, int b);

	DLL_EXPORT int readdatatest2(char* controllerId, int& a, int& b);
	/// <summary>
	/// 单轴工艺参数设定
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="SingleStartRelTrgt">起始点设定</param>
	/// <param name="SingleEndRelTrgt">终点设定</param>
	/// <param name="SingleMotionGap">单次采样步距</param>
	/// <param name="SingleSampleNum">单次采样次数</param>
	/// <param name="SingleMotionSpeed">运动速度</param>
	/// <param name="SingleAxisSelect">遍历轴号选择</param>
	/// <param name="SingleAInSelect">选择模拟量通道，0-AInPort[1]，1-AInPort[2]，2-AInPort[3]</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int SingleMotion(char* controllerId, int SingleStartRelTrgt, int SingleEndRelTrgt, int SingleMotionGap, int SingleSampleNum,
		int SingleMotionSpeed, int SingleAxisSelect, int SingleAInSelect);

	DLL_EXPORT int SingleMotion_(char* controllerId, int SingleStartRelTrgt, int SingleEndRelTrgt, int SingleMotionGap, int SingleSampleNum,
		int SingleAxisSelect, int SingleAInSelect);
	/// <summary>
	/// 单轴工艺参数获取
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="SingleStartRelTrgt">起始点设定</param>
	/// <param name="SingleEndRelTrgt">终点设定</param>
	/// <param name="SingleMotionGap">单次采样步距</param>
	/// <param name="SingleSampleNum">单次采样次数</param>
	/// <param name="SingleMotionSpeed">单轴遍历速度</param>
	/// <param name="SingleAxisSelect">遍历轴号选择</param>
	/// <param name="SingleRtnSpeed">单轴遍历缓冲速度</param>
	/// <param name="SingleAInSelect">模拟量通道，0-AInPort[1]，1-AInPort[2]，2-AInPort[3]</param>
	/// <returns></returns>
	DLL_EXPORT int GetSingleMotionPara(char* controllerId, int& SingleStartRelTrgt, int& SingleEndRelTrgt, int& SingleMotionGap, int& SingleSampleNum,
		int& SingleMotionSpeed, int& SingleAxisSelect, int& SingleRtnSpeed, int& SingleAInSelect);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="SingleMaxAIn">存储最大模拟量值</param>
	/// <param name="SingleMaxAbsPos">存储最大模拟量点位置</param>
	/// <returns></returns>
	DLL_EXPORT int GetSingleMotionPara_(char* controllerId, int& SingleMaxAIn, int& SingleMaxAbsPos);
	/// <summary>
	/// 单轴遍历启动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int SingleMotionOn(char* controllerId);
	/// <summary>
	/// 判断单轴遍历是否在进行中
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>处于单轴遍历模式中true，不在该模式下false</returns>
	DLL_EXPORT bool IsSingleMotionOn(char* controllerId);
	/// <summary>
	/// 获取单轴运动模拟量值数组，索引3001-5000，用户定义的数组长度需不小于2000
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="data">用户定义的数组，用来输出获取的数据</param>
	/// <returns></returns>
	DLL_EXPORT int GetSingleAInArray(char* controllerId,  std::vector<int>& data);
	/// <summary>
	/// 获取单轴运动模拟量值，索引3001-5000
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="index">查询的索引值</param>
	/// <param name="data">用户定义的变量，用来输出获取的数据</param>
	/// <returns></returns>
	DLL_EXPORT int GetSingleAIn(char* controllerId, int index, int& data);
	/// <summary>
	/// 获取数组存储数据的数量
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="number"></param>
	/// <returns></returns>
	DLL_EXPORT int GetSingleAInArrayNumber(char* controllerId, int& number);
	/// <summary>
	/// 螺旋线工艺参数设定
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="HelixDir">旋转方向选择</param>
	/// <param name="HelixPitch">螺旋间距</param>
	/// <param name="HelixTurnNum">螺旋圈数</param>
	/// <param name="HelixAInSelect">模拟电压采样通道选择</param>
	/// <param name="HelixXSelect">螺旋X轴选择</param>
	/// <param name="HelixYSelect">螺旋Y轴选择</param>
	/// <param name="HelixMaxAInThd">模拟电压阈值设定</param>
	/// <param name="HelixRtnSpeed">螺旋返回速度</param>
	/// <param name="HelixStopMode">螺旋停止模式</param>
	/// <param name="HelixRtnMode">螺旋返回模式</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int HelixMotion(char* controllerId, int HelixDir, int HelixPitch, int HelixTurnNum,
		int HelixAInSelect, int HelixXSelect, int HelixYSelect, int HelixMaxAInThd, int HelixStopMode, int HelixRtnMode);
	/// <summary>
	/// 螺旋线工艺参数获取
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="HelixDir">旋转方向选择</param>
	/// <param name="HelixPitch">螺旋间距</param>
	/// <param name="HelixXRes">X反馈范围</param>
	/// <param name="HelixYRes">Y反馈范围</param>
	/// <param name="HelixTurnNum">螺旋圈数</param>
	/// <param name="HelixSpeed">螺旋速度</param>
	/// <param name="HelixPointNum">螺旋每圈采样点数</param>
	/// <param name="HelixAInSelect">模拟电压采样通道选择</param>
	/// <param name="HelixXSelect">螺旋X轴选择</param>
	/// <param name="HelixYSelect">螺旋Y轴选择</param>
	/// <param name="HelixMaxAInThd">模拟电压阈值设定</param>
	/// <param name="HelixRtnSpeed">螺旋返回速度</param>
	/// <param name="HelixStopMode">螺旋停止模式</param>
	/// <param name="HelixRtnMode">螺旋返回模式</param>
	/// <returns></returns>
	DLL_EXPORT int GetHelixMotionPara(char* controllerId, int& HelixDir, int& HelixPitch, int& HelixXRes, int& HelixYRes, int& HelixTurnNum, int& HelixSpeed, int& HelixPointNum,
		int& HelixAInSelect, int& HelixXSelect, int& HelixYSelect, int& HelixMaxAInThd, int& HelixRtnSpeed, int& HelixStopMode, int& HelixRtnMode);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="HelixMaxXAbsPos">单次螺旋工艺的X坐标最大光强</param>
	/// <param name="HelixMaxYAbsPos">单次螺旋工艺的Y坐标最大光强</param>
	/// <param name="HelixMaxAInVal">单次螺旋工艺采样的最大模拟电压</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int GetHelixMotionPara_(char* controllerId, int& HelixMaxXAbsPos, int& HelixMaxYAbsPos, int& HelixMaxAInVal);
	/// <summary>
	/// 螺旋找光启动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int HelixMotionOn(char* controllerId);
	/// <summary>
	/// 判断螺旋找光模式是否在进行中
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>处于螺旋找光模式中true，不在该模式下false</returns>
	DLL_EXPORT bool IsHelixMotionOn(char* controllerId);
	/// <summary>
	/// 获取单轴单向半程运动模拟量值数组，索引2001-5000，用户定义的数组长度需不小于3000
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="data">用户定义的数组，用来输出获取的数据</param>
	/// <returns></returns>
	DLL_EXPORT int GetHelixAInArray(char* controllerId,  std::vector<int>& data);
	/// <summary>
	/// 获取单轴单向半程运动模拟量值，索引2001-5000
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="index">查询的索引值</param>
	/// <param name="data">用户定义的变量，用来输出获取的数据</param>
	/// <returns></returns>
	DLL_EXPORT int GetHelixAIn(char* controllerId, int index, int& data);
	/// <summary>
	/// 获取数组存储数据的数量
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="number"></param>
	/// <returns></returns>
	DLL_EXPORT int GetHelixAInArrayNumber(char* controllerId, int& number);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="ainexception">报告模拟量出现异常，当值为1时表示模拟量出现非正值</param>
	/// <param name="motorexception">值=0，正常；值=1，X轴下使能；值=2，Y轴下使能；值=3，X、Y均下使能</param>
	/// <returns></returns>
	DLL_EXPORT int GetHelixRtnVal(char* controllerId, int& ainexception, int& motorexception);
	/// <summary>
	/// 单轴单向半程运动参数设定
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="HalfSigAxisSelect">选择运动轴，0-A轴，1-B轴，2-C轴</param>
	/// <param name="HalfSigRelTrgt">相对起点最大位移</param>
	/// <param name="HalfSigMotionGap">步距</param>
	/// <param name="HalfSigSampleNum">每点模拟量采样次数</param>
	/// <param name="HalfSigMotionSpeed">半程运动速度</param>
	/// <param name="HalfSigDecrNum">下降次数</param>
	/// <param name="HalfSigAInSelect">选择模拟量通道，0 for AInPort[1], 1 for AInPort[2], 2 for AInPort[3]</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int HalfSigMotion(char* controllerId, int HalfSigAxisSelect, int HalfSigRelTrgt, int HalfSigMotionGap, int HalfSigSampleNum, int HalfSigMotionSpeed, int HalfSigDecrNum
		, int HalfSigAInSelect);
	/// <summary>
	/// 获取单轴单向半程运动参数
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="HalfSigAxisSelect">选择运动轴，0-A轴，1-B轴，2-C轴</param>
	/// <param name="HalfSigRelTrgt">相对起点最大位移</param>
	/// <param name="HalfSigMotionGap">步距</param>
	/// <param name="HalfSigSampleNum">每点模拟量采样次数</param>
	/// <param name="HalfSigMotionSpeed">半程运动速度</param>
	/// <param name="HalfSigDecrNum">下降次数</param>
	/// <param name="HalfSigRtnSpeed">半程运动返回最大值位置速度</param>
	/// <param name="HalfSigAInSelect">选择模拟量通道，0 for AInPort[1], 1 for AInPort[2], 2 for AInPort[3]</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int GetHalfSigMotionPara(char* controllerId, int& HalfSigAxisSelect, int& HalfSigRelTrgt, int& HalfSigMotionGap, int& HalfSigSampleNum, int& HalfSigMotionSpeed,
		int& HalfSigDecrNum, int& HalfSigRtnSpeed, int& HalfSigAInSelect);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="HalfMaxAIn">存储最大模拟量值</param>
	/// <param name="HalfMaxPos">存储最大模拟量点位置</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int GetHalfSigMotionPara_(char* controllerId, int& HalfMaxAIn, int& HalfMaxPos);
	/// <summary>
	/// 单轴单向半程运动启动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int HalfSigMotionOn(char* controllerId);
	/// <summary>
	/// 判断单轴单向半程运动模式是否在进行中
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>处于单轴单向半程运动模式中true，不在该模式下false</returns>
	DLL_EXPORT bool IsHalfSigMotionOn(char* controllerId);
	/// <summary>
	/// 获取单轴单向半程运动模拟量值数组，索引1000-2999，用户定义的数组长度需不小于2000
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="data">用户定义的数组，用来输出获取的数据</param>
	/// <returns></returns>
	DLL_EXPORT int GetHalfAInArray(char* controllerId, std::vector<int>& data);
	/// <summary>
	/// 获取单轴单向半程运动模拟量值，索引1000-2999
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="index">查询的索引值</param>
	/// <param name="data">用户定义的变量，用来输出获取的数据</param>
	/// <returns></returns>
	DLL_EXPORT int GetHalfAIn(char* controllerId, int index, int& data);
	/// <summary>
	/// 获取数组存储数据的数量
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="number"></param>
	/// <returns></returns>
	DLL_EXPORT int GetHalfAInArrayNumber(char* controllerId, int& number);
	/// <summary>
	/// 单轴单向半程运动+diff参数设定
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="DiffAxisSelect">选择运动轴，0-A轴，1-B轴，2-C轴</param>
	/// <param name="DiffSigRelTrgt">相对起点最大位移</param>
	/// <param name="DiffMotionGap">步距</param>
	/// <param name="DiffSampleNum">每点模拟量采样次数</param>
	/// <param name="DiffMotionSpeed">半程运动速度</param>
	/// <param name="DiffDecrNum">下降次数</param>
	/// <param name="DiffValue">diff差值</param>
	/// <param name="DiffAInSelect">选择模拟量通道，0 for AInPort[1], 1 for AInPort[2], 2 for AInPort[3]</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int DiffMotion(char* controllerId,int DiffAxisSelect, int DiffSigRelTrgt, int DiffMotionGap, int DiffSampleNum, int DiffMotionSpeed, int DiffDecrNum
		, int DiffValue, int DiffAInSelect);

	DLL_EXPORT int DiffMotion_(char* controllerId, int DiffAxisSelect, int DiffSigRelTrgt, int DiffMotionGap, int DiffSampleNum, int DiffDecrNum
		, int DiffValue, int DiffAInSelect);
	/// <summary>
	/// 获取单轴单向半程运动+diff参数
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="DiffAxisSelect">选择运动轴，0-A轴，1-B轴，2-C轴</param>
	/// <param name="DiffSigRelTrgt">相对起点最大位移</param>
	/// <param name="DiffMotionGap">步距</param>
	/// <param name="DiffSampleNum">每点模拟量采样次数</param>
	/// <param name="DiffMotionSpeed">半程运动速度</param>
	/// <param name="DiffDecrNum">下降次数</param>
	/// <param name="DiffValue">diff差值</param>
	/// <param name="DiffRtnSpeed">半程运动返回最大值位置速度</param>
	/// <param name="DiffAInSelect">选择模拟量通道，0 for AInPort[1], 1 for AInPort[2], 2 for AInPort[3]</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int GetDiffMotionPara(char* controllerId, int& DiffAxisSelect, int& DiffSigRelTrgt, int& DiffMotionGap, int& DiffSampleNum, int& DiffMotionSpeed, int& DiffDecrNum
		, int& DiffValue, int& DiffRtnSpeed, int& DiffAInSelect);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="DiffMaxAIn">存储最大模拟量值</param>
	/// <param name="DiffMaxPos">存储最大模拟量点位置</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int GetDiffMotionPara_(char* controllerId, int& DiffMaxAIn, int& DiffMaxPos);
	/// <summary>
	/// 单轴单向半程运动+diff启动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int DiffMotionOn(char* controllerId);
	/// <summary>
	/// 判断单轴单向半程运动+diff模式是否在进行中
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>处于单轴单向半程运动+diff模式中true，不在该模式下false</returns>
	DLL_EXPORT bool IsDiffMotionOn(char* controllerId);
	/// <summary>
	/// 获取单轴单向半程运动+diff模拟量值数组，索引1000-2999，用户定义的数组长度需不小于2000
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="data">用户定义的数组，用来输出获取的数据</param>
	/// <returns></returns>
	DLL_EXPORT int GetDiffAInArray(char* controllerId,  std::vector<int>& data);
	/// <summary>
	/// 获取单轴单向半程运动+diff模拟量值，索引1000-2999
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="index">查询的索引值</param>
	/// <param name="data">用户定义的变量，用来输出获取的数据</param>
	/// <returns></returns>
	DLL_EXPORT int GetDiffAIn(char* controllerId, int index, int& data);
	/// <summary>
	/// 获取数组存储数据的数量
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="number"></param>
	/// <returns></returns>
	DLL_EXPORT int GetDiffAInArrayNumber(char* controllerId, int& number);
	/// <summary>
	/// 单轴遍历+dB参数设定
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="DBStartRelTrgt">相对当前位置的起点偏移（相对位置）</param>
	/// <param name="DBEndRelTrgt">相对当前位置的终点偏移（相对位置）</param>
	/// <param name="DBMotionGap">步距</param>
	/// <param name="DBSampleNum">每点模拟量采样次数</param>
	/// <param name="DBMotionSpeed">遍历速度</param>
	/// <param name="DBAxisSelect">选择运动轴，0-A轴，1-B轴，2-C轴</param>
	/// <param name="DBValue">下降dB阈值，mV</param>
	/// <param name="DBAInSelect">选择模拟量通道，0 for AInPort[1], 1 for AInPort[2], 2 for AInPort[3]</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int DBMotion(char* controllerId, int DBStartRelTrgt, int DBEndRelTrgt, int DBMotionGap, int DBSampleNum, int DBMotionSpeed, int DBAxisSelect
		, int DBValue, int DBAInSelect);

	DLL_EXPORT int DBMotion_(char* controllerId, int DBStartRelTrgt, int DBEndRelTrgt, int DBMotionGap, int DBSampleNum, int DBAxisSelect
		, int DBValue, int DBAInSelect);
	/// <summary>
	/// 获取单轴遍历+dB参数
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="DBStartRelTrgt">相对当前位置的起点偏移（相对位置）</param>
	/// <param name="DBEndRelTrgt">相对当前位置的终点偏移（相对位置）</param>
	/// <param name="DBMotionGap">步距</param>
	/// <param name="DBSampleNum">每点模拟量采样次数</param>
	/// <param name="DBMotionSpeed">遍历速度</param>
	/// <param name="DBAxisSelect">选择运动轴，0-A轴，1-B轴，2-C轴</param>
	/// <param name="DBRtnSpeed">返回dB最大值速度</param>
	/// <param name="DBValue">下降dB阈值，mV</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int GetDBMotionPara(char* controllerId, int& DBStartRelTrgt, int& DBEndRelTrgt, int& DBMotionGap, int& DBSampleNum, int& DBMotionSpeed, int& DBAxisSelect
		, int& DBRtnSpeed, int& DBValue);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="DBLeftPos">单轴遍历+DB，左位置值</param>
	/// <param name="DBRightPos">单轴遍历+DB，右位置值</param>
	/// <param name="DBMaxAIn">单轴遍历+DB，最大模拟量值</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int GetDBMotionPara_(char* controllerId, int& DBLeftPos, int& DBRightPos, int& DBMaxAIn);
	/// <summary>
    /// 单轴遍历+dB启动
    /// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int DBMotionOn(char* controllerId);
	/// <summary>
	/// 判断单轴遍历+dB模式是否在进行中
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>处于单轴遍历+dB模式中true，不在该模式下false</returns>
	DLL_EXPORT bool IsDBMotionOn(char* controllerId);
	/// <summary>
	/// 获取单轴遍历+dB模拟量值数组，索引1000-2999，用户定义的数组长度需不小于2000
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="data">用户定义的数组，用来输出获取的数据</param>
	/// <returns></returns>
	DLL_EXPORT int GetDBAInArray(char* controllerId, std::vector<int>& data);
	/// <summary>
	/// 获取单轴遍历+dB模拟量值，索引1000-2999
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="index">查询的索引值</param>
	/// <param name="data">用户定义的变量，用来输出获取的数据</param>
	/// <returns></returns>
	DLL_EXPORT int GetDBAIn(char* controllerId, int index, int& data);
	/// <summary>
	/// 获取数组存储数据的数量
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="number"></param>
	/// <returns></returns>
	DLL_EXPORT int GetDBAInArrayNumber(char* controllerId, int& number);
	/// <summary>
	/// 离焦循光参数设定
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="OFAInMin">Analog左限，单位：0.1mV</param>
	/// <param name="OFAInMax">Analog右限，单位：0.1mV</param>
	/// <param name="OFAInSelect">0-AInPort[1], 1-AInPort[2], 2-AInPort[3]</param>
	/// <param name="OFStepMax">最大运动步距</param>
	/// <param name="OFStepDiv">步距细分倍数</param>
	/// <param name="OFStepCnt">细分次数</param>
	/// <param name="OFTimeout">离焦保护范围</param>
	/// <param name="OFStepSpeed">X轴步进速度</param>
	/// <param name="OFStepDir">X运动方向，1为正方向，-1为负方向</param>
	/// <param name="OFAxisSelect">选择X轴，0 for A_Axis, 1 for B_Axis, 2 for C_Axis，第1次运动的轴</param>
	/// <param name="OFSampleNum">每点模拟量采集次数</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int OFStep(char* controllerId, int OFAInMin, int OFAInMax, int OFAInSelect, int OFStepMax, int OFStepDiv, int OFStepCnt
		, int OFTimeout, int OFStepSpeed, int OFStepDir, int OFAxisSelect, int OFSampleNum);

	DLL_EXPORT int OFStep_(char* controllerId, int OFAInMin, int OFAInMax, int OFAInSelect, int OFStepMax, int OFStepDiv, int OFStepCnt
		, int OFTimeout, int OFStepDir, int OFAxisSelect, int OFSampleNum);
	/// <summary>
	/// 获取离焦循光参数
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="OFAInMin">Analog左限，单位：0.1mV</param>
	/// <param name="OFAInMax">Analog右限，单位：0.1mV</param>
	/// <param name="OFAInSelect">0-AInPort[1], 1-AInPort[2], 2-AInPort[3]</param>
	/// <param name="OFStepMax">最大运动步距</param>
	/// <param name="OFStepDiv">步距细分倍数</param>
	/// <param name="OFStepCnt">细分次数</param>
	/// <param name="OFTimeout">延时(ms)</param>
	/// <param name="OFStepSpeed">X轴步进速度</param>
	/// <param name="OFStepDir">X运动方向，1为正方向，-1为负方向</param>
	/// <param name="OFAxisSelect">选择X轴，0 for A_Axis, 1 for B_Axis, 2 for C_Axis，第1次运动的轴</param>
	/// <param name="OFSampleNum">每点模拟量采集次数</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int GetOFStepPara(char* controllerId, int& OFAInMin, int& OFAInMax, int& OFAInSelect, int& OFStepMax, int& OFStepDiv, int& OFStepCnt
		, int& OFTimeout, int& OFStepSpeed, int& OFStepDir, int& OFAxisSelect, int& OFSampleNum);
	/// <summary>
	/// 
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="OFMaxPos">存储最大模拟量X轴位置</param>
	/// <param name="OFMaxAInValue">存储X最大模拟量值</param>
	/// <param name="OFRtnValue">返回程序信息，=100成功，-1超时失败</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int GetOFStepPara_(char* controllerId, int& OFMaxPos, int& OFMaxAInValue, int& OFRtnValue);
	/// <summary>
	/// 离焦循光启动
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>成功0,失败-1</returns>
	DLL_EXPORT int OFStepOn(char* controllerId);
	/// <summary>
	/// 判断离焦循光模式是否在进行中
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <returns>处于单轴遍历+dB模式中true，不在该模式下false</returns>
	DLL_EXPORT bool IsOFStepOn(char* controllerId);
	/// <summary>
	/// 获取离焦循光模拟量值数组，索引1000-2999，用户定义的数组长度需不小于2000
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="data">用户定义的数组，用来输出获取的数据</param>
	/// <returns></returns>
	DLL_EXPORT int GetOFAInArray(char* controllerId, std::vector<int>& data);
	/// <summary>
	/// 获取离焦循光模拟量值，索引1000-2999
	/// </summary>
	/// <param name="controllerId">控制器ID</param>
	/// <param name="index">查询的索引值</param>
	/// <param name="data">用户定义的变量，用来输出获取的数据</param>
	/// <returns></returns>
	DLL_EXPORT int GetOFAIn(char* controllerId, int index, int& data);

	int getarray(char* controllerId, int startinx, int endinx, std::vector<int> data);

	int getarray2(char* controllerId, int startinx, int endinx, std::vector<int>& data);

	DLL_EXPORT int returnarraytest(char* controllerId, int startinx, int endinx, std::vector<int>& data);

	DLL_EXPORT int returnstringarraytest(char* controllerId, int startinx, int endinx, std::vector<char*>& data);

#ifdef _cplusplus
}
#endif


#endif
}

