extern "C" const char mediapipe_selfie_segmentation_landscape_int8_ncnn_param_text[] = R"(
7767517
114 138
Input                    in0                      0 1 in0
Convolution              padconv_0                1 1 in0 2 0=16 1=3 3=2 15=1 16=1 5=1 6=432 8=2 9=6 -23310=2,1.666667e-01,5.000000e-01
Split                    splitncnn_0              1 3 2 3 4 5
Convolution              convrelu_0               1 1 5 6 0=16 1=1 5=1 6=256 8=102 9=1
ConvolutionDepthWise     convdwrelu_0             1 1 6 7 0=16 1=3 3=2 15=1 16=1 5=1 6=144 7=16 8=1 9=1
Split                    splitncnn_1              1 2 7 8 9
Reduction                mean_95                  1 1 9 10 0=3 1=0 -23303=2,1,2 4=1 5=1
Convolution              convrelu_1               1 1 10 11 0=8 1=1 5=1 6=128 8=102 9=1
Convolution              convsigmoid_16           1 1 11 12 0=16 1=1 5=1 6=128 8=2 9=4
BinaryOp                 mul_0                    2 1 8 12 13 0=2
Convolution              conv_55                  1 1 13 14 0=16 1=1 5=1 6=256 8=2
Split                    splitncnn_2              1 3 14 15 16 17
Convolution              convrelu_2               1 1 17 18 0=72 1=1 5=1 6=1152 8=102 9=1
ConvolutionDepthWise     convdwrelu_1             1 1 18 19 0=72 1=3 3=2 15=1 16=1 5=1 6=648 7=72 8=101 9=1
Convolution              conv_57                  1 1 19 20 0=24 1=1 5=1 6=1728 8=2
Split                    splitncnn_3              1 2 20 21 22
Convolution              convrelu_3               1 1 22 23 0=88 1=1 5=1 6=2112 8=102 9=1
ConvolutionDepthWise     convdwrelu_2             1 1 23 24 0=88 1=3 4=1 5=1 6=792 7=88 8=101 9=1
Convolution              conv_59                  1 1 24 25 0=24 1=1 5=1 6=2112 8=2
BinaryOp                 add_1                    2 1 21 25 26
Split                    splitncnn_4              1 3 26 27 28 29
Convolution              conv_60                  1 1 29 31 0=96 1=1 5=1 6=2304 8=102 9=6 -23310=2,1.666667e-01,5.000000e-01
ConvolutionDepthWise     padconvdw_2              1 1 31 33 0=96 1=5 3=2 4=1 14=0 15=2 16=2 5=1 6=2400 7=96 8=1 9=6 -23310=2,1.666667e-01,5.000000e-01
Split                    splitncnn_5              1 2 33 34 35
Reduction                mean_96                  1 1 35 36 0=3 1=0 -23303=2,1,2 4=1 5=1
Convolution              convrelu_4               1 1 36 37 0=24 1=1 5=1 6=2304 8=102 9=1
Convolution              convsigmoid_17           1 1 37 38 0=96 1=1 5=1 6=2304 8=2 9=4
BinaryOp                 mul_2                    2 1 34 38 39 0=2
Convolution              conv_63                  1 1 39 40 0=32 1=1 5=1 6=3072 8=2
Split                    splitncnn_6              1 2 40 41 42
Convolution              conv_64                  1 1 42 44 0=128 1=1 5=1 6=4096 8=102 9=6 -23310=2,1.666667e-01,5.000000e-01
ConvolutionDepthWise     convdw_109               1 1 44 46 0=128 1=5 4=2 5=1 6=3200 7=128 8=1 9=6 -23310=2,1.666667e-01,5.000000e-01
Split                    splitncnn_7              1 2 46 47 48
Reduction                mean_97                  1 1 48 49 0=3 1=0 -23303=2,1,2 4=1 5=1
Convolution              convrelu_5               1 1 49 50 0=32 1=1 5=1 6=4096 8=102 9=1
Convolution              convsigmoid_18           1 1 50 51 0=128 1=1 5=1 6=4096 8=2 9=4
BinaryOp                 mul_3                    2 1 47 51 52 0=2
Convolution              conv_67                  1 1 52 53 0=32 1=1 5=1 6=4096 8=2
BinaryOp                 add_4                    2 1 41 53 54
Split                    splitncnn_8              1 2 54 55 56
Convolution              conv_68                  1 1 56 58 0=128 1=1 5=1 6=4096 8=102 9=6 -23310=2,1.666667e-01,5.000000e-01
ConvolutionDepthWise     convdw_110               1 1 58 60 0=128 1=5 4=2 5=1 6=3200 7=128 8=1 9=6 -23310=2,1.666667e-01,5.000000e-01
Split                    splitncnn_9              1 2 60 61 62
Reduction                mean_98                  1 1 62 63 0=3 1=0 -23303=2,1,2 4=1 5=1
Convolution              convrelu_6               1 1 63 64 0=32 1=1 5=1 6=4096 8=102 9=1
Convolution              convsigmoid_19           1 1 64 65 0=128 1=1 5=1 6=4096 8=2 9=4
BinaryOp                 mul_5                    2 1 61 65 66 0=2
Convolution              conv_71                  1 1 66 67 0=32 1=1 5=1 6=4096 8=2
BinaryOp                 add_6                    2 1 55 67 68
Split                    splitncnn_10             1 2 68 69 70
Convolution              conv_72                  1 1 70 72 0=96 1=1 5=1 6=3072 8=102 9=6 -23310=2,1.666667e-01,5.000000e-01
ConvolutionDepthWise     convdw_111               1 1 72 74 0=96 1=5 4=2 5=1 6=2400 7=96 8=1 9=6 -23310=2,1.666667e-01,5.000000e-01
Split                    splitncnn_11             1 2 74 75 76
Reduction                mean_99                  1 1 76 77 0=3 1=0 -23303=2,1,2 4=1 5=1
Convolution              convrelu_7               1 1 77 78 0=24 1=1 5=1 6=2304 8=102 9=1
Convolution              convsigmoid_20           1 1 78 79 0=96 1=1 5=1 6=2304 8=2 9=4
BinaryOp                 mul_7                    2 1 75 79 80 0=2
Convolution              conv_75                  1 1 80 81 0=32 1=1 5=1 6=3072 8=2
BinaryOp                 add_8                    2 1 69 81 82
Split                    splitncnn_12             1 2 82 83 84
Convolution              conv_76                  1 1 84 86 0=96 1=1 5=1 6=3072 8=102 9=6 -23310=2,1.666667e-01,5.000000e-01
ConvolutionDepthWise     convdw_112               1 1 86 88 0=96 1=5 4=2 5=1 6=2400 7=96 8=1 9=6 -23310=2,1.666667e-01,5.000000e-01
Split                    splitncnn_13             1 2 88 89 90
Reduction                mean_100                 1 1 90 91 0=3 1=0 -23303=2,1,2 4=1 5=1
Convolution              convrelu_8               1 1 91 92 0=24 1=1 5=1 6=2304 8=102 9=1
Convolution              convsigmoid_21           1 1 92 93 0=96 1=1 5=1 6=2304 8=2 9=4
BinaryOp                 mul_9                    2 1 89 93 94 0=2
Convolution              conv_79                  1 1 94 95 0=32 1=1 5=1 6=3072 8=2
BinaryOp                 add_10                   2 1 83 95 96
Split                    splitncnn_14             1 2 96 97 98
Reduction                mean_101                 1 1 98 99 0=3 1=0 -23303=2,1,2 4=1 5=1
Convolution              convrelu_9               1 1 97 100 0=128 1=1 5=1 6=4096 8=2 9=1
Convolution              convsigmoid_22           1 1 99 101 0=128 1=1 5=1 6=4096 8=2 9=4
BinaryOp                 mul_11                   2 1 100 101 102 0=2
Interp                   interpolate_11           1 1 102 103 0=2 3=18 4=32
Convolution              conv_82                  1 1 103 104 0=24 1=1 5=1 6=3072 8=2
Split                    splitncnn_15             1 2 104 105 106
BinaryOp                 add_12                   2 1 105 27 107
Reduction                mean_102                 1 1 107 108 0=3 1=0 -23303=2,1,2 4=1 5=1
Convolution              convrelu_10              1 1 108 109 0=24 1=1 5=1 6=576 8=102 9=1
Convolution              convsigmoid_23           1 1 109 110 0=24 1=1 5=1 6=576 8=2 9=4
BinaryOp                 mul_13                   2 1 28 110 111 0=2
BinaryOp                 add_14                   2 1 106 111 112
Convolution              convrelu_11              1 1 112 113 0=24 1=1 5=1 6=576 8=2 9=1
Split                    splitncnn_16             1 2 113 114 115
ConvolutionDepthWise     convdwrelu_3             1 1 115 116 0=24 1=3 4=1 5=1 6=216 7=24 8=1 9=1
BinaryOp                 add_15                   2 1 114 116 117
Interp                   interpolate_12           1 1 117 118 0=2 3=36 4=64
Convolution              conv_86                  1 1 118 119 0=16 1=1 5=1 6=384 8=2
Split                    splitncnn_17             1 2 119 120 121
BinaryOp                 add_16                   2 1 120 15 122
Reduction                mean_103                 1 1 122 123 0=3 1=0 -23303=2,1,2 4=1 5=1
Convolution              convrelu_12              1 1 123 124 0=16 1=1 5=1 6=256 8=102 9=1
Convolution              convsigmoid_24           1 1 124 125 0=16 1=1 5=1 6=256 8=2 9=4
BinaryOp                 mul_17                   2 1 16 125 126 0=2
BinaryOp                 add_18                   2 1 121 126 127
Convolution              convrelu_13              1 1 127 128 0=16 1=1 5=1 6=256 8=2 9=1
Split                    splitncnn_18             1 2 128 129 130
ConvolutionDepthWise     convdwrelu_4             1 1 130 131 0=16 1=3 4=1 5=1 6=144 7=16 8=1 9=1
BinaryOp                 add_19                   2 1 129 131 132
Interp                   interpolate_13           1 1 132 133 0=2 3=72 4=128
Convolution              conv_90                  1 1 133 134 0=16 1=1 5=1 6=256 8=2
Split                    splitncnn_19             1 2 134 135 136
BinaryOp                 add_20                   2 1 135 3 137
Reduction                mean_104                 1 1 137 138 0=3 1=0 -23303=2,1,2 4=1 5=1
Convolution              convrelu_14              1 1 138 139 0=16 1=1 5=1 6=256 8=102 9=1
Convolution              convsigmoid_25           1 1 139 140 0=16 1=1 5=1 6=256 8=2 9=4
BinaryOp                 mul_21                   2 1 4 140 141 0=2
BinaryOp                 add_22                   2 1 136 141 142
Convolution              convrelu_15              1 1 142 143 0=16 1=1 5=1 6=256 8=2 9=1
Split                    splitncnn_20             1 2 143 144 145
ConvolutionDepthWise     convdwrelu_5             1 1 145 146 0=16 1=3 4=1 5=1 6=144 7=16 8=1 9=1
BinaryOp                 add_23                   2 1 144 146 147
Deconvolution            deconvsigmoid_0          1 1 147 out0 0=1 1=2 3=2 5=1 6=64 9=4
)";
