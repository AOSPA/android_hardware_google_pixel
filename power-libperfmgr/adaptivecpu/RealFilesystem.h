#pragma once

/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <memory>
#include <ostream>
#include <vector>

#include "IFilesystem.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

class RealFilesystem : public IFilesystem {
  public:
    virtual ~RealFilesystem() {}
    std::vector<std::string> listDirectory(const std::string &path) const override;
    std::unique_ptr<std::istream> readFileStream(const std::string &path) const override;
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
