# Atom

A basic chess bot written as a fun / learning / hobby project in C++. Still in development, expect many bugs!

> [!NOTE]
> This bot does not have time control support; it will run indefinitely. To play it, you should manually set a max search depth.

Current perft score (i5-11400H @ 4.5GHz):
- starting position (depth 7): 3195901860 nodes, 4250 ms, 751976908 nps
- kiwipete position (depth 7): 374190009323 nodes, 274811 ms, 1361626751 nps

## Installation

Requires g++ compiler. Some parts (i.e memory) may only work on linux.
```bash
git clone https://github.com/thegogy/atom
cd atom
make release
```
This will automatically download the NNUE files and compile the bot.

## Playing

This is [UCI](https://en.wikipedia.org/wiki/Universal_Chess_Interface) compatible. You can use any UCI-compatible interface to play it, such as [en-croissant](https://github.com/franciscoBSalgueiro/en-croissant) or [cutechess](https://cutechess.com/).

## Tuning

This bot uses [weather-factory](https://github.com/jnlt3/weather-factory) for tuning. In order to tune the bot, use the following steps:

```bash
# Install weather factory
git clone https://github.com/jnlt3/weather-factory tuning
# Make tuner dir
mkdir tuning/tuner
```

You should also install [fastchess](https://github.com/Disservin/fastchess) and place the binary in the `tuner` dir.

```bash
# Build engine for tuning
make clean
make tune
cp atom tuning/tuner/engine
```

```bash
# Run tuning
cd tuning

# Put tunables in config.json
./tuner/engine tunables > config.json

# Run tuning
python main.py
```

## Inspiration

Move generation takes a lot of inspiration from [VincentBab](https://github.com/vincentbab)'s [Belette](https://github.com/vincentbab/Belette/), as well as [Daniel inführ](https://github.com/Gigantua)'s [Gigantua](https://www.codeproject.com/Articles/5313417/Worlds-fastest-Bitboard-Chess-Movegenerator), as well as many techniques from the [Chess Programming Wiki](https://www.chessprogramming.org/Move_Generation).

[perft_massive](./tests/perft_massive.txt) from [Elcabesa](https://github.com/elcabesa)'s engine, [Vajolet](https://github.com/elcabesa/vajolet).

NNUE from [Stockfish](https://github.com/official-stockfish/Stockfish/). For more information, see [the NNUE readme in this repo](./src/nnue/README.md).

## Todo

[ ] Tune parameters
[ ] Time management
[ ] EGTB pruning
[ ] Testing for search
