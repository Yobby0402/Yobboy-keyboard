# 使用说明
* 固件在build文件夹中，使用配置好的vscode可以烧录， 也可以使用乐鑫的flashdownloadtool烧录
* managed_components是IDF组件库中的组件，不必修改
* 这个项目基于乐鑫官方tusb_hid的例程
## main文件夹
* [main](main/tusb_hid_example_main.c)是主程序文件
* [led.c](main/led.c)是led灯的函数，里面目前只实现了一种灯光
* [key_bind.c](main/key_bind.c)是按键函数，用于实现按键的发送与绑定
* [hc165.c](main/hc165.c)是按键扫描函数，用于扫描哪些按键被按下
* [Kconfig](main/Kconfig)用于配置项目，可以在里面定义按键映射

### 自定义基本按键说明
对于单个按键触发的按键，也就是只用按一个按键就能触发，例如字母键等等的添加，过程为：
1. 首先在Kconfig中添加配置，在菜单"General key number define"中添加，例如:

        config KEY_Q_NUM
            int "Key number of the Q button"
            default 63
            range 1 79

这是添加键盘的Q按键，并将其绑定到HC165扫描的63号按键（与PCB位号相同）。

2. 在key_bind.c文件中的基本按键映射表basic_key_mappings中添加：

        {CONFIG_KEY_Q_NUM, HID_KEY_Q},

其中第一个是在sdkconfig.h文件中你的配置选项，名字就是config选项前面加CONFIG，第二个HID_KEY_Q是tinyusb库中的[hid.h](managed_components/espressif__tinyusb/src/class/hid/hid.h)文件中找到HID KEYCODE（354行）

### 自定义键盘修饰符
与自定义基本按键相同，但是映射在modifier_key_mappings，此外第二个只能添加hid.h文件中的hid_keyboard_modifier_bm_t类型的按键，在文件的第330行，如果要去掉某个按键，将键值定义为0即可

###自定义多层按键
1. 目前的多层按键只能按住Fn触发，你可以更换Fn按键的位置，但注意不要有基本按键和Fn按键冲突
2. 在Kconfig文件的"Function key number"菜单中添加多层按键的定义如下：

        config KEY_HOME_NUM
            int "Key number of the HOME button.(When pressed Fn)"
            default 21
            range 0 79

这是将HOME按键添加到21号按键，当你按住Fn再按21号按键，则将转为发送HOME按键的键码。
3. 在key_bind.c文件的double_layer_keys中添加对应的键值：

        {CONFIG_KEY_HOME_NUM, HID_KEY_HOME},

第一个是添加的键值，第二个是hid.h中对应的键码。

## 注意，修改完Kconfig文件后请注意执行clean命令，重启编辑器后再执行menuconfig，最后再编译，否则改动可能不生效，或者在执行完menuconfig命令之后检查sdkconfig中相应的配置是否变动



