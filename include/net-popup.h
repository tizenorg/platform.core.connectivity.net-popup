/*
*  net-popup
*
* Copyright 2012-2013  Samsung Electronics Co., Ltd
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#ifndef __DEF_NET_POPUP_H__
#define __DEF_NET_POPUP_H__

#include <dlog.h>
#include <Elementary.h>

#define PACKAGE		"net-popup"
#define ALERT_STR_LEN_MAX	100

#define NET_POPUP	"NET_POPUP"
#define log_print(tag, format, args...) LOG(LOG_DEBUG, \
		tag, "%s:%d "format, __func__, __LINE__, ##args)

#endif /* __DEF_NET_POPUP_H__ */
