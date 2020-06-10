#include <engine/include.hpp>



oe::graphics::Window* window;
oe::assets::DefaultShader* shader;
oe::graphics::Shader* compute_shader;
oe::graphics::PrimitiveRenderer* fb_screen_quad;
oe::graphics::SpritePack* sprites;
oe::gui::GUI* gui_manager;

oe::graphics::Texture* compute_texture;
oe::graphics::Sprite* compute_texture_sprite;

uint64_t next_power_of_2(uint64_t x)
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return ++x;
}

void render(float update_fraction)
{
    auto work_group_size = compute_shader->workGroupSize();
    size_t res = next_power_of_2(590);

    compute_texture->bindCompute();
    compute_shader->bind();
    compute_shader->dispatchCompute({ res / work_group_size.x, res / work_group_size.y, 1 });
    compute_texture->unbindCompute();

    sprites->bind();
    shader->bind();
    fb_screen_quad->render();

	gui_manager->render();
}

void update()
{

}

void update_2()
{
	auto& gameloop = window->getGameloop(); 
	spdlog::info("frametime: {:3.3f} ms ({} fps)", gameloop.getFrametimeMS(), gameloop.getAverageFPS());
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
    window_info.size = { 900, 600 };
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
    auto fonts = new oe::graphics::Font(sprites);
	oe::graphics::Text::setFont(*fonts);
    sprites->construct();
    // sprites->empty_sprite();
    oe::TextureInfo ti;
    ti.empty = true;
    ti.size = { 590, 590 };
    ti.offset = { 0, 0 };
    ti.data_format = oe::formats::rgba;
    compute_texture = engine.createTexture(ti);
    compute_texture_sprite = new oe::graphics::Sprite(compute_texture);

    // events
    window->connect_render_listener<&render>();
    window->connect_update_listener<60, &update>();
    window->connect_update_listener<2, &update_2>();
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

	// gui
	gui_manager = new oe::gui::GUI(window);
    {
        oe::gui::SpritePanelInfo spi;
        spi.size = { 590, 590 };
        spi.align_parent = oe::alignments::top_left;
        spi.align_render = oe::alignments::top_left;
        spi.offset_position = { 0, 0 };
        spi.sprite = compute_texture_sprite;
        spi.color = oe::colors::white;
        oe::gui::SpritePanel* sp = new oe::gui::SpritePanel(gui_manager, spi);
        gui_manager->addSubWidget(sp);
    }
    {
        oe::gui::DecoratedButtonInfo dbi;
        dbi.align_parent = oe::alignments::top_right;
        dbi.align_render = oe::alignments::top_right;
        dbi.size = { 295, 50 };
        dbi.text = "button";
        dbi.sprite = sprites->empty_sprite();
        oe::gui::DecoratedButton* btn = new oe::gui::DecoratedButton(gui_manager, dbi);
        gui_manager->addSubWidget(btn);
    }

    // start
    window->getGameloop().start();

    // cleanup
    delete compute_texture_sprite;
    delete compute_texture;
    delete fonts;
    delete shader;
    delete compute_shader;
    delete sprites;
    delete fb_screen_quad;
    delete window;
}