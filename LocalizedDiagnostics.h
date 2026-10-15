#pragma once
#include "Localization.h"
#include <utility>

namespace app_language {
// Translate on the UI thread, not in transport/storage code: a cached error
// changes language immediately and SDK/OS text, paths and numbers stay intact.
inline std::string Diagnostic(const std::string& message) {
    if (!IsRussian()) return message;
    static const std::pair<const char*, const char*> translations[] = {
        {"Invalid value rule type.", "Недопустимый тип ограничения значения."},
        {"Value rule address must be nonzero.", "Адрес ограничения значения не должен быть нулевым."},
        {"Value rule extends beyond the 32-bit address space.", "Ограничение значения выходит за пределы 32-битного адресного пространства."},
        {"Unsigned bounds must be exact decimal integers.", "Границы беззнакового значения должны быть целыми десятичными числами."},
        {"Unsigned bounds exceed the selected type.", "Границы беззнакового значения выходят за диапазон выбранного типа."},
        {"Value rule minimum exceeds maximum.", "Минимум ограничения значения превышает максимум."},
        {"Signed bounds must be exact decimal integers.", "Границы знакового значения должны быть целыми десятичными числами."},
        {"Signed bounds exceed the selected type.", "Границы знакового значения выходят за диапазон выбранного типа."},
        {"Float32 bounds must be finite, representable numbers.", "Границы Float32 должны быть конечными числами, представимыми в этом типе."},
        {"Float64 bounds must be finite, representable numbers.", "Границы Float64 должны быть конечными числами, представимыми в этом типе."},
        {"Write value is non-finite or outside the configured bounds.", "Записываемое значение не является конечным числом или выходит за установленные границы."},
        {"Target writes are locked (read-only).", "Запись в целевое устройство заблокирована (только чтение)."},
        {"Write size must be nonzero.", "Размер записи не должен быть нулевым."},
        {"Write data is null.", "Нет данных для записи."},
        {"Write extends beyond the 32-bit address space.", "Запись выходит за пределы 32-битного адресного пространства."},
        {"Configured write address range is invalid.", "Недопустимый диапазон адресов записи."},
        {"Entire write must be within the allowed address range.", "Все записываемые байты должны находиться в разрешённом диапазоне адресов."},
        {"Write overlaps a typed rule without matching its exact address and width.", "Запись пересекает типизированное ограничение, но не совпадает с его точным адресом и размером."},
        {"Connect to the target before enabling writes.", "Подключитесь к целевому устройству перед разрешением записи."},
        {"Connection or protection settings changed. Confirm the new session again.", "Подключение или настройки защиты изменились. Подтвердите новый сеанс ещё раз."},
        {"Invalid inclusive write address range.", "Недопустимый включительный диапазон адресов записи."},
        {"At most 32 value rules are allowed.", "Допускается не более 32 ограничений значений."},
        {"Value rules must not overlap.", "Ограничения значений не должны пересекаться."},
        {"J-Link serial does not match the original probe; refusing to use a different device.", "Серийный номер J-Link не совпадает с исходным: использование другого устройства запрещено."},
        {"Target reference voltage is zero. Check target power and VTref wiring.", "Опорное напряжение целевого устройства равно нулю. Проверьте питание и подключение VTref."},
        {"Cannot create J-Link communication thread.", "Не удалось создать поток обмена с J-Link."},
        {"Cannot locate executable directory", "Не удалось определить папку исполняемого файла"},
        {"Invalid settings key", "Недопустимый ключ настройки"},
        {"Settings have not been loaded safely; refusing to overwrite", "Настройки не удалось безопасно загрузить; перезапись запрещена"},
        {"Cannot inspect settings file: ", "Не удалось проверить файл настроек: "},
        {"Settings path is not a readable regular file: ", "Путь настроек не является доступным для чтения обычным файлом: "},
        {"Cannot read settings file: ", "Не удалось открыть файл настроек для чтения: "},
        {"Failed reading settings file: ", "Ошибка чтения файла настроек: "},
        {"Unsupported settings encoding (UTF-16 or NUL bytes): ", "Неподдерживаемая кодировка настроек (UTF-16 или нулевые байты): "},
        {"Malformed settings value on line ", "Некорректное значение настройки в строке "},
        {"Malformed settings line ", "Некорректная строка настроек "},
        {"Cannot load settings: ", "Не удалось загрузить настройки: "},
        {"Cannot save settings: ", "Не удалось сохранить настройки: "},
        {"Invalid setting: ", "Некорректная настройка: "},
        {"J-Link open failed: ", "Ошибка открытия J-Link: "},
        {"Cannot select the original J-Link (serial ", "Не удалось выбрать исходный J-Link (серийный номер "},
        {"Target configuration failed (", "Ошибка настройки целевого устройства ("},
        {"Cannot select SWD (", "Не удалось выбрать SWD ("},
        {"Target connection failed (", "Ошибка подключения к целевому устройству ("},
        {"Target did not pass the connection health check (", "Проверка связи с целевым устройством не пройдена ("},
        {"Read failed at ", "Ошибка чтения по адресу "},
        {"Write failed at ", "Ошибка записи по адресу "},
        {"J-Link/target connection lost (health check ", "Соединение с J-Link/целевым устройством потеряно (проверка связи "},
    };
    for (const auto& entry : translations) {
        const std::string_view english(entry.first);
        if (message.compare(0, english.size(), english) != 0) continue;
        std::string result = entry.second + message.substr(english.size());
        const std::pair<const char*, const char*> endings[] = {
            {" (using default)", " (используется значение по умолчанию)"},
            {"). Check target power, SWD wiring and selected device.", "). Проверьте питание, подключение SWD и выбранное устройство."},
            {" bytes). Target may have been modified; pending writes discarded, not replayed.", " байт). Целевое устройство могло измениться; ожидающие записи удалены и не будут повторены."},
            {" bytes).", " байт)."},
            {" bytes). Cache invalidated; pending writes discarded.", " байт). Кэш сброшен; ожидающие записи удалены."},
        };
        for (const auto& ending : endings) {
            const size_t length = std::strlen(ending.first);
            if (result.size() >= length && result.compare(result.size() - length, length, ending.first) == 0) {
                result.replace(result.size() - length, length, ending.second);
                break;
            }
        }
        return result;
    }
    return message; // Preserve unknown OS/SDK diagnostics verbatim.
}
} // namespace app_language
