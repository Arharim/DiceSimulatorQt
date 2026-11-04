// Юнит-тесты ядра (без Qt): точные аналитические значения для костей
// против свёртки, инварианты симуляции, проверка валидации ввода.

#include "dice.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace {

int failures = 0;

#define CHECK(cond)                                                                                \
  do {                                                                                             \
    if (!(cond)) {                                                                                 \
      ++failures;                                                                                  \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);                                  \
    }                                                                                              \
  } while (false)

#define CHECK_NEAR(a, b, eps)                                                                      \
  do {                                                                                             \
    const double _x = (a), _y = (b);                                                               \
    if (!(std::abs(_x - _y) <= (eps))) {                                                           \
      ++failures;                                                                                  \
      std::printf("FAIL %s:%d  %s = %.12f, expected %.12f (eps %g)\n",                             \
                  __FILE__,                                                                        \
                  __LINE__,                                                                        \
                  #a,                                                                              \
                  _x,                                                                              \
                  _y,                                                                              \
                  eps);                                                                            \
    }                                                                                              \
  } while (false)

void expectParseError(const std::string &input) {
  try {
    dice::parse(input);
    ++failures;
    std::printf("FAIL: expected ParseError for input '%s'\n", input.c_str());
  } catch (const dice::ParseError &) {
    // ok
  }
}

std::vector<dice::DiceGroup> mustParse(const std::string &input) {
  try {
    return dice::parse(input);
  } catch (const std::exception &e) {
    ++failures;
    std::printf("FAIL: unexpected ParseError for '%s': %s\n", input.c_str(), e.what());
    return {};
  }
}

// ln(Gamma(x)). Lanczos, g = 7, девять коэффициентов; здесь x >= 1.
double lnGamma(double x) {
  static const double kCoeff[9] = {
      0.99999999999980993, 676.5203681218851, -1259.1392167224028,
      771.32342877765313,  -176.61502916214059, 12.507343278686905,
      -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7};
  x -= 1.0;
  double a = kCoeff[0];
  const double t = x + 7.5;
  for (int i = 1; i < 9; ++i) {
    a += kCoeff[i] / (x + i);
  }
  return 0.5 * std::log(2 * 3.14159265358979323846) + (x + 0.5) * std::log(t) -
         t + std::log(a);
}

// Верхний хвост хи-квадрат: Q(df/2, chi2/2) через регулярризованную
// неполную гамма-функцию. Ниже x < a + 1 - ряд, выше - цепная дробь
// (Abramowitz & Stegun 6.5.29, Numerical Recipes gammq).
double chiSquarePValue(double chi2, int df) {
  const double a = df / 2.0;
  const double x = chi2 / 2.0;
  const double prefix = std::exp(-x + a * std::log(x) - lnGamma(a));
  if (x < a + 1.0) {
    double ap = a;
    double term = 1.0 / a;
    double sum = term;
    for (int n = 1; n < 200; ++n) {
      ap += 1.0;
      term *= x / ap;
      sum += term;
      if (std::abs(term) < std::abs(sum) * 1e-15) {
        break;
      }
    }
    return 1.0 - prefix * sum;
  }
  const double kTiny = 1e-300;
  double b = x + 1.0 - a;
  double c = 1.0 / kTiny;
  double d = 1.0 / b;
  double h = d;
  for (int i = 1; i < 200; ++i) {
    const double an = -i * (i - a);
    b += 2.0;
    d = an * d + b;
    if (std::abs(d) < kTiny) {
      d = kTiny;
    }
    c = b + an / c;
    if (std::abs(c) < kTiny) {
      c = kTiny;
    }
    d = 1.0 / d;
    const double del = d * c;
    h *= del;
    if (std::abs(del - 1.0) < 1e-15) {
      break;
    }
  }
  return prefix * h;
}

// Статистика хи-квадрат наблюдаемых частот против точных вероятностей.
double chiSquareStatistic(const std::map<int, long long> &observed,
                          const std::map<int, double> &probability,
                          long long throws, int *df) {
  double stat = 0.0;
  int bins = 0;
  for (const auto &[sum, p] : probability) {
    const double expected = static_cast<double>(throws) * p;
    const double diff = static_cast<double>(observed.at(sum)) - expected;
    stat += diff * diff / expected;
    ++bins;
  }
  *df = bins - 1;
  return stat;
}

// Самопроверка Q на известных критических значениях хи-квадрат.
void testChiSquarePValue() {
  CHECK_NEAR(chiSquarePValue(3.8415, 1), 0.05, 5e-5);
  CHECK_NEAR(chiSquarePValue(6.6349, 1), 0.01, 5e-5);
  CHECK_NEAR(chiSquarePValue(18.3070, 10), 0.05, 5e-5);
  CHECK_NEAR(chiSquarePValue(0.0, 2), 1.0, 1e-12);
}

void testParse() {
  CHECK((mustParse("1d6 2d8 1d4") == std::vector<dice::DiceGroup>{{1, 6}, {2, 8}, {1, 4}}));
  CHECK((mustParse("3D20") == std::vector<dice::DiceGroup>{{3, 20}})); // регистр
  CHECK((mustParse("d6") == std::vector<dice::DiceGroup>{{1, 6}}));    // count по умолчанию
  CHECK((mustParse("2d6+2d4") == std::vector<dice::DiceGroup>{{2, 6}, {2, 4}}));
  CHECK((mustParse("2d6+3") == std::vector<dice::DiceGroup>{{2, 6}})); // константы пропускаются
  CHECK((mustParse("  1d20  ") == std::vector<dice::DiceGroup>{{1, 20}}));
  CHECK((mustParse("1d19+1d20") == std::vector<dice::DiceGroup>{{1, 19}, {1, 20}}));

  expectParseError("");
  expectParseError("   ");
  expectParseError("abc");
  expectParseError("42");
  expectParseError("0d6");              // нулевое количество - теперь явная ошибка
  expectParseError("1d0");              // ноль граней - явная ошибка
  expectParseError("1d1001");           // граней больше kMaxEdges
  expectParseError("101d6");            // в группе больше kMaxCountPerGroup
  expectParseError("250d6 250d6");      // суммарно больше kMaxTotalDice
  expectParseError("1d99999999999999"); // краш оригинала -> теперь ошибка формата
  expectParseError("99999999999d6");    // краш оригинала -> теперь ошибка формата
}

void testTheorySingleDie() {
  const auto t = dice::theoretical(mustParse("1d20"));
  CHECK(t.probabilities.size() == 20);
  CHECK_NEAR(t.mean, 10.5, 1e-12);
  CHECK_NEAR(t.variance, 33.25, 1e-12);
  CHECK_NEAR(t.std_dev, std::sqrt(33.25), 1e-12);
  double sum = 0.0;
  int key = 1;
  for (const auto &[k, p] : t.probabilities) {
    CHECK(k == key++);
    CHECK_NEAR(p, 0.05, 1e-15);
    sum += p;
  }
  CHECK_NEAR(sum, 1.0, 1e-12);
}

void testTheoryTwoD6() {
  const auto t = dice::theoretical(mustParse("2d6"));
  const double expected[11] = {1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1}; // из 36
  CHECK(t.probabilities.size() == 11);
  CHECK_NEAR(t.mean, 7.0, 1e-12);
  CHECK_NEAR(t.variance, 35.0 / 6.0, 1e-12); // 2 * 35/12
  double sum = 0.0;
  for (const auto &[k, p] : t.probabilities) {
    const int idx = k - 2;
    CHECK(idx >= 0 && idx < 11);
    CHECK_NEAR(p, expected[idx] / 36.0, 1e-15);
    sum += p;
  }
  CHECK_NEAR(sum, 1.0, 1e-12);
}

void testTheoryMixed() {
  const auto t = dice::theoretical(mustParse("2d6 3d4"));
  CHECK_NEAR(t.mean, 7.0 + 3 * 2.5, 1e-12); // 14.5
  CHECK_NEAR(t.variance, 35.0 / 6.0 + 3 * (15.0 / 12.0), 1e-12);
  double sum = 0.0;
  for (const auto &[k, p] : t.probabilities) {
    sum += p;
    CHECK(k >= 5);  // 2+3 костей минимум
    CHECK(k <= 24); // 12+12 максимум
  }
  CHECK_NEAR(sum, 1.0, 1e-12);
}

void testTheoryBigGroup() {
  // Патологический ввод, который вешал бы наивную свёртку.
  const auto t = dice::theoretical(mustParse("100d1000"));
  double sum = 0.0;
  int firstKey = -1, lastKey = 0;
  for (const auto &[k, p] : t.probabilities) {
    sum += p;
    if (firstKey < 0) {
      firstKey = k;
    }
    lastKey = k;
  }
  CHECK_NEAR(sum, 1.0, 1e-9);
  CHECK(firstKey == 100);
  CHECK(lastKey == 100000);
  CHECK_NEAR(t.mean, 100 * 500.5, 1e-9);
  CHECK_NEAR(t.variance, 100 * ((1000.0 * 1000.0 - 1) / 12.0), 1e-6);
}

void testSimulate() {
  std::mt19937_64 rngZero{1};
  try {
    dice::simulate(mustParse("1d6"), 0, rngZero);
    ++failures;
    std::printf("FAIL: expected ParseError for throws=0\n");
  } catch (const dice::ParseError &) {
    // ok
  }

  // 1d6: частоты покрывают [1..6], суммарно == число бросков.
  std::mt19937_64 rng{42};
  const long long n = 1'000'000;
  auto sim = dice::simulate(mustParse("1d6"), n, rng);
  long long total = 0;
  for (const auto &[k, c] : sim.frequency) {
    CHECK(k >= 1 && k <= 6);
    total += c;
  }
  CHECK(total == n);
  CHECK_NEAR(sim.empirical_mean, 3.5, 0.02);

  // 1d6: форма распределения - хи-квадрат (df = 5).
  {
    const auto theory = dice::theoretical(mustParse("1d6"));
    int df = 0;
    const double stat = chiSquareStatistic(sim.frequency, theory.probabilities, n, &df);
    std::printf("chi2 1d6: stat=%.2f df=%d p=%.4f\n", stat, df,
                chiSquarePValue(stat, df));
    CHECK(chiSquarePValue(stat, df) > 0.001);
  }

  // 3d8: форма распределения - хи-квадрат против точной свёртки (df = 21).
  rng.seed(7);
  const auto sim2 = dice::simulate(mustParse("3d8"), 500'000, rng);
  const auto theory = dice::theoretical(mustParse("3d8"));
  CHECK_NEAR(sim2.empirical_mean, theory.mean, 0.05);
  CHECK(sim2.frequency.size() == theory.probabilities.size());
  {
    int df = 0;
    const double stat = chiSquareStatistic(sim2.frequency, theory.probabilities, 500'000, &df);
    const double p = chiSquarePValue(stat, df);
    std::printf("chi2 3d8: stat=%.2f df=%d p=%.4f\n", stat, df, p);
    CHECK(p > 0.001);
  }
}

void testSimulateDeterministic() {
  std::mt19937_64 rng{123};
  const auto a = dice::simulate(mustParse("2d10"), 10'000, rng);
  rng.seed(123);
  const auto b = dice::simulate(mustParse("2d10"), 10'000, rng);
  CHECK(a.frequency == b.frequency);
  CHECK_NEAR(a.empirical_mean, b.empirical_mean, 1e-12);
}

} // namespace

int main() {
  testParse();
  testChiSquarePValue();
  testTheorySingleDie();
  testTheoryTwoD6();
  testTheoryMixed();
  testTheoryBigGroup();
  testSimulate();
  testSimulateDeterministic();

  if (failures == 0) {
    std::printf("Все тесты пройдены.\n");
    return 0;
  }
  std::printf("Провалено проверок: %d\n", failures);
  return 1;
}
