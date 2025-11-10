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

/// Pixmap structure for ARGB32 icon data
pub const Pixmap = extern struct {
    width: i32,
    height: i32,
    data: [*]u32, // ARGB32 format, each pixel is 32 bits

    /// Creates a new pixmap from ARGB32 data
    /// Returns a pointer that must be destroyed with destroy()
    pub fn create(width: i32, height: i32, data: ?[]const u32) !*Pixmap {
        const data_ptr = if (data) |d|
            @as([*c]u32, @ptrCast(@constCast(d.ptr)))
        else
            null;

        const c_pixmap = c.stray_pixmap_create(width, height, data_ptr) orelse
            return error.PixmapCreateFailed;

        return @ptrCast(@alignCast(c_pixmap));
    }

    pub fn setIconPixmap(
        self: *Icon,
        width: i32,
        height: i32,
        data: []const u32,
    ) !void {
        c.stray_set_icon_pixmap_data(
            self.handle,
            width,
            height,
            @ptrCast(data.ptr),
        );
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

/// System tray icon.
pub const Icon = struct {
    handle: *c.TrayIcon,
    allocator: std.mem.Allocator,
    click_context: ?*CallbackContext = null,
    menu: ?*Menu = null,
    owned_pixmaps: std.array_list.Managed(*Pixmap),

    /// Creates a new tray icon.
    pub fn create(
        allocator: std.mem.Allocator,
        app_name: []const u8,
        icon_name: ?[]const u8,
        title: ?[]const u8,
    ) !Icon {
        const app_name_z = try allocator.dupeZ(u8, app_name);
        defer allocator.free(app_name_z);

        const icon_name_z = if (icon_name) |name|
            try allocator.dupeZ(u8, name)
        else
            null;

        defer if (icon_name_z) |name| allocator.free(name);

        const title_z = if (title) |t|
            try allocator.dupeZ(u8, t)
        else
            null;

        defer if (title_z) |t| allocator.free(t);

        const handle = c.stray_create(
            app_name_z.ptr,
            if (icon_name_z) |name| name.ptr else null,
            if (title_z) |t| t.ptr else null,
        ) orelse return error.CreateFailed;

        return Icon{
            .handle = handle,
            .allocator = allocator,
            .owned_pixmaps = std.array_list.Managed(*Pixmap).init(allocator),
        };
    }

    /// Registers the tray icon with the system.
    pub fn register(self: Icon) !void {
        if (c.stray_register(self.handle) == 0) {
            return error.RegisterFailed;
        }
    }

    /// Sets the click callback.
    pub fn setClickCallback(
        self: *Icon,
        callback: ClickCallback,
        user_data: ?*anyopaque,
    ) !void {
        const Wrapper = struct {
            fn call(data: ?*anyopaque) callconv(.c) void {
                const ctx = @as(*CallbackContext, @ptrCast(@alignCast(data)));
                ctx.callback(ctx.user_data);
            }
        };

        if (self.click_context) |old_ctx| {
            self.allocator.destroy(old_ctx);
        }

        const ctx = try self.allocator.create(CallbackContext);
        ctx.* = .{ .callback = callback, .user_data = user_data };
        self.click_context = ctx;

        c.stray_set_click_callback(self.handle, Wrapper.call, ctx);
    }

    /// Processes pending events (non-blocking).
    pub fn processEvents(self: Icon) void {
        c.stray_process_events(self.handle);
    }

    /// Sets the icon name.
    pub fn setIcon(self: *Icon, icon_name: []const u8) !void {
        const icon_name_z = try self.allocator.dupeZ(u8, icon_name);
        defer self.allocator.free(icon_name_z);
        c.stray_set_icon(self.handle, icon_name_z.ptr);
    }

    /// Sets the title.
    pub fn setTitle(self: *Icon, title: []const u8) !void {
        const title_z = try self.allocator.dupeZ(u8, title);
        defer self.allocator.free(title_z);
        c.stray_set_title(self.handle, title_z.ptr);
    }

    /// Sets the tooltip for the tray icon.
    /// The tooltip has a title and descriptive text.
    pub fn setTooltip(self: *Icon, title: ?[]const u8, text: ?[]const u8) !void {
        const title_z = if (title) |t|
            try self.allocator.dupeZ(u8, t)
        else
            null;
        defer if (title_z) |t| self.allocator.free(t);

        const text_z = if (text) |txt|
            try self.allocator.dupeZ(u8, txt)
        else
            null;
        defer if (text_z) |txt| self.allocator.free(txt);

        c.stray_set_tooltip(
            self.handle,
            if (title_z) |t| t.ptr else null,
            if (text_z) |txt| txt.ptr else null,
        );
    }

    /// Sets the icon using pixmap data
    /// The icon takes ownership of the pixmaps
    pub fn setIconPixmap(
        self: *Icon,
        width: i32,
        height: i32,
        data: []const u32,
    ) !void {
        // clear the named icon so the system uses the pixmap instead
        c.stray_set_icon(self.handle, "");
        c.stray_set_icon_pixmap(
            self.handle,
            width,
            height,
            @ptrCast(data.ptr),
        );
    }

    /// Sets the menu for this tray icon.
    /// The icon takes ownership, so the menu will be destroyed with the icon.
    pub fn setMenu(self: *Icon, menu: *Menu) void {
        self.menu = menu;
        c.stray_set_menu(self.handle, menu.handle);
    }

    /// Sets whether a menu item state.
    pub fn setMenuItemEnabled(self: *Icon, item_id: i32, enabled: bool) void {
        if (self.menu) |menu| {
            menu.setItemEnabled(item_id, enabled);
        }
    }

    /// Sets whether a menu item is checked.
    pub fn setMenuItemChecked(self: *Icon, item_id: i32, checked: bool) void {
        if (self.menu) |menu| {
            menu.setItemChecked(item_id, checked);
        }
    }

    /// Sets a menu item's label.
    pub fn setMenuItemLabel(self: *Icon, item_id: i32, label: []const u8) !void {
        if (self.menu) |menu| {
            try menu.setItemLabel(item_id, label);
        }
    }

    /// Destroys the tray icon.
    /// This will also destroy the menu.
    pub fn destroy(self: *Icon) void {
        if (self.click_context) |ctx| {
            self.allocator.destroy(ctx);
            self.click_context = null;
        }

        self.owned_pixmaps.deinit();

        // clean up the menu contexts
        if (self.menu) |menu| {
            for (menu.contexts.items) |ctx| {
                self.allocator.destroy(ctx);
            }

            menu.contexts.deinit();
            self.menu = null;
        }

        c.stray_destroy(self.handle);
    }
};

/// Context menu for the tray icon.
pub const Menu = struct {
    handle: ?*c.TrayMenu,
    allocator: std.mem.Allocator,
    contexts: std.array_list.Managed(*MenuCallbackContext),

    /// Creates a new menu.
    pub fn create(allocator: std.mem.Allocator) !Menu {
        const handle = c.stray_menu_create() orelse
            return error.MenuCreateFailed;

        return Menu{
            .handle = handle,
            .allocator = allocator,
            .contexts = std.array_list.Managed(*MenuCallbackContext)
                .init(allocator),
        };
    }

    /// Adds a regular menu item.
    pub fn addItem(
        self: *Menu,
        label: []const u8,
        callback: MenuCallback,
        user_data: ?*anyopaque,
    ) !i32 {
        const label_z = try self.allocator.dupeZ(u8, label);
        defer self.allocator.free(label_z);

        const Wrapper = struct {
            fn call(menu_id: c_int, data: ?*anyopaque) callconv(.c) void {
                const ctx = @as(
                    *MenuCallbackContext,
                    @ptrCast(@alignCast(data)),
                );

                ctx.callback(menu_id, ctx.user_data);
            }
        };

        const ctx = try self.allocator.create(MenuCallbackContext);
        errdefer self.allocator.destroy(ctx);

        ctx.* = .{ .callback = callback, .user_data = user_data };
        try self.contexts.append(ctx);

        const id = c.stray_menu_add_item(
            self.handle,
            label_z.ptr,
            Wrapper.call,
            ctx,
        );

        if (id < 0) return error.AddItemFailed;
        return id;
    }

    /// Adds a separator.
    pub fn addSeparator(self: *Menu) !i32 {
        const id = c.stray_menu_add_separator(self.handle);
        if (id < 0) return error.AddSeparatorFailed;
        return id;
    }

    /// Adds a checkable menu item.
    pub fn addCheckItem(
        self: *Menu,
        label: []const u8,
        callback: MenuCallback,
        user_data: ?*anyopaque,
    ) !i32 {
        const label_z = try self.allocator.dupeZ(u8, label);
        defer self.allocator.free(label_z);

        const Wrapper = struct {
            fn call(menu_id: c_int, data: ?*anyopaque) callconv(.c) void {
                const ctx = @as(
                    *MenuCallbackContext,
                    @ptrCast(@alignCast(data)),
                );

                ctx.callback(menu_id, ctx.user_data);
            }
        };

        const ctx = try self.allocator.create(MenuCallbackContext);
        errdefer self.allocator.destroy(ctx);

        ctx.* = .{ .callback = callback, .user_data = user_data };
        try self.contexts.append(ctx);

        const id = c.stray_menu_add_check_item(
            self.handle,
            label_z.ptr,
            Wrapper.call,
            ctx,
        );

        if (id < 0) return error.AddCheckItemFailed;
        return id;
    }

    /// Sets whether a menu item is checked.
    pub fn setItemChecked(self: Menu, item_id: i32, checked: bool) void {
        c.stray_menu_set_item_checked(
            self.handle,
            item_id,
            if (checked) 1 else 0,
        );
    }

    /// Sets whether a menu item is enabled.
    pub fn setItemEnabled(self: Menu, item_id: i32, enabled: bool) void {
        c.stray_menu_set_item_enabled(
            self.handle,
            item_id,
            if (enabled) 1 else 0,
        );
    }

    /// Sets a menu item's label.
    pub fn setItemLabel(self: *Menu, item_id: i32, label: []const u8) !void {
        const label_z = try self.allocator.dupeZ(u8, label);
        defer self.allocator.free(label_z);
        c.stray_menu_set_item_label(self.handle, item_id, label_z.ptr);
    }
};

const c = @cImport({
    @cInclude("stray.h");
});

const std = @import("std");
