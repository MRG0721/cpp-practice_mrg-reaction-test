# cpp-practice_mrg-reaction-test

一个用 **C++ 和 Win32 API** 编写的“反应速度测试”小程序，主题为：

> C++学习——简单的 Win32 程序与 GDI 绘制

程序启动后显示一个蓝色窗口，点击即可开始测试：屏幕先变绿并随机等待 0.5～2 秒，
之后突然变红，玩家要尽快点击鼠标。程序会记录你 5 轮的反应时间，并显示平均值。
整个项目只依赖 Windows 系统自带的 API，不需要任何第三方库。

## 功能一览

- 完整的 Win32 窗口生命周期：注册窗口类、创建窗口、消息循环、窗口过程。
- 用简单的状态机控制游戏流程：等待开始 → 等待变红 → 红色计时 → 显示结果。
- 用 `SetTimer` / `WM_TIMER` 实现随机延迟后“变红”。
- 用 `GetTickCount()` 精确到毫秒地测量反应时间。
- 用 GDI 完成全部绘制：`FillRect` 填充背景色、`CreateFont` + `DrawText` 居中绘制文字。
- 5 轮测试结束后自动计算并显示平均反应时间（单位：毫秒）。

## 运行环境

- 操作系统：Windows
- 编译器：Visual Studio（社区版即可），代码未使用任何额外的第三方库
- 说明：源码引用了 `resource.h` 与图标资源 `IDI_ICON1`。
  在 Visual Studio 里新建“Windows 桌面应用程序”项目后，
  把本文件内容替换进项目主源文件即可；如果项目中没有 `resource.h` / 图标资源，
  可以把 `#include "resource.h"` 以及 `LoadIcon` 两行删掉，程序仍可正常运行。

## 使用方法

1. 在 Visual Studio 中创建 Windows 桌面项目，并放入本源码。
2. 编译运行（或直接按 `F5`）。
3. 点击蓝色窗口开始测试。
4. 窗口变绿后耐心等待，变红的一瞬间立刻点击鼠标。
5. 完成 5 轮后，窗口会显示你的平均反应时间。

## 设计思路：状态机

程序用一个枚举类型描述所有游戏状态，这也是整个程序最核心的设计：

```cpp
enum AppState {
    STATE_WAITING_START, // 等待开始（蓝色，点击开始）
    STATE_WAITING_RED,   // 等待变红（绿色，请等待）
    STATE_RED,           // 红色，计时中
    STATE_FINISHED       // 测试结束，显示平均值
};
```

状态之间的切换都由窗口消息驱动：

| 当前状态 | 触发事件 | 动作 | 下一状态 |
| --- | --- | --- | --- |
| 等待开始（蓝） | 鼠标左键 `WM_LBUTTONDOWN` | 生成 0.5～2 秒随机延迟并启动定时器 | 等待变红（绿） |
| 等待变红（绿） | 定时器触发 `WM_TIMER` | 记录 `GetTickCount()` 作为起始时间 | 红色计时 |
| 红色计时 | 鼠标左键 | 计算本轮用时，轮数 +1 | 下一轮“等待开始”，满 5 轮则进入“结束” |

这种写法把“界面显示什么”和“程序处于什么阶段”彻底分开：
背景颜色由 `GetBackgroundColor()` 按状态决定，提示文字由 `GetDisplayText()` 按状态决定，
而点击和定时器只负责切换状态。理解这一点，对以后写更复杂的游戏或交互程序很有帮助。

## 代码结构解析

### 程序入口 WinMain

```cpp
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // 1. 初始化随机数种子
    srand((unsigned)time(NULL));

    // 2. 注册窗口类
    WNDCLASSEX wcex = { 0 };
    wcex.lpfnWndProc = WndProc;
    // ... 设置图标、光标、背景画刷等
    RegisterClassEx(&wcex);

    // 3. 创建窗口
    HWND hWnd = CreateWindow(_T("ReactionTestWindow"), _T("MRG 反应测试"), ...);
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // 4. 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
```

这是一个标准 Win32 程序的骨架，学习时建议记住“四步曲”：

1. 注册窗口类（告诉 Windows 这个窗口长什么样、消息由谁处理）；
2. 创建窗口并显示；
3. 进入消息循环，把消息分发给窗口过程；
4. 收到 `WM_DESTROY` 后调用 `PostQuitMessage` 退出循环。

`_T()` 宏配合 `tchar.h`，让代码在 Unicode 与多字节字符集下都能编译。

### 窗口过程 WndProc

`WndProc` 是程序的心脏，所有用户交互都通过消息传到这里：

- `WM_PAINT`：调用 `BeginPaint` / `EndPaint` 获取设备上下文（HDC），然后交给 `DrawApp` 绘制；
- `WM_LBUTTONDOWN`：按当前状态决定是“开始一轮”还是“记录成绩”；
- `WM_TIMER`：定时器到点，屏幕变红并开始计时；
- `WM_DESTROY`：窗口关闭时退出消息循环。

未处理的消息一律交给 `DefWindowProc`，这是 Win32 编程的基本规范。

### 随机延迟与计时

```cpp
int delay = rand() % (MAX_DELAY - MIN_DELAY + 1) + MIN_DELAY;
SetTimer(hWnd, TIMER_ID, delay, NULL);
```

`MIN_DELAY` 和 `MAX_DELAY` 是 500 与 2000，所以延迟范围是 0.5～2 秒，
这样可以防止玩家“凭节奏预测”变红时刻。定时器触发时用 `GetTickCount()`
记录系统启动以来的毫秒数，点击时再取一次，两者相减就是反应时间：

```cpp
DWORD elapsed = GetTickCount() - g_startTick;
```

这里使用 `GetTickCount()` 而不是 `time()`，因为它返回的是毫秒级精度，
足以满足反应测试的需要。

### GDI 绘制 DrawApp

`DrawApp` 是“GDI 绘制”这一主题的重点：

```cpp
// 填充背景
HBRUSH hBrush = CreateSolidBrush(GetBackgroundColor());
FillRect(hdc, &clientRect, hBrush);
DeleteObject(hBrush);

// 创建 32 像素高、加粗的微软雅黑字体
HFONT hFont = CreateFont(-32, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                         CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                         DEFAULT_PITCH | FF_DONTCARE, _T("微软雅黑"));
HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

// 居中绘制文字
SetBkMode(hdc, TRANSPARENT);            // 文字背景透明
SetTextColor(hdc, RGB(255, 255, 255));  // 白色文字
DrawText(hdc, text, -1, &clientRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

// 恢复旧字体并释放资源
SelectObject(hdc, hOldFont);
DeleteObject(hFont);
```

几个值得记住的 GDI 学习点：

- **HDC（设备上下文）**：GDI 不直接画在窗口上，而是画在窗口的 HDC 上，`WM_PAINT` 里必须用 `BeginPaint` / `EndPaint` 成对获取与释放；
- **对象与句柄**：`CreateSolidBrush`、`CreateFont` 创建的是系统资源，用完必须 `DeleteObject`，否则会造成资源泄漏；
- **SelectObject 的返回值**：每次选中新对象时返回旧对象句柄，结束前要把旧对象选回去，再删除自己创建的；
- **CreateFont 第一个参数是负值**：`-32` 表示“字符高度 32 像素”（正值则表示逻辑单位），这是最常用的调用方式；
- **DrawText 的格式化标志**：`DT_CENTER | DT_VCENTER | DT_SINGLELINE` 组合实现水平垂直居中。

### 状态与显示解耦

`GetBackgroundColor()` 和 `GetDisplayText()` 都只做一件事：读取 `g_state`，
返回对应的颜色或文字。例如：

```cpp
case STATE_RED: return RGB(255, 0, 0); // 红色
```

每轮结束时程序会把 5 次成绩求和取平均，用 `_stprintf_s` 格式化成
“平均反应时间: xxx ms”显示在窗口中。

## 全局数据说明

| 变量 | 作用 |
| --- | --- |
| `g_state` | 当前游戏状态 |
| `g_round` | 已完成的轮数（0～4） |
| `g_reactionTimes[5]` | 保存每一轮的反应时间 |
| `g_startTick` | 屏幕变红瞬间的系统 tick，用于计算耗时 |
| `TIMER_ID` | 定时器标识，用于区分不同定时器 |



```text
MRG的反应测试.cpp   本程序全部源码（约 200 行）
README.md           本文档
```
