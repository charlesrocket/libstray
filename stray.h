/* stray.h - library for system tray icons */

#ifndef STRAY_H
#define STRAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define STRAY_OBJECT_PATH "/StatusNotifierItem"
#define STRAY_INTERFACE_NAME "org.kde.StatusNotifierItem"
#define STRAY_WATCHER_SERVICE "org.kde.StatusNotifierWatcher"
#define STRAY_WATCHER_PATH "/StatusNotifierWatcher"

#define STRAY_DEFAULT_ICON "application-x-executable"
#define STRAY_DEFAULT_TITLE "My Application"
#define STRAY_DEFAULT_ID "my-app"

typedef struct TrayIcon TrayIcon;
typedef void (*TrayClickCallback)(void *user_data);

/* public API */
TrayIcon *systray_create(const char *app_name, const char *icon_name,
                         const char *title);
void systray_set_click_callback(TrayIcon *icon, TrayClickCallback callback,
                                void *user_data);
void systray_process_events(TrayIcon *icon);
void systray_set_icon(TrayIcon *icon, const char *icon_name);
void systray_set_title(TrayIcon *icon, const char *title);
void systray_destroy(TrayIcon *icon);

#ifdef STRAY_IMPL

struct TrayIcon {
    DBusConnection *conn;
    char *service_name;
    char *icon_name;
    char *title;
    TrayClickCallback click_callback;
    void *user_data;
};

static void add_string_property(DBusMessageIter *array, const char *prop_name,
                                const char *value) {
    DBusMessageIter dict_entry, variant;
    dbus_message_iter_open_container(array, DBUS_TYPE_DICT_ENTRY, NULL,
                                     &dict_entry);
    dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &prop_name);
    dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "s",
                                     &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &value);
    dbus_message_iter_close_container(&dict_entry, &variant);
    dbus_message_iter_close_container(array, &dict_entry);
}

static void add_string_variant(DBusMessageIter *args, const char *value) {
    DBusMessageIter variant;
    dbus_message_iter_open_container(args, DBUS_TYPE_VARIANT, "s", &variant);
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_STRING, &value);
    dbus_message_iter_close_container(args, &variant);
}

static void add_empty_pixmap_array(DBusMessageIter *variant) {
    DBusMessageIter pixmap_array;
    dbus_message_iter_open_container(variant, DBUS_TYPE_ARRAY, "(iiay)",
                                     &pixmap_array);
    dbus_message_iter_close_container(variant, &pixmap_array);
}

static void emit_signal(TrayIcon *icon, const char *signal_name) {
    DBusMessage *msg;
    if (!icon)
        return;

    msg = dbus_message_new_signal(STRAY_OBJECT_PATH, STRAY_INTERFACE_NAME,
                                  signal_name);

    if (msg) {
        dbus_connection_send(icon->conn, msg, NULL);
        dbus_message_unref(msg);
    }
}

static void emit_properties_changed(TrayIcon *icon, const char *property_name) {
    DBusMessage *msg;
    DBusMessageIter args, changed_props, invalidated_props;
    const char *interface;
    const char *current_icon;
    const char *current_title;

    if (!icon)
        return;

    msg = dbus_message_new_signal(STRAY_OBJECT_PATH,
                                  "org.freedesktop.DBus.Properties",
                                  "PropertiesChanged");
    if (!msg)
        return;

    interface = STRAY_INTERFACE_NAME;

    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &interface);

    /* build changed properties array */
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}",
                                     &changed_props);

    current_icon = icon->icon_name ? icon->icon_name : STRAY_DEFAULT_ICON;
    current_title = icon->title ? icon->title : STRAY_DEFAULT_TITLE;

    if (strcmp(property_name, "IconName") == 0 ||
        strcmp(property_name, "All") == 0) {
        add_string_property(&changed_props, "IconName", current_icon);
    }

    if (strcmp(property_name, "Title") == 0 ||
        strcmp(property_name, "All") == 0) {
        add_string_property(&changed_props, "Title", current_title);
    }

    dbus_message_iter_close_container(&args, &changed_props);

    /* empty invalidated properties array */
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "s",
                                     &invalidated_props);
    dbus_message_iter_close_container(&args, &invalidated_props);

    dbus_connection_send(icon->conn, msg, NULL);
    dbus_message_unref(msg);
}

static void handle_property_get_all(DBusConnection *conn, DBusMessage *msg,
                                    TrayIcon *icon) {
    DBusMessageIter args, array;
    DBusMessage *reply;
    DBusMessageIter dict_entry, variant;
    dbus_bool_t item_is_menu;
    const char *prop_item_is_menu;
    const char *prop_menu;
    const char *prop_pixmap;
    const char *current_icon;
    const char *current_title;
    const char *menu_path;
    reply = dbus_message_new_method_return(msg);
    if (!reply)
        return;

    dbus_message_iter_init_append(reply, &args);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &array);

    current_icon = icon->icon_name ? icon->icon_name : STRAY_DEFAULT_ICON;
    current_title = icon->title ? icon->title : STRAY_DEFAULT_TITLE;

    /* add standard properties */
    add_string_property(&array, "Category", "ApplicationStatus");
    add_string_property(&array, "Id", STRAY_DEFAULT_ID);
    add_string_property(&array, "Title", current_title);
    add_string_property(&array, "Status", "Active");
    add_string_property(&array, "IconName", current_icon);
    add_string_property(&array, "IconThemePath", "");

    /* add IconPixmap property (empty array) */
    prop_pixmap = "IconPixmap";
    dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, NULL,
                                     &dict_entry);
    dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &prop_pixmap);
    dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "a(iiay)",
                                     &variant);
    add_empty_pixmap_array(&variant);
    dbus_message_iter_close_container(&dict_entry, &variant);
    dbus_message_iter_close_container(&array, &dict_entry);

    /* add Menu property */
    prop_menu = "Menu";
    dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, NULL,
                                     &dict_entry);
    dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &prop_menu);
    dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "o",
                                     &variant);
    menu_path = "/NO_DBUSMENU";
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_OBJECT_PATH, &menu_path);
    dbus_message_iter_close_container(&dict_entry, &variant);
    dbus_message_iter_close_container(&array, &dict_entry);

    /* add ItemIsMenu property */
    prop_item_is_menu = "ItemIsMenu";
    dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, NULL,
                                     &dict_entry);
    dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING,
                                   &prop_item_is_menu);
    dbus_message_iter_open_container(&dict_entry, DBUS_TYPE_VARIANT, "b",
                                     &variant);
    item_is_menu = FALSE;
    dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN, &item_is_menu);
    dbus_message_iter_close_container(&dict_entry, &variant);
    dbus_message_iter_close_container(&array, &dict_entry);

    dbus_message_iter_close_container(&args, &array);
    dbus_connection_send(conn, reply, NULL);
    dbus_message_unref(reply);
}

static void handle_property_get(DBusConnection *conn, DBusMessage *msg,
                                TrayIcon *icon, const char *prop) {
    const char *current_icon;
    const char *current_title;
    const char *menu_path;
    dbus_bool_t item_is_menu;
    DBusMessageIter args;
    DBusMessage *reply = dbus_message_new_method_return(msg);
    if (!reply)
        return;

    dbus_message_iter_init_append(reply, &args);

    current_icon = icon->icon_name ? icon->icon_name : STRAY_DEFAULT_ICON;
    current_title = icon->title ? icon->title : STRAY_DEFAULT_TITLE;

    if (strcmp(prop, "Category") == 0) {
        add_string_variant(&args, "ApplicationStatus");
    } else if (strcmp(prop, "Id") == 0) {
        add_string_variant(&args, STRAY_DEFAULT_ID);
    } else if (strcmp(prop, "Title") == 0) {
        add_string_variant(&args, current_title);
    } else if (strcmp(prop, "Status") == 0) {
        add_string_variant(&args, "Active");
    } else if (strcmp(prop, "IconName") == 0) {
        add_string_variant(&args, current_icon);
    } else if (strcmp(prop, "IconThemePath") == 0) {
        add_string_variant(&args, "");
    } else if (strcmp(prop, "IconPixmap") == 0) {
        DBusMessageIter variant;
        dbus_message_iter_open_container(&args, DBUS_TYPE_VARIANT, "a(iiay)",
                                         &variant);
        add_empty_pixmap_array(&variant);
        dbus_message_iter_close_container(&args, &variant);
    } else if (strcmp(prop, "Menu") == 0) {
        DBusMessageIter variant;
        dbus_message_iter_open_container(&args, DBUS_TYPE_VARIANT, "o",
                                         &variant);
        menu_path = "/NO_DBUSMENU";
        dbus_message_iter_append_basic(&variant, DBUS_TYPE_OBJECT_PATH,
                                       &menu_path);
        dbus_message_iter_close_container(&args, &variant);
    } else if (strcmp(prop, "ItemIsMenu") == 0) {
        DBusMessageIter variant;
        dbus_message_iter_open_container(&args, DBUS_TYPE_VARIANT, "b",
                                         &variant);
        item_is_menu = FALSE;
        dbus_message_iter_append_basic(&variant, DBUS_TYPE_BOOLEAN,
                                       &item_is_menu);
        dbus_message_iter_close_container(&args, &variant);
    } else {
        DBusMessage *error = dbus_message_new_error(
            msg, "org.freedesktop.DBus.Error.InvalidArgs",
            "Property not found");
        dbus_connection_send(conn, error, NULL);
        dbus_message_unref(error);
        dbus_message_unref(reply);

        return;
    }

    dbus_connection_send(conn, reply, NULL);
    dbus_message_unref(reply);
}

static DBusHandlerResult message_handler(DBusConnection *conn, DBusMessage *msg,
                                         void *data) {
    const char *interface;
    const char *member;
    DBusMessage *reply;
    TrayIcon *icon = (TrayIcon *)data;

    if (!icon)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    interface = dbus_message_get_interface(msg);
    member = dbus_message_get_member(msg);

    if (!interface || !member)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    /* handle property requests */
    if (strcmp(interface, "org.freedesktop.DBus.Properties") == 0) {
        if (strcmp(member, "GetAll") == 0) {
            handle_property_get_all(conn, msg, icon);
            return DBUS_HANDLER_RESULT_HANDLED;
        } else if (strcmp(member, "Get") == 0) {
            const char *iface, *prop;
            dbus_message_get_args(msg, NULL, DBUS_TYPE_STRING, &iface,
                                  DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);
            handle_property_get(conn, msg, icon, prop);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
    }

    /* handle Activate method (left-click) */
    if (strcmp(interface, STRAY_INTERFACE_NAME) == 0 &&
        strcmp(member, "Activate") == 0) {
        if (icon->click_callback) {
            icon->click_callback(icon->user_data);
        }

        reply = dbus_message_new_method_return(msg);

        if (reply) {
            dbus_connection_send(conn, reply, NULL);
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    /* handle NewIcon method */
    if (strcmp(interface, STRAY_INTERFACE_NAME) == 0 &&
        strcmp(member, "NewIcon") == 0) {
        DBusMessage *reply = dbus_message_new_method_return(msg);
        if (reply) {
            dbus_connection_send(conn, reply, NULL);
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static int register_with_watcher(DBusConnection *conn,
                                 const char *service_name) {
    const char *item_path;
    DBusError err;
    DBusMessage *reply;
    DBusMessage *msg = dbus_message_new_method_call(
        STRAY_WATCHER_SERVICE, STRAY_WATCHER_PATH, STRAY_WATCHER_SERVICE,
        "RegisterStatusNotifierItem");

    if (!msg)
        return 0;

    item_path = STRAY_OBJECT_PATH;
    dbus_message_append_args(msg, DBUS_TYPE_STRING, &item_path,
                             DBUS_TYPE_INVALID);

    dbus_error_init(&err);

    reply = dbus_connection_send_with_reply_and_block(conn, msg, 5000, &err);
    dbus_message_unref(msg);

    if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
        return 0;
    }

    if (reply)
        dbus_message_unref(reply);
    return 1;
}

static void process_events_with_timeout(DBusConnection *conn, int timeout_ms) {
    long elapsed_ms;
    int remaining_ms;
    DBusDispatchStatus status;
    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL);

    while (1) {
        /* calculate remaining time */
        gettimeofday(&current_time, NULL);
        elapsed_ms = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                     (current_time.tv_usec - start_time.tv_usec) / 1000;

        if (elapsed_ms >= timeout_ms)
            break;

        remaining_ms = timeout_ms - elapsed_ms;

        /* process events with the remaining timeout */
        dbus_connection_read_write(conn, remaining_ms);

        do {
            status = dbus_connection_dispatch(conn);
        } while (status == DBUS_DISPATCH_DATA_REMAINS);

        /* if we processed something, continue for a bit more to catch any */
        /* follow-up messages */
        if (status == DBUS_DISPATCH_COMPLETE) {
            /* 10ms to allow batched messages */
            usleep(10000);
        } else {
            break;
        }
    }
}

static char *safe_strdup(const char *str) { return str ? strdup(str) : NULL; }

static void safe_free(char **str) {
    if (str && *str) {
        free(*str);
        *str = NULL;
    }
}

TrayIcon *systray_create(const char *app_name, const char *icon_name,
                         const char *title) {
    char service_name[256];
    TrayIcon *icon;
    DBusConnection *conn;
    DBusError err;
    DBusObjectPathVTable vtable;
    int ret;
    if (!app_name)
        return NULL;

    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);

    if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
        return NULL;
    }

    snprintf(service_name, sizeof(service_name),
             "org.kde.StatusNotifierItem-%d-%s", getpid(), app_name);

    ret = dbus_bus_request_name(conn, service_name,
                                DBUS_NAME_FLAG_REPLACE_EXISTING, &err);

    if (dbus_error_is_set(&err)) {
        dbus_error_free(&err);
        dbus_connection_unref(conn);
        return NULL;
    }

    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        dbus_connection_unref(conn);
        return NULL;
    }

    icon = calloc(1, sizeof(TrayIcon));

    if (!icon) {
        dbus_connection_unref(conn);
        return NULL;
    }

    icon->conn = conn;
    icon->service_name = strdup(service_name);

    if (!icon->service_name)
        goto cleanup_icon;

    /* set initial values */
    icon->icon_name = safe_strdup(icon_name ? icon_name : STRAY_DEFAULT_ICON);
    icon->title = safe_strdup(title ? title : STRAY_DEFAULT_TITLE);

    if (!icon->icon_name || !icon->title)
        goto cleanup_full;

    vtable.unregister_function = NULL;
    vtable.message_function = message_handler;

    if (!dbus_connection_register_object_path(conn, STRAY_OBJECT_PATH, &vtable,
                                              icon)) {
        goto cleanup_full;
    }

    /* process any pending events before (!) registering with the watcher */
    process_events_with_timeout(conn, 100);

    /* now register with the watcher - this will trigger property queries */
    if (!register_with_watcher(conn, service_name)) {
        goto cleanup_full;
    }

    /* ensure the tray gets the initial state */
    emit_properties_changed(icon, "All");

    /* process any final events */
    process_events_with_timeout(conn, 100);

    return icon;

cleanup_full:
    systray_destroy(icon);
    return NULL;

cleanup_icon:
    free(icon);
    dbus_connection_unref(conn);
    return NULL;
}

void systray_set_click_callback(TrayIcon *icon, TrayClickCallback callback,
                                void *user_data) {
    if (icon) {
        icon->click_callback = callback;
        icon->user_data = user_data;
    }
}

void systray_process_events(TrayIcon *icon) {
    DBusDispatchStatus status;

    if (icon) {
        dbus_connection_read_write(icon->conn, 0);

        do {
            status = dbus_connection_dispatch(icon->conn);
        } while (status == DBUS_DISPATCH_DATA_REMAINS);
    }
}

void systray_set_icon(TrayIcon *icon, const char *icon_name) {
    if (!icon)
        return;

    safe_free(&icon->icon_name);
    icon->icon_name = safe_strdup(icon_name ? icon_name : STRAY_DEFAULT_ICON);

    if (!icon->icon_name)
        return;

    /* emit signals to notify about the change */
    emit_signal(icon, "NewIcon");
    emit_properties_changed(icon, "IconName");
}

void systray_set_title(TrayIcon *icon, const char *title) {
    if (!icon)
        return;

    safe_free(&icon->title);
    icon->title = safe_strdup(title ? title : STRAY_DEFAULT_TITLE);

    if (!icon->title)
        return;

    /* emit signals to notify about the change */
    emit_signal(icon, "NewTitle");
    emit_properties_changed(icon, "Title");
}

void systray_destroy(TrayIcon *icon) {
    if (!icon)
        return;

    safe_free(&icon->service_name);
    safe_free(&icon->icon_name);
    safe_free(&icon->title);

    if (icon->conn) {
        dbus_connection_unref(icon->conn);
    }

    free(icon);
}

#endif /* STRAY_IMPL */

#ifdef __cplusplus
}
#endif

#endif /* STRAY_H */
