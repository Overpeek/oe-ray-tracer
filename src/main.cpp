#include <engine/include.hpp>
#include <engine/graphics/opengl/gl_shader.hpp>
#include <engine/graphics/opengl/gl_primitive_renderer.hpp>
#include <engine/graphics/opengl/buffers/storageBuffer.hpp>
#include <glm/gtx/rotate_vector.hpp>



struct Vertex {
    glm::vec3 position;
    float __padding0;
    glm::vec2 uv_texture;
    float __padding1[2];
    glm::vec2 uv_reflect;
    float __padding2[2];
    glm::vec4 color;

    Vertex()
        : position(0.0f), uv_texture(0.0f), color(0.0f)
    {}

    Vertex(glm::vec3 _position, glm::vec2 _uv_texture, glm::vec2 _uv_reflectiveness, glm::vec4 _color)
        : position(_position), uv_texture(_uv_texture), uv_reflect(_uv_reflectiveness), color(_color)
    {}
};
struct SphereObj {
	glm::vec3 position;
	float __padding0;
	glm::vec4 extra0;
	glm::vec4 extra1;
	glm::vec4 color;

    SphereObj()
        : position(0.0f), extra0(0.0f, 0.0f, 0.0f, 0.0f), extra1(0.0f, 0.0f, 0.0f, 0.0f), color(0.0f)
    {}

    SphereObj(glm::vec3 _position, glm::vec4 _extra0, glm::vec4 _extra1, glm::vec4 _color)
        : position(_position), extra0(_extra0), extra1(_extra1), color(_color)
    {}
};



typedef std::array<const oe::graphics::Sprite*, 2> tex_ref_sprite;

oe::graphics::Window window;
oe::graphics::Shader compute_shader;
oe::graphics::PrimitiveRenderer fb_screen_quad;
oe::assets::DefaultShader* shader;
oe::graphics::SpritePack* sprites;
const oe::graphics::Sprite* oe_sprite;
tex_ref_sprite checkerboard_sprite;
tex_ref_sprite obj_sprite;

oe::graphics::StorageBuffer* vertex_ssbo;
oe::graphics::StorageBuffer* sphereobj_ssbo;

oe::gui::GUI* gui_manager;
std::vector<oe::gui::Slider*> gui_sliders;
oe::gui::Checkbox* gui_checkbox;

oe::graphics::Texture compute_texture;
oe::graphics::Sprite compute_texture_sprite;

glm::vec3 cam_pos = { 0.0f, 0.5f, 0.0f, };
glm::vec2 cam_orient = { 0.0f, 0.0f };



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

void append_quad(const glm::vec3& pos, const glm::vec2& size, const glm::mat4& orient, const tex_ref_sprite& sprites, const glm::vec4& color, std::vector<Vertex>& vertices)
{
    glm::vec2 size_half = size * 0.5f;

    vertices.push_back({ { -size_half.x, 0.0f, -size_half.y }, { sprites[0]->position.x,                      sprites[0]->position.y                      }, { sprites[1]->position.x,                      sprites[1]->position.y                      }, color });
    vertices.push_back({ { -size_half.x, 0.0f,  size_half.y }, { sprites[0]->position.x + sprites[0]->size.x, sprites[0]->position.y                      }, { sprites[1]->position.x + sprites[1]->size.x, sprites[1]->position.y                      }, color });
    vertices.push_back({ {  size_half.x, 0.0f,  size_half.y }, { sprites[0]->position.x + sprites[0]->size.x, sprites[0]->position.y + sprites[0]->size.y }, { sprites[1]->position.x + sprites[1]->size.x, sprites[1]->position.y + sprites[1]->size.y }, color });
     
    vertices.push_back({ { -size_half.x, 0.0f, -size_half.y }, { sprites[0]->position.x,                      sprites[0]->position.y                      }, { sprites[1]->position.x,                      sprites[1]->position.y                      }, color });
    vertices.push_back({ {  size_half.x, 0.0f,  size_half.y }, { sprites[0]->position.x + sprites[0]->size.x, sprites[0]->position.y + sprites[0]->size.y }, { sprites[1]->position.x + sprites[1]->size.x, sprites[1]->position.y + sprites[1]->size.y }, color });
    vertices.push_back({ {  size_half.x, 0.0f, -size_half.y }, { sprites[0]->position.x,                      sprites[0]->position.y + sprites[0]->size.y }, { sprites[1]->position.x,                      sprites[1]->position.y + sprites[1]->size.y }, color });

    for (auto& iter = vertices.rbegin(); iter != vertices.rbegin() + 6; iter++)
    {
        iter->position = pos + glm::vec3(orient * glm::vec4(iter->position, 0.0f));
    }
}

void render(float update_fraction)
{
    std::function<float(int)> v = [](int i) { return gui_sliders.at(i)->slider_info.initial_value; };
    float t = oe::utils::Clock::getSingleton().getSessionMillisecond() / 1000.0f;

    // vertices
    glm::mat4 vertical = glm::rotate(glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
    std::vector<Vertex> vertices;
    append_quad(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(10.0f), glm::mat4(1.0f), checkerboard_sprite, oe::colors::white, vertices);
    append_quad(glm::vec3(4.0f, 2.5f, 4.0f), glm::vec2(10.0f), glm::rotate(glm::half_pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f)) * vertical, { sprites->empty_sprite(), sprites->empty_sprite() }, oe::colors::light_grey, vertices);
    append_quad(glm::vec3(0.0f, 0.5f, 0.5f), glm::vec2(1.0f), glm::rotate(2.0f, glm::vec3(0.0f, 1.0f, 0.0f)) * vertical, obj_sprite, oe::colors::white, vertices);
    vertex_ssbo->setBufferData(vertices.data(), vertices.size() * sizeof(Vertex));
    // spheres
    const std::vector<SphereObj> sphereobjs = 
    {
        { { 0.0f, 5.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, oe::colors::white }
    };
    sphereobj_ssbo->setBufferData(sphereobjs.data(), sphereobjs.size() * sizeof(SphereObj));

    // view matrix
    glm::mat4 vw_matrix = glm::lookAt(cam_pos, cam_pos + glm::vec3(cos(cam_orient.y) * cos(cam_orient.x), sin(cam_orient.y), cos(cam_orient.y) * sin(cam_orient.x)), glm::vec3(0.0f, 1.0f, 0.0f));
    compute_shader->bind();
    compute_shader->setUniformMat4("vw_matrix", vw_matrix);
    
    // compute
    sprites->bind();
    auto work_group_size = compute_shader.getGL()->workGroupSize();
    size_t res = next_power_of_2(590 * 2);
    compute_texture->bindCompute();
    compute_shader.getGL()->bindSSBO("vertex_buffer", vertex_ssbo, 1);
    compute_shader.getGL()->bindSSBO("sphereobj_buffer", sphereobj_ssbo, 2);
    compute_shader->setUniform1i("triangle_count", 6);
    compute_shader->setUniform1i("sphere_count", 1);
    compute_shader->setUniform1i("normaldraw", gui_checkbox->m_checkbox_info.initial);
    compute_shader.getGL()->dispatchCompute({ res / work_group_size.x, res / work_group_size.y, 1 });
    compute_texture->unbindCompute();

    // draw compute result to gui
    sprites->bind();
    shader->bind();
    fb_screen_quad->render();

	gui_manager->render();
}

void update()
{
    // arrow keys
    const float rotation_speed = 0.06f;
    if (window->key(oe::keys::key_left))
        cam_orient.x -= rotation_speed;
    if (window->key(oe::keys::key_right))
        cam_orient.x += rotation_speed;
    if (window->key(oe::keys::key_up))
        cam_orient.y += rotation_speed;
    if (window->key(oe::keys::key_down))
        cam_orient.y -= rotation_speed;
    cam_orient.y = oe::utils::clamp(cam_orient.y, -glm::half_pi<float>() + 0.00001f, glm::half_pi<float>() - 0.00001f);
    
    // wasd
    glm::vec3 dir(0.0f);
    const float movement_speed = 0.06f;
    if (window->key(oe::keys::key_w))
        dir += glm::vec3(cos(cam_orient.y) * cos(cam_orient.x), sin(cam_orient.y), cos(cam_orient.y) * sin(cam_orient.x));
    if (window->key(oe::keys::key_s))
        dir -= glm::vec3(cos(cam_orient.y) * cos(cam_orient.x), sin(cam_orient.y), cos(cam_orient.y) * sin(cam_orient.x));
    if (window->key(oe::keys::key_a))
        dir += glm::vec3(sin(cam_orient.x), 0.0f, -cos(cam_orient.x));
    if (window->key(oe::keys::key_d))
        dir -= glm::vec3(sin(cam_orient.x), 0.0f, -cos(cam_orient.x));
    if (window->key(oe::keys::key_space))
        dir += glm::vec3(0.0f, 1.0f, 0.0f);
    if (window->key(oe::keys::key_c))
        dir -= glm::vec3(0.0f, 1.0f, 0.0f);
    if (glm::dot(dir, dir) > 0.01f && (window->key(oe::keys::key_w) || window->key(oe::keys::key_a) || window->key(oe::keys::key_s) || window->key(oe::keys::key_d) || window->key(oe::keys::key_space) || window->key(oe::keys::key_c)))
        cam_pos += glm::normalize(dir) * movement_speed;
    return;
}

void update_2()
{
	auto& gameloop = window->getGameloop(); 
	spdlog::info("frametime: {:3.3f} ms ({} fps)", gameloop.getFrametimeMS(), gameloop.getAverageFPS());
}

void keyboardEvent(const oe::KeyboardEvent& e)
{
	if (e.action == oe::actions::press && e.key == oe::keys::key_f12)
	{
		// save screenshot
		auto img = compute_texture->getImageData();
		oe::utils::FileIO::getSingleton().saveImage("screenshot.png", img);
	}
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
	window_info.multisamples = 2;
    window = oe::graphics::Window(window_info);
	window->connect_listener<oe::KeyboardEvent, keyboardEvent>();

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
    fb_screen_quad = oe::graphics::PrimitiveRenderer(renderer_info);
    fb_screen_quad->vertexCount() = 4;

    // SSBO:s
    vertex_ssbo = new oe::graphics::StorageBuffer(nullptr, 1000 * sizeof(Vertex), oe::types::dynamic_type);
    sphereobj_ssbo = new oe::graphics::StorageBuffer(nullptr, 1000 * sizeof(SphereObj), oe::types::dynamic_type);
    
    // sprites
    sprites = new oe::graphics::SpritePack();
    auto fonts = new oe::graphics::Font();
	oe::graphics::Text::setFont(fonts);
    checkerboard_sprite = { sprites->addSprite("res/checkerboard.tex.png"), sprites->addSprite("res/checkerboard.ref.png") };
    obj_sprite = { sprites->addSprite("res/obj.tex.png"), sprites->addSprite("res/obj.ref.png") };
    oe_sprite = sprites->addSprite(oe::assets::TextureSet::oe_logo_img);
    sprites->construct();
    // sprites->empty_sprite();
    oe::TextureInfo ti;
    ti.empty = true;
    ti.size = { 590 * 2, 590 * 2 };
    ti.offset = { 0, 0 };
    ti.data_format = oe::formats::rgba;
    compute_texture = oe::graphics::Texture(ti);
    compute_texture_sprite.m_owner = compute_texture;

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
    compute_shader = oe::graphics::Shader(shader_info);
    shader = new oe::assets::DefaultShader();

	// gui
	gui_manager = new oe::gui::GUI(window);
    {
        oe::gui::SpritePanelInfo spi;
        spi.size = { 590, 590 };
        spi.align_parent = oe::alignments::top_left;
        spi.align_render = oe::alignments::top_left;
        spi.offset_position = { 0, 0 };
        spi.sprite = &compute_texture_sprite;
        spi.color = oe::colors::white;
        oe::gui::SpritePanel* sp = new oe::gui::SpritePanel(spi);
        gui_manager->addSubWidget(sp);
    }
    {
        oe::gui::DecoratedButtonInfo dbi;
        dbi.align_parent = oe::alignments::top_right;
        dbi.align_render = oe::alignments::top_right;
        dbi.size = { 295, 50 };
        dbi.text = U"button";
        dbi.sprite = sprites->empty_sprite();
        oe::gui::DecoratedButton* btn = new oe::gui::DecoratedButton(dbi);
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
        auto slider = new oe::gui::Slider(si);
        gui_sliders.push_back(slider);
        gui_manager->addSubWidget(slider);
    }
    {
        oe::gui::CheckboxInfo ci;
        ci.align_parent = oe::alignments::top_right;
        ci.align_render = oe::alignments::top_right;
        ci.offset_position = { 0, 360 };
        ci.sprite = sprites->empty_sprite();
        gui_checkbox = new oe::gui::Checkbox(ci);
        gui_manager->addSubWidget(gui_checkbox);
    }
    std::function<void(int, float)> initial = [](int i, float f) { gui_sliders.at(i)->slider_info.initial_value = f; };
    initial(0, -0.9f);
    initial(1, -0.7f);
    initial(2, 0.0f);

    // start
    window->getGameloop().start();

    // cleanup
    delete gui_manager;
    for (auto i : gui_sliders) { delete i; }
    delete fonts;
    delete shader;
    delete sprites;
}