#!/bin/bash

NCNN_VERSION="20250916"
PYTHON="python3.11"

set -euo pipefail

cd "$(dirname "$0")"

if ! [[ -d ncnn ]]; then
  git clone https://github.com/Tencent/ncnn.git -b "$NCNN_VERSION"
fi

cmake -B build_ncnn \
  -DCMAKE_BUILD_TYPE=Release \
  -DNCNN_BUILD_TOOLS=ON \
  -DNCNN_BUILD_EXAMPLES=OFF \
  -DNCNN_BUILD_BENCHMARK=OFF \
  -DNCNN_BUILD_TESTS=OFF \
  ncnn

cmake --build build_ncnn

PATH="$(pwd)/build_ncnn/tools:$(pwd)/build_ncnn/tools/quantize:$PATH"

if ! [[ -d .venv ]]; then
  $PYTHON -m venv .venv
fi

. .venv/bin/activate

pip install pnnx numpy==1.26.4 paddlepaddle paddleseg paddle2onnx onnx onnxruntime

mkdir -p portrait_pp_humansegv2_lite
pushd portrait_pp_humansegv2_lite

if ! [[ -d PaddleSeg ]]; then
  git clone https://github.com/PaddlePaddle/PaddleSeg.git -b "release/2.10"
fi

curl -fsSLO https://github.com/PaddlePaddle/PaddleSeg/raw/refs/heads/release/2.10/contrib/PP-HumanSeg/configs/portrait_pp_humansegv2_lite.yml

mkdir -p output_onnx

python PaddleSeg/tools/model/export_onnx.py \
    --config portrait_pp_humansegv2_lite.yml \
    --width 256 \
    --height 144 \
    --save_dir output_onnx

popd

curl -L -o lfw-dataset.zip "https://www.kaggle.com/api/v1/datasets/download/jessicali9530/lfw-dataset"
mkdir -p lfw
unzip -qo lfw-dataset.zip -d lfw

find $(pwd)/lfw/lfw-deepfunneled/lfw-deepfunneled -type f > portrait_pp_humansegv2_lite/output_onnx/imagelist.txt

pushd "portrait_pp_humansegv2_lite/output_onnx"

cp portrait_pp_humansegv2_lite_model.onnx model.onnx

pnnx model.onnx inputshape="[1,3,144,256]"

ncnnoptimize model.ncnn.param model.ncnn.bin model-opt.ncnn.param model-opt.ncnn.bin 0

ncnn2table model-opt.ncnn.param model-opt.ncnn.bin imagelist.txt model.table \
           mean="[0,0,0]" norm="[0.003921,0.003921,0.003921]" \
           shape="[256,144,3]" pixel=RGB method=kl

ncnn2int8 model-opt.ncnn.param model-opt.ncnn.bin model-int8.ncnn.param model-int8.ncnn.bin model.table

ls -l model-int8.ncnn.*

cp model-int8.ncnn.param ../../../../data/models/portrait_pp_humansegv2_lite_int8.ncnn.param
cp model-int8.ncnn.bin ../../../../data/models/portrait_pp_humansegv2_lite_int8.ncnn.bin
