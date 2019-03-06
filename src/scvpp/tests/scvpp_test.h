/*
 * Copyright (c) 2019 Cisco
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SCVPP_TEST_H
#define __SCVPP_TEST_H__

#include <cmocka.h>

#define IFACE_TEST_SIZE 6
extern const struct CMUnitTest iface_tests[IFACE_TEST_SIZE];

#define IP_TEST_SIZE 3
extern const struct CMUnitTest ip_tests[IP_TEST_SIZE];

#define NAT_TEST_SIZE 2
extern const struct CMUnitTest nat_tests[NAT_TEST_SIZE];

#endif /* __SCVPP_TEST_H__ */
