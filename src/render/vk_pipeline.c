/**
 * @file vk_pipeline.c
 * @brief SPIR-V shader loading and SDF pipeline creation.
 */

#include "vk_pipeline.h"
#include "../core/nk_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

/**
 * Resolve a path relative to the directory containing the running executable.
 * E.g. if exe is /foo/buildDir/nykhara-demo and name is "sdf_rect.vert.spv",
 * returns "/foo/buildDir/sdf_rect.vert.spv".
 */
static const char *resolve_shader_path(const char *name, char *buf, size_t buf_size) {
    // First, try the path as-is (e.g. absolute or already correct CWD)
    if (access(name, R_OK) == 0) return name;

    // Resolve relative to executable directory via /proc/self/exe
    ssize_t len = readlink("/proc/self/exe", buf, buf_size - 1);
    if (len <= 0) return name; // Fallback
    buf[len] = '\0';

    // dirname modifies the buffer — get the directory part
    char *dir = dirname(buf);
    size_t dir_len = strlen(dir);

    // Build: dir + "/" + name
    if (dir_len + 1 + strlen(name) >= buf_size) return name;
    // Move dir to start of buf if dirname returned a pointer into buf
    memmove(buf, dir, dir_len);
    buf[dir_len] = '/';
    strcpy(buf + dir_len + 1, name);

    return buf;
}

NkResult nk_vk_load_shader(VkDevice device, const char *path, VkShaderModule *out) {
    // Resolve the shader path relative to executable
    char resolved[1024];
    const char *actual_path = resolve_shader_path(path, resolved, sizeof(resolved));

    FILE *f = fopen(actual_path, "rb");
    if (!f) {
        NK_LOG_ERROR("Failed to open shader: %s (tried: %s)", path, actual_path);
        return NK_ERROR_IO;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint32_t *code = malloc((size_t)size);
    fread(code, 1, (size_t)size, f);
    fclose(f);

    VkShaderModuleCreateInfo ci = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = (size_t)size,
        .pCode    = code,
    };

    VkResult vr = vkCreateShaderModule(device, &ci, NULL, out);
    free(code);

    if (vr != VK_SUCCESS) {
        NK_LOG_ERROR("vkCreateShaderModule failed for %s: %d", path, vr);
        return NK_ERROR_VULKAN;
    }
    return NK_SUCCESS;
}

NkResult nk_pipeline_create_sdf(NkVkContext *vk, VkFormat color_format, NkPipeline *out) {
    memset(out, 0, sizeof(*out));

    // Push constant range
    VkPushConstantRange push_range = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset     = 0,
        .size       = sizeof(NkSdfPushConstants),
    };

    VkPipelineLayoutCreateInfo layout_ci = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &push_range,
    };

    VkResult vr = vkCreatePipelineLayout(vk->device, &layout_ci, NULL, &out->layout);
    if (vr != VK_SUCCESS) {
        NK_LOG_ERROR("vkCreatePipelineLayout failed: %d", vr);
        return NK_ERROR_VULKAN;
    }

    // Load shaders
    VkShaderModule vert_mod, frag_mod;
    NkResult r = nk_vk_load_shader(vk->device, "sdf_rect.vert.spv", &vert_mod);
    if (r != NK_SUCCESS) return r;
    r = nk_vk_load_shader(vk->device, "sdf_rect.frag.spv", &frag_mod);
    if (r != NK_SUCCESS) {
        vkDestroyShaderModule(vk->device, vert_mod, NULL);
        return r;
    }

    VkPipelineShaderStageCreateInfo stages[] = {
        {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert_mod,
            .pName  = "main",
        },
        {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag_mod,
            .pName  = "main",
        },
    };

    // No vertex input — fullscreen quad generated in vertex shader
    VkPipelineVertexInputStateCreateInfo vi = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkPipelineInputAssemblyStateCreateInfo ia = {
        .sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    VkPipelineViewportStateCreateInfo vp = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount  = 1,
    };

    VkPipelineRasterizationStateCreateInfo rs = {
        .sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode    = VK_CULL_MODE_NONE,
        .frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth   = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo ms = {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendAttachmentState blend_att = {
        .blendEnable         = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,                   // premultiplied alpha
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                               VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    VkPipelineColorBlendStateCreateInfo cb = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments    = &blend_att,
    };

    VkDynamicState dyn_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo ds = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates    = dyn_states,
    };

    // Dynamic rendering (Vulkan 1.3 — no render pass)
    VkPipelineRenderingCreateInfo rendering_ci = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = &color_format,
    };

    VkGraphicsPipelineCreateInfo pipe_ci = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext                = &rendering_ci,
        .stageCount          = 2,
        .pStages             = stages,
        .pVertexInputState   = &vi,
        .pInputAssemblyState = &ia,
        .pViewportState      = &vp,
        .pRasterizationState = &rs,
        .pMultisampleState   = &ms,
        .pColorBlendState    = &cb,
        .pDynamicState       = &ds,
        .layout              = out->layout,
    };

    vr = vkCreateGraphicsPipelines(vk->device, VK_NULL_HANDLE, 1, &pipe_ci, NULL, &out->pipeline);

    vkDestroyShaderModule(vk->device, vert_mod, NULL);
    vkDestroyShaderModule(vk->device, frag_mod, NULL);

    if (vr != VK_SUCCESS) {
        NK_LOG_ERROR("vkCreateGraphicsPipelines failed: %d", vr);
        return NK_ERROR_VULKAN;
    }

    NK_LOG_INFO("SDF pipeline created");
    return NK_SUCCESS;
}

void nk_pipeline_destroy(NkVkContext *vk, NkPipeline *p) {
    if (p->pipeline) vkDestroyPipeline(vk->device, p->pipeline, NULL);
    if (p->layout)   vkDestroyPipelineLayout(vk->device, p->layout, NULL);
    memset(p, 0, sizeof(*p));
}

