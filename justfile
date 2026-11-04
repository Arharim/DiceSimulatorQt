# DiceSimulatorQt - common commands. `just` lists recipes.

build_dir := "build"
bin := build_dir / "DiceSimulatorQt"

default:
    @just --list --unsorted

# Configure + build
build:
    cmake -S . -B {{build_dir}} -G Ninja
    cmake --build {{build_dir}}

# Run the app
run *args: build
    {{bin}} {{args}}

# Build once, then the core unit tests
test: build
    ctest --test-dir {{build_dir}} --output-on-failure

# clang-format every project source
fmt:
    clang-format -i src/*.h src/*.cpp tests/*.cpp

clean:
    cmake -E rm -rf {{build_dir}}
