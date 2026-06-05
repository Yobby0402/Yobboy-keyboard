# Yobboy Keyboard Firmware

基于 ESP32-S3 的自定义键盘固件，包含：

- USB HID 键盘
- USB CDC 调试 / 配置通道
- WebSerial 网页配置器
- 灯光、键位、功耗配置持久化到 NVS
- Fn 双层映射
- SOCD
- 反向补键（Reverse Tap Assist）

仓库里同时包含固件代码和静态网页配置器，网页可以本地运行，也可以部署到 GitHub Pages。

## 当前能力

### 固件

- ESP32-S3 USB HID 键盘
- 组合 USB 设备（HID + CDC）
- HC165 按键扫描
- Windows Dynamic Lighting / Lamp Array
- 多灯光预设、预览、切换
- 键盘名称、布局元数据、键位映射写入 NVS
- 功耗档位与扫描参数配置
- SOCD（WASD）
- 反向补键（WASD）

### 网页配置器

- WebSerial 连接键盘配置 CDC
- 读取 / 编辑 / 保存 profile
- 中英文切换
- KLE 布局导入
- 键位编辑、Fn 层编辑、键盘选键器
- 灯光配置编辑与设备预览
- 运行状态查看
- 变更列表

## 目录说明

```text
main/                  固件主代码
main/include/          固件头文件
main/lamp_array/       灯光协议与灯效实现
web-config/            静态网页配置器
tools/serve_configurator.py
                       本地启动网页配置器的轻量服务器
KEYBOARD_LAYOUT.md     键盘布局说明
DMA_NVS_CONFIG_GUIDE.md
                       DMA / NVS 配置说明
WEB_SERIAL_CONFIG_PROTOCOL.md
                       配置协议说明
```

## 固件开发

### 环境

- ESP-IDF 5.4 或更新版本
- VS Code + ESP-IDF 插件（推荐）

### 编译与烧录

```powershell
idf.py build
idf.py -p COM3 flash
idf.py -p COM3 monitor
```

如果硬件上保留了 `BOOT` 和 `RST`，需要时可手动进入下载模式再烧录。

## 本地运行网页配置器

配置器是纯静态页面，不依赖前端构建工具。

```powershell
python tools/serve_configurator.py
```

默认会在本机启动一个 `http://127.0.0.1:8765/` 页面。

建议使用支持 WebSerial 的浏览器：

- Microsoft Edge
- Google Chrome

## 网页配置器使用说明

### 连接方式

1. 将键盘通过 USB 连接电脑
2. 打开网页配置器
3. 点击“连接”
4. 选择键盘暴露出来的配置 CDC 端口
5. 点击“读取”加载当前配置

### 数据保存

- 网页侧修改后点击“保存”
- 配置写入设备 NVS
- 键盘重启后仍然保留

### 关于 SOCD 和反向补键

这两个功能现在都只针对 `W/A/S/D` 生效。

#### SOCD

SOCD 用于处理“相反方向同时按下”时的输出冲突。

例子：

- 按住 `A`
- 再按 `D`
- 键盘会按设置的延时后切到 `D`

适合需要稳定处理对冲输入的场景。

#### 反向补键

反向补键用于“松开方向键后自动补一个反方向短按”。

例子：

- 按住 `W`
- 松开 `W`
- 等待设定延时
- 自动短按一次 `S`

现在支持三个参数：

- 是否启用
- 补键延迟
- 补键时长
- 随机模式（对延迟和时长都生效）

适合 FPS 场景下的急停 / counter-strafe 需求。

## GitHub Pages 部署

网页配置器位于 `web-config/`，已经可以作为静态站点发布到 GitHub Pages。

仓库里已加入 GitHub Actions 工作流：

- `.github/workflows/deploy-pages.yml`

它会在推送到 `master` 或 `main` 时，自动把 `web-config/` 发布到 GitHub Pages。

### 首次部署步骤

1. 把仓库推送到 GitHub
2. 打开 GitHub 仓库页面
3. 进入 `Settings -> Pages`
4. 在 `Build and deployment` 里把 `Source` 设为 `GitHub Actions`
5. 回到仓库首页，推送一次包含工作流的提交
6. 等待 `Actions` 里的 `Deploy Configurator to GitHub Pages` 跑完

部署成功后，页面地址通常是：

```text
https://<你的用户名>.github.io/Yobboy-keyboard/
```

如果仓库名发生变化，末尾路径也会跟着变化。

### 注意事项

- GitHub Pages 是 HTTPS，适合 WebSerial 使用
- 如果是公司电脑或受限浏览器策略，WebSerial 可能被禁用
- 网页配置器负责“配置”和“预览”，不负责固件烧录
- 固件烧录仍建议通过 ESP-IDF / esptool，并在需要时手动按 `BOOT` / `RST`

## 推荐工作流

### 固件开发

```text
修改固件 -> 编译 -> 烧录 -> 用网页配置器验证
```

### 配置器开发

```text
修改 web-config/ -> 本地运行 tools/serve_configurator.py -> 浏览器验证
```

### 发布配置器

```text
提交 web-config/ 和 .github/workflows/deploy-pages.yml
-> 推送到 GitHub
-> GitHub Actions 自动部署到 Pages
```

## 相关文档

- [KEYBOARD_LAYOUT.md](KEYBOARD_LAYOUT.md)
- [DMA_NVS_CONFIG_GUIDE.md](DMA_NVS_CONFIG_GUIDE.md)
- [WEB_SERIAL_CONFIG_PROTOCOL.md](WEB_SERIAL_CONFIG_PROTOCOL.md)

## 备注

当前仓库里如果存在大量 `managed_components/` 变更，建议先确认这些改动是否需要一并提交；如果只是依赖同步噪音，最好单独清理后再推送，避免把无关内容带上去。
