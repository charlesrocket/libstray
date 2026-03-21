# Changelog

All notable changes to this project will be documented in this file.

## [0.4.0] - 2026-03-21

### Bug Fixes

- Update app layout
- Improve submenu handling
- Add test item
- Change label
- Check all strings in `stray_create`
- Improve menu checks
- Handle multiple instances
- Improve scroll handling
- Ensure `NewStatus` is flushed
- Check message type in `register_with_watcher()`
- Clean up `emit_properties_changed()`
- Remove redundant signals from `stray_set_icon_pixmap()`
- Use global menu id root
- Drop redundant `WindowId`
- Check `create_menu_item()`
- Safeguard filter/match rule
- Check pixmap size
- Update `setIconPixmap()`
- Correct checked item

### Documentation

- Update `fd()` description

### Features

- Add file descriptor
- [**breaking**] Track registrations
- [**breaking**] Add x/y callback coordinates
- Add `onScroll`
- Add `window_id`
- Add `onRemovePixmap()`

### Performance

- Drop `usleep()`

### Refactor

- Move `*RadioGroup`
- Improve `stray_destroy()` logic
- Consistent user data fields
- Drop redundant zeroing loop
- Drop redundant `emit_properties_changed()`

### Styling

- Fix formatting
- Disable `AlignAfterOpenBracket`
- Align open brackets
- Align operands

### Testing

- Add callbacks

## [0.3.2] - 2026-03-18

### Bug Fixes

- Improve `Properties` handling
- Correct pixel data
- Update custom icon color
- Fix `Menu` property
- Add status argument
- Update `AboutToShowGroup`
- Safeguard iterators
- Check watcher presence
- Track `app_id`
- Flush connections
- Set active status

## [0.3.1] - 2026-03-14

### Bug Fixes

- Add connection filter
- Track registrations

## [0.3.0] - 2026-01-31

### Bug Fixes

- Resolve recursive submenu
- Implement menu revision
- Clean up redundancies
- Signal updated layout
- Drop redundant `setIconPixmap()`
- Signal new status
- Adjust demo status signal

### Features

- Add submenu support
- Add menu icons
- Implement icon statuses
- Add button/scroll callback

### Operations

- Bump actions/checkout from 5 to 6

### Refactor

- Switch to `ArrayList`

### Testing

- Add submenu
- Add icons

## [0.2.0] - 2025-11-10

### Bug Fixes

- Drop redundant error print
- Improve watcher error
- Resolve leaks
- `icon` destroys the menu
- Set proper service name
- Resolve dynamic updates
- Clean up pixmap infra

### Documentation

- Update description
- Fix gnome description

### Features

- [**breaking**] Add allow item control
- [**breaking**] Add pixmap support
- Add tooltips
- Add radio item support

### Styling

- Fix `stray_register()`
- Update format

### Testing

- Add disabled item
- Add title
- Update pixmaps
- Add checked item

## [0.1.1] - 2025-11-02

### Bug Fixes

- Drop `c_allocator`
- Resolve leaking items
- Fix array list
- Update `onQuit`

### Documentation

- Update description
- Adjust description
- Reformat comments

### Miscellaneous tasks

- Add CI badge

### Operations

- Add test job

### Testing

- Add basic app

### Build

- Bump MSZV to 0.15.2

## [0.1.0] - 2025-10-30

### Bug Fixes

- Update `callconv()`
- Update `sleep()`

### Documentation

- Add README.md
- Update features
- Fix `TrayIcon` comment
- Update comments

### Features

- Add menu support
- Add demo app

### Miscellaneous tasks

- Add LICENSE
- Set indentations
- Add gitignore
- Ignore `zig-out`

### Operations

- Add demo test
- Install dependencies

### Refactor

- Move declarations

### Styling

- Change alignment
- Fix formatting
- Break open brackets

### Build

- Add `docs` step
- Fix library links


