/*
*  net-popup
*
* Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
*
* Contact: Sanghoon Cho <sanghoon80.cho@samsung.com>
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



#include <stdio.h>
#include <appcore-efl.h>
#include <syspopup.h>
#include <syspopup_caller.h>

#include "net-popup.h"
#include "net-popup-strings.h"


static int __net_popup_term(bundle *b, void *data)
{
	log_print(NET_POPUP, "\nnet-popup: terminate");
	return 0;
}

static int __net_popup_timeout(bundle *b, void *data)
{
	log_print(NET_POPUP, "\nnet-popup: timeout");
	return 0;
}

syspopup_handler handler = {
	.def_term_fn = __net_popup_term,
	.def_timeout_fn = __net_popup_timeout
};

static int __net_popup_create(void *data)
{
	log_print(NET_POPUP, "__net_popup_create()\n");

	bindtextdomain(PACKAGE, LOCALEDIR);

	return 0;
}

static int __net_popup_terminate(void *data)
{
	return 0;
}

static int __net_popup_pause(void *data)
{
	return 0;
}

static int __net_popup_resume(void *data)
{
	return 0;
}

static int __net_popup_reset(bundle *b, void *data)
{
	int ret = 0;
	bundle *b_noti = NULL;

	log_print(NET_POPUP, "__net_popup_reset()\n");

	b_noti = bundle_create();
	bundle_add(b_noti, "2", "1");
	bundle_add(b_noti, "3", "3");

	const char* mode = bundle_get_val(b, "_SYSPOPUP_CONTENT_");
	log_print(NET_POPUP, "content = %s\n", mode);
	
	if (strcmp(mode, "connected") == 0) {
		bundle_add(b_noti, "1", ALERT_STR_3G_CHARGE);
		log_print(NET_POPUP, "alert 3g\n");
	} else if (strcmp(mode, "fail to connect") == 0) {
		bundle_add(b_noti, "1", ALERT_STR_ERR_UNAVAILABLE);
		log_print(NET_POPUP, "alert err\n");
	} else if (strcmp(mode, "unable to connect") == 0) {
		bundle_add(b_noti, "1", ALERT_STR_ERR_CONNECT);
		log_print(NET_POPUP, "alert unable to connect\n");
	} else if (strcmp(mode, "not support") == 0) {
		bundle_add(b_noti, "1", ALERT_STR_ERR_NOT_SUPPORT);
		log_print(NET_POPUP, "alert not support\n");
	} else if (strcmp(mode, "wifi restricted") == 0) {
		bundle_add(b_noti, "1", ALERT_STR_RESTRICTED_USE_WIFI);
		log_print(NET_POPUP, "alert wifi restricted\n");
	} else if (strcmp(mode, "wifi connected") == 0) {
		char buf[ALERT_STR_LEN_MAX] = "";
		char *ap_name = bundle_get_val(b, "_AP_NAME_");
		snprintf(buf, ALERT_STR_LEN_MAX, "%s  %s", ap_name, ALERT_STR_WIFI_CONNECTED);
		bundle_add(b_noti, "1", buf);
		log_print(NET_POPUP, "alert wifi connected\n");
	} else {
		bundle_add(b_noti, "1", mode);
		log_print(NET_POPUP, "%s\n", mode);
	}

	syspopup_launch("tickernoti-syspopup", b_noti);
	bundle_free(b_noti);

	return 0;
}

int main(int argc, char *argv[])
{
	struct appcore_ops ops = {
		.create = __net_popup_create,
		.terminate = __net_popup_terminate,
		.pause = __net_popup_pause,
		.resume = __net_popup_resume,
		.reset = __net_popup_reset,
	};

	return appcore_efl_main(PACKAGE, &argc, &argv, &ops);
}

