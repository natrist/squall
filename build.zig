const std = @import("std");
const system = @import("system");

pub fn build(b: *std.Build) void {
  const target = b.standardTargetOptions(.{});
  const optimize = b.standardOptimizeOption(.{});

  const t = target.result;

  // Squall library
  const storm = b.addStaticLibrary(.{
    .name = "storm",
    .target = target,
    .optimize = optimize
  });
  // Link C++ standard library
  storm.linkLibCpp();
  // Add system detection defines
  system.add_defines(storm);

  // Get dependencies
  const bc = b.dependency("bc", .{});
  // Link bc
  storm.linkLibrary(bc.artifact("bc"));

  // Include squall project directory
  storm.addIncludePath(b.path("."));

  const storm_compiler_flags = [_][]const u8 {
    "-std=c++11",
    "-Wno-invalid-offsetof"
  };

  const storm_sources = [_][]const u8 {
    "storm/big/BigBuffer.cpp",
    "storm/big/BigData.cpp",
    "storm/big/BigStack.cpp",
    "storm/big/Ops.cpp",

    "storm/hash/Hashkey.cpp",

    "storm/queue/CSBasePriority.cpp",
    "storm/queue/CSBasePriorityQueue.cpp",

    "storm/string/bjhash.cpp",

    "storm/thread/CCritSect.cpp",
    "storm/thread/CSRWLock.cpp",
    "storm/thread/S_Thread.cpp",
    "storm/thread/SCritSect.cpp",
    "storm/thread/SEvent.cpp",
    "storm/thread/SSemaphore.cpp",
    "storm/thread/SSyncObject.cpp",
    "storm/thread/SThread.cpp",

    "storm/Atomic.cpp",
    "storm/Big.cpp",
    "storm/Command.cpp",
    "storm/Crypto.cpp",
    "storm/Error.cpp",
    "storm/Log.cpp",
    "storm/Region.cpp",
    "storm/String.cpp",
    "storm/Thread.cpp",
    "storm/Unicode.cpp"
  };

  const storm_windows_sources = [_][]const u8{
    "storm/error/win/Display.cpp",
    "storm/error/win/Error.cpp",
    "storm/thread/win/S_Thread.cpp",
    "storm/thread/win/SRWLock.cpp",
    "storm/thread/win/Thread.cpp"
  };

  const storm_macos_sources = [_][]const u8{
    "storm/error/unix/Display.cpp",
    "storm/thread/mac/S_Thread.mm",
    "storm/thread/mac/SThreadRunner.mm",
    "storm/thread/mac/Thread.mm"
  };

  const storm_linux_sources = [_][]const u8{
    "storm/error/unix/Display.cpp",
    "storm/thread/linux/S_Thread.cpp",
    "storm/thread/linux/Thread.cpp"
  };

  storm.addCSourceFiles(.{
    .files = &storm_sources,
    .flags = &storm_compiler_flags
  });

  switch (t.os.tag) {
    .windows => {
      storm.addCSourceFiles(.{
        .files = &storm_windows_sources,
        .flags = &storm_compiler_flags
      });
    },
    .macos => {
      storm.addCSourceFiles(.{
        .files = &storm_macos_sources,
        .flags = &storm_compiler_flags
      });
    },
    else => {
      storm.addCSourceFiles(.{
        .files = &storm_linux_sources,
        .flags = &storm_compiler_flags
      });
    }
  }

  storm.installHeadersDirectory(b.path("storm"), "storm", .{ .include_extensions = &.{"hpp"} });

  // StormTest executable
  const storm_test_exe = b.addExecutable(.{
    .name = "StormTest",
    .target = target,
    .optimize = optimize
  });
  // Link C++ standard library
  storm_test_exe.linkLibCpp();
  // Add system detection defines
  system.add_defines(storm_test_exe);

  storm_test_exe.linkLibrary(storm);
  storm_test_exe.addIncludePath(b.path("."));

  storm_test_exe.addCSourceFiles(.{
    .files = &.{
      "test/big/Ops.cpp",
      "test/Array.cpp",
      "test/Atomic.cpp",
      "test/Big.cpp",
      "test/Crypto.cpp",
      "test/Hash.cpp",
      "test/List.cpp",
      "test/Log.cpp",
      "test/Memory.cpp",
      "test/Queue.cpp",
      "test/Region.cpp",
      "test/String.cpp",
      "test/Test.cpp",
      "test/Thread.cpp",
      "test/Unicode.cpp",
    },

    .flags = &storm_compiler_flags
  });

  b.installArtifact(storm_test_exe);
  b.installArtifact(storm);
}
