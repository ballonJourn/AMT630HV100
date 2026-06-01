# CP(CarPlay) 按键操作逻辑说明文档

## 1. 概述

本项目为嵌入式仪表工程，基于 FreeRTOS + AWTK，手机互联支持亿连(EC)和CP(CarPlay)。CP模式下无触摸屏，所有交互通过物理按键完成，按键事件经中间层转换为 CarPlay 旋钮协议（sendKnobInfo）发送给手机端。

## 2. 按键数据流

```
物理按键(GPIO/ADC)
    ↓
硬件按键扫描线程 (gpio_key_thread / keypad_thread)
    ↓ 防抖 + 长按/短按判定
send_key_event() [hcn_key_common.c]
    ↓ 当前代码：无条件调用
send_keyevent_to_cp() [hcn_cp_keyevent.c]
    ↓ 按键映射
carlink_send_key_event(key_code, pressed) [carlink_common.c]
    ↓ 封装为 CARLINK_EVENT_KEY_EVENT 事件
carlink_event_queue (FreeRTOS消息队列)
    ↓ carlink_event_proc 线程分发
onEventCarplay() → carlink_cp_input_event_proc() [carlink_cp.c]
    ↓
sendKnobInfo() / carplay_send_change_modes() → 发送给iPhone
```

## 3. 硬件按键输入

### 3.1 GPIO按键

文件：`hcn/mw/key_module/hcn_gpio_key.c`

| GPIO引脚 | 按键名称 | 宏定义 |
|----------|---------|--------|
| KEY_UP_GPIO (GPIO1) | UP键 | UP_KEY |
| KEY_MODE_GPIO (GPIO5) | DOWN/MODE键 | DOWN_KEY |
| KEY_SET_GPIO (GPIO7) | ENTER/SET键 | ENTER_KEY |
| KEY_BACK_GPIO (GPIO4) | BACK键 | BACK_KEY |

按键检测参数：
- 扫描周期：20ms
- 消抖时间：60ms（3个扫描周期）
- 长按判定：2500ms（125个扫描周期）
- 超长按判定：8000ms（仅ENTER键，400个扫描周期）

### 3.2 ADC按键

文件：`hcn/mw/key_module/hcn_adc_key.c`

| ADC值 | 按键名称 |
|-------|---------|
| 17 | ADC_KEY_UP |
| 18 | ADC_KEY_DOWN |
| 19 | ADC_KEY_RIGHT |
| 20 | ADC_KEY_LEFT |
| 27 | ADC_KEY_ESC |
| 10 | ADC_KEY_ENTER |
| 2  | ADC_KEY_HOME |
| 4  | ADC_KEY_BACK |
| 5  | ADC_KEY_SET |
| 6  | ADC_KEY_MODE |

长按判定：2000ms

## 4. 按键事件枚举

文件：`hcn/mw/key_module/hcn_key_common.h`

| 枚举值 | 数值 | 说明 |
|--------|------|------|
| MODE_KEY_SHORT_PR | 0x01 | MODE键短按 |
| MODE_KEY_LONG_PR | 0x02 | MODE键长按 |
| SET_KEY_SHORT_PR | 0x03 | SET键短按 |
| SET_KEY_LONG_PR | 0x04 | SET键长按 |
| COM_KEY_SHORT_PR | 0x05 | COM键短按 |
| COM_KEY_LONG_PR | 0x06 | COM键长按 |
| UP_KEY_SHORT_PR | 0x07 | UP键短按 |
| UP_KEY_LONG_PR | 0x08 | UP键长按 |
| DOWN_KEY_SHORT_PR | 0x09 | DOWN键短按 |
| DOWN_KEY_LONG_PR | 0x0A | DOWN键长按 |
| ENTER_KEY_SHORT_PR | 0x10 | ENTER键短按 |
| ENTER_KEY_LONG_PR | 0x11 | ENTER键长按 |
| BACK_KEY_SHORT_PR | 0x12 | BACK键短按 |
| BACK_KEY_LONG_PR | 0x13 | BACK键长按 |
| COM_KEY_SHORT_PR1 | 0x14 | COM键1短按 |
| COM_KEY_LONG_PR1 | 0x15 | COM键1长按 |
| SET_KEY_SUPER_LONG_PR | 0x16 | SET键超长按(8s) |

## 5. 物理按键 → 按键事件映射

### 5.1 GPIO按键映射

| GPIO按键 | 短按事件 | 长按事件 | 超长按事件 |
|----------|---------|---------|-----------|
| UP键 | UP_KEY_SHORT_PR | UP_KEY_LONG_PR | - |
| DOWN/MODE键 | MODE_KEY_SHORT_PR | MODE_KEY_LONG_PR | - |
| ENTER/SET键 | SET_KEY_SHORT_PR | SET_KEY_LONG_PR | SET_KEY_SUPER_LONG_PR |
| BACK键 | BACK_KEY_SHORT_PR | BACK_KEY_LONG_PR | - |

### 5.2 ADC按键映射

| ADC按键 | 短按事件 | 长按事件 |
|---------|---------|---------|
| ADC_KEY_UP | UP_KEY_SHORT_PR | UP_KEY_LONG_PR |
| ADC_KEY_DOWN | MODE_KEY_SHORT_PR | MODE_KEY_LONG_PR |
| ADC_KEY_ESC | SET_KEY_SHORT_PR | SET_KEY_LONG_PR |
| ADC_KEY_LEFT | BACK_KEY_SHORT_PR | BACK_KEY_LONG_PR |

## 6. 按键事件 → CP按键码映射

文件：`hcn/mw/key_module/hcn_cp_keyevent.c`

| 按键事件 | CP按键码(key) | 按下状态(pressed) | 说明 |
|---------|--------------|-------------------|------|
| MODE_KEY_SHORT_PR | 17 | true | 下一曲(Next) |
| MODE_KEY_LONG_PR | 27 | false | 确认键释放(Enter Release) |
| SET_KEY_SHORT_PR | 27 | true | 确认键按下(Enter Press) |
| SET_KEY_LONG_PR | 20 | true | 回前台/恢复视频流(Untake) |
| BACK_KEY_SHORT_PR | 19 | true | 退后台/暂停视频流(Take) |
| BACK_KEY_LONG_PR | - | - | **未处理** |
| UP_KEY_SHORT_PR | 18 | true | 上一曲(Previous) |
| UP_KEY_LONG_PR | - | - | **未处理** |

## 7. CP按键码 → CarPlay协议动作

文件：`carlink/CP/src/carlink_cp.c` 中的 `carlink_cp_input_event_proc()`

| CP按键码 | pressed | 执行函数 | 功能说明 |
|---------|---------|---------|---------|
| 17 | true | sendKnobInfo(0,0,0,0,0,-1) | 旋钮逆时针旋转 - 下一曲(Next) |
| 18 | true | sendKnobInfo(0,0,0,0,0,1) | 旋钮顺时针旋转 - 上一曲(Previous) |
| 19 | true | carplay_send_change_modes(Take, ...) | CarPlay退到后台，手机停止发送视频流 |
| 20 | true | carplay_send_change_modes(Untake, ...) + request_UI() | CarPlay回到前台，手机重新发送视频流 |
| 27 | true | sendKnobInfo(1,0,0,0,0,0) | 旋钮按下(Enter Press) |
| 27 | false | sendKnobInfo(0,0,0,0,0,0) | 旋钮释放(Enter Release) |

### sendKnobInfo 参数说明

```c
sendKnobInfo(pressed, clockwise, counterclockwise, delta_x, delta_y, rotate)
```

| 调用场景 | pressed | rotate | 含义 |
|---------|---------|--------|------|
| 确认键按下(key=27, pressed=true) | 1 | 0 | 旋钮按下 |
| 确认键释放(key=27, pressed=false) | 0 | 0 | 旋钮释放 |
| 下一曲(key=17, pressed=true) | 0 | -1 | 旋钮逆时针旋转1格 |
| 上一曲(key=18, pressed=true) | 0 | 1 | 旋钮顺时针旋转1格 |

## 8. 端到端按键映射总表

| 物理按键 | 操作 | 按键事件 | CP按键码 | CarPlay动作 |
|---------|------|---------|---------|------------|
| MODE/DOWN | 短按 | MODE_KEY_SHORT_PR | key=17, pressed=true | 下一曲(Next) |
| MODE/DOWN | 长按 | MODE_KEY_LONG_PR | key=27, pressed=false | 确认键释放 |
| SET/ENTER | 短按 | SET_KEY_SHORT_PR | key=27, pressed=true | 确认键按下(Enter) |
| SET/ENTER | 长按 | SET_KEY_LONG_PR | key=20, pressed=true | CarPlay回前台(Untake) |
| SET/ENTER | 超长按 | SET_KEY_SUPER_LONG_PR | - | **未映射到CP** |
| BACK | 短按 | BACK_KEY_SHORT_PR | key=19, pressed=true | CarPlay退后台(Take) |
| BACK | 长按 | BACK_KEY_LONG_PR | - | **未映射到CP** |
| UP | 短按 | UP_KEY_SHORT_PR | key=18, pressed=true | 上一曲(Previous) |
| UP | 长按 | UP_KEY_LONG_PR | - | **未映射到CP** |

## 9. 当前代码问题与注意事项

### 9.1 按键事件分发逻辑被短路

文件：`hcn/mw/key_module/hcn_key_common.c` 第33-44行

```c
void send_key_event(uint8_t key_event) {
    send_keyevent_to_cp(key_event);  // 无条件发送给CP
    return;                          // 直接返回，后续UI回调代码不执行
    // 以下代码不可达：
    if (vehicle_get_data(VEH_CARLINK_CP_STATUS)
            && vehicle_get_data(VEH_CARLINK_CONNECTED) == 0) {
         printf("send cp key event directly, event: %d\r\n", key_event);
    } else {
        if (key_event_cb) {
            key_event_cb(key_event);  // UI层回调，当前永远不会被调用
        }
    }
}
```

**影响**：当前所有按键事件都无条件发送给CP，UI层（AWTK）无法接收到按键事件，导致CP连接时本地UI无法响应按键。

**原始设计意图**：
- CP已连接 且 carlink未连接（无投屏）→ 按键直接发CP
- 其他情况 → 按键发给UI层回调

### 9.2 未映射的按键事件

以下按键事件在 `send_keyevent_to_cp()` 中未处理（走default分支）：
- BACK_KEY_LONG_PR（BACK长按）
- UP_KEY_LONG_PR（UP长按）
- SET_KEY_SUPER_LONG_PR（SET超长按8s）

### 9.3 CP连接状态变量

| 变量 | 含义 |
|------|------|
| VEH_CARLINK_CP_STATUS | CarPlay连接状态：0=未连接，1=已连接 |
| VEH_CARLINK_CONNECTED | Carlink投屏状态：0=未投屏，1=投屏中 |
| VEH_CARLINK_AA_STATUS | Android Auto连接状态 |

CP连接时序：
1. `CARLINK_EVENT_BT_IAP_READY` → 设置 `VEH_CARLINK_CP_STATUS=1`
2. `CARLINK_EVENT_MSG_SESSION_CONNECT` → 设置 `VEH_CARLINK_CP_STATUS=1`，`VEH_CARLINK_CONNECTED=1`
3. `CARLINK_EVENT_MSG_SESSION_STOP` → 设置 `VEH_CARLINK_CP_STATUS=0`
4. `CARLINK_EVENT_BT_DISCONNECT` → 设置 `VEH_CARLINK_CP_STATUS=0`

### 9.4 CarPlay旋钮模式配置

文件：`carlink/CP/src/carlink_cp.c` 第567行

```c
g_link_info->has_knob = HAS_KNOB;    // 是否有旋钮
g_link_info->HiFiTouch = 1;          // 高精度触摸=1（有触摸屏能力声明）
g_link_info->LoFiTouch = 0;          // 低精度触摸=0
```

CP声明有旋钮，使用 `sendKnobInfo` 协议与iPhone交互。虽然声明了HiFiTouch=1（触摸屏），但当前CP模式实际通过旋钮协议操作。

## 10. 亿连(EC)与CP对比

| 对比项 | 亿连(EC) | CP(CarPlay) |
|-------|---------|-------------|
| 触摸支持 | 有触摸屏 | 无触摸屏 |
| 按键交互 | 按键发UI层回调 | 按键转CarPlay旋钮协议 |
| 按键映射 | 通用按键事件 | 映射为key code(17/18/19/20/27) |
| 通信协议 | ECTiny SDK | CarPlay协议(sendKnobInfo) |
| 连接状态 | VEH_CARLINK_CONNECTED | VEH_CARLINK_CP_STATUS |
| 后台/前台切换 | N/A | change_modes(Take/Untake) |

## 11. 文件索引

| 文件路径 | 说明 |
|---------|------|
| `hcn/mw/key_module/hcn_gpio_key.c` | GPIO按键扫描与事件产生 |
| `hcn/mw/key_module/hcn_gpio_key.h` | GPIO按键引脚定义 |
| `hcn/mw/key_module/hcn_adc_key.c` | ADC按键扫描与事件产生 |
| `hcn/mw/key_module/hcn_adc_key.h` | ADC按键类型定义 |
| `hcn/mw/key_module/hcn_key_common.c` | 按键事件分发（核心调度） |
| `hcn/mw/key_module/hcn_key_common.h` | 按键事件枚举定义 |
| `hcn/mw/key_module/hcn_cp_keyevent.c` | 按键事件→CP按键码映射 |
| `hcn/mw/key_module/hcn_cp_keyevent.h` | CP按键事件接口 |
| `carlink/common/carlink_common.c` | CarLink事件队列与分发 |
| `carlink/common/carlink_common.h` | CarLink事件类型定义 |
| `carlink/CP/src/carlink_cp.c` | CP连接管理、按键处理、CarPlay协议交互 |
| `hcn/mw/vehicle_param/vehicle_param.h` | 车辆参数定义(含CP状态) |
| `hcn/ui/HCN_DC001/src/view/view_manager.c` | UI层按键处理与分发 |
| `hcn/ui/HCN_DC001/src/view/link_view/link_page_key.c` | 互联页面按键处理 |
| `hcn/ui/HCN_DC001/src/view/home_view/home_page_key.c` | 主页按键处理 |
| `hcn/ui/HCN_DC001/src/view/home_view/music_page_key.c` | 音乐页按键处理 |
| `hcn/ui/HCN_DC001/src/view/set_view/set_page_key.c` | 设置页按键处理 |
