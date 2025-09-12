import coremltools as ct
import onnx
import sys

print("coremltools version:", ct.__version__)

# --- 設定項目 ---
INPUT_ONNX_MODEL = 'model.onnx'
OUTPUT_COREML_MODEL = 'SelfieSegmentation.mlmodel'

# --- 変換処理 ---
try:
    # 1. モデルの入力情報を定義します。（ここは変更なし、良い設計です）
    scale = 1.0 / 127.5
    bias = [-1.0, -1.0, -1.0]

    image_input = ct.ImageType(
        name='input',
        shape=(1, 256, 256, 3),
        scale=scale,
        bias=bias,
        color_layout='RGB',
        channel_first=False
    )

    print(f"{INPUT_ONNX_MODEL} をCoreMLモデルに変換します...")

    # 2. 統一された ct.convert() 関数を使用 (こちらが最新の推奨APIです)
    #    - convert_to="mlprogram":
    #      より柔軟で強力な新しいモデル形式。近年のモデル変換では推奨。
    #    - minimum_deployment_target:
    #      ターゲットOSを指定。これにより、そのOSで利用可能な最新のオペレータを使って
    #      最適化された変換が行われ、互換性が向上します。
    mlmodel = ct.convert(
        model=INPUT_ONNX_MODEL,
        convert_to="mlprogram", # "neuralnetwork" (旧) から "mlprogram" (新) へ
        inputs=[image_input],
        compute_units=ct.ComputeUnit.ALL,
        minimum_deployment_target=ct.target.macOS12 # OBSのターゲットOSに合わせて調整
    )

    # 3. モデルのメタデータを設定
    mlmodel.author = 'Kaito Udagawa'
    mlmodel.short_description = 'MediaPipe Selfie Segmentation model converted to Core ML.'
    mlmodel.input_description['input'] = '256x256 RGB image to be segmented.'
    
    # 出力名が'output'でない場合があるため、動的に取得して設定
    output_name = mlmodel.outputs[0].name
    mlmodel.output_description[output_name] = '256x256 segmentation mask.'

    # 4. 変換後のモデルを保存
    mlmodel.save(OUTPUT_COREML_MODEL)

    print(f"変換成功！ モデルを {OUTPUT_COREML_MODEL} に保存しました。")

except Exception as e:
    print(f"\n変換中にエラーが発生しました:")
    print(e)
    sys.exit(1)