cmake_minimum_required(VERSION 3.16)
project(psp3ds_host C)
set(CMAKE_C_STANDARD 11)
add_library(psp_core STATIC source/psp_core.c source/psp_loader.c source/psp_hle.c source/psp_imports.c source/psp_nids.c source/psp_prx.c source/psp_prx_inventory.c source/psp_module.c source/psp_reloc.c source/psp_reloc2.c source/psp_reloc_image.c source/psp_perf.c source/psp_iso.c source/psp_cso.c source/psp_disc.c source/psp_paths.c source/psp_catalog.c source/psp_demand.c source/psp_threads.c source/psp_thread_cpu.c source/psp_umd.c source/psp_io.c source/psp_boot.c source/psp_boot_auto.c source/psp_slot2_probe.c source/psp_benchmark.c source/psp_guest_window.c source/psp_guest_bus.c source/psp_rom_pager.c source/psp_rom_file.c source/psp_platform_budget.c source/psp_ds_memory.c source/psp_memory_guard.c source/psp_play_cutbacks.c source/psp_guarded_prx.c source/psp_startup_trace.c source/psp_3ds_memory.c source/psp_3ds_storage.c source/psp_prx_image.c source/psp_runtime_tuning.c source/psp_runtime_guest.c source/psp_lazy_backends.c source/psp_on_demand_guest.c source/psp_video_layout.c source/psp_nintendo_input.c source/psp_native_texture.c source/psp_ge_triangles.c source/psp_features.c source/psp_scene.c source/psp_scene_batch.c source/psp_ge_subset.c)
target_include_directories(psp_core PUBLIC include)
find_package(ZLIB REQUIRED)
target_link_libraries(psp_core PUBLIC ZLIB::ZLIB)
enable_testing()
foreach(name core loader integer allegrex4 exceptions hle imports prx module reloc)
  add_executable(test_${name} tests/test_${name}.c)
  target_link_libraries(test_${name} PRIVATE psp_core)
  add_test(NAME ${name} COMMAND test_${name})
endforeach()

add_executable(test_reloc_hilo tests/test_reloc_hilo.c)
target_link_libraries(test_reloc_hilo PRIVATE psp_core)
add_test(NAME reloc_hilo COMMAND test_reloc_hilo)

add_executable(test_reloc2 tests/test_reloc2.c)
target_link_libraries(test_reloc2 PRIVATE psp_core)
add_test(NAME reloc2 COMMAND test_reloc2)

foreach(name perf iso perf_runtime)
  add_executable(test_${name} tests/test_${name}.c)
  target_link_libraries(test_${name} PRIVATE psp_core)
  add_test(NAME ${name} COMMAND test_${name})
endforeach()

add_executable(test_cso tests/test_cso.c)
target_link_libraries(test_cso PRIVATE psp_core)
add_test(NAME cso COMMAND test_cso)

add_executable(psp_disc_probe tools/psp_disc_probe.c)
target_link_libraries(psp_disc_probe PRIVATE psp_core)
add_executable(test_disc tests/test_disc.c)
target_link_libraries(test_disc PRIVATE psp_core)
add_test(NAME disc COMMAND test_disc)

add_executable(test_paths tests/test_paths.c)
target_link_libraries(test_paths PRIVATE psp_core)
add_test(NAME paths COMMAND test_paths)

add_executable(test_demand tests/test_demand.c)
target_link_libraries(test_demand PRIVATE psp_core)
add_test(NAME demand COMMAND test_demand)

add_executable(test_demand_runtime tests/test_demand_runtime.c)
target_link_libraries(test_demand_runtime PRIVATE psp_core)
add_test(NAME demand_runtime COMMAND test_demand_runtime)

# Verify release layout without installing any Nintendo cross toolchain.
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
  add_test(NAME packaging COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_packaging.py)
endif()

add_executable(test_catalog tests/test_catalog.c)
target_link_libraries(test_catalog PRIVATE psp_core)
add_test(NAME catalog COMMAND test_catalog)

if(Python3_Interpreter_FOUND)
  add_test(NAME native_scripts COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_native_scripts.py)
endif()

add_executable(test_umd tests/test_umd.c)
target_link_libraries(test_umd PRIVATE psp_core)
add_test(NAME umd COMMAND test_umd)

add_executable(test_io tests/test_io.c)
target_link_libraries(test_io PRIVATE psp_core)
add_test(NAME io COMMAND test_io)

add_executable(test_real_nids tests/test_real_nids.c)
target_link_libraries(test_real_nids PRIVATE psp_core)
add_test(NAME real_nids COMMAND test_real_nids)

add_executable(test_module_link tests/test_module_link.c)
target_link_libraries(test_module_link PRIVATE psp_core)
add_test(NAME module_link COMMAND test_module_link)

add_executable(test_boot tests/test_boot.c)
target_link_libraries(test_boot PRIVATE psp_core)
add_test(NAME boot COMMAND test_boot)

if(Python3_Interpreter_FOUND)
  add_test(NAME installers COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_installers.py)
endif()

if(Python3_Interpreter_FOUND)
  add_test(NAME cia_packaging COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_cia_packaging.py)
endif()

if(Python3_Interpreter_FOUND)
  add_test(NAME emulator_screenshot_ci COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_emulator_screenshot_ci.py)
endif()

if(Python3_Interpreter_FOUND)
  add_test(NAME azahar_screenshot_ci COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_azahar_screenshot_ci.py)
endif()

if(Python3_Interpreter_FOUND)
  add_test(NAME capture_manifest COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_capture_manifest.py)
endif()

if(Python3_Interpreter_FOUND)
  add_test(NAME capture_artifact COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_capture_artifact.py)
endif()

add_executable(test_event_control tests/test_event_control.c)
target_link_libraries(test_event_control PRIVATE psp_core)
add_test(NAME event_control COMMAND test_event_control)

add_executable(test_wait_signal tests/test_wait_signal.c)
target_link_libraries(test_wait_signal PRIVATE psp_core)
add_test(NAME wait_signal COMMAND test_wait_signal)

add_executable(test_threads tests/test_threads.c)
target_link_libraries(test_threads PRIVATE psp_core)
add_test(NAME threads COMMAND test_threads)

add_executable(test_thread_cpu tests/test_thread_cpu.c)
target_link_libraries(test_thread_cpu PRIVATE psp_core)
add_test(NAME thread_cpu COMMAND test_thread_cpu)

add_executable(test_thread_hle tests/test_thread_hle.c)
target_link_libraries(test_thread_hle PRIVATE psp_core)
add_test(NAME thread_hle COMMAND test_thread_hle)

if(Python3_Interpreter_FOUND)
  add_test(NAME azahar_boot_marker COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_azahar_boot_marker.py)
endif()

add_executable(test_loader_safety tests/test_loader_safety.c)
target_link_libraries(test_loader_safety PRIVATE psp_core)
add_test(NAME loader_safety COMMAND test_loader_safety)

add_executable(test_reloc_image tests/test_reloc_image.c)
target_link_libraries(test_reloc_image PRIVATE psp_core)
add_test(NAME reloc_image COMMAND test_reloc_image)

add_executable(test_boot_relocated tests/test_boot_relocated.c)
target_link_libraries(test_boot_relocated PRIVATE psp_core)
add_test(NAME boot_relocated COMMAND test_boot_relocated)

add_executable(test_boot_auto tests/test_boot_auto.c)
target_link_libraries(test_boot_auto PRIVATE psp_core)
add_test(NAME boot_auto COMMAND test_boot_auto)

add_executable(test_slot2_probe tests/test_slot2_probe.c)
target_link_libraries(test_slot2_probe PRIVATE psp_core)
add_test(NAME slot2_probe COMMAND test_slot2_probe)

add_executable(test_benchmark tests/test_benchmark.c)
target_link_libraries(test_benchmark PRIVATE psp_core)
add_test(NAME benchmark COMMAND test_benchmark)

if(Python3_Interpreter_FOUND)
  add_test(NAME melonds_ci COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_melonds_ci.py)
endif()

add_executable(test_slot2_sweep tests/test_slot2_sweep.c)
target_link_libraries(test_slot2_sweep PRIVATE psp_core)
add_test(NAME slot2_sweep COMMAND test_slot2_sweep)

add_executable(test_guest_window tests/test_guest_window.c)
target_link_libraries(test_guest_window PRIVATE psp_core)
add_test(NAME guest_window COMMAND test_guest_window)

add_executable(test_guest_bus tests/test_guest_bus.c)
target_link_libraries(test_guest_bus PRIVATE psp_core)
add_test(NAME guest_bus COMMAND test_guest_bus)

add_executable(test_platform_budget tests/test_platform_budget.c)
target_link_libraries(test_platform_budget PRIVATE psp_core)
add_test(NAME platform_budget COMMAND test_platform_budget)

add_executable(test_features tests/test_features.c)
target_link_libraries(test_features PRIVATE psp_core)
add_test(NAME feature_parity COMMAND test_features)

if(Python3_Interpreter_FOUND)
  add_test(NAME feature_wiring COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_feature_wiring.py)
endif()

add_executable(test_scene tests/test_scene.c)
target_link_libraries(test_scene PRIVATE psp_core)
add_test(NAME shared_scene COMMAND test_scene)

add_executable(test_scene_batch tests/test_scene_batch.c)
target_link_libraries(test_scene_batch PRIVATE psp_core)
add_test(NAME scene_batch COMMAND test_scene_batch)

if(Python3_Interpreter_FOUND)
  add_test(NAME azahar_gpu_scene_detector COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test_azahar_gpu_scene.py)
endif()

add_executable(test_ge_subset tests/test_ge_subset.c)
target_link_libraries(test_ge_subset PRIVATE psp_core)
add_test(NAME ge_subset COMMAND test_ge_subset)

# OpenGL-free video mapping is shared by DS, DSi, original and New 3DS.
add_executable(test_video_layout tests/test_video_layout.c)
target_link_libraries(test_video_layout PRIVATE psp_core)
add_test(NAME video_layout COMMAND test_video_layout)

# PSP sceCtrl-compatible Nintendo pad mapping, shared across DS/DSi/3DS.
add_executable(test_nintendo_input tests/test_nintendo_input.c)
target_link_libraries(test_nintendo_input PRIVATE psp_core)
add_test(NAME nintendo_input COMMAND test_nintendo_input)

# Source/texture decode without OpenGL, Vulkan, Direct3D or runtime allocations.
add_executable(test_native_texture tests/test_native_texture.c)
target_link_libraries(test_native_texture PRIVATE psp_core)
add_test(NAME native_texture COMMAND test_native_texture)

# The same fail-closed PSP GE triangle commands compile on DS, Pak, DSi, 3DS.
add_executable(test_ge_triangles tests/test_ge_triangles.c)
target_link_libraries(test_ge_triangles PRIVATE psp_core)
add_test(NAME ge_triangles COMMAND test_ge_triangles)

# Native CPU clock/graphics workload policy is larger only on New 3DS.
add_executable(test_runtime_tuning tests/test_runtime_tuning.c)
target_link_libraries(test_runtime_tuning PRIVATE psp_core)
add_test(NAME runtime_tuning COMMAND test_runtime_tuning)

# Per-model policy now schedules actual shared MIPS/HLE instructions/events.
add_executable(test_runtime_guest tests/test_runtime_guest.c)
target_link_libraries(test_runtime_guest PRIVATE psp_core)
add_test(NAME runtime_guest COMMAND test_runtime_guest)

# Large real-world-class PSP PRX relocation workload (synthetic data only).
add_executable(test_reloc_large tests/test_reloc_large.c)
target_link_libraries(test_reloc_large PRIVATE psp_core)
add_test(NAME reloc_large COMMAND test_reloc_large)

# PSP relocatable EBOOT (ET_SCE_PRX) mapped image with fail-closed rollback.
add_executable(test_prx_image tests/test_prx_image.c)
target_link_libraries(test_prx_image PRIVATE psp_core)
add_test(NAME prx_image COMMAND test_prx_image)

# User-supplied decrypted ET_SCE_PRX file probe. Never stages private game
# bytes, does not execute guest code, and is not a runtime test target.
add_executable(psp_prx_probe tools/psp_prx_probe.c)
target_link_libraries(psp_prx_probe PRIVATE psp_core)

# Read-only import inventory for PRX with >64 HLE imports (145 synthetic).
add_executable(test_prx_inventory tests/test_prx_inventory.c)
target_link_libraries(test_prx_inventory PRIVATE psp_core)
add_test(NAME prx_inventory COMMAND test_prx_inventory)

# 512 MB-capable ROM address paging with 4 KiB RAM pages and optional Slot-2 tier.
add_executable(test_rom_pager tests/test_rom_pager.c)
target_link_libraries(test_rom_pager PRIVATE psp_core)
add_test(NAME rom_pager COMMAND test_rom_pager)

# Original/New 3DS live application-region sizing and 64-bit SD streaming.
add_executable(test_3ds_memory tests/test_3ds_memory.c)
target_link_libraries(test_3ds_memory PRIVATE psp_core)
add_test(NAME memory_3ds COMMAND test_3ds_memory)
add_executable(test_3ds_storage tests/test_3ds_storage.c)
target_link_libraries(test_3ds_storage PRIVATE psp_core)
add_test(NAME storage_3ds COMMAND test_3ds_storage)

# DS and DSi memory plans: physical 4/16MiB, sampled Slot-2 RAM, small hot
# guest-heap caps and fail-closed access after peripheral shutdown.
add_executable(test_ds_memory tests/test_ds_memory.c)
target_link_libraries(test_ds_memory PRIVATE psp_core)
add_test(NAME memory_ds COMMAND test_ds_memory)

# Safe PSP game-session resource quotas and ownership/leak tracking for all
# five supported device profiles (DS, verified Pak, DSi, old/New 3DS).
add_executable(test_memory_guard tests/test_memory_guard.c)
target_link_libraries(test_memory_guard PRIVATE psp_core)
add_test(NAME memory_guard COMMAND test_memory_guard)

# Ownership-checked executable loads on the real shared PSP PRX mapper;
# denies oversized DS/DSi images rather than exhausting the console RAM.
add_executable(test_guarded_prx tests/test_guarded_prx.c)
target_link_libraries(test_guarded_prx PRIVATE psp_core)
add_test(NAME guarded_prx COMMAND test_guarded_prx)

# Render/workload cutbacks and soft resource admissions, preserving CPU/HLE
# and input/audio updates on all five hardware families.
add_executable(test_play_cutbacks tests/test_play_cutbacks.c)
target_link_libraries(test_play_cutbacks PRIVATE psp_core)
add_test(NAME play_cutbacks COMMAND test_play_cutbacks)

# On-demand PSP CPU/HLE plus lazy GPU/audio/UMD requests across all console
# profiles. Never skip guest instructions, pending work, or virtual timers.
add_executable(test_lazy_backends tests/test_lazy_backends.c)
target_link_libraries(test_lazy_backends PRIVATE psp_core)
add_test(NAME lazy_backends COMMAND test_lazy_backends)
add_executable(test_on_demand_guest tests/test_on_demand_guest.c)
target_link_libraries(test_on_demand_guest PRIVATE psp_core)
add_test(NAME on_demand_guest COMMAND test_on_demand_guest)

# All-console startup-checkpoint parser and deterministic first-missing-PSP-feature trace.
add_executable(test_startup_trace tests/test_startup_trace.c)
target_link_libraries(test_startup_trace PRIVATE psp_core)
add_test(NAME startup_trace COMMAND test_startup_trace)
add_executable(psp_startup_probe tools/psp_startup_probe.c)
target_link_libraries(psp_startup_probe PRIVATE psp_core)

# Exact firmware import reached by the private PSP startup diagnostic:
# verified SysMemUserForUser SDK-version setter, not other fake sysmem calls.
add_executable(test_hle_sdk_version tests/test_hle_sdk_version.c)
target_link_libraries(test_hle_sdk_version PRIVATE psp_core)
add_test(NAME hle_sdk_version COMMAND test_hle_sdk_version)
