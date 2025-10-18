#!/bin/bash

PYTHON="python3.11"

set -euo pipefail

cd "$(dirname "$0")/.."

if ! [[ -d mediapipe_selfie_segmentation_landscape ]]; then
  git lfs install
  git clone https://huggingface.co/onnx-community/mediapipe_selfie_segmentation_landscape
fi

if ! [[ -d .venv ]]; then
  $PYTHON -m venv .venv
fi

.venv/bin/pip install coremltools onnx onnx2torch torch

.venv/bin/python Scripts/convert.py \
  mediapipe_selfie_segmentation_landscape/onnx/model.onnx \
  Sources/SelfieSegmenterLandscapeModel.mlpackage
