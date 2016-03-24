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

#include <glib.h>
#include <stdio.h>
#if 0
#include <Ecore_X.h>
#endif
#include <syspopup.h>
#include <notification.h>
#include <notification_list.h>
#include <notification_text_domain.h>
#include <app.h>
#include <appsvc.h>
#include <vconf.h>
#include <vconf-keys.h>
#include <gio/gio.h>
#include <dbus/dbus.h>
#include <efl_extension.h>
#include <tzplatform_config.h>

#include "net-popup.h"
#include "net-popup-strings.h"

#define LOCALEDIR			"/usr/share/locale"
#define NETPOPUP_EDJ 			tzplatform_mkpath(TZ_SYS_RO_UG, "/res/edje/net-popup/netpopup-custom.edj")
#define QP_PRELOAD_NOTI_ICON_PATH	tzplatform_mkpath(TZ_SYS_RO_APP, "/org.tizen.quickpanel/shared/res/noti_icons/Wi-Fi")

#define NETCONFIG_NOTIFICATION_WIFI_ICON \
					tzplatform_mkpath(TZ_SYS_RO_ICONS, "/noti_wifi_in_range.png")
#define NETCONFIG_NOTIFICATION_WIFI_ICON_LITE \
					tzplatform_mkpath(TZ_SYS_RO_ICONS, "/noti_wifi_in_range_ongoing.png")
#define NETCONFIG_NOTIFICATION_WIFI_CAPTIVE_ICON \
					tzplatform_mkpath(TZ_SYS_RO_ICONS, "/B03_notify_Wi-fi_range.png")
#define NETCONFIG_NOTIFICATION_WIFI_IN_RANGE_ICON \
					tzplatform_mkpath(TZ_SYS_RO_ICONS, "/Q02_Notification_wifi_in_range.png")
#define NETCONFIG_NOTIFICATION_WIFI_IN_RANGE_ICON_LITE \
					tzplatform_mkpath(TZ_SYS_RO_ICONS, "/noti_wifi_in_range.png")
#define NETCONFIG_NOTIFICATION_WIFI_FOUND_TITLE \
		dgettext(PACKAGE, "IDS_COM_BODY_WI_FI_NETWORKS_AVAILABLE")
#define NETCONFIG_NOTIFICATION_WIFI_FOUND_CONTENT \
		dgettext(PACKAGE, "IDS_WIFI_SBODY_TAP_HERE_TO_CONNECT")
#define NETCONFIG_NOTIFICATION_WIFI_PORTAL_TITLE \
			dgettext(PACKAGE, "IDS_WIFI_HEADER_SIGN_IN_TO_WI_FI_NETWORK_ABB")
#define NETCONFIG_NOTIFICATION_WIFI_PORTAL_CONTENT "\"%s\""

#define USER_RESP_LEN 30
#define ICON_PATH_LEN 128
#define RESP_REMAIN_CONNECTED "RESP_REMAIN_CONNECTED"
#define RESP_WIFI_TETHERING_OFF "RESP_TETHERING_TYPE_WIFI_OFF"
#define RESP_WIFI_AP_TETHERING_OFF "RESP_TETHERING_TYPE_WIFI_AP_OFF"
#define RESP_TETHERING_ON "RESP_TETHERING_ON"
#define CAPTIVE_PORTAL_LOGIN "Login required to access Internet"
#define CAPTIVE_PORTAL_LOGIN_ERROR "Login not completed. Disconnected active Wifi"

#define ALERT_STR_ENABLE_VPN "Enable VPN"

static Ecore_Event_Handler *ecore_event_evas_handler;
static Ecore_Event_Handler *ecore_event_evas_quick_panel_handler;

#define BUF_SIZE 1024
long sizes[] = {1073741824, 1048576, 1024, 0};
char *units[] = {"GB", "MB", "KB", "B"};

static app_control_h g_req_handle = NULL;
static char * resp_popup_mode = NULL;

static GDBusConnection *conn = NULL;
static GDBusProxy *proxy = NULL;

static int __net_popup_show_notification(app_control_h request, void *data);
static int __toast_popup_show(app_control_h request, void *data);
static int __net_popup_show_popup(app_control_h request, void *data);
static void __net_popup_add_found_ap_noti(void);
static void __net_popup_del_found_ap_noti(void);
static void __net_popup_add_portal_noti(app_control_h request);
static void __net_popup_del_portal_noti(void);
static void __net_popup_show_popup_with_user_resp(app_control_h request, void *data);
static void __net_popup_show_popup_for_vpn_service(app_control_h request, void *data);
static int _net_popup_send_user_resp(char *resp, Eina_Bool state);


GDBusProxy *__net_popup_init_dbus(void)
{
	log_print(NET_POPUP, "__net_popup_init_dbus()\n");

	GError *err = NULL;

	conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
	if (err != NULL) {
		g_error_free(err);
		return NULL;
	}

	proxy = g_dbus_proxy_new_sync(conn, G_DBUS_PROXY_FLAGS_NONE, NULL,
			"net.netpopup",
			"/Netpopup",
			"net.netpopup",
			NULL, &err);

	if (proxy == NULL) {
		g_object_unref(conn);
		conn = NULL;
	}

	return proxy;
}

void __net_popup_deinit_dbus(void)
{
	log_print(NET_POPUP, "__net_popup_deinit_dbus()\n");

	if (proxy) {
		g_object_unref(proxy);
		proxy = NULL;
	}

	if (conn) {
		g_object_unref(conn);
		conn = NULL;
	}

	return;
}

int __net_popup_send_dbus_msg(const char *resp)
{
	log_print(NET_POPUP, "__net_popup_send_dbus_msg()\n");

	if (conn == NULL || resp == NULL) {
		return -1;
	}

	GDBusConnection *gconn = NULL;
	GVariant *msg = NULL;
	char *module = "wifi";
	GError *err = NULL;

	gconn = g_bus_get_sync(DBUS_BUS_SYSTEM, NULL, &err);
	if (err != NULL) {
		g_error_free(err);
		err = NULL;
		return -1;
	}

	msg = g_variant_new("(ss)", module, resp);
	g_dbus_connection_emit_signal(gconn, NULL, "/Org/Tizen/Quickpanel",
			"org.tizen.quickpanel", "ACTIVITY", msg, &err);
	if (err) {
		g_error_free(err);
		return -1;
	}

	g_variant_unref(msg);

	if (gconn)
		g_object_unref(gconn);


	return 0;
}

static bool __net_popup_create(void *data)
{
	log_print(NET_POPUP, "__net_popup_create()\n");

	bindtextdomain(PACKAGE, LOCALEDIR);

	return true;
}

static void __net_popup_terminate(void *data)
{
	log_print(NET_POPUP, "__net_popup_terminate()\n");

	if (ecore_event_evas_handler) {
		ecore_event_handler_del(ecore_event_evas_handler);
		ecore_event_evas_handler = NULL;
	}
	if (ecore_event_evas_quick_panel_handler) {
		ecore_event_handler_del(ecore_event_evas_quick_panel_handler);
		ecore_event_evas_quick_panel_handler = NULL;
	}

	return;
}

static void __net_popup_pause(void *data)
{
	log_print(NET_POPUP, "__net_popup_pause()\n");
}

static void __net_popup_resume(void *data)
{
	return;
}

static Eina_Bool __key_release_event_cb(void *data, int type,
		void *event)
{
	log_print(NET_POPUP, "__key_release_event_cb()\n");

	Evas_Event_Key_Down *ev = (Evas_Event_Key_Down *) event;

	if (!ev) {
		return ECORE_CALLBACK_RENEW;
	}

	if (!ev->keyname) {
		return ECORE_CALLBACK_RENEW;
	}

	log_print(NET_POPUP, "key_release : %s", ev->keyname);
	if (g_strcmp0(ev->keyname, "XF86Phone") == 0 ||
			g_strcmp0(ev->keyname, "XF86Stop") == 0) {
		elm_exit();
	}

	return ECORE_CALLBACK_DONE;
}

#if 0
static Eina_Bool _ecore_event_client_message_cb(void *data, int type,
						 void *event)
{
	Ecore_X_Event_Client_Message *ev = event;

	if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE) {
		if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_OFF) {
			log_print(NET_POPUP, "ECORE_X_ATOM_E_ILLUME_QUICKPANEL_OFF");
		} else if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_ON) {
			log_print(NET_POPUP, "ECORE_X_ATOM_E_ILLUME_QUICKPANEL_ON");
		}
	}
	return ECORE_CALLBACK_RENEW;
}
#endif

static void __net_popup_service_cb(app_control_h request, void *data)
{
	log_print(NET_POPUP, "__net_popup_service_cb()\n");

	int ret = 0;
	char *type = NULL;

	if (ecore_event_evas_handler == NULL) {
		ecore_event_evas_handler = ecore_event_handler_add(ECORE_EVENT_KEY_UP,
				__key_release_event_cb, NULL);
	}

#if 0
	if (ecore_event_evas_quick_panel_handler == NULL) {
		ecore_event_evas_quick_panel_handler = ecore_event_handler_add(
				ECORE_X_EVENT_CLIENT_MESSAGE, _ecore_event_client_message_cb, NULL);
	}
#endif

	ret = app_control_get_extra_data(request, "_SYSPOPUP_TYPE_", &type);

	if (APP_CONTROL_ERROR_NONE != ret) {
		log_print(NET_POPUP, "Failed to get _SYSPOPUP_TYPE_ ret = %d", ret);
		g_free(type);
		elm_exit();
		return;
	}

	log_print(NET_POPUP, "type = %s\n", type);

	if (g_str_equal(type, "notification")) {
		__net_popup_show_notification(request, data);
		elm_exit();
	} else if (g_str_equal(type, "toast_popup")) {
		__toast_popup_show(request, data);
	} else if (g_str_equal(type, "popup")) {
		__net_popup_show_popup(request, data);
	} else if (g_str_equal(type, "add_found_ap_noti")) {
		__net_popup_add_found_ap_noti();
		elm_exit();
	} else if (g_str_equal(type, "del_found_ap_noti")) {
		__net_popup_del_found_ap_noti();
		elm_exit();
	} else if (g_str_equal(type, "add_portal_noti")) {
		__net_popup_add_portal_noti(request);
		elm_exit();
	} else if (g_str_equal(type, "del_portal_noti")) {
		__net_popup_del_portal_noti();
		elm_exit();
	} else if (g_str_equal(type, "popup_user_resp")) {
		app_control_clone(&g_req_handle, request);
		__net_popup_show_popup_with_user_resp(request, data);
	} else if(g_str_equal(type, "popup_vpn_service")){
		app_control_clone(&g_req_handle, request);
		__net_popup_show_popup_for_vpn_service(request, data);
	}else {
		__net_popup_show_notification(request, data);
		elm_exit();
	}
	g_free(type);

	return;
}

static void __net_popup_set_orientation(Evas_Object *win)
{
	log_print(NET_POPUP, "__net_popup_set_orientation()\n");

	int rots[4] = { 0, 90, 180, 270 };

	if (!elm_win_wm_rotation_supported_get(win)) {
		return;
	}

	elm_win_wm_rotation_available_rotations_set(win, rots, 4);
}

static Evas_Object* __net_popup_create_win(void)
{
	log_print(NET_POPUP, "__net_popup_create_win()\n");

	Evas_Object *win = NULL;
	Evas *e = NULL;
	Ecore_Evas *ee = NULL;
#if 0
	int w, h;
#endif

	win = elm_win_add(NULL, PACKAGE, ELM_WIN_NOTIFICATION);

	e = evas_object_evas_get(win);
	ee = ecore_evas_ecore_evas_get(e);
	ecore_evas_name_class_set(ee,"APP_POPUP","APP_POPUP");

	elm_win_alpha_set(win, EINA_TRUE);
	elm_win_borderless_set(win, EINA_TRUE);
#if 0
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &w, &h);
	evas_object_resize(win, w, h);
	utilx_set_system_notification_level (ecore_x_display_get(),
				elm_win_xwindow_get(win),
				UTILX_NOTIFICATION_LEVEL_LOW);
#endif

	__net_popup_set_orientation(win);

	return win;
}

static void _ok_button_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
	log_print(NET_POPUP, "_ok_button_clicked_cb()\n");

	if (data)
		evas_object_del(data);
	elm_exit();
}

static void _timeout_cb(void *data, Evas_Object *obj, void *event_info)
{
	log_print(NET_POPUP, "_timeout_cb()\n");

	evas_object_del(obj);

	elm_exit();
}

static int __toast_popup_show(app_control_h request, void *data)
{
	log_print(NET_POPUP, "__toast_popup_show()\n");

	char buf[ALERT_STR_LEN_MAX] = "";
	int ret = 0;
	char *mode = NULL;
	Evas_Object *twin = NULL;
	Evas_Object *tpop = NULL;
	char *ap_name = NULL;
	char *restricted_type = NULL;

	ret = app_control_get_extra_data(request, "_SYSPOPUP_CONTENT_", &mode);
	if (ret != APP_CONTROL_ERROR_NONE) {
		log_print(NET_POPUP, "Failed to get _SYSPOPUP_CONTENT_ ret = %d", ret);
		g_free(mode);
		elm_exit();
		return 0;
	}

	log_print(NET_POPUP, "_SYSPOPUP_CONTENT_ = %s\n", mode);

	twin = __net_popup_create_win();
	tpop = elm_popup_add(twin);
	elm_object_style_set(tpop, "toast");
	evas_object_size_hint_weight_set(tpop, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	elm_popup_timeout_set(tpop, 2.0);

	evas_object_smart_callback_add(tpop, "timeout", _timeout_cb, twin);
	if (strcmp(mode, "wrong password") == 0) {
		log_print(NET_POPUP, "alert wrong password\n");

		g_snprintf(buf, ALERT_STR_LEN_MAX, ALERT_STR_ERR_WRONG_PASSWORD);
	} else if (strcmp(mode, "no ap found") == 0) {
		log_print(NET_POPUP, "alert no ap found\n");

		g_snprintf(buf, ALERT_STR_LEN_MAX, ALERT_STR_NO_AP_FOUND);
	} else if (strcmp(mode, "unable to connect") == 0) {
		log_print(NET_POPUP, "alert no ap found\n");

		g_snprintf(buf, ALERT_STR_LEN_MAX, ALERT_STR_ERR_CONNECT);
	} else if (strcmp(mode, "wifi connected") == 0) {
		ret = app_control_get_extra_data(request, "_AP_NAME_", &ap_name);

		if (APP_CONTROL_ERROR_NONE != ret) {
			log_print(NET_POPUP, "Failed to get _AP_NAME_ ret = %d", ret);
			g_free(mode);
			return 0;
		}

		if (ap_name != NULL)
			g_snprintf(buf, ALERT_STR_LEN_MAX, ALERT_STR_WIFI_CONNECTED, ap_name);
		else
			g_snprintf(buf, ALERT_STR_LEN_MAX, ALERT_STR_WIFI_CONNECTED, "");
		log_print(NET_POPUP, "alert wifi connected\n");
		g_free(ap_name);
	} else if (strcmp(mode, "security restriction") == 0) {
		ret = app_control_get_extra_data(request, "_RESTRICTED_TYPE_", &restricted_type);
		if (APP_CONTROL_ERROR_NONE != ret) {
			log_print(NET_POPUP, "Failed to get _RESTRICTED_TYPE_ ret = %d", ret);
			g_free(mode);
			return 0;
		}

		if (strcmp(restricted_type, "wifi") == 0)
			g_snprintf(buf, ALERT_STR_LEN_MAX, ALERT_STR_RESTRICTED_USE_WIFI);
		else if (strcmp(restricted_type, "wifi tethering") == 0)
			g_snprintf(buf, ALERT_STR_LEN_MAX, ALERT_STR_SECURITY_RESTRICTION, ALERT_STR_WIFI_TETHERING);
		else if (strcmp(restricted_type, "bt tethering") == 0)
			g_snprintf(buf, ALERT_STR_LEN_MAX, ALERT_STR_SECURITY_RESTRICTION, ALERT_STR_BT_TETHERING);
		else if (strcmp(restricted_type, "usb tethering") == 0)
			g_snprintf(buf, ALERT_STR_LEN_MAX, ALERT_STR_SECURITY_RESTRICTION, ALERT_STR_USB_TETHERING);

		log_print(NET_POPUP, "alert security restriction\n");
		g_free(restricted_type);
	} else
		log_print(NET_POPUP, "%s\n", mode);

	elm_object_text_set(tpop, buf);
	evas_object_show(tpop);
	evas_object_show(twin);
	g_free(mode);

	return 0;
}

static int __net_popup_show_notification(app_control_h request, void *data)
{
	log_print(NET_POPUP, "__net_popup_show_notification()\n");

	int ret = 0;
	char *mode = NULL;
	char buf[ALERT_STR_LEN_MAX] = "";
	char *ap_name = NULL;

	ret = app_control_get_extra_data(request, "_SYSPOPUP_CONTENT_", &mode);

	if (APP_CONTROL_ERROR_NONE != ret) {
		log_print(NET_POPUP, "Failed to get _SYSPOPUP_CONTENT_");
		return 0;
	}

	secure_log_print(NET_POPUP, "_SYSPOPUP_CONTENT_ = %s\n", mode);

	if (strcmp(mode, "connected") == 0) {
		notification_status_message_post(ALERT_STR_MOBILE_NETWORKS_CHARGE);
		log_print(NET_POPUP, "alert 3g\n");
	} else if (strcmp(mode, "fail to connect") == 0) {
		notification_status_message_post(ALERT_STR_ERR_UNAVAILABLE);
		log_print(NET_POPUP, "alert err\n");
	} else if (strcmp(mode, "unable to connect") == 0) {
		notification_status_message_post(ALERT_STR_ERR_CONNECT);
		log_print(NET_POPUP, "alert unable to connect\n");
	} else if (strcmp(mode, "wrong password") == 0) {
		notification_status_message_post(ALERT_STR_ERR_WRONG_PASSWORD);
		log_print(NET_POPUP, "alert wrong password\n");
	} else if (strcmp(mode, "not support") == 0) {
		notification_status_message_post(ALERT_STR_ERR_NOT_SUPPORT);
		log_print(NET_POPUP, "alert not support\n");
	} else if (strcmp(mode, "wifi restricted") == 0) {
		notification_status_message_post(ALERT_STR_RESTRICTED_USE_WIFI);
		log_print(NET_POPUP, "alert wifi restricted\n");
	} else if (strcmp(mode, "no ap found") == 0) {
		notification_status_message_post(ALERT_STR_NO_AP_FOUND);
		log_print(NET_POPUP, "alert no ap found\n");
	} else if (strcmp(mode, "Lengthy Password") == 0) {
		notification_status_message_post(ALERT_STR_LENGHTY_PASSWORD);
		log_print(NET_POPUP, "Password entered crosses 64 bytes\n");
	} else if (strcmp(mode, "Portal Login") == 0) {
		notification_status_message_post(CAPTIVE_PORTAL_LOGIN);
		log_print(NET_POPUP, "Please login to access Internet\n");
	} else if (strcmp(mode, "Portal Login Error") == 0) {
		notification_status_message_post(CAPTIVE_PORTAL_LOGIN_ERROR);
		log_print(NET_POPUP, "Login not completed. Disconnected Wifi\n");
	} else if (strcmp(mode, "wifi connected") == 0) {
		ret = app_control_get_extra_data(request, "_AP_NAME_", &ap_name);

		if (APP_CONTROL_ERROR_NONE != ret) {
			log_print(NET_POPUP, "Failed to get _AP_NAME_ ret = %d", ret);
			g_free(mode);
			return 0;
		}

		if (ap_name != NULL)
			g_snprintf(buf, ALERT_STR_LEN_MAX, ALERT_STR_WIFI_CONNECTED, ap_name);
		else
			g_snprintf(buf, ALERT_STR_LEN_MAX, ALERT_STR_WIFI_CONNECTED, "");

		notification_status_message_post(buf);

		log_print(NET_POPUP, "alert wifi connected\n");
		g_free(ap_name);
	} else {
		notification_status_message_post(mode);
		log_print(NET_POPUP, "%s\n", mode);
	}
	g_free(mode);

	return 0;
}

static int _net_popup_send_user_resp(char *resp, Eina_Bool state)
{
	log_print(NET_POPUP, "_net_popup_send_user_resp()\n");

	int ret = 0;
	app_control_h reply = NULL;
	char checkbox_str[USER_RESP_LEN] = { '\0', };

	log_print(NET_POPUP, "Send the user response to the caller");
	ret = app_control_create(&reply);
	if (APP_CONTROL_ERROR_NONE != ret) {
		log_print(NET_POPUP, "Failed to create service ret = %d", ret);
		app_control_destroy(g_req_handle);
		g_req_handle = NULL;

		return false;
	}

	if (TRUE == state)
		g_strlcpy(checkbox_str, "TRUE", USER_RESP_LEN);
	else
		g_strlcpy(checkbox_str, "FALSE", USER_RESP_LEN);

	log_print(NET_POPUP, "Checkbox_str[%s]", checkbox_str);

	ret = app_control_add_extra_data(reply, "_SYSPOPUP_RESP_", resp);
	if (APP_CONTROL_ERROR_NONE == ret) {
		ret = app_control_add_extra_data(reply, "_SYSPOPUP_CHECKBOX_RESP_",
				checkbox_str);
		if (APP_CONTROL_ERROR_NONE == ret) {
			ret = app_control_reply_to_launch_request(reply, g_req_handle,
					APP_CONTROL_RESULT_SUCCEEDED);
			if (APP_CONTROL_ERROR_NONE == ret) {
				log_print(NET_POPUP, "Service reply success");
				ret = TRUE;
			} else {
				log_print(NET_POPUP, "Service reply failed ret = %d", ret);
			}
		} else {
			log_print(NET_POPUP, "Service data addition failed ret = %d", ret);
		}
	} else {
		log_print(NET_POPUP, "Service data addition failed ret = %d", ret);
	}

	app_control_destroy(reply);
	app_control_destroy(g_req_handle);
	g_req_handle = NULL;

	return ret;
}

void _tethering_wifi_btn_yes_cb(void *data, Evas_Object *obj, void *event_info)
{
	log_print(NET_POPUP, "_tethering_wifi_btn_yes_cb()\n");

	bool result = FALSE;
	Evas_Object *popup = (Evas_Object *)data;

	__net_popup_init_dbus();
	__net_popup_send_dbus_msg("progress_off");
	__net_popup_deinit_dbus();

	result = _net_popup_send_user_resp(RESP_WIFI_TETHERING_OFF, FALSE);
	if (true != result)
		log_print(NET_POPUP, "Failed to send user response ");

	if (popup)
		evas_object_del(popup);

	elm_exit();
}

void _tethering_wifi_ap_btn_yes_cb(void *data, Evas_Object *obj, void *event_info)
{
	log_print(NET_POPUP, "_tethering_wifi_ap_btn_yes_cb()\n");

	bool result = FALSE;
	Evas_Object *popup = (Evas_Object *)data;

	__net_popup_init_dbus();
	__net_popup_send_dbus_msg("progress_off");
	__net_popup_deinit_dbus();

	result = _net_popup_send_user_resp(RESP_WIFI_AP_TETHERING_OFF, FALSE);
	if (true != result)
		log_print(NET_POPUP, "Failed to send user response ");

	if (popup)
		evas_object_del(popup);

	elm_exit();
}

void _vpn_btn_yes_cb(void *data, Evas_Object *obj, void *event_info)
{
	log_print(NET_POPUP, "+");
	Evas_Object *popup = (Evas_Object *)data;

	if (popup)
		evas_object_del(popup);
	if (obj)
		evas_object_del(obj);

	elm_exit();
}

void _vpn_btn_no_cb(void *data, Evas_Object *obj, void *event_info)
{
	log_print(NET_POPUP, "vpn - cancel");
	evas_object_del(obj);
	elm_exit();
}

void _btn_no_cb(void *data, Evas_Object *obj, void *event_info)
{
	log_print(NET_POPUP, "_btn_no_cb()\n");

	bool result = FALSE;
	Evas_Object *popup = (Evas_Object *)data;

	result = _net_popup_send_user_resp(RESP_TETHERING_ON, FALSE);
	if (true != result)
		log_print(NET_POPUP, "Failed to send user response ");

	if (popup)
		evas_object_del(popup);

	elm_exit();
}

static void __net_popup_show_popup_with_user_resp(app_control_h request,
		void *data)
{
	log_print(NET_POPUP, "__net_popup_show_popup_with_user_resp()\n");

	Evas_Object *win;
	Evas_Object *popup;
	Evas_Object *layout;
	Evas_Object *label;
	Evas_Object *btn1;
	Evas_Object *btn2;
	int ret = 0;

	ret = app_control_get_extra_data(request, "_SYSPOPUP_CONTENT_", &resp_popup_mode);
	if (APP_CONTROL_ERROR_NONE != ret)
		log_print(NET_POPUP, "Failed to get _SYSPOPUP_CONTENT_ ret = %d", ret);

	secure_log_print(NET_POPUP, "_SYSPOPUP_CONTENT_ = %s\n", resp_popup_mode);

	win = __net_popup_create_win();
	evas_object_show(win);

	popup = elm_popup_add(win);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND,
			EVAS_HINT_EXPAND);

	if (g_strcmp0(resp_popup_mode, "TETHERING_TYPE_WIFI") == 0 ||
			g_strcmp0(resp_popup_mode, "TETHERING_TYPE_WIFI_AP") == 0) {
		log_print(NET_POPUP, "Drawing Wi-Fi Tethering OFF popup");

		__net_popup_init_dbus();
		elm_object_part_text_set(popup, "title,text", ALERT_STR_WIFI);

		layout = elm_layout_add(popup);
		elm_layout_file_set(layout, NETPOPUP_EDJ, "popup");
		evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);

		__net_popup_send_dbus_msg("progress_on");
		label = elm_label_add(popup);
		elm_label_line_wrap_set(label, ELM_WRAP_MIXED);
		elm_object_text_set(label, ALERT_STR_WIFI_MOBILE_AP_ON);
		evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND,
				EVAS_HINT_EXPAND);
		evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
		evas_object_show(label);

		elm_object_part_content_set(layout, "elm.swallow.content", label);
		evas_object_show(layout);
		elm_object_style_set(label, "popup/default");
		eext_object_event_callback_add(popup, EEXT_CALLBACK_BACK, _btn_no_cb, popup);
		elm_object_content_set(popup, label);

		btn1 = elm_button_add(popup);
		elm_object_style_set(btn1, "popup");
		elm_object_text_set(btn1, ALERT_STR_CANCEL);
		elm_object_part_content_set(popup, "button1", btn1);
		evas_object_smart_callback_add(btn1, "clicked",
					_btn_no_cb, popup);

		btn2 = elm_button_add(popup);
		elm_object_style_set(btn2, "popup");
		elm_object_text_set(btn2, ALERT_STR_OK);
		elm_object_part_content_set(popup, "button2", btn2);

		if (g_strcmp0(resp_popup_mode, "TETHERING_TYPE_WIFI") == 0)
			evas_object_smart_callback_add(btn2, "clicked",
				_tethering_wifi_btn_yes_cb, popup);
		else if (g_strcmp0(resp_popup_mode, "TETHERING_TYPE_WIFI_AP") == 0)
			evas_object_smart_callback_add(btn2, "clicked",
				_tethering_wifi_ap_btn_yes_cb, popup);

		evas_object_show(popup);
		evas_object_show(win);
		__net_popup_deinit_dbus();
	}
}

static void __net_popup_show_popup_for_vpn_service(app_control_h request, void *data)
{
	Evas_Object *win;
	Evas_Object *popup;
	Evas_Object *button1;
	Evas_Object *button2;
	int ret = 0;
	char *mode = NULL;

	ret = app_control_get_extra_data(request, "_SYSPOPUP_CONTENT_", &mode);
	if (APP_CONTROL_ERROR_NONE != ret) {
		log_print(NET_POPUP, "Failed to get _SYSPOPUP_CONTENT_ ret = %d", ret);
		g_free(mode);
		elm_exit();
		return;
	}

	secure_log_print(NET_POPUP, "_SYSPOPUP_CONTENT_ = %s\n", mode);

	win = __net_popup_create_win();

	popup = elm_popup_add(win);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_text_set(popup, mode);
	elm_object_part_text_set(popup, "title,text", ALERT_STR_ENABLE_VPN);
	log_print(NET_POPUP, "%s\n", mode);

	/* OK button */
	button1 = elm_button_add(popup);
	elm_object_style_set(button1, "bottom");
	elm_object_text_set(button1, "OK");
	elm_object_part_content_set(popup, "button1", button1);
	evas_object_smart_callback_add(button1, "clicked", _vpn_btn_yes_cb, popup);

	button2 = elm_button_add(popup);
	elm_object_style_set(button2, "bottom");
	elm_object_text_set(button2, "Cancel");
	elm_object_part_content_set(popup, "button2", button2);
	evas_object_smart_callback_add(button2, "clicked", _vpn_btn_no_cb, popup);

	evas_object_show(popup);
	evas_object_show(win);
	g_free(mode);

	return;
}

static int __net_popup_show_popup(app_control_h request, void *data)
{
	log_print(NET_POPUP, "__net_popup_show_popup()\n");

	Evas_Object *win;
	Evas_Object *popup;
	Evas_Object *button;
	int ret = 0;
	char *mode = NULL;

	ret = app_control_get_extra_data(request, "_SYSPOPUP_CONTENT_", &mode);
	if (APP_CONTROL_ERROR_NONE != ret) {
		log_print(NET_POPUP, "Failed to get _SYSPOPUP_CONTENT_ ret = %d", ret);
		g_free(mode);
		elm_exit();
		return 0;
	}

	secure_log_print(NET_POPUP, "_SYSPOPUP_CONTENT_ = %s\n", mode);

	win = __net_popup_create_win();

	popup = elm_popup_add(win);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	if (strcmp(mode, "connected") == 0) {
		elm_object_text_set(popup, ALERT_STR_MOBILE_NETWORKS_CHARGE);
		log_print(NET_POPUP, "alert 3g\n");
	} else if (strcmp(mode, "fail to connect") == 0) {
		elm_object_text_set(popup, ALERT_STR_ERR_UNAVAILABLE);
		log_print(NET_POPUP, "alert err\n");
	} else if (strcmp(mode, "unable to connect") == 0) {
		elm_object_text_set(popup, ALERT_STR_ERR_CONNECT);
		log_print(NET_POPUP, "alert unable to connect\n");
	} else if (strcmp(mode, "wrong password") == 0) {
		elm_object_text_set(popup, ALERT_STR_ERR_WRONG_PASSWORD);
		log_print(NET_POPUP, "alert wrong password\n");
	} else if (strcmp(mode, "not support") == 0) {
		elm_object_text_set(popup, ALERT_STR_ERR_NOT_SUPPORT);
		log_print(NET_POPUP, "alert not support\n");
	} else if (strcmp(mode, "wifi restricted") == 0) {
		elm_object_text_set(popup, ALERT_STR_RESTRICTED_USE_WIFI);
		log_print(NET_POPUP, "alert wifi restricted\n");
	} else if (strcmp(mode, "no ap found") == 0) {
		elm_object_text_set(popup, ALERT_STR_NO_AP_FOUND);
		log_print(NET_POPUP, "alert no ap found\n");
	} else if (strcmp(mode, "wifi connected") == 0) {
		char buf[ALERT_STR_LEN_MAX] = "";
		char *ap_name = NULL;

		ret = app_control_get_extra_data(request, "_AP_NAME_", &ap_name);

		if (ap_name != NULL)
			g_snprintf(buf, ALERT_STR_LEN_MAX, ALERT_STR_WIFI_CONNECTED, ap_name);
		else
			g_snprintf(buf, ALERT_STR_LEN_MAX, ALERT_STR_WIFI_CONNECTED, "");

		elm_object_text_set(popup, buf);

		log_print(NET_POPUP, "alert wifi connected\n");
		g_free(ap_name);
	} else {
		elm_object_text_set(popup, mode);
		log_print(NET_POPUP, "%s\n", mode);
	}

	button = elm_button_add(popup);
	elm_object_style_set(button, "popup");
	elm_object_text_set(button, ALERT_STR_OK);
	elm_object_part_content_set(popup, "button1", button);
	evas_object_smart_callback_add(button, "clicked", _ok_button_clicked_cb, popup);
	evas_object_show(popup);
	evas_object_show(win);
	g_free(mode);

	return 0;
}

static void __net_popup_add_found_ap_noti(void)
{
	log_print(NET_POPUP, "__net_popup_add_found_ap_noti()\n");

	int ret = 0, noti_flags = 0;
	char icon_path[ICON_PATH_LEN];
	notification_h noti = NULL;
	notification_list_h noti_list = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;
	app_control_h service_handle = NULL;

	notification_get_detail_list("net.netpopup", NOTIFICATION_GROUP_ID_NONE,
			NOTIFICATION_PRIV_ID_NONE, -1, &noti_list);
	if (noti_list != NULL) {
		notification_free_list(noti_list);
		return;
	}

	noti = notification_create(NOTIFICATION_TYPE_ONGOING);
	if (noti == NULL) {
		log_print(NET_POPUP, "Failed to create notification");
		return;
	}

	noti_err = notification_set_time(noti, time(NULL));
	if(noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "Failed to notification_set_time : %d", noti_err);
		goto error;
	}

	g_snprintf(icon_path, sizeof(icon_path), "%s%s", QP_PRELOAD_NOTI_ICON_PATH, "/noti_wifi_in_range.png");
	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, icon_path);
	if(noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "Failed to notification_set_image : %d", noti_err);
		goto error;
	}

	noti_err = notification_set_layout(noti, NOTIFICATION_LY_ONGOING_EVENT);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "Failed to notification_set_layout : %d", noti_err);
		goto error;
	}

	noti_err = notification_set_text_domain(noti, PACKAGE,
			LOCALEDIR);
	if(noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "Failed to notification_set_text_domain : %d", noti_err);
		goto error;
	}

	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE,
			NETCONFIG_NOTIFICATION_WIFI_FOUND_TITLE,
			"IDS_COM_BODY_WI_FI_NETWORKS_AVAILABLE",
			NOTIFICATION_VARIABLE_TYPE_NONE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "Failed to notification_set_title : %d", noti_err);
		goto error;
	}

	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
			NETCONFIG_NOTIFICATION_WIFI_FOUND_CONTENT,
			"IDS_WIFI_SBODY_TAP_HERE_TO_CONNECT",
			NOTIFICATION_VARIABLE_TYPE_NONE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "Failed to notification_set_content: %d", noti_err);
		goto error;
	}

	noti_flags = NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY | NOTIFICATION_DISPLAY_APP_INDICATOR;
	noti_err = notification_set_display_applist(noti, noti_flags);
	if(noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "Failed to notification_set_display_applist: %d", noti_err);
		goto error;
	}

	ret = app_control_create(&service_handle);
	log_print(NET_POPUP, "service create ret[%d]", ret);
	if(ret != APP_CONTROL_ERROR_NONE)
		goto error;

	ret = app_control_add_extra_data(service_handle, "caller", "notification");
	log_print(NET_POPUP, "Service data addition ret = %d", ret);
	if(ret != APP_CONTROL_ERROR_NONE)
		goto error;

	noti_err = notification_set_launch_option(noti,
			NOTIFICATION_LAUNCH_OPTION_APP_CONTROL, service_handle);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "Failed to notification_set_launch_option");
		goto error;
	}

	noti_err = notification_post(noti);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "Failed to insert notification[error# is %d]", noti_err);
		goto error;
	}

	log_print(NET_POPUP, "Successfully added notification");

error:

	if (noti != NULL)
		notification_free(noti);
}

static void __net_popup_del_found_ap_noti(void)
{
	log_print(NET_POPUP, "__net_popup_del_found_ap_noti()\n");

	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	noti_err = notification_delete_all(NOTIFICATION_TYPE_ONGOING);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_delete_by_priv_id");
		return;
	}

	log_print(NET_POPUP, "Successfully deleted notification");
}

static void __net_popup_add_portal_noti(app_control_h request)
{
	log_print(NET_POPUP, "__net_popup_add_portal_noti()\n");

	int ret = 0;
	int noti_flags = 0;
	char *ap_name = NULL;
	char icon_path[ICON_PATH_LEN];
	notification_h noti = NULL;
	app_control_h service_handle = NULL;
	notification_list_h noti_list = NULL;
	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	ret = app_control_get_extra_data(request, "_AP_NAME_", &ap_name);

	if (ap_name == NULL || ret != APP_CONTROL_ERROR_NONE) {
		log_print(NET_POPUP, "Failed to retrieve connected AP name!!");
		g_free(ap_name);
		return;
	}

	log_print(NET_POPUP, "Successfully added notification");

	notification_get_detail_list("net.netpopup", NOTIFICATION_GROUP_ID_NONE,
			NOTIFICATION_PRIV_ID_NONE, -1, &noti_list);
	if (noti_list != NULL) {
		notification_free_list(noti_list);
		g_free(ap_name);
		return;
	}

	noti = notification_create(NOTIFICATION_TYPE_NOTI);
	if (noti == NULL) {
		log_print(NET_POPUP, "fail to create notification");
		g_free(ap_name);
		return;
	}

	noti_err = notification_set_time(noti, time(NULL));
	if(noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_time : %d", noti_err);
		goto error;
	}

	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON,
			NETCONFIG_NOTIFICATION_WIFI_ICON);
	if(noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_image : %d", noti_err);
		goto error;
	}

	g_snprintf(icon_path, sizeof(icon_path), "%s%s", QP_PRELOAD_NOTI_ICON_PATH, "/noti_wifi_in_range.png");
	noti_err = notification_set_image(noti, NOTIFICATION_IMAGE_TYPE_ICON, icon_path);
	if(noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_image : %d", noti_err);
		goto error;
	}
	noti_err = notification_set_layout(noti, NOTIFICATION_LY_NOTI_EVENT_MULTIPLE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_layout : %d", noti_err);
		goto error;
	}

	noti_err = notification_set_text_domain(noti, PACKAGE,
			LOCALEDIR);
	if(noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_text_domain : %d", noti_err);
		goto error;
	}

	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_TITLE,
			NETCONFIG_NOTIFICATION_WIFI_PORTAL_TITLE,
			"IDS_WIFI_HEADER_SIGN_IN_TO_WI_FI_NETWORK_ABB",
			NOTIFICATION_VARIABLE_TYPE_NONE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_title : %d", noti_err);
		goto error;
	}

	noti_err = notification_set_text(noti, NOTIFICATION_TEXT_TYPE_CONTENT,
			NETCONFIG_NOTIFICATION_WIFI_PORTAL_CONTENT,
			NETCONFIG_NOTIFICATION_WIFI_PORTAL_CONTENT,
			NOTIFICATION_VARIABLE_TYPE_STRING, ap_name,
			NOTIFICATION_VARIABLE_TYPE_NONE);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_content : %d", noti_err);
		goto error;
	}

	noti_flags = NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY | NOTIFICATION_DISPLAY_APP_INDICATOR;
	noti_err = notification_set_display_applist(noti, noti_flags);
	if(noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_display_applist : %d", noti_err);
		goto error;
	}

	ret = app_control_create(&service_handle);
	log_print(NET_POPUP, "service create ret[%d]", ret);
	if(ret != APP_CONTROL_ERROR_NONE)
		goto error;

	ret = app_control_set_operation(service_handle, APP_CONTROL_OPERATION_VIEW);
	if(ret != APP_CONTROL_ERROR_NONE)
		goto error;

	log_print(NET_POPUP, "service set operation is successful");

	ret = app_control_set_uri(service_handle, "http://www.google.com");

	if(ret != APP_CONTROL_ERROR_NONE)
		goto error;

	noti_err = notification_set_launch_option(noti,
			NOTIFICATION_LAUNCH_OPTION_APP_CONTROL, service_handle);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_set_launch_option");
		goto error;
	}

	noti_err = notification_post(noti);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_post");
		goto error;
	}

	log_print(NET_POPUP, "Successfully added notification");

error:
	g_free(ap_name);
	if (noti != NULL)
		notification_free(noti);

	if (service_handle != NULL)
		app_control_destroy(service_handle);
}

static void __net_popup_del_portal_noti(void)
{
	log_print(NET_POPUP, "__net_popup_del_portal_noti()\n");

	notification_error_e noti_err = NOTIFICATION_ERROR_NONE;

	noti_err = notification_delete_all(NOTIFICATION_TYPE_NOTI);
	if (noti_err != NOTIFICATION_ERROR_NONE) {
		log_print(NET_POPUP, "fail to notification_delete_all");
		return;
	}

	log_print(NET_POPUP, "Successfully deleted notification");
}

EXPORT_API int main(int argc, char *argv[])
{
	log_print(NET_POPUP, "main()\n");

	ui_app_lifecycle_callback_s app_callback = {
		.create = __net_popup_create,
		.terminate = __net_popup_terminate,
		.pause = __net_popup_pause,
		.resume = __net_popup_resume,
		.app_control = __net_popup_service_cb,
	};

	return ui_app_main(argc, argv, &app_callback, NULL);
}

