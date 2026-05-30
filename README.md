# AutoClicker PRO v3.0

[![Build Status](https://github.com/Alex767-star/AutoClicker/actions/workflows/build.yml/badge.svg)](https://github.com/Alex767-star/AutoClicker/actions)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Профессиональный программный кликер с многопоточной архитектурой и поддержкой профилей.

## Features

- ⚡ **8 режимов кликов**: Single, Double, Triple, Hold, Random, Burst, Sequence, Multi-target
- 🎯 **5 типов целей**: Current position, Saved position, Moving target, Pattern, Grid
- 💾 **Профили**: Сохранение/загрузка до 5 профилей настроек
- 📊 **Статистика**: CPS, общее время, количество кликов
- 🎨 **Чистая архитектура**: Модульная структура на C++17
- 🔧 **Конфигурация**: Файлы настроек в JSON-подобном формате
- 🤖 **CI/CD**: Автоматическая сборка бинарников под Linux

## Screenshots

![Main Window](docs/screenshots/main.png)
![Settings](docs/screenshots/settings.png)

## Quick Start

### From Release
```bash
wget https://github.com/Alex767-star/AutoClicker/releases/latest/download/autoclicker
chmod +x autoclicker
./autoclicker

Build from Source
bash

git clone https://github.com/Alex767-star/AutoClicker.git
cd AutoClicker
mkdir build && cd build
cmake .. && make
sudo make install

Requirements

    Linux with X11

    libx11-dev

    libxtst-dev

    CMake 3.10+

    C++17 compiler

Installation
bash

# Install dependencies
sudo apt-get install libx11-dev libxtst-dev cmake g++

# Clone and build
git clone https://github.com/Alex767-star/AutoClicker.git
cd AutoClicker
mkdir build && cd build
cmake .. && make -j$(nproc)

# Run
./autoclicker

CLI Commands
Command	Description
s	Start clicking
t	Stop clicking
d 100	Set delay to 100ms
c	Show click count
q	Quit
Configuration

Profiles stored in ~/.config/autoclicker/:

    profile_1.cfg through profile_5.cfg

License

MIT License - see LICENSE for details
Author

ellilot Anderson - GitHub

Made with ❤️ for automation
