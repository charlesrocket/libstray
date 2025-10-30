/// Menu item types
pub const MenuItemType = enum(c_int) {
    normal = c.STRAY_MENU_ITEM_NORMAL,
    separator = c.STRAY_MENU_ITEM_SEPARATOR,
    check = c.STRAY_MENU_ITEM_CHECK,
    radio = c.STRAY_MENU_ITEM_RADIO,
};

/// Click callback function.
pub const ClickCallback = *const fn (user_data: ?*anyopaque) void;

/// Menu callback function.
pub const MenuCallback = *const fn (menu_id: i32, user_data: ?*anyopaque) void;

/// System tray icon.
pub const TrayIcon = struct {
    handle: *c.TrayIcon,

    /// Creates a new tray icon.
    pub fn create(app_name: []const u8, icon_name: ?[]const u8, title: ?[]const u8) !TrayIcon {
        const app_name_z = try std.heap.c_allocator.dupeZ(u8, app_name);
        defer std.heap.c_allocator.free(app_name_z);

        const icon_name_z = if (icon_name) |name|
            try std.heap.c_allocator.dupeZ(u8, name)
        else
            null;

        defer if (icon_name_z) |name| std.heap.c_allocator.free(name);

        const title_z = if (title) |t|
            try std.heap.c_allocator.dupeZ(u8, t)
        else
            null;

        defer if (title_z) |t| std.heap.c_allocator.free(t);

        const handle = c.stray_create(
            app_name_z.ptr,
            if (icon_name_z) |name| name.ptr else null,
            if (title_z) |t| t.ptr else null,
        ) orelse return error.CreateFailed;

        return TrayIcon{ .handle = handle };
    }

    /// Registers the tray icon with the system.
    pub fn register(self: TrayIcon) !void {
        if (c.stray_register(self.handle) == 0) {
            return error.RegisterFailed;
        }
    }

    /// Sets the click callback.
    pub fn setClickCallback(self: TrayIcon, callback: ClickCallback, user_data: ?*anyopaque) void {
        const Wrapper = struct {
            fn call(data: ?*anyopaque) callconv(.C) void {
                const ctx = @as(*CallbackContext, @ptrCast(@alignCast(data)));
                ctx.callback(ctx.user_data);
            }
        };

        const ctx = std.heap.c_allocator.create(CallbackContext) catch return;
        ctx.* = .{ .callback = callback, .user_data = user_data };

        c.stray_set_click_callback(self.handle, Wrapper.call, ctx);
    }

    /// Processes pending events (non-blocking).
    pub fn processEvents(self: TrayIcon) void {
        c.stray_process_events(self.handle);
    }

    /// Sets the icon name.
    pub fn setIcon(self: TrayIcon, icon_name: []const u8) !void {
        const icon_name_z = try std.heap.c_allocator.dupeZ(u8, icon_name);
        defer std.heap.c_allocator.free(icon_name_z);
        c.stray_set_icon(self.handle, icon_name_z.ptr);
    }

    /// Sets the title.
    pub fn setTitle(self: TrayIcon, title: []const u8) !void {
        const title_z = try std.heap.c_allocator.dupeZ(u8, title);
        defer std.heap.c_allocator.free(title_z);
        c.stray_set_title(self.handle, title_z.ptr);
    }

    /// Sets the menu for this tray icon.
    pub fn setMenu(self: TrayIcon, menu: TrayMenu) void {
        c.stray_set_menu(self.handle, menu.handle);
    }

    /// Destroys the tray icon.
    pub fn destroy(self: TrayIcon) void {
        c.stray_destroy(self.handle);
    }
};

/// Context menu for the tray icon.
pub const TrayMenu = struct {
    handle: *c.TrayMenu,

    /// Creates a new menu.
    pub fn create() !TrayMenu {
        const handle = c.stray_menu_create() orelse return error.CreateFailed;
        return TrayMenu{ .handle = handle };
    }

    /// Adds a regular menu item.
    pub fn addItem(self: TrayMenu, label: []const u8, callback: MenuCallback, user_data: ?*anyopaque) !i32 {
        const label_z = try std.heap.c_allocator.dupeZ(u8, label);
        defer std.heap.c_allocator.free(label_z);

        const Wrapper = struct {
            fn call(menu_id: c_int, data: ?*anyopaque) callconv(.C) void {
                const ctx = @as(*MenuCallbackContext, @ptrCast(@alignCast(data)));
                ctx.callback(menu_id, ctx.user_data);
            }
        };

        const ctx = try std.heap.c_allocator.create(MenuCallbackContext);
        ctx.* = .{ .callback = callback, .user_data = user_data };

        const id = c.stray_menu_add_item(self.handle, label_z.ptr, Wrapper.call, ctx);
        if (id < 0) return error.AddItemFailed;
        return id;
    }

    /// Adds a separator.
    pub fn addSeparator(self: TrayMenu) !i32 {
        const id = c.stray_menu_add_separator(self.handle);
        if (id < 0) return error.AddSeparatorFailed;
        return id;
    }

    /// Adds a checkable menu item.
    pub fn addCheckItem(self: TrayMenu, label: []const u8, callback: MenuCallback, user_data: ?*anyopaque) !i32 {
        const label_z = try std.heap.c_allocator.dupeZ(u8, label);
        defer std.heap.c_allocator.free(label_z);

        const Wrapper = struct {
            fn call(menu_id: c_int, data: ?*anyopaque) callconv(.C) void {
                const ctx = @as(*MenuCallbackContext, @ptrCast(@alignCast(data)));
                ctx.callback(menu_id, ctx.user_data);
            }
        };

        const ctx = try std.heap.c_allocator.create(MenuCallbackContext);
        ctx.* = .{ .callback = callback, .user_data = user_data };

        const id = c.stray_menu_add_check_item(self.handle, label_z.ptr, Wrapper.call, ctx);
        if (id < 0) return error.AddCheckItemFailed;
        return id;
    }

    /// Sets whether a menu item is checked.
    pub fn setItemChecked(self: TrayMenu, item_id: i32, checked: bool) void {
        c.stray_menu_set_item_checked(self.handle, item_id, if (checked) 1 else 0);
    }

    /// Sets whether a menu item is enabled.
    pub fn setItemEnabled(self: TrayMenu, item_id: i32, enabled: bool) void {
        c.stray_menu_set_item_enabled(self.handle, item_id, if (enabled) 1 else 0);
    }

    /// Sets a menu item's label.
    pub fn setItemLabel(self: TrayMenu, item_id: i32, label: []const u8) !void {
        const label_z = try std.heap.c_allocator.dupeZ(u8, label);
        defer std.heap.c_allocator.free(label_z);
        c.stray_menu_set_item_label(self.handle, item_id, label_z.ptr);
    }

    /// Destroys the menu.
    pub fn destroy(self: TrayMenu) void {
        c.stray_menu_destroy(self.handle);
    }
};

const CallbackContext = struct {
    callback: ClickCallback,
    user_data: ?*anyopaque,
};

const MenuCallbackContext = struct {
    callback: MenuCallback,
    user_data: ?*anyopaque,
};

const std = @import("std");
const c = @cImport({
    @cInclude("stray.h");
});
