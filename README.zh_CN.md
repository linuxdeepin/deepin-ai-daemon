# deepin-ai-daemon

deepin-ai-daemon 是由深度公司自主研发的 AI 搜索应用服务，包含自然语言的解析和本地文件索引。

### 依赖

### 构建依赖

_当前的开发分支为**master**，编译依赖可能会在没有更新本说明的情况下发生变化，请参考`./debian/control`以获取构建依赖项列表_

  * pkg-config
*  qt5-qmake
*  qtbase5-dev
*  libdtkwidget-dev
*  libdtkcore-dev
*  libdtkgui-dev
*  libdtkocr-dev
*  libdtkcore5-bin,
*  cmake
*  libtag1-dev
*  liblucene++-dev
*  libnl-genl-3-dev
*  libicu-dev
*  libboost-filesystem-dev

## 安装

### 构建过程

1. 确保已经安装所有依赖库。

   _不同发行版的软件包名称可能不同，如果您的发行版提供了deepin-ai-daemon，请检查发行版提供的打包脚本。_

如果你使用的是[Deepin](https://distrowatch.com/table.php?distribution=deepin)或其他提供了文件管理器的基于Debian的发行版：

``` shell
$ git clone https://github.com/linuxdeepin/deepin-ai-daemon
$ cd deepin-ai-daemon
$ sudo apt build-dep ./
```

2. 构建:
```shell
$ cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
$ cmake --build build
```

3. 安装:
```shell
$ sudo cmake --build build --target install
```

可执行程序为 `/usr/bin/deepin-ai-daemon`

## 使用

执行 `deepin-ai-daemon`

## 文档

- 待补充

## 帮助

- [官方论坛](https://bbs.deepin.org/) 
- [开发者中心](https://github.com/linuxdeepin/developer-center) 
- [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
- [聊天室](https://webchat.freenode.net/?channels=deepin)
- [Wiki](https://wiki.deepin.org/)

## 贡献指南

我们鼓励您报告问题并做出更改

- [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en) (English)

- [开发者代码贡献指南](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers) (中文)
- [在Transife上翻译您的语言](https://www.transifex.com/linuxdeepin/deepin-file-manager/)

## 开源许可证

deepin-ai-daemon 在 [GPL-3.0-or-later](LICENSE.txt)下发布。
