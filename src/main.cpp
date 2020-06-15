#include <engine/include.hpp>
#include <engine/graphics/opengl/gl_shader.hpp>
#include <engine/graphics/opengl/gl_primitive_renderer.hpp>
#include <engine/graphics/opengl/buffers/storageBuffer.hpp>



struct RTVertex {
    glm::vec3 position;
    float __padding0;
    glm::vec2 uv_texture;
    float __padding1[2];
    glm::vec2 uv_reflect;
    float __padding2[2];
    glm::vec4 color;

    RTVertex()
        : position(0.0f), uv_texture(0.0f), color(0.0f)
    {}

    RTVertex(glm::fvec3 _position, glm::fvec2 _uv_texture, glm::fvec2 _uv_reflectiveness, glm::fvec4 _color)
        : position(_position), uv_texture(_uv_texture), uv_reflect(_uv_reflectiveness), color(_color)
    {}

    RTVertex(glm::fvec2 _position, glm::fvec2 _uv_texture, glm::fvec2 _uv_reflectiveness, glm::fvec4 _color)
        : position(_position, 0.0f), uv_texture(_uv_texture), uv_reflect(_uv_reflectiveness), color(_color)
    {}
};



oe::graphics::Window* window;
oe::assets::DefaultShader* shader;
oe::graphics::GLShader* compute_shader;
oe::graphics::PrimitiveRenderer* fb_screen_quad;
oe::graphics::SpritePack* sprites;
const oe::graphics::Sprite* checkerboard_sprite;
const oe::graphics::Sprite* obj_sprite;
const oe::graphics::Sprite* obj_ref_sprite;

oe::graphics::StorageBuffer* ssbo;

oe::gui::GUI* gui_manager;
std::vector<oe::gui::Slider*> gui_sliders;

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
    std::function<float(int)> v = [](int i) { return gui_sliders.at(i)->slider_info.initial_value; };

    std::array<glm::vec2, 3> p = { checkerboard_sprite->position, obj_sprite->position, obj_ref_sprite->position };
    std::array<glm::vec2, 3> s = { checkerboard_sprite->size, obj_sprite->size, obj_ref_sprite->size };
    std::vector<RTVertex> vertices = {
        // floor
        { { -5.0f, 0.0f, -5.0f }, { p[0].x,          p[0].y          }, { p[0].x,          p[0].y          }, oe::colors::white },
        { { -5.0f, 0.0f,  5.0f }, { p[0].x + s[0].x, p[0].y          }, { p[0].x + s[0].x, p[0].y          }, oe::colors::white },
        { {  5.0f, 0.0f,  5.0f }, { p[0].x + s[0].x, p[0].y + s[0].y }, { p[0].x + s[0].x, p[0].y + s[0].y }, oe::colors::white },

        { { -5.0f, 0.0f, -5.0f }, { p[0].x,          p[0].y          }, { p[0].x,          p[0].y          }, oe::colors::white },
        { {  5.0f, 0.0f,  5.0f }, { p[0].x + s[0].x, p[0].y + s[0].y }, { p[0].x + s[0].x, p[0].y + s[0].y }, oe::colors::white },
        { {  5.0f, 0.0f, -5.0f }, { p[0].x,          p[0].y + s[0].y }, { p[0].x,          p[0].y + s[0].y }, oe::colors::white },

        // wall
        { { -2.0f,  0.0f, 6.0f }, { p[0].x,          p[0].y          }, { p[0].x,          p[0].y          }, oe::colors::white },
        { { -2.0f, 10.0f, 6.0f }, { p[0].x + s[0].x, p[0].y          }, { p[0].x + s[0].x, p[0].y          }, oe::colors::white },
        { {  8.0f, 10.0f, 0.0f }, { p[0].x + s[0].x, p[0].y + s[0].y }, { p[0].x + s[0].x, p[0].y + s[0].y }, oe::colors::white },

        { { -2.0f,  0.0f, 6.0f }, { p[0].x,          p[0].y          }, { p[0].x,          p[0].y          }, oe::colors::white },
        { {  8.0f, 10.0f, 0.0f }, { p[0].x + s[0].x, p[0].y + s[0].y }, { p[0].x + s[0].x, p[0].y + s[0].y }, oe::colors::white },
        { {  8.0f,  0.0f, 0.0f }, { p[0].x,          p[0].y + s[0].y }, { p[0].x,          p[0].y + s[0].y }, oe::colors::white },

        // obj
        { { -0.9f, 1.0f, -1.0f }, { p[1].x,          p[1].y          }, { p[2].x,          p[2].y          }, oe::colors::white },
        { { -0.9f, 0.0f, -1.0f }, { p[1].x + s[1].x, p[1].y          }, { p[2].x + s[2].x, p[2].y          }, oe::colors::white },
        { {  0.0f, 0.0f, -0.7f }, { p[1].x + s[1].x, p[1].y + s[1].y }, { p[2].x + s[2].x, p[2].y + s[2].y }, oe::colors::white },

        { { -0.9f, 1.0f, -1.0f }, { p[1].x,          p[1].y          }, { p[2].x,          p[2].y          }, oe::colors::white },
        { {  0.0f, 0.0f, -0.7f }, { p[1].x + s[1].x, p[1].y + s[1].y }, { p[2].x + s[2].x, p[2].y + s[2].y }, oe::colors::white },
        { {  0.0f, 1.0f, -0.7f }, { p[1].x,          p[1].y + s[1].y }, { p[2].x,          p[2].y + s[2].y }, oe::colors::white },
    };
    ssbo->setBufferData(vertices.data(), vertices.size() * sizeof(RTVertex));

    glm::mat4 vw_matrix = glm::lookAt(glm::vec3(cos(v(0) * 7.0f), v(1), sin(v(0) * 7.0f)), glm::vec3(0.0f, v(2), 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    compute_shader->bind();
    compute_shader->setUniformMat4("vw_matrix", vw_matrix);
    

    // compute
    sprites->bind();
    auto work_group_size = compute_shader->workGroupSize();
    size_t res = next_power_of_2(590);
    compute_texture->bindCompute();
    compute_shader->bindSSBO("vertex_buffer", ssbo, 1);
    compute_shader->setUniform1i("triangle_count", 6);
    compute_shader->dispatchCompute({ res / work_group_size.x, res / work_group_size.y, 1 });
    compute_texture->unbindCompute();

    // draw compute result to gui
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
    // engine_info.ignore_errors = true;
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
    renderer_info.arrayRenderType = oe::types::static_type;
    renderer_info.staticVBOBuffer_data = (void*)&vertices[0];
    fb_screen_quad = (oe::graphics::PrimitiveRenderer*)engine.createPrimitiveRenderer(renderer_info);
    fb_screen_quad->vertexCount() = 4;

    // SSBO
    ssbo = new oe::graphics::StorageBuffer(nullptr, 1000 * sizeof(RTVertex), oe::types::dynamic_type);
    
    // sprites
    sprites = new oe::graphics::SpritePack();
    auto fonts = new oe::graphics::Font(sprites);
	oe::graphics::Text::setFont(*fonts);
    checkerboard_sprite = sprites->addSprite("res/checkerboard.png");
    obj_ref_sprite = sprites->addSprite("res/obj.reflectiveness.png");
    obj_sprite = sprites->addSprite("res/obj.texture.png");
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
        { oe::shader_stages::compute_shader, shader_source, "res" }
    };
    compute_shader = static_cast<oe::graphics::GLShader*>(engine.createShader(shader_info));
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
    for (int i = 0; i < 5; i++) {
        oe::gui::SliderInfo si;
        si.align_parent = oe::alignments::top_right;
        si.align_render = oe::alignments::top_right;
        si.offset_position = { 0, (i + 1) * 60 };
        si.slider_size = { 295, 50 };
        si.knob_size = { 50, 50 };
        si.draw_value = true;
        si.min_value = -2.0f;
        si.max_value = 2.0f;
        si.slider_sprite = sprites->empty_sprite();
        si.knob_sprite = sprites->empty_sprite();
        auto slider = new oe::gui::Slider(gui_manager, si);
        gui_sliders.push_back(slider);
        gui_manager->addSubWidget(slider);
    }
    std::function<void(int, float)> initial = [](int i, float f) { gui_sliders.at(i)->slider_info.initial_value = f; };
    initial(0, -1.15f);
    initial(1, 1.36f);
    initial(2, 1.0f);

    // start
    window->getGameloop().start();

    // cleanup
    delete gui_manager;
    for (auto i : gui_sliders) { delete i; }
    delete compute_texture_sprite;
    delete compute_texture;
    delete fonts;
    delete shader;
    delete compute_shader;
    delete sprites;
    delete fb_screen_quad;
    delete window;
}