#!/bin/bash

NCNN_VERSION="20250916"
PYTHON="python3.11"

set -euo pipefail

cd "$(dirname "$0")"

if ! [[ -d ncnn ]]; then
  git clone https://github.com/Tencent/ncnn.git -b "$NCNN_VERSION"
fi

cmake -B build_ncnn \
  -DCMAKE_BUILD_TYPE=Debug \
  -DNCNN_BUILD_TOOLS=ON \
  -DNCNN_BUILD_EXAMPLES=OFF \
  -DNCNN_BUILD_BENCHMARK=OFF \
  -DNCNN_BUILD_TESTS=OFF \
  ncnn

cmake --build build_ncnn

rm -rf .venv
$PYTHON -m venv .venv
. .venv/bin/activate

pip install huggingface_hub pnnx kaggle

hf download \
    onnx-community/mediapipe_selfie_segmentation_landscape \
    --local-dir mediapipe_selfie_segmentation_landscape

pnnx mediapipe_selfie_segmentation_landscape/onnx/model.onnx inputshape="[1,3,144,256]"

curl -L -o lfw-dataset.zip "https://www.kaggle.com/api/v1/datasets/download/jessicali9530/lfw-dataset"
mkdir -p lfw
unzip -qo lfw-dataset.zip -d lfw

find lfw/lfw-deepfunneled/lfw-deepfunneled -type f > mediapipe_selfie_segmentation_landscape/onnx/imagelist.txt

cd "$(dirname "$0")/mediapipe_selfie_segmentation_landscape/onnx"
ncnnoptimize model.ncnn.param model.ncnn.bin model-opt.ncnn.param model-opt.ncnn.bin 0

ncnn2table model-opt.ncnn.param model-opt.ncnn.bin imagelist.txt model.table \
           mean="[0,0,0]" norm="[0.003921,0.003921,0.003921]" \
           shape="[256,144,3]" pixel=RGB method=kl

ncnn2int8 model-opt.ncnn.param model-opt.ncnn.bin model-int8.ncnn.param model-int8.ncnn.bin model.table

ls -l model-int8.ncnn.*

cp model-int8.ncnn.param ../../../../data/models/mediapipe_selfie_segmentation_landscape_int8.ncnn.param
cp model-int8.ncnn.bin ../../../../data/models/mediapipe_selfie_segmentation_landscape_int8.ncnn.bin
