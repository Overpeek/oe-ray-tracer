#include <engine/include.hpp>



oe::graphics::Window* window;
oe::assets::DefaultShader* shader;
oe::graphics::Shader* compute_shader;
oe::graphics::PrimitiveRenderer* fb_screen_quad;
oe::graphics::SpritePack* sprites;


void render(float update_fraction)
{
    sprites->bind();
    shader->bind();
    fb_screen_quad->render();
}

void update()
{

}

void resize(const oe::ResizeEvent& event)
{
    glm::mat4 pr_matrix = glm::ortho(-event.aspect, event.aspect, 1.0f, -1.0f);
    shader->setProjectionMatrix(pr_matrix);
    spdlog::info("aspect: {}", event.aspect);
}

int main()
{
    // engine init
    auto& engine = oe::Engine::getSingleton();
    oe::EngineInfo engine_info;
    engine_info.debug_messages = true;
    engine.init(engine_info);

    // window
    oe::WindowInfo window_info;
    window_info.size = { 600, 600 };
    window_info.title = "oe-ray-tracer";
    window_info.resizeable = false;
    window_info.swap_interval = 0;
    window = engine.createWindow(window_info);

    // renderer
    const std::vector<oe::graphics::VertexData> vertices = {
        { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, oe::colors::red },
        { { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }, oe::colors::green },
        { { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, oe::colors::blue },
        { { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }, oe::colors::white },
    };
    oe::RendererInfo renderer_info;
    renderer_info.max_primitive_count = 1;
    renderer_info.arrayRenderType = oe::types::staticrender;
    renderer_info.staticVBOBuffer_data = (void*)&vertices[0];
    fb_screen_quad = (oe::graphics::PrimitiveRenderer*)engine.createPrimitiveRenderer(renderer_info);
    fb_screen_quad->vertexCount() = 4;
    
    // sprites
    sprites = new oe::graphics::SpritePack();
    sprites->construct();
    // sprites->empty_sprite();

    // events
    window->connect_render_listener<&render>();
    window->connect_update_listener<60, &update>();
    window->connect_listener<oe::ResizeEvent, &resize>();

    std::string shader_source;
    oe::utils::FileIO::getSingleton().readString(shader_source, "res/compute-rays.glsl");
    // shaders
    oe::ShaderInfo shader_info;
    shader_info.name = "compute-rays";
    shader_info.shader_stages = {
        { oe::shader_stages::compute_shader, shader_source }
    };
    compute_shader = engine.createShader(shader_info);
    shader = new oe::assets::DefaultShader();

    // start
    window->getGameloop().start();

    // cleanup
    delete shader;
    delete compute_shader;
    delete sprites;
    delete fb_screen_quad;
    delete window;
}