/*
 * Copyright 2020 Grant Elliott <grant@grantelliott.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <future>
#include <memory>

#include "audience.hpp"
#include "musician.hpp"

namespace audiogene {

using KeyMap = std::map<std::string, std::map<std::string, std::string>>;

class Performance {
    std::shared_ptr<spdlog::logger> logger;
    YAML::Node config;

    std::shared_ptr<audiogene::Audience> audience;
    std::unique_ptr<audiogene::Musician> musician;

    void seatAudience();
    void assembleMusicians();
    
public:
    Performance(const YAML::Node config);
    Performance(const Performance&) = delete;
    Performance& operator=(Performance const&) = delete;

    std::future<void> play();
};

}  // namespace audiogene
