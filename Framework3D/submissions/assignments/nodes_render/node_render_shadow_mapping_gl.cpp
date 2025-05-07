

#include "../geometries/mesh.h"
#include "../light.h"
#include "nodes/core/def/node_def.hpp"
#include "pxr/base/gf/frustum.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/hd/tokens.h"
#include "render_node_base.h"
#include "rich_type_buffer.hpp"
#include "utils/draw_fullscreen.h"
NODE_DEF_OPEN_SCOPE
NODE_DECLARATION_FUNCTION(shadow_mapping)
{
    b.add_input<int>("resolution").default_val(1024).min(256).max(4096);
    b.add_input<std::string>("Shader").default_val("shaders/shadow_mapping.fs");
    b.add_output<TextureHandle>("Shadow Maps");
}

NODE_EXECUTION_FUNCTION(shadow_mapping)
{
    int resolution = params.get_input<int>("resolution");
    std::string shaderPath = params.get_input<std::string>("Shader");

    TextureDesc desc;
    desc.array_size = static_cast<int>(lights.size());
    desc.size       = GfVec2i(resolution);
    desc.format     = HdFormatFloat32;
    auto shadowTex  = resource_allocator.create(desc);

    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0, 0, resolution, resolution);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    ShaderDesc sd;
    sd.set_vertex_path(
        std::filesystem::path(RENDER_NODES_FILES_DIR)/"shaders/shadow_mapping.vs");
    sd.set_fragment_path(
        std::filesystem::path(RENDER_NODES_FILES_DIR)/shaderPath);
    auto shader = resource_allocator.create(sd);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Helper: 手写 LookAt 替代 GfMatrix4d::LookAt
    auto MakeLookAt = [](const GfVec3f &eye,
                         const GfVec3f &center,
                         const GfVec3f &up) {
        GfVec3f f = center - eye; f.Normalize();
        GfVec3f s = GfCross(f, up);    s.Normalize();
        GfVec3f u = GfCross(s, f);
        float tx = -GfDot(s, eye);
        float ty = -GfDot(u, eye);
        float tz =  GfDot(f, eye);
        return GfMatrix4f(
            s[0],  s[1],  s[2],  tx,
            u[0],  u[1],  u[2],  ty,
           -f[0], -f[1], -f[2],  tz,
            0,     0,     0,     1
        );
    };

    for (int i = 0; i < (int)lights.size(); ++i) {
        if (lights[i]->GetLightType() != HdPrimTypeTokens->sphereLight) continue;
        GlfSimpleLight lp = lights[i]->Get(HdTokens->params).Get<GlfSimpleLight>();
        GfVec3f pos(lp.GetPosition()[0],lp.GetPosition()[1],lp.GetPosition()[2]);
        float rad = lights[i]->Get(HdLightTokens->radius).Get<float>();

        GfMatrix4f view = MakeLookAt(pos, GfVec3f(0.0f), GfVec3f(0,1,0));
        GfFrustum fr; fr.SetPerspective(90.0f,1.0f,0.1f,rad);
        GfMatrix4f proj = GfMatrix4f(fr.ComputeProjectionMatrix());

        shader->shader.use();
        shader->shader.setMat4("light_view", view);
        shader->shader.setMat4("light_projection", proj);

        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                  shadowTex->texture_id, 0, i);
        glClear(GL_DEPTH_BUFFER_BIT);

        for (auto mesh : meshes) {
            shader->shader.setMat4("model", mesh->transform);
            mesh->RefreshGLBuffer();
            glBindVertexArray(mesh->VAO);
            glDrawElements(GL_TRIANGLES,
                           (GLsizei)(mesh->triangulatedIndices.size()*3),
                           GL_UNSIGNED_INT, nullptr);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fbo);
    resource_allocator.destroy(shader);
    params.set_output("Shadow Maps", shadowTex);
}

NODE_DECLARATION_UI(shadow_mapping);
NODE_DEF_CLOSE_SCOPE
