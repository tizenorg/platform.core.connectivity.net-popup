/*
*  net-popup
*
* Copyright 2012  Samsung Electronics Co., Ltd
*
* Licensed under the Flora License, Version 1.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.tizenopensource.org/license
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
#include <appsvc.h>
#include <notification.h>
#include <syspopup.h>
#include <syspopup_caller.h>

#include "net-popup.h"
#include "net-popup-strings.h"

static void __net_popup_wifi_add_notification(void *data);
static void __net_popup_wifi_del_notification(void *data);
static int __net_popup_show_tickernoti(bundle *b, void *data);
static int __net_popup_show_popup(bundle *b, void *data);

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
	log_print(NET_POPUP, "__net_popup_reset()\n");

	const char* type = bundle_get_val(b, "_SYSPOPUP_TYPE_");

	if (type == NULL) {
		elm_exit();
		return 0;
	}

	log_print(NET_POPUP, "type = %s\n", type);

	if (g_str_equal(type, "wifi_notification")) {
		const char* mode = bundle_get_val(b, "_SYSPOPUP_CONTENT_");
		if (mode == NULL) {
			elm_exit();
			return 0;
		}

		if (g_str_equal(mode, "add")) {
			__net_popup_wifi_add_notification(data);
		} else if(g_str_equal(mode, "delete")) {
			__net_popup_wifi_del_notification(data);
		} else {
			log_print(NET_POPUP, "error mode = %s\n", mode);
		}

		elm_exit();
	} else if (g_str_equal(type, "notification")) {
		__net_popup_show_tickernoti(b, data);
		elm_exit();
	} else if (g_str_equal(type, "popup")) {
		__net_popup_show_popup(b, data);
	} else {
		__net_popup_show_tickernoti(b, data);
		elm_exit();
	}

	return 0;
}

static void _ok_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (data)
		evas_object_del(data);
	elm_exit();
}

static void __net_popup_wifi_add_notification(void *data)
{
	int noti_flags = 0;
	notification_h noti = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	bundle *b = NULL;

	noti = notification_new(NOTIFICATION_TYPE_NOTI, NOTIFICATION_GROUP_ID_NONE, NETPOPUP_NOTIFICATION_WIFI_FOUND_PRIV_ID);
	if (noti == NULL) {
		log_print(NET_POPUP, "fail to create notification");
		return;
	}

	noti_err = notification_set_time(noti, time(NULL));
	if(noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_time : %d", noti_err);
		return;
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, NETPOPUP_NOTIFICATION_WIFI_ICON);
	if(noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_image : %d", noti_err);
		return;
	}

	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE, NOTIFICATION_STR_WIFI_FOUND_TITLE, NOTIFICATION_STR_WIFI_FOUND_TITLE, NOTIFICATION_VARIABLE_TYPE_NONE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_title : %d", noti_err);
		return;
	}

	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT, NOTIFICATION_STR_WIFI_FOUND_CONTENT, NOTIFICATION_STR_WIFI_FOUND_CONTENT, NOTIFICATION_VARIABLE_TYPE_NONE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_content : %d", noti_err);
		return;
	}

	noti_flags = NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY;
	noti_err = notification_set_display_applist(noti, noti_flags);
	if(noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_display_applist : %d", noti_err);
		return;
	}

	b = bundle_create();
	bundle_add(b, "_INTERNAL_SYSPOPUP_NAME_", "wifi-qs");
	appsvc_set_pkgname(b, "net.wifi-qs");
	noti_err = notification_set_execute_option(noti, NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH, "Launch", NULL, b);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_execute_option");
		return;
	}

	noti_err = notification_insert(noti, NULL);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_insert");
		return;
	}

	noti_err = notification_free(noti);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_free");
		return;
	}

	log_print(NET_POPUP, "Successfuly added notification");
}

static void __net_popup_wifi_del_notification(void *data)
{
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	noti_err = notification_delete_by_priv_id(NULL, NOTIFICATION_TYPE_NOTI, NETPOPUP_NOTIFICATION_WIFI_FOUND_PRIV_ID);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_delete_by_priv_id");
	}

	log_print(NET_POPUP, "Successfuly deleted notification");
}

static int __net_popup_show_tickernoti(bundle *b, void *data)
{
	bundle *b_noti = NULL;

	b_noti = bundle_create();
	bundle_add(b_noti, "2", "1");
	bundle_add(b_noti, "3", "3");

	const char* mode = bundle_get_val(b, "_SYSPOPUP_CONTENT_");

	if (mode == NULL) {
		bundle_free(b_noti);
		return 0;
	}

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

		if (ap_name != NULL)
			snprintf(buf, ALERT_STR_LEN_MAX, "%s  %s", ap_name, ALERT_STR_WIFI_CONNECTED);
		else
			snprintf(buf, ALERT_STR_LEN_MAX, "%s", ALERT_STR_WIFI_CONNECTED);

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

static int __net_popup_show_popup(bundle *b, void *data)
{
	Evas_Object *win;
	Evas_Object *popup;
	Evas_Object *button;
	int w, h;

	const char* mode = bundle_get_val(b, "_SYSPOPUP_CONTENT_");

	if (mode == NULL) {
		elm_exit();
		return 0;
	}

	log_print(NET_POPUP, "content = %s\n", mode);

	win = elm_win_add(NULL, PACKAGE, ELM_WIN_BASIC);
	elm_win_alpha_set(win, EINA_TRUE);
	elm_win_borderless_set(win, EINA_TRUE);
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
	evas_object_resize(win, w, h);

	popup = elm_popup_add(win);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	if (strcmp(mode, "connected") == 0) {
		elm_object_text_set(popup, ALERT_STR_3G_CHARGE);
		log_print(NET_POPUP, "alert 3g\n");
	} else if (strcmp(mode, "fail to connect") == 0) {
		elm_object_text_set(popup, ALERT_STR_ERR_UNAVAILABLE);
		log_print(NET_POPUP, "alert err\n");
	} else if (strcmp(mode, "unable to connect") == 0) {
		elm_object_text_set(popup, ALERT_STR_ERR_CONNECT);
		log_print(NET_POPUP, "alert unable to connect\n");
	} else if (strcmp(mode, "not support") == 0) {
		elm_object_text_set(popup, ALERT_STR_ERR_NOT_SUPPORT);
		log_print(NET_POPUP, "alert not support\n");
	} else if (strcmp(mode, "wifi restricted") == 0) {
		elm_object_text_set(popup, ALERT_STR_RESTRICTED_USE_WIFI);
		log_print(NET_POPUP, "alert wifi restricted\n");
	} else if (strcmp(mode, "wifi connected") == 0) {
		char buf[ALERT_STR_LEN_MAX] = "";
		char *ap_name = bundle_get_val(b, "_AP_NAME_");

		if (ap_name != NULL)
			snprintf(buf, ALERT_STR_LEN_MAX, "%s  %s", ap_name, ALERT_STR_WIFI_CONNECTED);
		else
			snprintf(buf, ALERT_STR_LEN_MAX, "%s", ALERT_STR_WIFI_CONNECTED);

		elm_object_text_set(popup, buf);

		log_print(NET_POPUP, "alert wifi connected\n");
	} else {
		elm_object_text_set(popup, mode);
		log_print(NET_POPUP, "%s\n", mode);
	}

	button = elm_button_add(popup);
	elm_object_text_set(button, "OK");
	elm_object_part_content_set(popup, "button1", button);
	evas_object_smart_callback_add(button, "clicked", _ok_button_clicked_cb, popup);
	evas_object_show(popup);
	evas_object_show(win);

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

