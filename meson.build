# ──────────────────────────────────────────────────────────────
# Nykhara — Modular Wayland+Vulkan GUI Framework
# Pure C23, Meson + Clang
# ──────────────────────────────────────────────────────────────

project('Nykhara', 'c',
    version : '0.1.0',
    default_options : [
        'warning_level=3',
        'c_std=c2x',
        'buildtype=debugoptimized',
        'b_lto=true',
        'b_ndebug=if-release',
    ],
    meson_version : '>= 1.0.0',
)

cc = meson.get_compiler('c')

# ── Compiler flags ──

add_project_arguments(
    '-Wno-unused-parameter',
    '-Wno-missing-field-initializers',
    '-D_POSIX_C_SOURCE=200809L',
    '-D_DEFAULT_SOURCE',
    language : 'c',
)

fast_args = [
    '-march=native',
    '-ffast-math',
    '-fno-plt',
    '-fomit-frame-pointer',
]

# ── Include directories ──

inc_root    = include_directories('include')
inc_src     = include_directories('src')

inc_external = [
    include_directories('include/volk'),
    include_directories('include/stb'),
    include_directories('include/luajit/src'),
    include_directories('include/nanosvg/src'),
]

# ── Dependencies ──

# Vulkan headers (we use volk, so no linking to libvulkan.so)
vulkan_headers = dependency('vulkan',
    required : true,
).partial_dependency(compile_args : true, includes : true)

# Wayland
wayland_client  = dependency('wayland-client', required : true)
wayland_protos  = dependency('wayland-protocols', required : true)
wayland_scanner_dep = dependency('wayland-scanner', required : true, native : true)
wayland_scanner = find_program(
    wayland_scanner_dep.get_variable('wayland_scanner'),
    required : true,
)

# System libs
# On musl libc, dl and m are built into libc — no separate libraries exist.
libdl = cc.find_library('dl', required : false)
libm  = cc.find_library('m',  required : false)
gbm   = dependency('gbm', required : true)

# LuaJIT — try system first, then local build
luajit_install = meson.project_source_root() / 'subprojects' / 'luajit-install'
luajit = dependency('luajit', required : false)
if not luajit.found()
    # Headers are in include/luajit/src ; library was built from subprojects/luajit-src
    luajit = declare_dependency(
        include_directories : include_directories('include/luajit/src'),
        dependencies : cc.find_library('luajit-5.1',
            dirs : [luajit_install / 'lib'],
            required : true),
    )
endif

# ── Vendored headers (single-file libs in include/) ──
# volk.h, nanosvg.h, nanosvgrast.h, stb_truetype.h, stb_rect_pack.h, etc.
# These are included directly; volk.c is compiled as a source file.

# ── Wayland protocol code generation ──

wl_protos_dir = wayland_protos.get_variable('pkgdatadir')

protocols = {
    'xdg-shell': wl_protos_dir / 'stable/xdg-shell/xdg-shell.xml',
}

protocol_sources = []
protocol_headers = []

foreach name, xml : protocols
    header = custom_target(
        name + '-client-header',
        input   : xml,
        output  : name + '-client-protocol.h',
        command : [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
    )
    source = custom_target(
        name + '-protocol-code',
        input   : xml,
        output  : name + '-protocol.c',
        command : [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
    )
    protocol_headers += header
    protocol_sources += source
endforeach

# ── Shader compilation (GLSL → SPIR-V) ──

glslc = find_program('glslc', required : true)

shader_files = [
    'shaders/sdf_rect.vert',
    'shaders/sdf_rect.frag',
    'shaders/sdf_text.vert',
    'shaders/sdf_text.frag',
]

compiled_shaders = []
foreach shader : shader_files
    spv_name = shader.replace('shaders/', '') + '.spv'
    compiled_shaders += custom_target(
        spv_name,
        input   : shader,
        output  : spv_name,
        command : [glslc, '--target-env=vulkan1.3', '-O', '@INPUT@', '-o', '@OUTPUT@'],
        install     : true,
        install_dir : get_option('datadir') / 'nykhara' / 'shaders',
    )
endforeach

# ── Source files ──

core_sources = [
    # Core utilities
    'src/core/arena.c',
    'src/core/ring_buffer.c',
    'src/core/hash.c',
    'src/core/nk_log.c',

    # volk implementation (compiled once)
    'include/volk/volk.c',

    # Vulkan render
    'src/render/vk_init.c',
    'src/render/vk_swapchain.c',
    'src/render/vk_pipeline.c',
    'src/render/vk_cmd.c',
    'src/render/vk_mem.c',
    'src/render/vk_render.c',
    'src/render/sdf_renderer.c',
    'src/render/font/font_atlas.c',
    'src/render/font/ttf_loader.c',
    'src/render/font/sdf_rasterizer.c',
    'src/render/font/rect_packer.c',

    # Layout engine
    'src/layout/node.c',
    'src/layout/flex.c',

    # Widgets
    'src/widget/widget_base.c',
    'src/widget/text_render.c',
    'src/widget/box.c',
    'src/widget/button.c',
    'src/widget/switch_widget.c',
    'src/widget/label.c',

    # Animation
    'src/animation/easing.c',
    'src/animation/anim_engine.c',
    'src/animation/morph.c',

    # Input
    'src/input/input_mgr.c',

    # Scripting (LuaJIT)
    'src/scripting/lua_vm.c',

    # SVG
    'src/svg/svg_loader.c',

    # Plugin system
    'src/plugin/plugin_loader.c',

    # Platform — Wayland
    'src/platform/wayland/wl_display.c',
]

# Debug sources (conditional)
debug_sources = []
if get_option('buildtype').startswith('debug') or get_option('enable_debug_profiling')
    debug_sources += 'src/debug/vk_timestamps.c'
endif

all_sources = core_sources + debug_sources + protocol_sources + protocol_headers

# ── All dependencies ──

core_deps = [
    vulkan_headers,
    wayland_client,
    gbm,
    libdl,
    libm,
    luajit,
]

# ── Shared library: libnykhara.so ──

libnykhara = shared_library('nykhara',
    all_sources,
    include_directories : [inc_root, inc_src] + inc_external,
    dependencies        : core_deps,
    c_args              : fast_args + ['-DVK_NO_PROTOTYPES', '-DVK_USE_PLATFORM_WAYLAND_KHR'],
    install             : true,
)

nykhara_dep = declare_dependency(
    link_with           : libnykhara,
    include_directories : [inc_root],
    dependencies        : core_deps,
)

# ── Demo executable ──

executable('nykhara-demo',
    'src/app/main.c',
    'src/app/morph_demo.c',
    'src/app/fallback_ui.c',
    'src/app/fps_counter.c',
    include_directories : [inc_root, inc_src] + inc_external,
    dependencies        : [nykhara_dep],
    c_args              : fast_args + ['-DVK_NO_PROTOTYPES', '-DVK_USE_PLATFORM_WAYLAND_KHR'],
    install             : true,
)

# ── Install Lua scripts ──

install_subdir('lua', install_dir : get_option('datadir') / 'nykhara')

# ── Install public headers ──

install_subdir('include/nykhara', install_dir : get_option('includedir'))
