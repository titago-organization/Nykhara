-- theme.lua
-- Material Design You color tokens.
-- Edit this file to retheme the entire UI without recompiling.

local M = {}

-- Material Design 3 — Purple baseline palette
M.colors = {
    -- Primary
    primary                  = { 0.40, 0.31, 0.64, 1.0 },  -- #6750A4
    on_primary               = { 1.0,  1.0,  1.0,  1.0 },  -- #FFFFFF
    primary_container        = { 0.92, 0.83, 1.0,  1.0 },  -- #EADDFF
    on_primary_container     = { 0.13, 0.03, 0.34, 1.0 },  -- #21005D

    -- Secondary
    secondary                = { 0.39, 0.36, 0.47, 1.0 },  -- #625B71
    on_secondary             = { 1.0,  1.0,  1.0,  1.0 },
    secondary_container      = { 0.91, 0.87, 0.96, 1.0 },  -- #E8DEF8
    on_secondary_container   = { 0.11, 0.09, 0.18, 1.0 },

    -- Tertiary
    tertiary                 = { 0.49, 0.33, 0.39, 1.0 },  -- #7D5260
    on_tertiary              = { 1.0,  1.0,  1.0,  1.0 },

    -- Surface
    surface                  = { 0.07, 0.07, 0.08, 1.0 },  -- #1C1B1F (dark)
    on_surface               = { 0.90, 0.89, 0.93, 1.0 },  -- #E6E1E5
    surface_container        = { 0.12, 0.11, 0.14, 1.0 },  -- #1F1D23

    -- Error
    error                    = { 0.70, 0.15, 0.15, 1.0 },  -- #B3261E
    on_error                 = { 1.0,  1.0,  1.0,  1.0 },

    -- Outline
    outline                  = { 0.47, 0.44, 0.52, 1.0 },  -- #79747E
}

-- Typography scale
M.typography = {
    display_large  = 57,
    display_medium = 45,
    display_small  = 36,
    headline_large = 32,
    headline_medium = 28,
    headline_small = 24,
    title_large    = 22,
    title_medium   = 16,
    title_small    = 14,
    body_large     = 16,
    body_medium    = 14,
    body_small     = 12,
    label_large    = 14,
    label_medium   = 12,
    label_small    = 11,
}

-- Spacing / Shape
M.shape = {
    corner_none   = 0,
    corner_xs     = 4,
    corner_sm     = 8,
    corner_md     = 12,
    corner_lg     = 16,
    corner_xl     = 28,
    corner_full   = 9999,  -- Pill shape
}

return M

