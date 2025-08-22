<h1 align=center>WowLan</h1>
<div align=center>简易的 WakeOnLan CLI 命令行工具</div>

## 使用方法
1. 从 [Github Release](https://github.com/KirsminX/wowlan/releases) 下载最新发行版；
2. 运行 WowLan：
```bash
./wowlan <MAC地址> <子网CIDR地址>
# 示例 ./wowlan 8C:FC:B5:FF:2C:27 192.168.31.0/24
```
3. 查看你的设备是否被唤醒，如果是，则测试成功。可以将其写入 crontab 工具定时唤醒（Linux），或者在任务计划程序添加（Windows）。
## 编译
虽然 Release 只有 Windows x32 x64、Linux ARM64 AMD64 MT7621.MUSL 版本，理论上此项目支持所有架构。

Rust 版本源码：[src/main.rs](https://github.com/KirsminX/wowlan/blob/main/src/main.rs)

C 版本源码：[src/main.c](https://github.com/KirsminX/wowlan/blob/main/src/main.c)

选择合适的编译器编译即可

## 其他

编写此项目起初只是为了使用红米 AC2100 唤醒服务器，但是 MT7621 架构难以使用 Rust 编译，所以改用 C 版本。在其他平台推荐使用 Rust 版本。

由于此项目只是 CLI 程序，没必要支持配置文件，输出和错误处理也比较简单。
