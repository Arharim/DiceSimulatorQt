#include "dice.h"

#include <cmath>
#include <utility>

namespace dice {

namespace {

bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

long long parseNumber(std::string_view digits) {
  if (digits.size() > 9) {
    throw ParseError("Слишком большое число в записи кубиков (до 9 цифр): '" + std::string(digits) +
                     "'");
  }
  long long value = 0;
  for (char c : digits) {
    value = value * 10 + (c - '0');
  }
  return value;
}

void addGroup(std::vector<DiceGroup> &out, long long count, long long edges) {
  const std::string token = std::to_string(count) + "d" + std::to_string(edges);
  if (count < 1) {
    throw ParseError("Количество кубиков должно быть >= 1: '" + token + "'");
  }
  if (edges < 1) {
    throw ParseError("Количество граней должно быть >= 1: '" + token + "'");
  }
  if (edges > kMaxEdges) {
    throw ParseError("Слишком много граней у кубика (максимум " + std::to_string(kMaxEdges) +
                     "): '" + token + "'");
  }
  if (count > kMaxCountPerGroup) {
    throw ParseError("Слишком много кубиков в одной группе (максимум " +
                     std::to_string(kMaxCountPerGroup) + "): '" + token + "'");
  }
  out.push_back(DiceGroup{static_cast<int>(count), static_cast<int>(edges)});
}

using Dist = std::vector<double>; // index i -> вероятность суммы (i + сдвиг)

Dist convolve(const Dist &a, const Dist &b) {
  Dist result(a.size() + b.size() - 1, 0.0);
  for (size_t i = 0; i < a.size(); ++i) {
    if (a[i] == 0.0) {
      continue;
    }
    for (size_t j = 0; j < b.size(); ++j) {
      result[i + j] += a[i] * b[j];
    }
  }
  return result;
}

Dist uniformDist(int edges) {
  return Dist(static_cast<size_t>(edges), 1.0 / edges);
}

// Распределение суммы count одинаковых кубиков (возведение в степень
// через повторные возведения в квадрат: O(log count) свёрток).
Dist groupDistribution(int count, int edges) {
  Dist base = uniformDist(edges);
  Dist result{1.0};
  int c = count;
  while (c > 0) {
    if (c & 1) {
      result = convolve(std::move(result), base);
    }
    c >>= 1;
    if (c > 0) {
      base = convolve(std::move(base), base);
    }
  }
  return result;
}

} // namespace

std::vector<DiceGroup> parse(std::string_view input) {
  std::vector<DiceGroup> out;
  long long totalDice = 0;
  const size_t n = input.size();
  size_t i = 0;
  while (i < n) {
    const char c = input[i];
    if (!isDigit(c) && c != 'd' && c != 'D') {
      ++i;
      continue;
    }
    size_t j = i;
    while (j < n && isDigit(input[j])) {
      ++j;
    }
    if (j < n && (input[j] == 'd' || input[j] == 'D')) {
      const size_t facesStart = j + 1;
      size_t k = facesStart;
      while (k < n && isDigit(input[k])) {
        ++k;
      }
      if (k > facesStart) {
        const long long count = (j > i) ? parseNumber(input.substr(i, j - i)) : 1;
        const long long edges = parseNumber(input.substr(facesStart, k - facesStart));
        addGroup(out, count, edges);
        totalDice += count;
        i = k;
        continue;
      }
    }
    ++i;
  }
  if (out.empty()) {
    throw ParseError("Введите корректные кубики в формате '1d6 2d8 1d4'!");
  }
  if (totalDice > kMaxTotalDice) {
    throw ParseError("Слишком много кубиков суммарно (максимум " + std::to_string(kMaxTotalDice) +
                     ")!");
  }
  return out;
}

SimulationResult simulate(const std::vector<DiceGroup> &diceList,
                          long long throws,
                          std::mt19937_64 &rng) {
  if (throws < 1) {
    throw ParseError("Количество бросков должно быть >= 1!");
  }
  if (throws > kMaxThrows) {
    throw ParseError("Слишком много бросков (максимум " + std::to_string(kMaxThrows) + ")!");
  }

  SimulationResult result;
  long long totalSum = 0;
  for (long long i = 0; i < throws; ++i) {
    int sum = 0;
    for (const DiceGroup &g : diceList) {
      std::uniform_int_distribution<int> roll(1, g.edges);
      for (int k = 0; k < g.count; ++k) {
        sum += roll(rng);
      }
    }
    ++result.frequency[sum];
    totalSum += sum;
  }
  result.empirical_mean = static_cast<double>(totalSum) / static_cast<double>(throws);
  return result;
}

TheoreticalResult theoretical(const std::vector<DiceGroup> &diceList) {
  double mean = 0.0;
  double variance = 0.0;
  int minSum = 0;
  for (const DiceGroup &g : diceList) {
    mean += (g.edges + 1) / 2.0 * g.count;
    variance += ((static_cast<double>(g.edges) * g.edges - 1.0) / 12.0) * g.count;
    minSum += g.count;
  }

  Dist acc{1.0};
  for (const DiceGroup &g : diceList) {
    acc = convolve(std::move(acc), groupDistribution(g.count, g.edges));
  }

  // Индекс i в свёртке соответствует сумме (i + minSum):
  // каждый кубик добавляет минимум 1 к сумме.
  std::map<int, double> probabilities;
  for (size_t i = 0; i < acc.size(); ++i) {
    if (acc[i] > 0.0) {
      probabilities[static_cast<int>(i) + minSum] = acc[i];
    }
  }

  return TheoreticalResult{mean, variance, std::sqrt(variance), std::move(probabilities)};
}

} // namespace dice
