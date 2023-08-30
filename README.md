### deepin-ai-daemon

Deepin AI Daemon is a AI search application server independently developed by Deepin Technology, includes natural language parsing and indexing of local file properties.

### Dependencies

### Build dependencies

_The **master** branch is current development branch, build dependencies may changes without update README.md, refer to `./debian/control` for a working build depends list_

- pkg-config
-  qt5-qmake
-  qtbase5-dev
-  libdtkwidget-dev
-  libdtkcore-dev
-  libdtkgui-dev
-  libdtkocr-dev
-  libdtkcore5-bin,
-  cmake
-  libtag1-dev
-  liblucene++-dev
-  libnl-genl-3-dev
-  libicu-dev
-  libboost-filesystem-dev

## Installation

### Build from source code

1. Make sure you have installed all dependencies.

_Package name may be different between distros, if deepin-ai-daemon is available from your distro, check the packaging script delivered from your distro is a better idea._

Assume you are using [Deepin](https://distrowatch.com/table.php?distribution=deepin) or other debian-based distro which got dde-file-manager delivered:

``` shell
$ git clone https://github.com/linuxdeepin/deepin-ai-daemon
$ cd deepin-ai-daemon
$ sudo apt build-dep ./
```

2. Build:
```shell
$ cmake -B build -DCMAKE_INSTALL_PREFIX=/usr
$ cmake --build build
```

3. Install:
```shell
$ sudo cmake --build build --target install
```

The executable binary file could be found at `/usr/bin/deepin-ai-daemon`

## Usage

Execute `deepin-ai-daemon`

## Documentations

 - todo

## Getting help

 - [Official Forum](https://bbs.deepin.org/)
 - [Developer Center](https://github.com/linuxdeepin/developer-center)
 - [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
 - [IRC Channel](https://webchat.freenode.net/?channels=deepin)
 - [Wiki](https://wiki.deepin.org/)

## Getting involved

We encourage you to report issues and contribute changes

 - [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en) (English)
 - [开发者代码贡献指南](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers) (中文)
 - [Translate for your language on Transifex](https://www.transifex.com/linuxdeepin/deepin-file-manager/)

## License

deepin-ai-daemon is licensed under [GPL-3.0-or-later](LICENSE)
