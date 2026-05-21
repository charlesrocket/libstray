const Proc = struct {
    term: std.process.Child.Term,
    out: []u8,
    err: []u8,
};

fn runner(args: [1][]const u8, io: std.Io) !Proc {
    var proc = try std.process.spawn(io, .{
        .argv = &args,
        .stdin = .pipe,
        .stdout = .pipe,
        .stderr = .pipe,
    });

    var out_rbuf: [4096]u8 = undefined;
    var err_rbuf: [4096]u8 = undefined;
    var out_reader = proc.stdout.?.reader(io, &out_rbuf);
    var err_reader = proc.stderr.?.reader(io, &err_rbuf);
    var out: []u8 = &.{};
    var err: []u8 = &.{};
    var group: std.Io.Group = .init;

    group.async(io, struct {
        fn f(r: *std.Io.Reader, result: *[]u8) void {
            result.* = r.readAlloc(allocator, 13312) catch &.{};
        }
    }.f, .{ &out_reader.interface, &out });

    group.async(io, struct {
        fn f(r: *std.Io.Reader, result: *[]u8) void {
            result.* = r.readAlloc(allocator, 13312) catch &.{};
        }
    }.f, .{ &err_reader.interface, &err });

    try group.await(io);

    const term = try proc.wait(io);
    if (err.len > 0) std.debug.print("{s}\n", .{err});
    return .{ .term = term, .out = out, .err = err };
}

test "app" {
    const argv = [1][]const u8{
        test_app,
    };

    const proc = try runner(argv, std.testing.io);
    defer {
        allocator.free(proc.out);
        allocator.free(proc.err);
    }

    try std.testing.expectEqual(proc.term.exited, 0);
}

const std = @import("std");
const allocator = std.testing.allocator;
const build_options = @import("build_options");
const test_app = build_options.test_app_path;
