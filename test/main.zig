const Proc = struct {
    term: std.process.Child.Term,
    out: []u8,
    err: []u8,
};

fn runner(args: [1][]const u8) !Proc {
    var proc = std.process.Child.init(&args, allocator);

    proc.stdout_behavior = .Pipe;
    proc.stderr_behavior = .Pipe;

    var stdout: std.ArrayListAlignedUnmanaged(u8, null) = .empty;
    var stderr: std.ArrayListAlignedUnmanaged(u8, null) = .empty;
    defer {
        stdout.deinit(allocator);
        stderr.deinit(allocator);
    }

    try proc.spawn();
    try proc.collectOutput(allocator, &stdout, &stderr, 13312);

    const term = try proc.wait();
    const out = try stdout.toOwnedSlice(allocator);
    const err = try stderr.toOwnedSlice(allocator);

    return Proc{ .term = term, .out = out, .err = err };
}

test "app" {
    const argv = [1][]const u8{
        test_app,
    };

    const proc = try runner(argv);
    defer {
        allocator.free(proc.out);
        allocator.free(proc.err);
    }

    try std.testing.expectEqual(proc.term.Exited, 0);
}

const std = @import("std");
const allocator = std.testing.allocator;

const build_options = @import("build_options");
const test_app = build_options.test_app_path;
