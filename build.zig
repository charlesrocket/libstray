const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const lib_mod = b.createModule(.{
        .root_source_file = b.path("root.zig"),
        .target = target,
        .optimize = optimize,
    });

    const lib = b.addLibrary(.{
        .linkage = .static,
        .name = "stray",
        .root_module = lib_mod,
    });

    lib_mod.linkSystemLibrary("dbus-1", .{});
    lib_mod.addIncludePath(b.path("."));
    lib.addCSourceFile(.{
        .file = b.path("wrapper.c"),
    });

    b.installArtifact(lib);

    const lib_unit_tests = b.addTest(.{
        .root_module = lib_mod,
    });

    const run_lib_unit_tests = b.addRunArtifact(lib_unit_tests);

    const test_step = b.step("test", "Run unit tests");
    test_step.dependOn(&run_lib_unit_tests.step);

    const demo_mod = b.createModule(.{
        .root_source_file = b.path("example/demo.zig"),
        .target = target,
        .optimize = optimize,
    });

    const demo_exe = b.addExecutable(.{
        .name = "demo",
        .root_module = demo_mod,
    });

    demo_exe.root_module.addImport("stray", lib_mod);

    const build_demo = b.addInstallArtifact(demo_exe, .{});
    const build_demo_step = b.step("demo", "Build the example app");
    build_demo_step.dependOn(&build_demo.step);

    const run_cmd = b.addRunArtifact(demo_exe);

    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the demo");
    run_step.dependOn(&run_cmd.step);

    const build_docs = b.addInstallDirectory(.{
        .source_dir = lib.getEmittedDocs(),
        .install_dir = .prefix,
        .install_subdir = "../docs",
    });

    const build_docs_step = b.step("docs", "Build the library documentation");
    build_docs_step.dependOn(&build_docs.step);

    const clean_step = b.step("clean", "Clean up the project directory");
    clean_step.dependOn(&b.addRemoveDirTree(b.path("docs")).step);
    clean_step.dependOn(&b.addRemoveDirTree(b.path("zig-out")).step);
    clean_step.dependOn(&b.addRemoveDirTree(b.path(".zig-cache")).step);
}
