#pragma once

// Ядро симулятора: чистый C++ без зависимостей от Qt/UI.
// Все лимиты и сообщения об ошибках собраны здесь, чтобы валидация
// была единой для UI и тестов.

#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace dice {

// Ограничения входных данных (защита от крашей/зависаний оригинала).
inline constexpr int kMaxEdges = 1000;        // граней у одного кубика
inline constexpr int kMaxCountPerGroup = 100; // кубиков в одной группе "NdM"
inline constexpr int kMaxTotalDice = 200;     // суммарно кубиков
inline constexpr long long kMaxThrows = 10'000'000;

struct DiceGroup {
  int count;
  int edges;
  bool operator==(const DiceGroup &other) const = default;
};

class ParseError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

// Разбирает строку вида "1d6 2d8 1d4", "3D20", "2d6+2d4".
// Регистр 'd' не важен; "d6" означает 1d6.
// Посторонние символы пропускаются (как в оригинале), но некорректные
// кубики ("0d6", "1d99999", превышение лимитов) дают явную ошибку.
// Бросает dice::ParseError с текстом для показа пользователю.
std::vector<DiceGroup> parse(std::string_view input);

struct SimulationResult {
  // Сумма броска -> сколько раз выпала. Ключи упорядочены по возрастанию.
  std::map<int, long long> frequency;
  double empirical_mean = 0.0;
};

// Симуляция numThrows бросков. Бросает ParseError при невалидном количестве.
SimulationResult simulate(const std::vector<DiceGroup> &dice,
                          long long throws,
                          std::mt19937_64 &rng);

struct TheoreticalResult {
  double mean = 0.0;     // M[X]
  double variance = 0.0; // D[X]
  double std_dev = 0.0;  // sigma = sqrt(D)
  // Сумма -> вероятность. Ключи упорядочены по возрастанию.
  std::map<int, double> probabilities;
};

// Точные M, D, sigma и распределение суммы (дискретная свёртка).
// Для групп "NdM" используется возведение распределения в степень,
// поэтому "100d1000" считается мгновенно.
TheoreticalResult theoretical(const std::vector<DiceGroup> &dice);

} // namespace dice
