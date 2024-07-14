#!/bin/sh

if grep -q "avx512f" /proc/cpuinfo; then
  echo "AVX512FLAGS"
elif grep -q "bmi2" /proc/cpuinfo; then
  echo "BMI2FLAGS"
elif grep -q "avx2" /proc/cpuinfo; then
  echo "AVX2FLAGS"
elif grep -q "sse4_1" /proc/cpuinfo; then
  echo "SSE4FLAGS"
else
  echo "SSE2FLAGS"
fi
