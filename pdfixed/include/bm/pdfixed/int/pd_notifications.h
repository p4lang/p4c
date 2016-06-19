/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#ifndef BM_PDFIXED_INT_PD_NOTIFICATIONS_H_
#define BM_PDFIXED_INT_PD_NOTIFICATIONS_H_

typedef void (*NotificationCb)(const char *hdr, const char *data);

int pd_notifications_add_device(int dev_id, const char *notifications_addr,
                                NotificationCb ageing_cb,
                                NotificationCb learning_cb);

int pd_notifications_remove_device(int dev_id);

#endif  // BM_PDFIXED_INT_PD_NOTIFICATIONS_H_
