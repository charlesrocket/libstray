const Context = struct {
    icon: *Icon,
    item_id: ?i32 = null,
    target_id: ?i32 = null,
};

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();
    var is_active = true;
    var is_checked = false;

    std.debug.print(
        "\x1b[1mSTRAY demo\x1b[0m\nPress Ctrl+C to exit\n",
        .{},
    );

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

    var sfd = [_]std.posix.pollfd{.{
        .fd = try icon.fd(),
        .events = std.posix.POLL.IN,
        .revents = 0,
    }};

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

    _ = try menu.addSeparator();

    // add demo items
    var new_icon_ctx = Context{ .icon = &icon, .target_id = open_item };
    const new_icon_item = try menu.addItem("Change item icon", onChangeIcon, &new_icon_ctx);
    try menu.setItemIcon(new_icon_item, "dialog-question");
    new_icon_ctx.item_id = new_icon_item;

    var status_ctx = Context{ .icon = &icon };
    const status_item = try menu.addItem("Change status", onAttention, &status_ctx);
    status_ctx.item_id = status_item;

    var tooltip_ctx = Context{ .icon = &icon };
    const tooltip_item = try menu.addItem("Change tooltip", onTooltip, &tooltip_ctx);
    tooltip_ctx.item_id = tooltip_item;

    var pixmap_ctx = Context{ .icon = &icon };
    const pixmap_item = try menu.addItem("Set custom Pixmap", onCustomPixmap, &pixmap_ctx);
    pixmap_ctx.item_id = pixmap_item;

    _ = try menu.addSeparator();

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

    // register with the system
    const registered = try icon.register();
    if (!registered) {
        std.debug.print("No tray watcher running yet\n", .{});
    }

    // set title
    try icon.setTitle("Demo title");

    // main event loop
    while (is_active) {
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

        // listen for D-Bus data
        _ = try std.posix.poll(&sfd, -1);
        icon.processEvents();
    }

    std.debug.print("Exiting\n", .{});
}

fn onClick(user_data: ?*anyopaque) void {
    _ = user_data;
    std.debug.print("Tray icon clicked!\n", .{});
}

fn onCustomPixmap(menu_id: i32, user_data: ?*anyopaque) void {
    _ = menu_id;
    std.debug.print("Switching to a custom pixmap\n", .{});

    const ctx = @as(*Context, @ptrCast(@alignCast(user_data.?)));
    const custom_icon = createCustomIcon(ctx.icon.allocator, 0xFFA020F0) catch return;
    defer ctx.icon.allocator.free(custom_icon);
    ctx.icon.setIconPixmap(16, 16, custom_icon) catch return;
    ctx.icon.setMenuItemEnabled(ctx.item_id.?, false);
}

fn onChangeIcon(menu_id: i32, user_data: ?*anyopaque) void {
    _ = menu_id;
    std.debug.print("Changing the item icon\n", .{});

    const ctx = @as(*Context, @ptrCast(@alignCast(user_data.?)));
    ctx.icon.menu.?.setItemIcon(ctx.item_id.?, "dialog-error") catch return;
    ctx.icon.setMenuItemEnabled(ctx.item_id.?, false);
}

fn onAttention(menu_id: i32, user_data: ?*anyopaque) void {
    _ = menu_id;
    std.debug.print("Changing the status to 'Needs attention'\n", .{});

    const ctx = @as(*Context, @ptrCast(@alignCast(user_data.?)));
    ctx.icon.setStatus(.needs_attention);
    ctx.icon.setMenuItemEnabled(ctx.item_id.?, false);
}

fn onTooltip(menu_id: i32, user_data: ?*anyopaque) void {
    _ = menu_id;
    std.debug.print("Changing the tooltip\n", .{});

    const ctx = @as(*Context, @ptrCast(@alignCast(user_data.?)));
    ctx.icon.setTooltip("Demo", "text") catch return;
    ctx.icon.setMenuItemEnabled(ctx.item_id.?, false);
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
