#!/bin/bash

set -euo pipefail

cd "$(dirname "$0")/.."

xcodebuild \
  -project CoreMLSelfieSegmenter.xcodeproj \
  -scheme CoreMLSelfieSegmenter \
  -configuration Debug \
  -sdk macosx \
  -derivedDataPath ../../DerivedData \
  -destination 'platform=macOS' \
  test

xcodebuild \
  -project CoreMLSelfieSegmenter.xcodeproj \
  -scheme CoreMLSelfieSegmenter \
  -configuration Release \
  -sdk macosx \
  -derivedDataPath ../../DerivedData \
  -destination 'platform=macOS' \
  test
