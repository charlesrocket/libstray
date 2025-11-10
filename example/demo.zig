pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    var is_active = true;
    var is_checked = false;

    // create tray icon
    var icon = try Icon.create(allocator, "stray-demo", "starred", "STRAY demo");
    defer icon.destroy();

    // create menu
    var menu = try Menu.create(allocator);

    // add menu items
    _ = try menu.addItem("Open", onOpen, null);
    const disabled_item = try menu.addItem("Disabled item", onToggle, null);
    const checked_item = try menu.addCheckItem(
        "Checked item",
        onCheck,
        &is_checked,
    );

    _ = try menu.addSeparator();
    _ = try menu.addItem("Quit", onQuit, &is_active);

    // set menu
    icon.setMenu(&menu);

    // set click callback
    try icon.setClickCallback(onClick, null);

    // disable menu item
    icon.setMenuItemEnabled(disabled_item, false);

    // register with system
    try icon.register();

    std.debug.print("STRAY demo. Press Ctrl+C to exit.\n", .{});

    // create and set custom pixmap icon
    const custom_icon = try createCustomIcon(allocator);
    defer allocator.free(custom_icon);

    // main event loop
    var count: usize = 0;

    std.debug.print("Switch to a custom pixmap in 5 seconds...\n", .{});

    while (is_active) {
        count += 1;
        if (count == 5) {
            try icon.setIconPixmap(16, 16, custom_icon);
        } else if (count == 30) {
            std.debug.print("Exiting\n", .{});
            is_active = false;
        }

        icon.setMenuItemChecked(checked_item, !is_checked);
        icon.processEvents();
        std.Thread.sleep(1 * std.time.ns_per_s);
    }
}

fn onClick(user_data: ?*anyopaque) void {
    _ = user_data;
    std.debug.print("Tray icon clicked!\n", .{});
}

fn onCheck(menu_id: i32, user_data: ?*anyopaque) void {
    _ = menu_id;

    if (user_data) |ptr| {
        const bool_ptr = @as(*bool, @ptrCast(@alignCast(ptr)));
        bool_ptr.* = !bool_ptr.*;
    }

    std.debug.print("Check clicked!\n", .{});
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

fn createCustomIcon(allocator: std.mem.Allocator) ![]u32 {
    const width: i32 = 16;
    const height: i32 = 16;
    const pixel_count = @as(usize, @intCast(width * height));

    var pixels = try allocator.alloc(u32, pixel_count);

    const color = 0xA020F0;

    for (0..pixel_count) |i| {
        pixels[i] = color;
    }

    return pixels;
}

const std = @import("std");
const stray = @import("stray");
const Icon = stray.Icon;
const Pixmap = stray.Pixmap;
const Menu = stray.Menu;
