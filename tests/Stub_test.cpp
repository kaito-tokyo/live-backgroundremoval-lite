/*
OBS Background Removal Lite
Copyright (C) 2025 Kaito Udagawa umireon@kaito.tokyo

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <gtest/gtest.h>

#include <iostream>
#include <onnxruntime_cxx_api.h>

TEST(StubTest, Stub)
{
    Ort::Env env(ORT_LOGGING_LEVEL_VERBOSE, "test");
    Ort::SessionOptions session_options;
    Ort::Session session(env, "data/models/model.onnx", session_options);
}
