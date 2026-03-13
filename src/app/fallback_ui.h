/**
 * @file fallback_ui.h
 * @brief Builds a simple placeholder UI when no Lua script is loaded.
 */

#ifndef NK_FALLBACK_UI_H
#define NK_FALLBACK_UI_H

#include "../widget/widget_base.h"

/**
 * Create the default fallback widget tree (FPS label, button, switch).
 * Returns NULL on allocation failure.
 */
NkWidget *nk_build_fallback_ui(void);

#endif /* NK_FALLBACK_UI_H */
