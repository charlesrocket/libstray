pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    var is_active = true;

    // create tray icon
    var icon = try Icon.create(allocator, "stray-demo", "starred", "STRAY demo");
    defer icon.destroy();

    // create menu
    var menu = try Menu.create(allocator);

    // add menu items
    _ = try menu.addItem("Open", onOpen, null);
    const disabled_item = try menu.addCheckItem("Disabled item", onToggle, null);
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
            try icon.setIconPixmapData(16, 16, custom_icon);
        } else if (count == 30) {
            std.debug.print("Exiting\n", .{});
            is_active = false;
        }

        icon.processEvents();
        std.Thread.sleep(1 * std.time.ns_per_s);
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

fn createCustomIcon(allocator: std.mem.Allocator) ![]u32 {
    const width: i32 = 16;
    const height: i32 = 16;
    const pixel_count = @as(usize, @intCast(width * height));

    std.debug.print(
        "Creating a pixmap: {}x{} ({} pixels)\n",
        .{ width, height, pixel_count },
    );

    // allocate and initialize pixel data
    var pixels = try allocator.alloc(u32, pixel_count);

    // create a simple blue circle on transparent background
    const center_x = width / 2;
    const center_y = height / 2;
    const radius = @min(center_x, center_y) - 1;

    std.debug.print(
        "Center: {},{} Radius: {}\n",
        .{ center_x, center_y, radius },
    );

    for (0..@as(usize, @intCast(height))) |y| {
        for (0..@as(usize, @intCast(width))) |x| {
            const idx = y * @as(usize, @intCast(width)) + x;
            const dx = @as(i32, @intCast(x)) - center_x;
            const dy = @as(i32, @intCast(y)) - center_y;

            // use i64 for the squared calculations to prevent overflow
            const dx_sq = @as(i64, dx) * @as(i64, dx);
            const dy_sq = @as(i64, dy) * @as(i64, dy);
            const radius_sq = @as(i64, radius) * @as(i64, radius);

            if (dx_sq + dy_sq <= radius_sq) {
                pixels[idx] = 0xFF0000FF; // ARGB: fully opaque blue
            } else {
                pixels[idx] = 0x00000000; // ARGB: fully transparent
            }
        }
    }

    return pixels;
}

const std = @import("std");
const stray = @import("stray");
const Icon = stray.Icon;
const Pixmap = stray.Pixmap;
const Menu = stray.Menu;
