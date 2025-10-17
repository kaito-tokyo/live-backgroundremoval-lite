import torch
import onnx
from onnx2torch import convert
import coremltools as ct
import sys

INPUT_SHAPE = (1, 3, 144, 256)

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <onnx_model_path> <output_path>")
        sys.exit(1)

    onnx_model_path = sys.argv[1]
    output_path = sys.argv[2]

    onnx_model = onnx.load(onnx_model_path)
    torch_model = convert(onnx_model)
    torch_model.eval()
    example_input = torch.randn(*INPUT_SHAPE)
    traced_model = torch.jit.trace(torch_model, example_input)
    mlpackage_model = ct.convert(
        traced_model,
        source="auto",
        inputs=[ct.ImageType(name="image", shape=example_input.shape, color_layout="RGB")],
        outputs=[ct.TensorType(name="segmentationMask")],
        convert_to="mlprogram"
    )
    mlpackage_model.save(output_path)

if __name__ == "__main__":
    main()
