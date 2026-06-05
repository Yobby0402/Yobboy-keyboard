# Yobboy Keyboard 固件

[English](./README.md)

这是一个基于 ESP32-S3 的开源自定义键盘项目，仓库同时包含：

- 键盘固件
- USB CDC 配置 / 调试通道
- WebSerial 网页配置器
- NVS 持久化配置
- Fn 双层键位
- WASD SOCD
- WASD 反向补键

如果你是第一次进入这个仓库，建议先看这份文档，再按需阅读下面的细分说明。

## 项目特性

### 固件能力

- ESP32-S3 USB 组合设备
- HID 键盘 + CDC 配置通道
- HC165 按键扫描
- 键位、灯光、功耗配置写入 NVS
- 多灯光预设
- Windows Dynamic Lighting / Lamp Array
- 可配置扫描与功耗档位
- WASD SOCD
- WASD 反向补键

### 网页配置器能力

- 浏览器通过 WebSerial 连接设备 CDC
- 读取、编辑、保存配置
- 中英文界面切换
- KLE 布局导入
- Base / Fn 层键位编辑
- 灯光预设编辑与设备预览
- 运行状态查看
- 保存前变更列表

## 仓库结构

```text
main/                              固件主代码
main/include/                      固件头文件
main/lamp_array/                   灯光与 Lamp Array 实现
web-config/                        静态网页配置器
tools/serve_configurator.py        本地启动配置器的轻量服务器
KEYBOARD_LAYOUT.md                 键盘布局说明
DMA_NVS_CONFIG_GUIDE.md            DMA / NVS 配置说明
WEB_SERIAL_CONFIG_PROTOCOL.md      配置协议说明
```

## 快速开始

### 固件开发

推荐环境：

- ESP-IDF 5.4 或更新版本
- VS Code + ESP-IDF 插件

编译与烧录：

```powershell
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

如果硬件上保留了 `BOOT` 和 `RST`，需要时可以手动进入下载模式。

### 本地运行网页配置器

网页配置器是纯静态页面，不依赖前端构建工具。

```powershell
python tools/serve_configurator.py
```

然后打开：

```text
http://127.0.0.1:8765/
```

推荐浏览器：

- Microsoft Edge
- Google Chrome

## 网页配置器的基本使用流程

1. 通过 USB 连接键盘
2. 打开网页配置器
3. 点击“连接”
4. 选择键盘暴露出来的配置 CDC 端口
5. 点击“读取”加载当前配置
6. 修改键位、灯光或功耗设置
7. 点击“保存”写入 NVS

配置保存后，设备重启仍然会保留。

## SOCD 与反向补键

这两个功能当前都只针对 `W / A / S / D` 生效。

### SOCD

SOCD 用于处理“相反方向同时按下”时的输出冲突。

示例：

- 按住 `A`
- 再按 `D`
- 键盘会按设定的延时和规则处理最终输出

适合需要稳定处理对冲输入的场景。

### 反向补键

反向补键用于在你松开方向键后，自动补一个很短的反方向输入。

示例：

- 按住 `W`
- 松开 `W`
- 等待设定的补键延迟
- 自动短按一次 `S`

当前可配置参数：

- 启用 / 禁用
- 补键延迟
- 补键时长
- 随机时序

适合 FPS 中的急停、counter-strafe 一类需求。

## GitHub Pages 部署

网页配置器位于 `web-config/`，可以直接部署到 GitHub Pages。

仓库已经包含部署工作流：

- `.github/workflows/deploy-pages.yml`

当你推送到 `master` 或 `main` 时，它会自动把 `web-config/` 发布到 Pages。

### 首次启用步骤

1. 把仓库推送到 GitHub
2. 打开 `Settings -> Pages`
3. 将 `Source` 设置为 `GitHub Actions`
4. 推送一个包含工作流的提交
5. 在 `Actions` 页面等待 `Deploy Configurator to GitHub Pages` 完成

部署成功后，站点地址通常是：

```text
https://<你的用户名>.github.io/Yobboy-keyboard/
```

## 说明

- GitHub Pages 使用 HTTPS，适合 WebSerial 使用
- 网页配置器负责配置与预览，不负责固件烧录
- 固件烧录仍建议使用 ESP-IDF / esptool
- 如果工作区里有大量无关改动，推送前建议先检查一遍

## 相关文档

- [English README](./README.md)
- [键盘布局说明](./KEYBOARD_LAYOUT.md)
- [DMA / NVS 配置说明](./DMA_NVS_CONFIG_GUIDE.md)
- [WebSerial 配置协议](./WEB_SERIAL_CONFIG_PROTOCOL.md)
