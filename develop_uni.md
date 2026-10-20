# develop_uni: одно ПО для HSS 013 и HSS 016

Документ для продолжения работы. База — ветка `develop_uni` (от `develop-MME016`). Алгоритмы измерений 016 не заменялись логикой 013. Из 013 взята только аппаратная схема «один PicoScope, два канала».

## Зачем

- `develop-MME013` — один осциллограф, канал A = ток, канал B = напряжение.
- `develop-MME016` — два осциллографа, на каждом только канал A (ток и напряжение на разных приборах). 016 новее: калибровки по диапазонам, trr, DUT trig, фильтры.
- `develop_uni` должна работать с обеими конфигурациями. Режим выбирается по `HighSpeedSampler.config`, не отдельной сборкой.

## Выбор режима

Файл: `HighSpeedSampler/HighSpeedSampler.config`, чтение в `HighSpeedSampler/HighSpeedSampler.cpp`.

Ключи `scope_voltage` и `scope_current` читаются с default `""` (отсутствие ключа не роняет старт). Явного `scope_mode` нет. Флаг выставляется один раз в `SAMPLER_Open` (`DualScope`, запрос через `SAMPLER_IsDualScope()`).

| Конфиг | Режим |
|---|---|
| оба серийника непустые и разные | DualScope — логика 016 |
| один задан, второй пуст, оба пустые или оба равны | SingleScope — схема 013 |

Открытие SingleScope:

- берётся непустой `scope_current`, иначе `scope_voltage`;
- оба пустые → `ps5000aOpenUnit(..., NULL, ...)` (первый найденный прибор).

`SCOPE_CURRENT_ONLY` в `Controller/Global.h` — отдельный диагностический режим «только ток». Это не переключатель 013/016. Не использовать его как режим одного осциллографа.

В логе при старте должна быть строка `Scope mode: dual-scope` или `Scope mode: single-scope two-channel`.

## Что общее и что ветвится

Расчёт после `GetValues` (`Logic.cpp`, `Calculate.cpp`, FIR, Spline), DataTable, endpoints и `Controller.cpp` общие. Они уже работают с двумя буферами `MEMBUF_ScopeI` / `MEMBUF_ScopeV`.

Ветвление только в `Controller/Sampler.cpp`.

Нельзя подставить один handle и в `VHandler`, и в `IHandler` и оставить вызовы 016 как есть: `RunBlock` / `Close` / `GetValues` выполнятся дважды, а `ConfigureChannelsX` включит канал A и выключит B.

| Операция | DualScope (016) | SingleScope (013) |
|---|---|---|
| Open / Close / Stop / GetTimebase / SetSimpleTrigger / RunBlock / GetValues | на каждый прибор | один раз на общий handle |
| Каналы | на каждом: A включён, B выключен (`SAMPLING_ACTIVE_CHANNEL` / `SAMPLING_OFF_CHANNEL`) | A = ток, B = напряжение (`SAMPLING_I_CHANNEL` / `SAMPLING_V_CHANNEL`), оба включены |
| Буферы | I и V с канала A своих приборов | I → A, V → B того же прибора |
| Callback | каждый handle ставит свой флаг | один `RunBlock` ставит оба `SamplingVDone` и `SamplingIDone` |
| `SCOPE_CURRENT_ONLY` | прибор напряжения не открывается | канал B не включается |
| Диапазоны | независимые, `SAMPLER_SelectRange` | те же независимые диапазоны. Фиксированный `SAMPLING_V_RANGE = 10V` из 013 не переносить |

`VHandler == IHandler` в SingleScope нужен только для лога `SAMPLER_GetHandlers`.

## Питание при открытии

`SAMPLER_OpenX`:

- `PICO_OK` и `PICO_EEPROM_CORRUPT` — прибор уже в рабочем режиме питания. `ps5000aChangePowerSource(..., NOT_CONNECTED)` не вызывать: на 013 это даёт `PICO_POWER_SUPPLY_REQUEST_INVALID` (`0x11B`, код 283) и ПО закрывается.
- `PICO_POWER_SUPPLY_NOT_CONNECTED` и `PICO_USB3_0_DEVICE_NON_USB3_0_PORT` — вызывать `ChangePowerSource`, как в старом 013.

## Триггер

Источник один для обоих режимов: `TRIGGER_SOURCE = PS5000A_EXTERNAL` (`Controller/Global.h`). Это отдельный внешний вход, не канал A/B. Порог `TRIGGER_LEVEL = 2000`, фронт `PS5000A_RISING`.

`ps5000aSetChannel` на старте теста сбрасывает ранее поставленный триггер. Поэтому в `SAMPLER_ActivateSampling` внешний триггер ставится заново после финальной настройки каналов и до `RunBlock`. Ошибка логируется как `External trigger setup failed`.

`PS5000A_TRIGGER_AUX` не используется. Старый 013 тоже триггерился от `PS5000A_EXTERNAL`.

## Разрешение и число семплов

`SAMPLING_RESOLUTION = PS5000A_DR_15BIT`. На старом HSS 013 два канала при 15 бит уже работали; разрешение само по себе не считать причиной отказа 013.

Кап `SAMPLING_SAMPLES = 800000`, фактическая длина — `LOGIC_GetSamplingSamples()` от `SAMPLING_T_CONST` и `REG_DC_FALL_TIME`. Кап заранее не резать. Если `GetTimebase` / `RunBlock` на одном приборе с двумя каналами вернёт ошибку нехватки памяти — уменьшать кап только для SingleScope.

## DataTable при переходе стенда 013 на uni

NV-область — регистры 0…127, файл `DataTable.bin` / `ACT_SAVE_TO_ROM`. Старый bin 013 нельзя оставлять как есть: совпадает по смыслу только шунт.

Словарь uni — `Platform/DeviceObjectDictionary.h` (это словарь 016, не 013).

Переносится:

- регистры 0–1, `REG_SHUNT_RES_N/D`, если шунт тот же.

Обязательно задать заново:

- регистр 5 — в 013 это был `REG_ENABLE_VOLTAGE_SMPL`, в uni это `REG_INVERT_CURRENT`. В старом 013 инверсия тока была зашита (`SAMPLING_I_INVERT_INPUT = true`). На uni для той же схемы нужно `REG_INVERT_CURRENT = 1`.
- регистры 9–10 — `REG_VOLTAGE_K_N/D`. Напряжение считается как `Vmax / K` и далее `ScopeU * K` (`Logic.cpp`). Это не старый фиксированный `SAMPLING_V_COEFFICIENT = 3753`.
- регистры 11–25 — полиномы напряжения `REG_U_5_*` … `REG_U_9_*` по диапазонам Pico от 500 mV до 10 V.
- регистры 26–43 — полиномы тока `REG_I_3_*` … `REG_I_8_*` по диапазонам от 100 mV до 5 V.
- регистры 44–46 — подстройка `REG_TRR_P2/P1/P0`.
- регистры 50–51 — `REG_FLATTOP_DUT_US` / `REG_FLATTOP_DUT_HYST` (дефолты 100 мкс и 900).

В словаре ещё есть `REG_I_FINE_*` (2–4) и `REG_V_FINE_*` (6–8), но текущий `Logic.cpp` их не читает. Старые коэффициенты 013 туда не «подхватятся».

Регистры 128+ (`REG_CURRENT_AMPL`, `REG_VOLTAGE_AMPL`, режим измерения) в ROM не хранятся.

Практический путь на стенде: сохранить старый bin, на новом ПО сбросить NV, записать шунт, инверсию, `VOLTAGE_K`, откалибровать полиномы, `ACT_SAVE_TO_ROM`.

## Что уже проверено на железе 013

19.10.2026, сборка с commit `040697f` (до повторной установки триггера перед `RunBlock`):

- прибор найден, серийник `CP893/051`, режим `single-scope two-channel`;
- оба open status `0x00000000`, один handle `16384` на V и I, `Picoscope init done`;
- `ACT_START_TEST` дошёл до печати диапазонов (`Range: 5` по току, `Voltage range: 6`) и не напечатал `Start error`;
- сразу пришёл второй `Start test request` (первый уже перевёл состояние в `DS_InProcess`, второй получает `ERR_OPERATION_BLOCKED`);
- QSU вернуло `FAULT_LOGIC_SCOPE` / `FAULTEX_CFG_TIMEOUT`.

Открытие и первичная инициализация каналов на 013 проходят. Таймаут — на вооружении захвата, не на `OpenUnit`. Повторная установка `PS5000A_EXTERNAL` перед `RunBlock` уже внесена в рабочее дерево и на стенде после этого ещё не подтверждалась.

## Куда смотреть в коде

- `HighSpeedSampler/HighSpeedSampler.cpp` — чтение серийников.
- `HighSpeedSampler/HighSpeedSampler.config` — комментарий правила режима.
- `HighSpeedSampler/Controller/Sampler.cpp`, `Sampler.h` — весь аппаратный развилочный код, питание, триггер, callback.
- `HighSpeedSampler/Controller/Global.h` — каналы, разрешение, `TRIGGER_SOURCE`, `SCOPE_CURRENT_ONLY`.
- `HighSpeedSampler/Controller/Logic.cpp` — лог режима, диапазоны, перевод ADC в ток/напряжение. Не ветвить расчёт по числу приборов.
- `HighSpeedSampler/Controller/Controller.cpp` — `ACT_START_TEST` вызывает `LOGIC_PicoScopeActivate()` синхронно внутри обработки SCCI. Долгий вызов Pico задерживает ответ мастеру.
- `HighSpeedSampler/Platform/DeviceObjectDictionary.h`, `Constraints.cpp` — регистры и дефолты.

## Чего не делать

- Не мержить алгоритмы и карту регистров 013 поверх 016.
- Не выключать канал B в SingleScope (это делает только `SCOPE_CURRENT_ONLY`).
- Не вызывать `ChangePowerSource(NOT_CONNECTED)` после `PICO_OK`.
- Не снижать 15 бит, пока API явно не отвергнет два канала в SingleScope.
- Не считать `SCOPE_CURRENT_ONLY` режимом стенда 013.
