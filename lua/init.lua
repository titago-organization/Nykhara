local Nykhara = require("Nykhara")
local Layouts = require("Nykhara.Layouts")
local theme = require("lua.theme")

return Layouts.Column {
    padding = 22,
    gap = 14,
    background = theme.colors.surface,

    Nykhara.Label {
        text = "FPS: 0",
        font_size = 16,
        color = theme.colors.on_surface,
        height = 26,
    },

    Layouts.Row {
        height = 62,
        gap = 12,
        padding = { all = 10 },
        align = "center",
        background = theme.colors.surface_container,
        corner_radius = 14,

        Nykhara.Button {
            text = "Reload test",
            width = 200,
            height = 44,
            background = theme.colors.primary,
            corner_radius = 22,
            on_click = function(ev)
                print(string.format("button click id=%d text=%s value=%s", ev.id, ev.text, tostring(ev.value)))
            end,
        },

        

        Layouts.Box {
        flex_grow = 1,
        background = { 0.12, 0.11, 0.14, 1.0 },
        corner_radius = 16,
        border_width = 1,
        border_color = { 0.47, 0.44, 0.52, 0.6 },
        },
    },
    Nykhara.Switch {
            value = true,
            width = 800,
            height = 470,
            track_on = theme.colors.primary,
            track_off = theme.colors.outline,
            on_click = function(ev)
                print("switch value =", ev.value)
            end,
    },
    Nykhara.Label {
        text = "Penis",
        font_size = 80,
        color = theme.colors.on_surface,
        height = 80,
    },


    
}
