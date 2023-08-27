/* Conquer Space
 * Copyright (C) 2021-2023 Conquer Space
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

#include "engine/asset/asset.h"

namespace cqsp {
namespace asset {
class AudioAsset : public Asset {
 public:
    /**
     * Length in seconds.
     */
    virtual float Length() = 0;

    virtual ~AudioAsset() = default;

    AssetType GetAssetType() override { return AssetType::AUDIO; }
};
}  // namespace asset
}  // namespace cqsp
