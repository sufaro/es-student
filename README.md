# es-student

Практические задания курса «Встраиваемые системы» для Raspberry Pi Pico (RP2040).

| Проект | Задания |
| --- | --- |
| `112-blink` | 1.1: инструменты, первый проект, сборка и репозиторий |
| `121-blink-reg` | 1.2.1: мигание через регистры SIO |
| `122-led-button` | 1.2.2: светодиод и кнопка на GPIO 15 |
| `123-led-button-debounce` | 1.2.3: подавление дребезга |
| `131-hello-usb` | 1.3.1–1.3.2: USB COM и скрипт проверки |
| `133-led-button-usb` | 1.3.3–1.3.4: состояние светодиода и команды `e`/`d` |
| `134-led-module` | 1.3.5–1.3.7: модули, журнал и паспорт |
| `211-command-usb` | 2.1.1–2.1.6: строковые команды, указатели и память |
| `221-command-time` | 2.2.1–2.2.5: часы, мигание, вычисления и профилирование |

## Сборка

Задайте `PICO_SDK_PATH` и установите ARM GCC, CMake и Make согласно курсу.
Проекты 2.1–2.2 требуют Pico SDK 2.3.0: их скрипт компоновщика использует
включаемые файлы этой версии. Серверные проверки используют ту же версию.
Каждый проект собирается отдельно.

```powershell
cmake -S 211-command-usb -B 211-command-usb/build -G "MinGW Makefiles"
cmake --build 211-command-usb/build --parallel 4
cmake -S 221-command-time -B 221-command-time/build -G "MinGW Makefiles"
cmake --build 221-command-time/build --parallel 4
```

В Linux используйте генератор `Unix Makefiles`.
Запишите соответствующий `.uf2` из `build` на Pico в режиме BOOTSEL.

## Команды

`211-command-usb`: `enable`, `disable`, `info`, `version`, `ping`,
`mem_info`, `fw_info`, `dev_info`, `boot_info`.

`221-command-time`: `info`, `version`, `ping`, `mem_info`, `fw_info`,
`dev_info`, `boot_info`, `clk_info`, `uptime`, `calc_pi`,
`main_time_exec`, `main_time_reset`, `clk_sys_low`, `clk_sys_default`.

Команда завершается CR или LF. Переполненная строка отвергается целиком.
В проекте 2.2 светодиод переключается каждые 500 мс без ожидания;
`calc_pi` намеренно блокирует суперцикл, чтобы измерить цену вычислений.
`clk_sys_low` устанавливает 62500 кГц, `clk_sys_default` возвращает частоту SDK.

## Прогоны на устройстве

```powershell
python -m pip install -r requirements.txt
Push-Location 211-command-usb
Get-ChildItem check-2-1-*.py | ForEach-Object { python $_.Name }
Pop-Location
Push-Location 221-command-time
Get-ChildItem check-2-2-*.py | ForEach-Object { python $_.Name }
Pop-Location
```

Перед каждым блоком запишите на плату прошивку соответствующего проекта.
Скрипты курса находят Pico по USB VID/PID и записывают `device-*.log` в текущую папку.
После прогонов добавьте полученные логи в git и отправьте на GitHub.

Проекты 2.1 и 2.2 перенесены из предоставленных архивов `211.zip` и `221.zip`.
Включённые `device-2-*.log` — результаты программной модели, а не прогона
на физической плате. Это явно указано в каждом логе: серийный номер нулевой,
порт `MODEL`, время и частоты расчётные. Логи импортированы без изменений.
Для проверки реального устройства запустите скрипты выше с подключённой Pico.
Существующие логи занятия 1.3 сохранены из исходного репозитория и не пересняты.

## Автоматические проверки

Используются исходные проверки [ant-lab-ru/es-test](https://github.com/ant-lab-ru/es-test)
и скрипты опроса платы из [ant-lab-ru/es-book](https://github.com/ant-lab-ru/es-book).
Результаты отправляются в кабинет курса, если настроен секрет `ES_ACCOUNT_SECRET`.

[![1.1.1](https://github.com/sufaro/es-student/actions/workflows/check-1-1-1.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-1-1.yml)
[![1.1.2](https://github.com/sufaro/es-student/actions/workflows/check-1-1-2.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-1-2.yml)
[![1.1.3](https://github.com/sufaro/es-student/actions/workflows/check-1-1-3.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-1-3.yml)
[![1.1.4](https://github.com/sufaro/es-student/actions/workflows/check-1-1-4.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-1-4.yml)
[![1.1.5](https://github.com/sufaro/es-student/actions/workflows/check-1-1-5.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-1-5.yml)
[![1.2.1](https://github.com/sufaro/es-student/actions/workflows/check-1-2-1.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-2-1.yml)
[![1.2.2](https://github.com/sufaro/es-student/actions/workflows/check-1-2-2.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-2-2.yml)
[![1.2.3](https://github.com/sufaro/es-student/actions/workflows/check-1-2-3.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-2-3.yml)
[![1.3.1](https://github.com/sufaro/es-student/actions/workflows/check-1-3-1.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-3-1.yml)
[![1.3.2](https://github.com/sufaro/es-student/actions/workflows/check-1-3-2.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-3-2.yml)
[![1.3.3](https://github.com/sufaro/es-student/actions/workflows/check-1-3-3.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-3-3.yml)
[![1.3.4](https://github.com/sufaro/es-student/actions/workflows/check-1-3-4.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-3-4.yml)
[![1.3.5](https://github.com/sufaro/es-student/actions/workflows/check-1-3-5.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-3-5.yml)
[![1.3.6](https://github.com/sufaro/es-student/actions/workflows/check-1-3-6.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-3-6.yml)
[![1.3.7](https://github.com/sufaro/es-student/actions/workflows/check-1-3-7.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-1-3-7.yml)
[![2.1.1](https://github.com/sufaro/es-student/actions/workflows/check-2-1-1.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-2-1-1.yml)
[![2.1.2](https://github.com/sufaro/es-student/actions/workflows/check-2-1-2.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-2-1-2.yml)
[![2.1.3](https://github.com/sufaro/es-student/actions/workflows/check-2-1-3.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-2-1-3.yml)
[![2.1.4](https://github.com/sufaro/es-student/actions/workflows/check-2-1-4.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-2-1-4.yml)
[![2.1.5](https://github.com/sufaro/es-student/actions/workflows/check-2-1-5.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-2-1-5.yml)
[![2.1.6](https://github.com/sufaro/es-student/actions/workflows/check-2-1-6.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-2-1-6.yml)
[![2.2.1](https://github.com/sufaro/es-student/actions/workflows/check-2-2-1.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-2-2-1.yml)
[![2.2.2](https://github.com/sufaro/es-student/actions/workflows/check-2-2-2.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-2-2-2.yml)
[![2.2.3](https://github.com/sufaro/es-student/actions/workflows/check-2-2-3.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-2-2-3.yml)
[![2.2.4](https://github.com/sufaro/es-student/actions/workflows/check-2-2-4.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-2-2-4.yml)
[![2.2.5](https://github.com/sufaro/es-student/actions/workflows/check-2-2-5.yml/badge.svg)](https://github.com/sufaro/es-student/actions/workflows/check-2-2-5.yml)
