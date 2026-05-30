# AutoClicker - Программный кликер на C++ с графическим интерфейсом

Автоматический кликер с настраиваемой скоростью, горячими клавишами и выбором кнопки мыши.

## Зависимости (Linux)

```bash
sudo apt-get install libx11-dev libxtst-dev libxmu-dev libxfixes-dev g++ make

Сборка
bash

cd ~/autoclicker_cpp
make

Запуск
bash

./autoclicker

Управление

    F6 - Старт/Стоп кликинга

    Esc - Закрыть программу

Сборка под Windows (MinGW)
bash

x86_64-w64-mingw32-g++ -o autoclicker.exe autoclicker.cpp -static -luser32 -lgdi32

Лицензия

MIT
