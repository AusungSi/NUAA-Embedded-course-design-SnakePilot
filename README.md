# SnakePilot

SnakePilot 是一个基于 STM32F107VC 的嵌入式贪吃蛇交互系统。项目在游戏逻辑之外加入了关卡地图、旋钮交互、按键控制、生命值、限时挑战、实时状态显示、LED 方向提示以及基于 DAC/DMA/TIM 的音频反馈，适合作为嵌入式系统课程设计或开发板综合实验项目。

当前工程面向 NUAA_CM3_107 实验开发板，使用 STM32F10x 标准外设库 V3.5.0，提供 Keil MDK-ARM 工程和 VS Code EIDE 工程配置。


## 作者
丁俊泽 鲁贇涛 何力

## 使用说明与学术诚信声明

> [!IMPORTANT]
> 本仓库代码以 MIT License 开源。考虑到本项目对应的课程设计验收周期为 2026 年 5 月 20 日至 2026 年 6 月 10 日，在该时间段内，请南京航空航天大学相关课程同学不要将本项目代码、报告、界面设计或功能实现用于课程设计或答辩展示，请 **独立完成**，请 **独立完成**，请 **独立完成**，验收之前**不接受**任何二次开发行为。

上述说明主要用于维护课程设计期间的学术诚信和评价公平性。2026 年 6 月 10 日之后，欢迎在遵守 MIT License 的前提下学习、参考、修改和二次开发本项目，并建议在使用时注明来源。

## 项目概况

项目主体运行在 STM32F107VCT6 上，LCD 采用 240 x 320 竖屏显示方式。游戏区域被划分为 15 x 20 的网格，每个格子大小为 12 像素。程序启动后进入 SnakePilot 首页，用户可以通过 ADC 旋钮选择起始关卡，通过板载按键进入游戏；游戏过程中可以使用四个按键进行方向控制，也可以通过旋钮完成相对转向。系统会根据分数动态提高移动速度，并在不同事件发生时给出声音和 LED 反馈。

工程当前已经实现的核心能力包括：

- LCD 图形界面：显示首页、关卡选择、状态栏、游戏地图、分数、生命值、最高分和倒计时。
- 五关地图机制：基础地图、障碍地图、传送门地图、道具地图和限时地图。
- 游戏逻辑：蛇身队列、食物生成、碰撞检测、传送门跳转、关卡推进、生命值管理和重新开始。
- 多种食物：普通食物、毒食物、奖励食物，不同食物对应不同得分和生命值效果。
- 本地交互：GPIO 按键控制上下左右，ADC 旋钮用于首页选关、游戏内相对转向和设置页音量调节。
- 声光反馈：LED 指示当前移动方向，DAC 输出不同频率的音效和关卡背景旋律。
- 动态难度：蛇的移动间隔会随分数缩短，后期关卡进一步加快节奏。

需要说明的是，`Project/target.txt` 中曾提出 USART 键盘控制、Flash 最高分持久化等扩展方向，但当前源码中尚未接入 USART 控制和 Flash 写入。README 下面只把已经在代码中实现的功能作为当前版本功能描述。

## 硬件平台

| 项目 | 配置 |
| --- | --- |
| 开发板 | NUAA_CM3_107 实验开发板 |
| MCU | STM32F107VCT6 |
| 内核 | ARM Cortex-M3 |
| Flash 配置 | IROM 起始地址 `0x08000000`，大小 `0x40000` |
| SRAM 配置 | IRAM 起始地址 `0x20000000`，大小 `0x10000` |
| LCD | 3.2 寸 TFT LCD，16 位并行 IO 驱动，竖屏 240 x 320 |
| 调试/下载 | ST-Link，SWD 接口 |
| 固件库 | STM32F10x Standard Peripheral Library V3.5.0 |
| 工具链 | ARMCC5 / Keil MDK-ARM |

## 外设与引脚分配

### 按键

游戏使用四个低电平有效的 GPIO 输入作为方向键。

| 功能 | GPIO |
| --- | --- |
| 上 | PD11 |
| 下 | PD12 |
| 左 | PC13 |
| 右 | PA0 |
| 暂停/继续 | 同时按下 PD11 与 PD12 |

在首页中，旋钮用于选择起始关卡，任意方向键开始游戏。右方向键还承担进入设置页的功能，设置页中可通过旋钮调整音量。

### LED

LED 用于显示蛇当前移动方向。程序中 `LED1(0)`、`LED2(0)` 等表示点亮，`LEDx(1)` 表示熄灭。

| LED | GPIO | 游戏含义 |
| --- | --- | --- |
| LED1 | PD2 | 当前方向为上 |
| LED2 | PD3 | 当前方向为下 |
| LED3 | PD4 | 当前方向为左 |
| LED4 | PD7 | 当前方向为右 |
| LED5 | PB7 | 硬件配置中保留，当前主逻辑未使用 |

### LCD

LCD 使用 16 位并行数据总线，主要连接如下：

| 信号 | GPIO |
| --- | --- |
| DB0 - DB15 | PE0 - PE15 |
| LCD_CS | PC6 |
| LCD_RS | PD13 |
| LCD_WR | PD14 |
| LCD_RD | PD15 |
| LCD_LED | PB14 |

`User/Lcd/lcd.c` 负责 LCD 初始化、清屏、窗口设置、画点、画线、矩形填充和字符串显示。游戏界面中的地图格子、首页卡片、状态栏和提示文字都建立在这些基础绘图函数之上。

### ADC 旋钮

| 功能 | 配置 |
| --- | --- |
| ADC 外设 | ADC1 |
| 通道 | ADC_Channel_3 |
| GPIO | PA3 |

旋钮在不同界面承担不同作用：

- 首页：将 0-4095 的 ADC 读数划分为 5 段，用于选择起始关卡。
- 游戏中：检测 ADC 值相对变化，转化为向左或向右的相对转向事件。
- 设置页：将 ADC 读数映射为 0-5 档音量。

### 音频输出

音频使用 `TIM3 + DAC Channel 2 + DMA2_Channel4` 输出 32 点正弦波。程序根据音符频率动态调整 TIM3 的自动重装载值，由 TIM3 更新事件触发 DAC 输出，DMA 以循环模式搬运波形表，从而形成不同音高的提示音和背景旋律。

| 功能 | 配置 |
| --- | --- |
| DAC 输出 | DAC Channel 2 |
| 输出引脚 | PA5 |
| 定时器触发 | TIM3 TRGO |
| DMA 通道 | DMA2_Channel4 |
| 波形表 | 32 点正弦波 |
| 音量范围 | 0-5 |

`PC0` 蜂鸣器 GPIO 在硬件配置中被初始化并在启动时拉低，但当前主要音效输出由 PA5 的 DAC 波形完成。

## 软件结构

仓库根目录位于 `C:\EIDE\SnakePilot`。`Project` 目录是 Keil/EIDE 工程目录，真正的业务源码主要在 `User` 目录中。

```text
SnakePilot
├── Doc
│   └── information.txt                  # 原实验说明，来源于 TIMx 定时器示例
├── Libraries
│   ├── CMSIS                            # Cortex-M3 和 STM32F10x CMSIS 支持
│   └── STM32F10x_StdPeriph_Driver_v3.5  # STM32F10x 标准外设库
├── Project
│   ├── Project.uvprojx                  # Keil MDK 工程
│   ├── Project.code-workspace           # VS Code 工作区
│   ├── .eide/eide.yml                   # EIDE 工程配置
│   ├── .vscode/tasks.json               # 构建、下载、清理任务
│   ├── .vscode/launch.json              # Cortex-Debug 调试配置
│   └── build                            # 构建产物目录
├── User
│   ├── Main
│   │   ├── main.c                       # SnakePilot 主程序和游戏逻辑
│   │   ├── hw_config.c/.h               # GPIO、LED、按键、蜂鸣器等基础硬件配置
│   │   ├── stm32f10x_it.c/.h            # 中断函数模板及定时器实验中断代码
│   │   ├── stm32f10x_conf.h             # 标准外设库头文件配置
│   │   └── DS18B20.c/.h                 # 温度传感器驱动，当前游戏主逻辑未接入
│   ├── Lcd
│   │   ├── lcd.c/.h                     # 当前游戏使用的 LCD 驱动
│   │   ├── font.h                       # 字库数据
│   │   ├── pic.h                        # 图片资源/显示相关头文件
│   │   └── NUAA107_32_Driver_IO16.*     # 开发板 LCD 示例驱动，当前工程未作为主显示入口
│   └── Timer
│       ├── Timer.c
│       └── Timer.h                      # 延时函数和课程定时器示例代码
├── LICENSE
└── README.md
```

## 主要源码说明

### `User/Main/main.c`

这是项目的核心文件，包含游戏配置、地图数据、音乐数据、输入处理、音频输出、LCD 绘制和主循环。

其中比较重要的配置项包括：

| 配置项 | 当前值 | 说明 |
| --- | --- | --- |
| `GRID_COLS` | 15 | 游戏地图列数 |
| `GRID_ROWS` | 20 | 游戏地图行数 |
| `CELL_SIZE` | 12 | 每个地图格子的像素尺寸 |
| `LEVEL_COUNT` | 5 | 关卡数量 |
| `MAX_SNAKE_LEN` | 300 | 蛇身最大长度，等于地图格子总数 |
| `VOLUME_MAX` | 5 | 最大音量档位 |
| `MUSIC_TICK_MS` | 5 | 背景音乐推进周期 |

程序使用两个数组 `snake_x[]` 和 `snake_y[]` 保存蛇身坐标，数组下标 0 表示蛇头。每次移动时先根据方向计算新蛇头坐标，再处理边界、障碍物、传送门、食物和自碰撞，最后整体移动蛇身并刷新 LCD。

主要函数职责如下：

| 函数 | 作用 |
| --- | --- |
| `Snake_ShowHome()` | 绘制首页、播放首页音乐、处理选关和进入设置页 |
| `Snake_ShowSettings()` | 设置页，使用旋钮调节音量 |
| `Snake_StartGame()` | 初始化分数、生命值并进入起始关卡 |
| `Snake_StartLevel()` | 初始化指定关卡的地图状态、时间、音乐、蛇身和食物 |
| `Snake_HandleInput()` | 扫描按键和旋钮，处理方向、暂停和暂停时切关 |
| `Snake_Step()` | 执行蛇的一步移动，是游戏规则判断的核心 |
| `Snake_PlaceFood()` | 在空白格中生成食物，并在道具关随机生成毒食物或奖励食物 |
| `Snake_ApplyFood()` | 根据食物类型更新分数、生命值和关卡进度 |
| `Snake_Render()` | 重绘状态栏、地图、食物和蛇身 |
| `Snake_AudioInit()` | 初始化 DAC、DMA、TIM3 和音频波形表 |
| `Snake_MusicTick()` | 推进背景音乐播放 |
| `Snake_StepDelay()` | 根据分数和关卡计算蛇的移动间隔 |

主循环位于 `main()` 中，整体流程为：

```text
系统初始化
  -> GPIO 初始化
  -> ADC 旋钮初始化
  -> DAC/DMA/TIM3 音频初始化
  -> LCD 初始化
  -> 显示首页并选择起始关卡
  -> 初始化游戏
  -> 循环等待下一步移动
       -> 处理输入、音乐、倒计时
       -> 执行移动
       -> 根据结果处理死亡、掉血、过关或通关
```

### `User/Main/hw_config.c/.h`

该模块完成基础 GPIO 初始化和硬件宏定义，主要包括 LED、按键、蜂鸣器、位带访问宏等。游戏入口 `main()` 调用了 `GPIO_Configuration()`，用于初始化 LED 输出、方向键输入和蜂鸣器 GPIO。

### `User/Lcd/lcd.c/.h`

LCD 驱动提供底层寄存器写入、GRAM 写入、窗口设置、清屏、区域填充、字符串显示和基本图形绘制。SnakePilot 的界面没有引入额外 GUI 框架，而是直接调用这些函数完成地图格子绘制和界面刷新。

### `User/Timer/Timer.c/.h`

该文件保留了课程实验中的定时器示例代码，并提供 `Delay_ms()` 延时函数。当前游戏节拍主要通过 `Snake_WaitStep()` 中的短延时循环推进；音频部分单独使用 TIM3 作为 DAC 触发源，不依赖 `TIM_Configuration()` 中的 TIM1/TIM2/TIM3 更新中断示例。

### `Libraries`

该目录包含 CMSIS 和 STM32F10x 标准外设库源码。项目编译时启用了 `USE_STDPERIPH_DRIVER` 和 `STM32F10X_CL` 宏，目标芯片为 STM32F107VC，属于 STM32F10x Connectivity Line。

## 游戏规则

### 关卡设计

| 关卡 | 名称 | 目标 | 特点 |
| --- | --- | --- | --- |
| 1 | BASIC | 吃到 3 个目标分 | 空地图，用于熟悉操作 |
| 2 | WALL | 吃到 4 个目标分 | 出现固定障碍物，撞墙或撞障碍会掉生命 |
| 3 | PORTAL | 吃到 4 个目标分 | 地图中存在 A/B 传送门，进入一个传送门会从另一个传送门出现 |
| 4 | ITEM | 吃到 6 个目标分 | 混合普通食物、毒食物和奖励食物 |
| 5 | TIME | 吃到 5 个目标分 | 45 秒限时挑战，超时会掉生命 |

关卡地图由 `level_map` 二维字符矩阵描述：

| 字符 | 含义 |
| --- | --- |
| `.` | 空白格 |
| `#` | 固定障碍物 |
| `A` | 传送门 A |
| `B` | 传送门 B |

### 食物与生命值

| 食物类型 | 显示颜色 | 效果 |
| --- | --- | --- |
| 普通食物 | 红色 | 分数 +1，关卡进度 +1，蛇身增长 |
| 毒食物 | 品红色 | 分数最多 -1，生命值 -1，蛇身不增长 |
| 奖励食物 | 青色 | 分数 +2，关卡进度 +2，生命值最多增加到 5 |

游戏初始生命值为 3。撞到边界、障碍物或自身时会损失 1 点生命；限时关倒计时归零也会损失生命。只要生命值没有归零，系统会在当前关卡重新放置蛇和食物后继续游戏。生命值为 0 时显示 `GAME OVER`，按任意键后从首页选择的起始关卡重新开始。

最高分变量 `high_score` 只保存在运行时内存中，断电或复位后会清零。

### 速度调节

蛇的移动间隔由 `Snake_StepDelay()` 计算。初始间隔约为 300 ms，分数越高间隔越短；第 4、5 关还会额外缩短移动间隔，最低约为 110 ms。这样可以在不引入复杂调度器的情况下形成逐步提升的游戏压力。

## 操作说明

### 首页

1. 转动旋钮选择起始关卡，首页下方会高亮当前选中的关卡。
2. 按任意方向键开始游戏。
3. 按右方向键进入设置页。

### 设置页

1. 转动旋钮调整音量，范围为 0-5。
2. 按任意方向键返回首页。

### 游戏中

| 操作 | 效果 |
| --- | --- |
| PD11 | 向上移动 |
| PD12 | 向下移动 |
| PC13 | 向左移动 |
| PA0 | 向右移动 |
| PD11 + PD12 | 暂停或继续 |
| 旋钮左转 | 相对当前方向左转 |
| 旋钮右转 | 相对当前方向右转 |
| 暂停时旋钮左/右转 | 切换关卡，保持暂停状态 |

按键扫描带有简单消抖处理，并通过 `turn_pending` 限制每个移动步长内只接受一次转向，避免连续按键导致方向抖动。

## 构建与下载

### 使用 Keil MDK

1. 安装 Keil MDK-ARM，建议使用 V5.15 或更高版本。
2. 安装 STM32F1xx Device Family Pack。工程文件中记录的 Pack 为 `Keil.STM32F1xx_DFP.2.2.0`，本机调试配置中使用过 `2.3.0` 的 SVD 文件，通常兼容 STM32F107VC 即可。
3. 打开 `Project/Project.uvprojx`。
4. 确认目标为 `TIMx应用-更新中断实验`，芯片为 `STM32F107VC`。
5. 编译工程，生成 AXF/HEX 文件。
6. 通过 ST-Link 连接开发板，下载程序并复位运行。

### 使用 VS Code + EIDE

1. 安装 VS Code。
2. 安装推荐插件：EIDE、Cortex-Debug、C/C++、Serial Monitor 等。
3. 打开 `Project/Project.code-workspace`。
4. 确认 `.vscode/settings.json` 中的工具链路径与本机一致，重点包括：
   - `EIDE.ARM.INI.Path`
   - `EIDE.ARM.ARMCC5.InstallDirectory`
   - `EIDE.STLink.ExePath`
   - `EIDE.OpenOCD.ExePath`
5. 在 VS Code 任务中运行 `build` 进行编译。
6. 使用 `flash` 或 `build and flash` 下载到开发板。

构建产物默认输出到：

```text
Project/build/Debug/Project.axf
Project/build/Debug/Project.hex
Project/build/Debug/Project.map
```

如果普通下载失败，可以使用 VS Code 任务中的 `flash recover stlink`，该任务会调用 STM32CubeProgrammer CLI，并使用 `mode=UR reset=HWrst` 方式尝试恢复下载。

## EIDE 工程配置摘要

`.eide/eide.yml` 中的主要配置如下：

| 配置项 | 当前值 |
| --- | --- |
| 工程类型 | ARM |
| 设备 | STM32F107VC |
| 工具链 | AC5 |
| CPU | Cortex-M3 |
| 浮点 | none |
| C 标准 | C99 |
| 优化等级 | level-0 |
| MicroLIB | 启用 |
| ROM Base | `0x08000000` |
| RAM Base | `0x20000000` |
| 下载器 | STLink |
| STLink 接口 | SWD |
| OpenOCD Target | `stm32f1x` |

工程包含的主要源文件包括：

- `../User/Main/main.c`
- `../User/Main/stm32f10x_it.c`
- `../User/Main/hw_config.c`
- `../User/Lcd/lcd.c`
- `../User/Timer/Timer.c`
- `../Libraries/CMSIS/CoreSupport/core_cm3.c`
- `../Libraries/CMSIS/DeviceSupport/system_stm32f10x.c`
- `../Libraries/CMSIS/DeviceSupport/Startup/startup_stm32f10x_cl.s`
- 标准外设库中的 GPIO、RCC、ADC、DAC、DMA、EXTI、TIM、misc 等模块

## 调试建议

- 如果 LCD 无显示，先检查 LCD 并行数据线和控制线是否与 `lcd.h` 中定义一致，再确认背光 PB14 是否拉高。
- 如果按键无响应，确认四个方向键是否为低电平有效，并检查 PD11、PD12、PC13、PA0 是否被其他模块占用。
- 如果旋钮选关异常，可以在 `Snake_ReadAdcChannel()` 附近临时显示 ADC 值，检查 PA3 输入是否在 0-4095 范围内变化。
- 如果没有声音，优先检查 PA5 是否接入音频输出电路，同时确认设置页音量不是 0。
- 如果 ST-Link 下载失败，可以降低 SWD 频率，或使用 `flash recover stlink` 任务进行硬复位下载。
- 如果 Keil 或 EIDE 报找不到头文件，检查包含路径是否包含 `Libraries/CMSIS/CoreSupport`、`Libraries/CMSIS/DeviceSupport`、`Libraries/STM32F10x_StdPeriph_Driver_V3.5/inc` 和 `User/Main`、`User/Lcd`。

## 可扩展方向

当前版本已经覆盖 GPIO、LCD、ADC、TIM、DAC、DMA 等多个课程实验点。后续如果继续完善，可以优先考虑以下方向：

- USART 控制：接收 PC 端 W/A/S/D、空格、R 等字符，实现串口键盘控制。
- Flash 记录：将最高分写入片内 Flash，复位后仍能读取历史记录。
- 更完整的设置页：增加起始生命值、难度、是否开启背景音乐等选项。
- 地图编辑：把 `level_map` 抽象为可配置数据，便于快速增加新关卡。
- 中断式节拍：将当前轮询式游戏节拍改为定时器中断标志位驱动，减少阻塞式延时对扩展功能的影响。

## 许可证

本项目使用 MIT License，详见 `LICENSE`。
