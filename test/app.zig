pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    var is_active = true;

    var icon = try TrayIcon.create(
        allocator,
        "stray-test",
        "face-devilish",
        "STRAY test",
    );
    defer icon.destroy();

    var menu = try TrayMenu.create(allocator);

    _ = try menu.addItem("Open", onOpen, null);
    _ = try menu.addSeparator();
    const disabled_item = try menu.addCheckItem("Test feature", onToggle, null);
    _ = try menu.addSeparator();
    _ = try menu.addItem("Quit", onQuit, &is_active);

    icon.setMenuItemEnabled(disabled_item, false);
    icon.setMenu(&menu);

    try icon.setClickCallback(onClick, null);

    // TODO set up D-Bus CI session
    icon.register() catch {};

    var count: usize = 0;
    while (is_active) {
        if (count == 3) is_active = false;
        count += 1;
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
    std.debug.print("Test item {} clicked!\n", .{menu_id});
}

fn onQuit(menu_id: i32, user_data: ?*anyopaque) void {
    _ = menu_id;

    if (user_data) |ptr| {
        const bool_ptr = @as(*bool, @ptrCast(@alignCast(ptr)));
        bool_ptr.* = false;
    }

    std.debug.print("Quitting!\n", .{});
}

const std = @import("std");
const stray = @import("stray");
const TrayIcon = stray.TrayIcon;
const TrayMenu = stray.TrayMenu;
