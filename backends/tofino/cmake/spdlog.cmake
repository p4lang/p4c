# SPDX-FileCopyrightText: 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

p4c_find_package(
  NAME spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.8.3
  EXCLUDE_FROM_ALL YES
  SYSTEM YES
  OPTIONS "SPDLOG_INSTALL OFF"
)
