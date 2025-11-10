pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    var is_active = true;

    var icon = try Icon.create(
        allocator,
        "stray-test",
        "face-devilish",
        "STRAY test",
    );

    defer icon.destroy();

    var menu = try Menu.create(allocator);

    _ = try menu.addItem("Open", onOpen, null);
    _ = try menu.addSeparator();
    const disabled_item = try menu.addCheckItem("Test feature", onToggle, null);
    _ = try menu.addSeparator();
    _ = try menu.addItem("Quit", onQuit, &is_active);

    icon.setMenuItemEnabled(disabled_item, false);
    icon.setMenu(&menu);

    _ = try icon.setTitle("TEST");
    try icon.setClickCallback(onClick, null);

    // TODO set up D-Bus CI session
    icon.register() catch {};

    const custom_icon = try createCustomIcon(allocator);
    defer allocator.free(custom_icon);

    var count: usize = 0;
    while (is_active) {
        if (count == 1) try icon.setIconPixmap(16, 16, custom_icon);
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

fn createCustomIcon(allocator: std.mem.Allocator) ![]u32 {
    const width: i32 = 16;
    const height: i32 = 16;
    const pixel_count = @as(usize, @intCast(width * height));

    var pixels = try allocator.alloc(u32, pixel_count);

    for (0..pixel_count) |i| {
        const b: u32 = 0x00; // blue
        const g: u32 = 0x00; // green
        const r: u32 = 0xFF; // red
        const a: u32 = 0xFF; // alpha

        pixels[i] = (b << 24) | (g << 16) | (r << 8) | a;
    }

    return pixels;
}

const std = @import("std");
const stray = @import("stray");
const Icon = stray.Icon;
const Menu = stray.Menu;
