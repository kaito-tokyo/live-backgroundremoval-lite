#!/bin/bash

set -euo pipefail

cd "$(dirname "$0")/.."

xcodegen generate

xcodebuild \
  -project CoreMLSelfieSegmenter.xcodeproj \
  -scheme CoreMLSelfieSegmenter \
  -configuration Debug \
  -sdk macosx \
  -derivedDataPath ../../DerivedData

xcodebuild \
  -project CoreMLSelfieSegmenter.xcodeproj \
  -scheme CoreMLSelfieSegmenter \
  -configuration Release \
  -sdk macosx \
  -derivedDataPath ../../DerivedData
