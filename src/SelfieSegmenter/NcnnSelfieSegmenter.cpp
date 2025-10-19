/*
Live Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "NcnnSelfieSegmenter.hpp"

#include "ShapeConverter.hpp"

namespace KaitoTokyo {
namespace SelfieSegmenter {

void NcnnSelfieSegmenter::process(const std::uint8_t *bgraData)
{
	if (!bgraData) {
		throw std::invalid_argument("bgraData is null");
	}

	copy_r8_bgra_to_float_chw(inputMat.channel(0), inputMat.channel(1), inputMat.channel(2), bgraData,
				  getPixelCount());

	ncnn::Extractor ex = selfieSegmenterNet.create_extractor();
	ex.input("in0", inputMat);
	ex.extract("out0", outputMat);

	maskBuffer.write([this](std::vector<std::uint8_t> &mask) {
		copy_float32_to_r8(mask.data(), outputMat.channel(0), getPixelCount());
	});
}

} // namespace SelfieSegmenter
} // namespace KaitoTokyo
