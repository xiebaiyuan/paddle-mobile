/* Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#include <fstream>
#include <iostream>
#include "../test_helper.h"
#include "../test_include.h"

const int max_run_times = 10;

int main(int argc, char **argv) {
  if (argc < 3) {
    std::cerr
        << "Usage: ./test_ocr [detect_model_dir|recog_model_dir] image_path"
        << std::endl;
    return 1;
  }
  std::string model_dir = argv[1];
  std::string image_path = argv[2];

  // init input, output params
  std::vector<float> input_vec;
  std::vector<int64_t> input_shape;
  std::vector<std::string> output_fetch_nodes;
  int PRINT_NODE_ELEM_NUM = 10;

  input_shape.emplace_back(1);
  input_shape.emplace_back(3);
  input_shape.emplace_back(192);
  input_shape.emplace_back(192);
  output_fetch_nodes.emplace_back("detection_output_0.tmp_0");
  std::shared_ptr<framework::LoDTensor> outputs[output_fetch_nodes.size()];

  // init paddle instance
  paddle_mobile::PaddleMobile<paddle_mobile::CPU> paddle_mobile;
  paddle_mobile.SetThreadNum(1);
  std::cout << "start load " << std::endl;
  auto load_success = paddle_mobile.Load(std::string(model_dir) + "/model",
                                         std::string(model_dir) + "/params",
                                         true, false, 1, true);
  std::cout << "load_success:" << load_success << std::endl;
  // input image raw tensor, generated by
  // [scripts](tools/python/imagetools/img2nchw.py)
  std::cout << "image_path: " << image_path << std::endl;
  std::cout << "input_shape: " << input_shape[0] << ", " << input_shape[1]
            << ", " << input_shape[2] << ", " << input_shape[3] << std::endl;
  GetInput<float>(image_path, &input_vec, input_shape);

  // model predict
  auto pred_start_time = paddle_mobile::time();
  for (int run_idx = 0; run_idx < max_run_times; ++run_idx) {
    paddle_mobile.Predict(input_vec, input_shape);
    for (int out_idx = 0; out_idx < output_fetch_nodes.size(); ++out_idx) {
      auto fetch_name = output_fetch_nodes[out_idx];
      outputs[out_idx] = paddle_mobile.Fetch(fetch_name);
    }
  }
  auto pred_end_time = paddle_mobile::time();

  // inference time
  double pred_time =
      paddle_mobile::time_diff(pred_start_time, pred_end_time) / max_run_times;
  std::cout << "predict time(ms): " << pred_time << std::endl;

  // output result
  for (int out_idx = 0; out_idx < output_fetch_nodes.size(); ++out_idx) {
    std::string node_id = output_fetch_nodes[out_idx];
    auto node_lod_tensor = outputs[out_idx];
    int node_elem_num = node_lod_tensor->numel();
    float *node_ptr = node_lod_tensor->data<float>();
    std::cout << "==== output_fetch_nodes[" << out_idx
              << "] =====" << std::endl;
    std::cout << "node_id: " << node_id << std::endl;
    std::cout << "node_elem_num: " << node_elem_num << std::endl;
    std::cout << "PRINT_NODE_ELEM_NUM: " << PRINT_NODE_ELEM_NUM << std::endl;
    PRINT_NODE_ELEM_NUM =
        (node_elem_num > PRINT_NODE_ELEM_NUM) ? PRINT_NODE_ELEM_NUM : 0;
    for (int eidx = 0; eidx < PRINT_NODE_ELEM_NUM; ++eidx) {
      std::cout << node_id << "[" << eidx << "]: " << node_ptr[eidx]
                << std::endl;
    }
    std::cout << std::endl;
  }

  return 0;
}
