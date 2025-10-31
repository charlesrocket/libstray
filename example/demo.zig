pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    var is_active = true;

    // create tray icon
    var icon = try TrayIcon.create(allocator, "stray-demo", "starred", "STRAY demo");
    defer icon.destroy();

    // create menu
    var menu = try TrayMenu.create(allocator);
    defer menu.destroy();

    // add menu items
    _ = try menu.addItem("Open", onOpen, null);
    _ = try menu.addSeparator();
    _ = try menu.addCheckItem("Enable feature", onToggle, null);
    _ = try menu.addSeparator();
    _ = try menu.addItem("Quit", onQuit, &is_active);

    // set menu
    icon.setMenu(&menu);

    // set click callback
    try icon.setClickCallback(onClick, null);

    // register with system
    try icon.register();

    std.debug.print("STRAY demo. Press Ctrl+C to exit.\n", .{});
    // main event loop
    while (is_active) {
        icon.processEvents();
        std.Thread.sleep(10 * std.time.ns_per_ms);
    }
}

fn onClick(user_data: ?*anyopaque) void {
    _ = user_data;
    std.debug.print("Tray icon clicked!\n", .{});
}

fn onOpen(menu_id: i32, user_data: ?*anyopaque) void {
    _ = menu_id;
    _ = user_data;
    std.debug.print("Open clicked!\n", .{});
}

fn onToggle(menu_id: i32, user_data: ?*anyopaque) void {
    _ = user_data;
    std.debug.print("Toggle item {} clicked!\n", .{menu_id});
}

fn onQuit(menu_id: i32, user_data: ?*anyopaque) void {
    _ = menu_id;

    if (user_data) |ptr| {
        const bool_ptr = @as(*bool, @ptrCast(@alignCast(ptr)));
        bool_ptr.* = false;
    }

    std.debug.print("Quit clicked!\n", .{});
}

const std = @import("std");
const stray = @import("stray");
const TrayIcon = stray.TrayIcon;
const TrayMenu = stray.TrayMenu;
