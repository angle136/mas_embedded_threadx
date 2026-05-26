# 先加载默认模板，再覆盖差异

include(${CMAKE_CURRENT_LIST_DIR}/../../modules/module_config.cmake)

# 模块开关（按板型覆盖默认值）
set(MODULES_SINGLE   OFFLINE REMOTE BMI088 INS REFEREE SUPERCAP VISION MOTOR)

# OFFLINE 默认参数
set(OFFLINE_WATCHDOG_ENABLE 1)    # 开启看门狗
set(OFFLINE_BEEP_ENABLE     1)    # 开启蜂鸣器
set(OFFLINE_TASK_STACK_SIZE 1024) # 任务栈大小
set(OFFLINE_TASK_PRIORITY   6)    # 任务优先级

# REMOTE 默认参数
set(REMOTE_UART             huart3) # 串口
set(REMOTE_VT_UART          huart1) # 图传串口
set(REMOTE_SOURCE           2)      # 遥控器选择: 0=none, 1=sbus, 2=dt7
set(REMOTE_VT_SOURCE        0)      # 图传选择:   0=none, 1=vt02, 2=vt03
set(REMOTE_DEAD_ZONE        10)     # 死区
set(REMOTE_TASK_STACK_SIZE  1024)   # 任务栈大小
set(REMOTE_TASK_PRIORITY    9)      # 任务优先级
set(REMOTE_TASK_VT_PRIORITY 8)      # 图传串口任务优先级
set(REMOTE_OFFLINE_ENABLE      1)   # 遥控器离线检测
set(REMOTE_VT_OFFLINE_ENABLE   1)   # 图传离线检测

# REFEREE 默认参数
set(REFEREE_UART            huart1) # 串口选择
set(REFEREE_TASK_STACK_SIZE 1024)   # 任务栈大小
set(REFEREE_TASK_PRIORITY   10)     # 任务优先级
set(REFEREE_OFFLINE_ENABLE  0)      # 离线检测开启

# SUPERCAP 默认参数
set(SUPERCAP_CAN            BSP_CAN_HANDLE2) # CAN 句柄
set(SUPERCAP_OFFLINE_ENABLE 0)      # 离线检测开启


set(VISION_OFFLINE_ENABLE    0)      # 离线检测开启
