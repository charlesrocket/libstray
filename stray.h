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

#define STRAY_OBJECT_PATH        "/StatusNotifierItem"
#define STRAY_INTERFACE_NAME     "org.kde.StatusNotifierItem"
#define STRAY_WATCHER_SERVICE    "org.kde.StatusNotifierWatcher"
#define STRAY_WATCHER_PATH       "/StatusNotifierWatcher"
#define STRAY_MENU_OBJECT_PATH   "/StatusNotifierMenu"
#define STRAY_DBUSMENU_INTERFACE "com.canonical.dbusmenu"

#define STRAY_DEFAULT_ICON       "application-x-executable"
#define STRAY_DEFAULT_TITLE      "My Application"
#define STRAY_DEFAULT_ID         "my-app"

typedef struct TrayIcon TrayIcon;
typedef struct TrayMenu TrayMenu;
typedef struct TrayMenuItem TrayMenuItem;

typedef void (*TrayClickCallback)(void *user_data);
typedef void (*TrayMenuCallback)(int menu_id, void *user_data);

/* Menu item types */
typedef enum {
    STRAY_MENU_ITEM_NORMAL = 0,
    STRAY_MENU_ITEM_SEPARATOR = 1,
    STRAY_MENU_ITEM_CHECK = 2,
    STRAY_MENU_ITEM_RADIO = 3
} TrayMenuItemType;

/* Icon API */
TrayIcon *
stray_create(const char *app_name, const char *icon_name, const char *title);
void stray_set_click_callback(
    TrayIcon *icon, TrayClickCallback callback, void *user_data);

void stray_process_events(TrayIcon *icon);
void stray_set_icon(TrayIcon *icon, const char *icon_name);
void stray_set_title(TrayIcon *icon, const char *title);
void stray_destroy(TrayIcon *icon);
int stray_register(TrayIcon *icon);

/* Menu API */
TrayMenu *stray_menu_create(void);
void stray_menu_destroy(TrayMenu *menu);
void stray_set_menu(TrayIcon *icon, TrayMenu *menu);
void stray_menu_set_item_label(TrayMenu *menu, int item_id, const char *label);
int stray_menu_add_separator(TrayMenu *menu);
int stray_menu_add_item(
    TrayMenu *menu, const char *label, TrayMenuCallback callback,
    void *user_data);
int stray_menu_add_check_item(
    TrayMenu *menu, const char *label, TrayMenuCallback callback,
    void *user_data);
void stray_menu_set_item_checked(
    TrayMenu *menu, int item_id, dbus_bool_t checked);
void stray_menu_set_item_enabled(
    TrayMenu *menu, int item_id, dbus_bool_t enabled);

#ifdef STRAY_IMPL

struct TrayMenuItem {
    int id;
    char *label;
    TrayMenuItemType type;
    dbus_bool_t enabled;
    dbus_bool_t checked;
    TrayMenuCallback callback;
    void *user_data;
    TrayMenuItem *next;
};

struct TrayMenu {
    TrayMenuItem *items;
    int item_count;
    int next_id;
};

struct TrayIcon {
    DBusConnection *conn;
    char *service_name;
    char *icon_name;
    char *title;
    TrayClickCallback click_callback;
    void *user_data;
    TrayMenu *menu;
};

/* helper functions */
static char *safe_strdup(const char *str) { return str ? strdup(str) : NULL; }
static void safe_free(char **str) {
    if (str && *str) {
        free(*str);
        *str = NULL;
    }
}

static void add_variant(
    DBusMessageIter *args, int type, const char *sig, const void *value) {
    DBusMessageIter variant;
    dbus_message_iter_open_container(args, DBUS_TYPE_VARIANT, sig, &variant);
    dbus_message_iter_append_basic(&variant, type, value);
    dbus_message_iter_close_container(args, &variant);
}

static void add_dict_entry(
    DBusMessageIter *array, const char *key, int type, const char *sig,
    const void *value) {
    DBusMessageIter dict_entry, variant;
    dbus_message_iter_open_container(
        array, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);

    dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &key);
    dbus_message_iter_open_container(
        &dict_entry, DBUS_TYPE_VARIANT, sig, &variant);

    dbus_message_iter_append_basic(&variant, type, value);
    dbus_message_iter_close_container(&dict_entry, &variant);
    dbus_message_iter_close_container(array, &dict_entry);
}

static void add_empty_pixmap_array(DBusMessageIter *variant) {
    DBusMessageIter pixmap_array;
    dbus_message_iter_open_container(
        variant, DBUS_TYPE_ARRAY, "(iiay)", &pixmap_array);

    dbus_message_iter_close_container(variant, &pixmap_array);
}

static void emit_signal(TrayIcon *icon, const char *signal_name) {
    DBusMessage *msg;

    if (!icon) return;

    msg = dbus_message_new_signal(
        STRAY_OBJECT_PATH, STRAY_INTERFACE_NAME, signal_name);

    if (msg) {
        dbus_connection_send(icon->conn, msg, NULL);
        dbus_message_unref(msg);
    }
}

static void emit_properties_changed(TrayIcon *icon, const char *property_name) {
    const char *interface;
    const char *current_icon;
    const char *current_title;
    const char *menu_path;
    DBusMessageIter args, changed_props, invalidated_props;
    DBusMessage *msg;
    dbus_bool_t item_is_menu;
    int all;

    if (!icon) return;

    msg = dbus_message_new_signal(
        STRAY_OBJECT_PATH, "org.freedesktop.DBus.Properties",
        "PropertiesChanged");
    if (!msg) return;

    interface = STRAY_INTERFACE_NAME;
    current_icon = icon->icon_name ? icon->icon_name : STRAY_DEFAULT_ICON;

    current_title = icon->title ? icon->title : STRAY_DEFAULT_TITLE;
    menu_path = icon->menu ? STRAY_MENU_OBJECT_PATH : "/NO_DBUSMENU";

    item_is_menu = (icon->menu != NULL);

    dbus_message_iter_init_append(msg, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &interface);
    dbus_message_iter_open_container(
        &args, DBUS_TYPE_ARRAY, "{sv}", &changed_props);

    all = strcmp(property_name, "All") == 0;

    if (all || strcmp(property_name, "IconName") == 0)
        add_dict_entry(
            &changed_props, "IconName", DBUS_TYPE_STRING, "s", &current_icon);

    if (all || strcmp(property_name, "Title") == 0)
        add_dict_entry(
            &changed_props, "Title", DBUS_TYPE_STRING, "s", &current_title);

    if (all || strcmp(property_name, "Menu") == 0)
        add_dict_entry(
            &changed_props, "Menu", DBUS_TYPE_STRING, "s", &menu_path);

    if (all || strcmp(property_name, "ItemIsMenu") == 0)
        add_dict_entry(
            &changed_props, "ItemIsMenu", DBUS_TYPE_BOOLEAN, "b",
            &item_is_menu);

    dbus_message_iter_close_container(&args, &changed_props);
    dbus_message_iter_open_container(
        &args, DBUS_TYPE_ARRAY, "s", &invalidated_props);

    dbus_message_iter_close_container(&args, &invalidated_props);
    dbus_connection_send(icon->conn, msg, NULL);
    dbus_message_unref(msg);
}

static void get_icon_properties(
    TrayIcon *icon, const char **out_icon, const char **out_title,
    const char **out_menu, dbus_bool_t *out_is_menu) {
    *out_icon = icon->icon_name ? icon->icon_name : STRAY_DEFAULT_ICON;
    *out_title = icon->title ? icon->title : STRAY_DEFAULT_TITLE;
    *out_menu = icon->menu ? STRAY_MENU_OBJECT_PATH : "/NO_DBUSMENU";
    *out_is_menu = (icon->menu != NULL);
}

static void
add_menu_item_properties(DBusMessageIter *props, TrayMenuItem *item) {
    dbus_bool_t visible;

    if (item->type == STRAY_MENU_ITEM_SEPARATOR) {
        const char *type_value = "separator";
        add_dict_entry(props, "type", DBUS_TYPE_STRING, "s", &type_value);
        return;
    }

    if (item->label)
        add_dict_entry(props, "label", DBUS_TYPE_STRING, "s", &item->label);

    add_dict_entry(props, "enabled", DBUS_TYPE_BOOLEAN, "b", &item->enabled);

    visible = TRUE;
    add_dict_entry(props, "visible", DBUS_TYPE_BOOLEAN, "b", &visible);

    if (item->type == STRAY_MENU_ITEM_CHECK ||
        item->type == STRAY_MENU_ITEM_RADIO) {
        dbus_int32_t toggle_state;

        const char *toggle_type =
            (item->type == STRAY_MENU_ITEM_CHECK) ? "checkmark" : "radio";
        add_dict_entry(
            props, "toggle-type", DBUS_TYPE_STRING, "s", &toggle_type);

        toggle_state = item->checked ? 1 : 0;
        add_dict_entry(
            props, "toggle-state", DBUS_TYPE_INT32, "i", &toggle_state);
    }
}

static void handle_property_get_all(
    DBusConnection *conn, DBusMessage *msg, TrayIcon *icon) {
    DBusMessageIter args, array, dict_entry, variant;
    const char *prop_menu;
    const char *current_icon, *current_title, *menu_path;
    const char *category_str;
    const char *id_str;
    const char *status_str;
    const char *empty_str;
    const char *prop_pixmap;
    dbus_bool_t item_is_menu;

    DBusMessage *reply = dbus_message_new_method_return(msg);
    if (!reply) return;

    get_icon_properties(
        icon, &current_icon, &current_title, &menu_path, &item_is_menu);

    dbus_message_iter_init_append(reply, &args);
    dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &array);

    category_str = "ApplicationStatus";
    id_str = STRAY_DEFAULT_ID;
    status_str = "Active";
    empty_str = "";

    /* add standard properties */
    add_dict_entry(&array, "Category", DBUS_TYPE_STRING, "s", &category_str);
    add_dict_entry(&array, "Id", DBUS_TYPE_STRING, "s", &id_str);
    add_dict_entry(&array, "Title", DBUS_TYPE_STRING, "s", &current_title);
    add_dict_entry(&array, "Status", DBUS_TYPE_STRING, "s", &status_str);
    add_dict_entry(&array, "IconName", DBUS_TYPE_STRING, "s", &current_icon);
    add_dict_entry(&array, "IconThemePath", DBUS_TYPE_STRING, "s", &empty_str);

    /* add IconPixmap property (special case/empty array) */
    prop_pixmap = "IconPixmap";
    dbus_message_iter_open_container(
        &array, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);

    dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &prop_pixmap);
    dbus_message_iter_open_container(
        &dict_entry, DBUS_TYPE_VARIANT, "a(iiay)", &variant);

    add_empty_pixmap_array(&variant);
    dbus_message_iter_close_container(&dict_entry, &variant);
    dbus_message_iter_close_container(&array, &dict_entry);

    /* add Menu property */
    prop_menu = "Menu";
    dbus_message_iter_open_container(
        &array, DBUS_TYPE_DICT_ENTRY, NULL, &dict_entry);

    dbus_message_iter_append_basic(&dict_entry, DBUS_TYPE_STRING, &prop_menu);
    dbus_message_iter_open_container(
        &dict_entry, DBUS_TYPE_VARIANT, "o", &variant);

    dbus_message_iter_append_basic(&variant, DBUS_TYPE_OBJECT_PATH, &menu_path);
    dbus_message_iter_close_container(&dict_entry, &variant);
    dbus_message_iter_close_container(&array, &dict_entry);

    /* add ItemIsMenu property */
    add_dict_entry(&array, "ItemIsMenu", DBUS_TYPE_BOOLEAN, "b", &item_is_menu);

    dbus_message_iter_close_container(&args, &array);
    dbus_connection_send(conn, reply, NULL);
    dbus_message_unref(reply);
}

static void handle_property_get(
    DBusConnection *conn, DBusMessage *msg, TrayIcon *icon, const char *prop) {
    DBusMessage *reply = dbus_message_new_method_return(msg);
    DBusMessageIter args;
    const char *current_icon, *current_title, *menu_path;
    const char *category_str;
    const char *id_str;
    const char *status_str;
    const char *theme_path;
    dbus_bool_t item_is_menu;

    if (!reply) return;

    get_icon_properties(
        icon, &current_icon, &current_title, &menu_path, &item_is_menu);

    dbus_message_iter_init_append(reply, &args);

    /* TODO */
    category_str = "ApplicationStatus";
    id_str = STRAY_DEFAULT_ID;
    status_str = "Active";
    theme_path = "";

    if (strcmp(prop, "Category") == 0) {
        add_variant(&args, DBUS_TYPE_STRING, "s", &category_str);
    } else if (strcmp(prop, "Id") == 0) {
        add_variant(&args, DBUS_TYPE_STRING, "s", &id_str);
    } else if (strcmp(prop, "Title") == 0) {
        add_variant(&args, DBUS_TYPE_STRING, "s", &current_title);
    } else if (strcmp(prop, "Status") == 0) {
        add_variant(&args, DBUS_TYPE_STRING, "s", &status_str);
    } else if (strcmp(prop, "IconName") == 0) {
        add_variant(&args, DBUS_TYPE_STRING, "s", &current_icon);
    } else if (strcmp(prop, "IconThemePath") == 0) {
        add_variant(&args, DBUS_TYPE_STRING, "s", &theme_path);
    } else if (strcmp(prop, "IconPixmap") == 0) {
        DBusMessageIter variant;
        dbus_message_iter_open_container(
            &args, DBUS_TYPE_VARIANT, "a(iiay)", &variant);
        add_empty_pixmap_array(&variant);
        dbus_message_iter_close_container(&args, &variant);
    } else if (strcmp(prop, "Menu") == 0) {
        DBusMessageIter variant;
        dbus_message_iter_open_container(
            &args, DBUS_TYPE_VARIANT, "o", &variant);
        dbus_message_iter_append_basic(
            &variant, DBUS_TYPE_OBJECT_PATH, &menu_path);
        dbus_message_iter_close_container(&args, &variant);
    } else if (strcmp(prop, "ItemIsMenu") == 0) {
        add_variant(&args, DBUS_TYPE_BOOLEAN, "b", &item_is_menu);
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

static TrayMenuItem *find_menu_item(TrayMenu *menu, dbus_int32_t id) {
    TrayMenuItem *item;

    for (item = menu->items; item; item = item->next) {
        if (item->id == id) return item;
    }

    return NULL;
}

static DBusHandlerResult
handle_menu_get_layout(DBusConnection *conn, DBusMessage *msg, TrayIcon *icon) {
    DBusMessageIter args, root_struct, root_props, root_children;
    DBusMessage *reply;
    TrayMenuItem *item;
    dbus_uint32_t revision;
    dbus_int32_t root_id;
    const char *prop_value;

    reply = dbus_message_new_method_return(msg);

    if (!reply) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    revision = 1;
    root_id = 0;

    dbus_message_iter_init_append(reply, &args);
    dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &revision);
    dbus_message_iter_open_container(
        &args, DBUS_TYPE_STRUCT, NULL, &root_struct);
    dbus_message_iter_append_basic(&root_struct, DBUS_TYPE_INT32, &root_id);
    dbus_message_iter_open_container(
        &root_struct, DBUS_TYPE_ARRAY, "{sv}", &root_props);

    prop_value = "submenu";
    add_dict_entry(
        &root_props, "children-display", DBUS_TYPE_STRING, "s", &prop_value);

    dbus_message_iter_close_container(&root_struct, &root_props);
    dbus_message_iter_open_container(
        &root_struct, DBUS_TYPE_ARRAY, "v", &root_children);

    for (item = icon->menu->items; item; item = item->next) {
        DBusMessageIter child_variant, child_struct, child_props,
            child_children;

        dbus_message_iter_open_container(
            &root_children, DBUS_TYPE_VARIANT, "(ia{sv}av)", &child_variant);
        dbus_message_iter_open_container(
            &child_variant, DBUS_TYPE_STRUCT, NULL, &child_struct);
        dbus_message_iter_append_basic(
            &child_struct, DBUS_TYPE_INT32, &item->id);
        dbus_message_iter_open_container(
            &child_struct, DBUS_TYPE_ARRAY, "{sv}", &child_props);

        add_menu_item_properties(&child_props, item);

        dbus_message_iter_close_container(&child_struct, &child_props);
        dbus_message_iter_open_container(
            &child_struct, DBUS_TYPE_ARRAY, "v", &child_children);
        dbus_message_iter_close_container(&child_struct, &child_children);
        dbus_message_iter_close_container(&child_variant, &child_struct);
        dbus_message_iter_close_container(&root_children, &child_variant);
    }

    dbus_message_iter_close_container(&root_struct, &root_children);
    dbus_message_iter_close_container(&args, &root_struct);

    dbus_connection_send(conn, reply, NULL);
    dbus_message_unref(reply);

    return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
handle_menu_event(DBusConnection *conn, DBusMessage *msg, TrayIcon *icon) {
    dbus_int32_t id;
    const char *type;
    DBusMessageIter iter;
    DBusMessage *reply;

    if (!dbus_message_iter_init(msg, &iter))
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    dbus_message_iter_get_basic(&iter, &id);
    dbus_message_iter_next(&iter);
    dbus_message_iter_get_basic(&iter, &type);

    if (strcmp(type, "clicked") == 0) {
        TrayMenuItem *item = find_menu_item(icon->menu, id);
        if (item && item->callback) { item->callback(id, item->user_data); }
    }

    reply = dbus_message_new_method_return(msg);

    if (reply) {
        dbus_connection_send(conn, reply, NULL);
        dbus_message_unref(reply);
    }

    return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult handle_menu_get_group_properties(
    DBusConnection *conn, DBusMessage *msg, TrayIcon *icon) {
    DBusMessageIter args, props_array, iter, id_array_iter;
    DBusMessage *reply = dbus_message_new_method_return(msg);

    if (!reply) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    dbus_message_iter_init_append(reply, &args);
    dbus_message_iter_open_container(
        &args, DBUS_TYPE_ARRAY, "(ia{sv})", &props_array);

    if (dbus_message_iter_init(msg, &iter) &&
        dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY) {
        dbus_message_iter_recurse(&iter, &id_array_iter);

        while (dbus_message_iter_get_arg_type(&id_array_iter) ==
               DBUS_TYPE_INT32) {
            TrayMenuItem *item;
            dbus_int32_t id;

            dbus_message_iter_get_basic(&id_array_iter, &id);
            item = find_menu_item(icon->menu, id);

            if (item) {
                DBusMessageIter tuple, item_props;
                dbus_message_iter_open_container(
                    &props_array, DBUS_TYPE_STRUCT, NULL, &tuple);
                dbus_message_iter_append_basic(&tuple, DBUS_TYPE_INT32, &id);
                dbus_message_iter_open_container(
                    &tuple, DBUS_TYPE_ARRAY, "{sv}", &item_props);

                add_menu_item_properties(&item_props, item);

                dbus_message_iter_close_container(&tuple, &item_props);
                dbus_message_iter_close_container(&props_array, &tuple);
            }

            dbus_message_iter_next(&id_array_iter);
        }
    }

    dbus_message_iter_close_container(&args, &props_array);
    dbus_connection_send(conn, reply, NULL);
    dbus_message_unref(reply);
    return DBUS_HANDLER_RESULT_HANDLED;
}

static DBusHandlerResult
menu_message_handler(DBusConnection *conn, DBusMessage *msg, void *data) {
    const char *interface;
    const char *member;
    TrayIcon *icon = (TrayIcon *)data;

    if (!icon || !icon->menu) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    interface = dbus_message_get_interface(msg);
    member = dbus_message_get_member(msg);

    if (!interface || strcmp(interface, STRAY_DBUSMENU_INTERFACE) != 0)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    if (strcmp(member, "GetLayout") == 0)
        return handle_menu_get_layout(conn, msg, icon);

    if (strcmp(member, "Event") == 0) return handle_menu_event(conn, msg, icon);

    if (strcmp(member, "AboutToShow") == 0) {
        DBusMessage *reply = dbus_message_new_method_return(msg);
        if (reply) {
            dbus_bool_t need_update = TRUE;
            DBusMessageIter args;
            dbus_message_iter_init_append(reply, &args);
            dbus_message_iter_append_basic(
                &args, DBUS_TYPE_BOOLEAN, &need_update);
            dbus_connection_send(conn, reply, NULL);
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(member, "AboutToShowGroup") == 0) {
        DBusMessage *reply = dbus_message_new_method_return(msg);
        if (reply) {
            dbus_bool_t need_update = FALSE;
            DBusMessageIter args, empty_array;
            dbus_message_iter_init_append(reply, &args);
            dbus_message_iter_append_basic(
                &args, DBUS_TYPE_BOOLEAN, &need_update);
            dbus_message_iter_open_container(
                &args, DBUS_TYPE_ARRAY, "u", &empty_array);
            dbus_message_iter_close_container(&args, &empty_array);
            dbus_message_iter_open_container(
                &args, DBUS_TYPE_ARRAY, "u", &empty_array);
            dbus_message_iter_close_container(&args, &empty_array);
            dbus_connection_send(conn, reply, NULL);
            dbus_message_unref(reply);
        }
        return DBUS_HANDLER_RESULT_HANDLED;
    }

    if (strcmp(member, "GetGroupProperties") == 0)
        return handle_menu_get_group_properties(conn, msg, icon);

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static DBusHandlerResult
message_handler(DBusConnection *conn, DBusMessage *msg, void *data) {
    const char *interface;
    const char *member;
    TrayIcon *icon = (TrayIcon *)data;
    if (!icon) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    interface = dbus_message_get_interface(msg);
    member = dbus_message_get_member(msg);

    if (!interface || !member) return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    /* handle property requests */
    if (strcmp(interface, "org.freedesktop.DBus.Properties") == 0) {
        if (strcmp(member, "GetAll") == 0) {
            handle_property_get_all(conn, msg, icon);
            return DBUS_HANDLER_RESULT_HANDLED;
        } else if (strcmp(member, "Get") == 0) {
            const char *iface, *prop;
            dbus_message_get_args(
                msg, NULL, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &prop,
                DBUS_TYPE_INVALID);

            handle_property_get(conn, msg, icon, prop);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
    }

    /* handle Activate method (left-click) */
    if (strcmp(interface, STRAY_INTERFACE_NAME) == 0) {
        DBusMessage *reply = NULL;

        if (strcmp(member, "Activate") == 0) {
            if (icon->click_callback) icon->click_callback(icon->user_data);
            reply = dbus_message_new_method_return(msg);
        } else if (
            strcmp(member, "ContextMenu") == 0 ||
            strcmp(member, "NewIcon") == 0) {
            reply = dbus_message_new_method_return(msg);
        }

        if (reply) {
            dbus_connection_send(conn, reply, NULL);
            dbus_message_unref(reply);
            return DBUS_HANDLER_RESULT_HANDLED;
        }
    }

    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static int
register_with_watcher(DBusConnection *conn, const char *service_name) {
    const char *item_path;
    DBusError err;
    DBusMessage *reply;
    DBusMessage *msg = dbus_message_new_method_call(
        STRAY_WATCHER_SERVICE, STRAY_WATCHER_PATH, STRAY_WATCHER_SERVICE,
        "RegisterStatusNotifierItem");
    if (!msg) return 0;

    item_path = STRAY_OBJECT_PATH;
    dbus_message_append_args(
        msg, DBUS_TYPE_STRING, &item_path, DBUS_TYPE_INVALID);

    dbus_error_init(&err);
    reply = dbus_connection_send_with_reply_and_block(conn, msg, 5000, &err);
    dbus_message_unref(msg);

    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Failed to register with watcher: %s\n", err.message);
        dbus_error_free(&err);
        return 0;
    }

    if (reply) dbus_message_unref(reply);
    return 1;
}

static void process_events_with_timeout(DBusConnection *conn, int timeout_ms) {
    long elapsed_ms;
    int remaining_ms;
    struct timeval start_time, current_time;

    gettimeofday(&start_time, NULL);

    while (1) {
        DBusDispatchStatus status;
        gettimeofday(&current_time, NULL);
        elapsed_ms = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                     (current_time.tv_usec - start_time.tv_usec) / 1000;

        if (elapsed_ms >= timeout_ms) break;

        remaining_ms = timeout_ms - elapsed_ms;
        /* process events with the remaining timeout */
        dbus_connection_read_write(conn, remaining_ms);

        /* do not stop processing to catch any follow-ups */
        do {
            status = dbus_connection_dispatch(conn);
        } while (status == DBUS_DISPATCH_DATA_REMAINS);

        if (status == DBUS_DISPATCH_COMPLETE)
            /* 10ms to allow batched messages */
            usleep(10000);
        else
            break;
    }
}

TrayIcon *
stray_create(const char *app_name, const char *icon_name, const char *title) {
    char service_name[256];
    TrayIcon *icon;
    DBusConnection *conn;
    DBusObjectPathVTable vtable;
    DBusObjectPathVTable menu_vtable;
    DBusError err;
    int ret;

    if (!app_name) return NULL;

    dbus_error_init(&err);

    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);

    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Failed to get DBus connection: %s\n", err.message);
        dbus_error_free(&err);
        return NULL;
    }

    snprintf(
        service_name, sizeof(service_name), "org.kde.StatusNotifierItem-%d-%s",
        getpid(), app_name);

    ret = dbus_bus_request_name(
        conn, service_name, DBUS_NAME_FLAG_REPLACE_EXISTING, &err);

    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Failed to request DBus name: %s\n", err.message);
        dbus_error_free(&err);
        dbus_connection_unref(conn);
        return NULL;
    }

    if (ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
        fprintf(stderr, "Failed to become primary owner of DBus name\n");
        dbus_connection_unref(conn);
        return NULL;
    }

    icon = calloc(1, sizeof(TrayIcon));

    if (!icon) {
        dbus_connection_unref(conn);
        return NULL;
    }

    /* set initial values */
    icon->conn = conn;
    icon->service_name = strdup(service_name);
    icon->icon_name = safe_strdup(icon_name ? icon_name : STRAY_DEFAULT_ICON);
    icon->title = safe_strdup(title ? title : STRAY_DEFAULT_TITLE);

    if (!icon->service_name || !icon->icon_name || !icon->title) {
        stray_destroy(icon);
        return NULL;
    }

    vtable.unregister_function = NULL;
    vtable.message_function = message_handler;

    if (!dbus_connection_register_object_path(
            conn, STRAY_OBJECT_PATH, &vtable, icon)) {
        fprintf(stderr, "Failed to register main object path\n");
        stray_destroy(icon);
        return NULL;
    }

    menu_vtable.unregister_function = NULL,
    menu_vtable.message_function = menu_message_handler;

    if (!dbus_connection_register_object_path(
            conn, STRAY_MENU_OBJECT_PATH, &menu_vtable, icon)) {
        fprintf(stderr, "Failed to register menu object path\n");
        stray_destroy(icon);
        return NULL;
    }

    /* process any final events */
    process_events_with_timeout(conn, 100);
    return icon;
}

int stray_register(TrayIcon *icon) {
    if (!icon) return 0;

    emit_properties_changed(icon, "All");
    process_events_with_timeout(icon->conn, 100);

    if (!register_with_watcher(icon->conn, icon->service_name)) { return 0; }

    process_events_with_timeout(icon->conn, 100);
    return 1;
}

void stray_set_click_callback(
    TrayIcon *icon, TrayClickCallback callback, void *user_data) {
    if (icon) {
        icon->click_callback = callback;
        icon->user_data = user_data;
    }
}

void stray_process_events(TrayIcon *icon) {
    DBusDispatchStatus status;

    if (!icon) return;

    dbus_connection_read_write(icon->conn, 0);

    do {
        status = dbus_connection_dispatch(icon->conn);
    } while (status == DBUS_DISPATCH_DATA_REMAINS);
}

void stray_set_icon(TrayIcon *icon, const char *icon_name) {
    if (!icon) return;

    safe_free(&icon->icon_name);
    icon->icon_name = safe_strdup(icon_name ? icon_name : STRAY_DEFAULT_ICON);
    if (!icon->icon_name) return;

    /* emit signals to notify about the change */
    emit_signal(icon, "NewIcon");
    emit_properties_changed(icon, "IconName");
}

void stray_set_title(TrayIcon *icon, const char *title) {
    if (!icon) return;

    safe_free(&icon->title);
    icon->title = safe_strdup(title ? title : STRAY_DEFAULT_TITLE);
    if (!icon->title) return;

    /* emit signals to notify about the change */
    emit_signal(icon, "NewTitle");
    emit_properties_changed(icon, "Title");
}

TrayMenu *stray_menu_create(void) {
    TrayMenu *menu = calloc(1, sizeof(TrayMenu));
    if (menu) menu->next_id = 1;
    return menu;
}

void stray_menu_destroy(TrayMenu *menu) {
    TrayMenuItem *item;
    TrayMenuItem *next;

    if (!menu) return;

    item = menu->items;

    while (item) {
        next = item->next;
        free(item->label);
        free(item);
        item = next;
    }

    free(menu);
}

static TrayMenuItem *create_menu_item(
    TrayMenu *menu, const char *label, TrayMenuItemType type,
    TrayMenuCallback callback, void *user_data) {
    TrayMenuItem *item = calloc(1, sizeof(TrayMenuItem));
    if (!item) return NULL;

    item->label = label ? strdup(label) : NULL;

    if (label && !item->label) {
        free(item);
        return NULL;
    }

    item->id = menu->next_id++;
    item->type = type;
    item->enabled = TRUE;
    item->checked = FALSE;
    item->callback = callback;
    item->user_data = user_data;
    if (label) item->label = strdup(label);

    if (!menu->items) {
        menu->items = item;
    } else {
        TrayMenuItem *last = menu->items;
        while (last->next)
            last = last->next;
        last->next = item;
    }

    menu->item_count++;
    return item;
}

int stray_menu_add_item(
    TrayMenu *menu, const char *label, TrayMenuCallback callback,
    void *user_data) {
    TrayMenuItem *item;

    if (!menu) return -1;

    item = create_menu_item(
        menu, label, STRAY_MENU_ITEM_NORMAL, callback, user_data);

    return item ? item->id : -1;
}

int stray_menu_add_separator(TrayMenu *menu) {
    TrayMenuItem *item;

    if (!menu) return -1;

    item = create_menu_item(menu, NULL, STRAY_MENU_ITEM_SEPARATOR, NULL, NULL);

    return item ? item->id : -1;
}

int stray_menu_add_check_item(
    TrayMenu *menu, const char *label, TrayMenuCallback callback,
    void *user_data) {
    TrayMenuItem *item;

    if (!menu) return -1;

    item = create_menu_item(
        menu, label, STRAY_MENU_ITEM_CHECK, callback, user_data);

    return item ? item->id : -1;
}

void stray_menu_set_item_checked(
    TrayMenu *menu, int item_id, dbus_bool_t checked) {
    TrayMenuItem *item;

    if (!menu) return;

    item = find_menu_item(menu, item_id);

    if (item) item->checked = checked;
}

void stray_menu_set_item_enabled(
    TrayMenu *menu, int item_id, dbus_bool_t enabled) {
    TrayMenuItem *item;

    if (!menu) return;

    item = find_menu_item(menu, item_id);

    if (item) item->enabled = enabled;
}

void stray_menu_set_item_label(TrayMenu *menu, int item_id, const char *label) {
    TrayMenuItem *item;

    if (!menu) return;

    item = find_menu_item(menu, item_id);

    if (item) {
        free(item->label);
        item->label = label ? strdup(label) : NULL;
    }
}

void stray_set_menu(TrayIcon *icon, TrayMenu *menu) {
    if (!icon) return;
    if (icon->menu) stray_menu_destroy(icon->menu);
    icon->menu = menu;
    emit_properties_changed(icon, "All");
}

void stray_destroy(TrayIcon *icon) {
    if (!icon) return;

    /* unregister DBus objects first */
    if (icon->conn) {
        dbus_connection_unregister_object_path(icon->conn, STRAY_OBJECT_PATH);
        dbus_connection_unregister_object_path(
            icon->conn, STRAY_MENU_OBJECT_PATH);
    }

    if (icon->menu) stray_menu_destroy(icon->menu);

    safe_free(&icon->service_name);
    safe_free(&icon->icon_name);
    safe_free(&icon->title);

    if (icon->conn) dbus_connection_unref(icon->conn);

    free(icon);
}

#endif /* STRAY_IMPL */

#ifdef __cplusplus
}
#endif

#endif /* STRAY_H */
