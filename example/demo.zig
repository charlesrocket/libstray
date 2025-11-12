pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    var is_active = true;
    var is_checked = false;

    // radio group state
    const RadioGroup = struct {
        selected_id: i32 = -1,
        option1_id: i32 = -1,
        option2_id: i32 = -1,
        option3_id: i32 = -1,
    };

    var radio_group = RadioGroup{};

    // create tray icon
    var icon = try Icon.create(allocator, "stray-demo", "starred", "STRAY demo");
    defer icon.destroy();

    // create menus
    var menu = try Menu.create(allocator);
    var submenu = try Menu.create(allocator);

    const submenu_open_item = try submenu.addItem("Open", onOpen, null);
    try submenu.setItemIcon(submenu_open_item, "document-open");

    // add menu items with icons
    const open_item = try menu.addItem("Open", onOpen, null);
    try menu.setItemIcon(open_item, "document-open");

    const disabled_item = try menu.addItem("Disabled item", onToggle, null);
    try menu.setItemIcon(disabled_item, "dialog-warning");

    const checked_item = try menu.addCheckItem(
        "Checked item",
        onCheck,
        &is_checked,
    );

    try menu.setItemIcon(checked_item, "emblem-default");

    // add a submenu to the main menu
    const submenu_item = try menu.addSubmenu("Submenu", &submenu);
    try menu.setItemIcon(submenu_item, "folder");

    _ = try menu.addSeparator();

    // add radio menu items and store their actual IDs
    radio_group.option1_id = try menu.addRadioItem(
        "Radio Option 1",
        onRadio,
        &radio_group,
    );

    try menu.setItemIcon(radio_group.option1_id, "audio-volume-low");

    radio_group.option2_id = try menu.addRadioItem(
        "Radio Option 2",
        onRadio,
        &radio_group,
    );

    try menu.setItemIcon(radio_group.option2_id, "audio-volume-medium");

    radio_group.option3_id = try menu.addRadioItem(
        "Radio Option 3",
        onRadio,
        &radio_group,
    );

    try menu.setItemIcon(radio_group.option3_id, "audio-volume-high");

    // set the initial radio state (Option 1 selected)
    radio_group.selected_id = radio_group.option1_id;
    icon.setMenuItemChecked(radio_group.option1_id, true);
    icon.setMenuItemChecked(radio_group.option2_id, false);
    icon.setMenuItemChecked(radio_group.option3_id, false);

    _ = try menu.addSeparator();

    const quit_item = try menu.addItem("Quit", onQuit, &is_active);
    try menu.setItemIcon(quit_item, "application-exit");

    // set menu
    icon.setMenu(&menu);

    // set click callback
    try icon.setClickCallback(onClick, null);

    // disable menu item
    icon.setMenuItemEnabled(disabled_item, false);

    // register with system
    try icon.register();

    // set title
    try icon.setTitle("Demo title");

    std.debug.print(
        "\x1b[1mSTRAY demo\x1b[0m\nPress Ctrl+C to exit\n",
        .{},
    );

    // create a custom pixmap icon
    const custom_icon = try createCustomIcon(allocator, 0xA020F0);
    defer allocator.free(custom_icon);

    // main event loop
    var count: usize = 0;

    std.debug.print("Switching to a custom pixmap in 5 seconds\n", .{});

    while (is_active) {
        count += 1;
        if (count == 5) {
            try icon.setIconPixmap(16, 16, custom_icon);
        } else if (count == 10) {
            std.debug.print("Setting the tooltip\n", .{});
            try icon.setTooltip("Demo", "text");
            icon.setStatus(.needs_attention);
        } else if (count == 15) {
            std.debug.print("Changing menu item icon\n", .{});
            try menu.setItemIcon(open_item, "document-save");
        } else if (count == 30) {
            std.debug.print("Exiting\n", .{});
            is_active = false;
        }

        // toggle the checked item state
        icon.setMenuItemChecked(checked_item, !is_checked);

        // update radio states based on current selection
        icon.setMenuItemChecked(
            radio_group.option1_id,
            radio_group.selected_id == radio_group.option1_id,
        );

        icon.setMenuItemChecked(
            radio_group.option2_id,
            radio_group.selected_id == radio_group.option2_id,
        );

        icon.setMenuItemChecked(
            radio_group.option3_id,
            radio_group.selected_id == radio_group.option3_id,
        );

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

    std.debug.print(
        "Check clicked! State: {}\n",
        .{if (user_data) |ptr|
            @as(*bool, @ptrCast(@alignCast(ptr))).*
        else
            false},
    );
}

fn onRadio(menu_id: i32, user_data: ?*anyopaque) void {
    if (user_data) |ptr| {
        const radio_group_ptr = @as(
            *struct {
                selected_id: i32,
                option1_id: i32,
                option2_id: i32,
                option3_id: i32,
            },
            @ptrCast(@alignCast(ptr)),
        );

        // update the selected radio button
        radio_group_ptr.selected_id = menu_id;

        // determine which option was selected for logging
        const option_name = if (menu_id == radio_group_ptr.option1_id)
            "Option 1"
        else if (menu_id == radio_group_ptr.option2_id)
            "Option 2"
        else if (menu_id == radio_group_ptr.option3_id)
            "Option 3"
        else
            "Unknown";

        std.debug.print(
            "Radio {s} selected! (ID: {d})\n",
            .{ option_name, menu_id },
        );
    }
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

fn createCustomIcon(allocator: std.mem.Allocator, color: u32) ![]u32 {
    const width: i32 = 16;
    const height: i32 = 16;
    const pixel_count = @as(usize, @intCast(width * height));

    var pixels = try allocator.alloc(u32, pixel_count);

    for (0..pixel_count) |i| {
        pixels[i] = color;
    }

    return pixels;
}

const std = @import("std");
const stray = @import("stray");
const Icon = stray.Icon;
const Menu = stray.Menu;
