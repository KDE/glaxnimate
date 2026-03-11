/*
 * SPDX-FileCopyrightText: 2019-2026 Mattia Basaglia <dev@dragon.best>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "glaxnimate/model/animation/animatable.hpp"

namespace glaxnimate::io {

/**
 * \brief Splits keyframes to avoid overshooting [0-1] easing values
 */
std::vector<std::unique_ptr<model::KeyframeBase>> split_keyframes(model::AnimatableBase* prop);

} // namespace glaxnimate::io
